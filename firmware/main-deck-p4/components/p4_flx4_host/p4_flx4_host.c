#include "p4_flx4_host.h"
#include "p4_flx4_map.h"
#include "p4_flx4_uac.h"
#include "p4_flx4_midi_gate.h"
#include "control_link.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include <stdatomic.h>
#include "usb/usb_host.h"

#include <string.h>

static const char *TAG = "p4_flx4";

#define FLX4_USB_TASK_STACK  4096
/* USB client callbacks refill the UAC isochronous queue. Keep them above the
 * audio producer (priority 6) so a producer burst cannot starve the consumer
 * long enough to fill the headphone ring. Enumeration and disconnect cleanup
 * do not have that deadline and run below audio output instead: descriptor
 * parsing, initial UAC queue priming and host teardown can otherwise deschedule
 * ae_output during a hot-plug transition. */
#define FLX4_USB_ACTIVE_TASK_PRIO      7
#define FLX4_USB_TRANSITION_TASK_PRIO  5

static usb_host_client_handle_t s_client_handle = NULL;
static usb_device_handle_t      s_dev_handle    = NULL;
static uint8_t                  s_dev_addr      = 0;
static bool                     s_connected     = false;
static flx4_map_state_t         s_map_state;
static p4_flx4_connection_cb_t  s_conn_cb       = NULL;
static void                    *s_conn_cb_ctx   = NULL;
static TaskHandle_t             s_midi_task     = NULL;

static usb_transfer_t          *s_in_xfer       = NULL;
static usb_transfer_t          *s_out_xfer      = NULL;
static uint8_t                  s_in_ep_addr    = 0x81;
static uint8_t                  s_out_ep_addr   = 0x01;
static uint16_t                 s_in_mps        = 64;
static uint16_t                 s_out_mps       = 64;

static p4_flx4_midi_gate_t      s_midi_gate;
static p4_flx4_audio_ring_t     s_audio_ring;
static int16_t                  s_audio_storage[FLX4_AUDIO_RING_FRAMES * FLX4_UAC_CHANNELS];
static p4_flx4_uac_packetizer_t s_packetizer;
static p4_flx4_uac_resampler_t  s_resampler;
static atomic_uint_fast32_t     s_audio_submitted_blocks;
static atomic_uint_fast32_t     s_audio_dropped_blocks;
static atomic_uint_fast32_t     s_audio_submitted_frames;

static portMUX_TYPE             s_flx4_mux      = portMUX_INITIALIZER_UNLOCKED;

bool p4_flx4_host_is_connected(void)
{
    portENTER_CRITICAL(&s_flx4_mux);
    bool c = s_connected;
    portEXIT_CRITICAL(&s_flx4_mux);
    return c;
}

void p4_flx4_host_set_connection_callback(p4_flx4_connection_cb_t cb, void *user_ctx)
{
    s_conn_cb = cb;
    s_conn_cb_ctx = user_ctx;
}

static void in_transfer_cb(usb_transfer_t *transfer)
{
    if (!transfer) return;

    if (transfer->status == USB_TRANSFER_STATUS_COMPLETED) {
        size_t bytes = transfer->actual_num_bytes;
        uint8_t *data = transfer->data_buffer;
        for (size_t i = 0; i + 4 <= bytes; i += 4) {
            flx4_midi_message_t msg;
            if (flx4_midi_parse_usb_packet(&data[i], &msg)) {
                flx4_control_event_t ev;
                portENTER_CRITICAL(&s_flx4_mux);
                bool translated = flx4_map_translate_message(&s_map_state, &msg, &ev);
                portEXIT_CRITICAL(&s_flx4_mux);
                if (translated) {
                    (void)control_link_inject_semantic(ev.type, ev.id, ev.value);
                }
            }
        }
    } else {
        ESP_LOGW(TAG, "FLX4 IN transfer completed with status: %d", transfer->status);
    }

    // Always resubmit transfer while connected so we continuously listen for controller events!
    if (s_dev_handle && s_connected) {
        esp_err_t sub_err = usb_host_transfer_submit(transfer);
        if (sub_err != ESP_OK) {
            ESP_LOGW(TAG, "Resubmit IN transfer failed: %s", esp_err_to_name(sub_err));
        }
    }
}

typedef struct {
    uint32_t generation;
    uint8_t packet[4];
} p4_flx4_out_item_t;

static QueueHandle_t            s_out_queue     = NULL;
static volatile bool            s_out_inflight  = false;

static void out_transfer_cb(usb_transfer_t *transfer);

static void try_submit_next_out(void)
{
    if (!s_connected || !s_out_xfer || !s_dev_handle || !s_out_queue) {
        return;
    }

    portENTER_CRITICAL(&s_flx4_mux);
    if (s_out_inflight) {
        portEXIT_CRITICAL(&s_flx4_mux);
        return;
    }
    s_out_inflight = true;
    portEXIT_CRITICAL(&s_flx4_mux);

    p4_flx4_out_item_t item;
    while (xQueueReceive(s_out_queue, &item, 0) == pdTRUE) {
        if (!p4_flx4_midi_gate_accepts_generation(&s_midi_gate,
                                                   item.generation)) {
            continue;
        }
        memcpy(s_out_xfer->data_buffer, item.packet, sizeof(item.packet));
        s_out_xfer->device_handle = s_dev_handle;
        s_out_xfer->bEndpointAddress = s_out_ep_addr;
        s_out_xfer->num_bytes = sizeof(item.packet);
        s_out_xfer->callback = out_transfer_cb;
        esp_err_t err = usb_host_transfer_submit(s_out_xfer);
        if (err != ESP_OK) {
            portENTER_CRITICAL(&s_flx4_mux);
            s_out_inflight = false;
            portEXIT_CRITICAL(&s_flx4_mux);
            ESP_LOGW(TAG, "OUT submit failed (EP 0x%02x): %s", s_out_ep_addr, esp_err_to_name(err));
        }
        return;
    }
    portENTER_CRITICAL(&s_flx4_mux);
    s_out_inflight = false;
    portEXIT_CRITICAL(&s_flx4_mux);
}

static void out_transfer_cb(usb_transfer_t *transfer)
{
    (void)transfer;
    portENTER_CRITICAL(&s_flx4_mux);
    s_out_inflight = false;
    portEXIT_CRITICAL(&s_flx4_mux);
    try_submit_next_out();
}

esp_err_t p4_flx4_host_send_packet(const uint8_t packet[4])
{
    if (!packet) {
        return ESP_ERR_INVALID_ARG;
    }
    p4_flx4_out_item_t item = { 0 };
    if (!p4_flx4_midi_gate_begin(&s_midi_gate, &item.generation)) {
        return ESP_ERR_INVALID_STATE;
    }
    memcpy(item.packet, packet, sizeof(item.packet));

    esp_err_t result = ESP_OK;
    if (!s_connected || !s_out_queue) {
        result = ESP_ERR_INVALID_STATE;
    } else if (xQueueSend(s_out_queue, &item, 0) != pdTRUE) {
        result = ESP_ERR_NO_MEM;
    }
    p4_flx4_midi_gate_end(&s_midi_gate);

    if (result == ESP_OK && s_client_handle) {
        (void)usb_host_client_unblock(s_client_handle);
    }
    if (result == ESP_OK) {
        try_submit_next_out();
    }
    return result;
}

esp_err_t p4_flx4_host_send_led(uint8_t led, uint8_t state, uint8_t deck)
{
    uint8_t packet[4] = {0};
    if (!flx4_led_midi_build_packet(led, state, deck, packet)) {
        return ESP_ERR_INVALID_ARG;
    }
    return p4_flx4_host_send_packet(packet);
}

static esp_err_t flx4_control_link_led_sink(uint8_t led, uint8_t state, uint8_t deck, void *user_ctx)
{
    (void)user_ctx;
    return p4_flx4_host_send_led(led, state, deck);
}

#define FLX4_ISOC_XFERS_NUM        3
#define FLX4_ISOC_PACKETS_PER_XFER 4
#define FLX4_ISOC_MAX_PACKET_BYTES 384

static usb_transfer_t          *s_isoc_xfers[FLX4_ISOC_XFERS_NUM] = { NULL };
static usb_transfer_t          *s_ctrl_xfer = NULL;
static uint8_t                  s_ctrl_step = 0;
static bool                     s_audio_claimed = false;
static uint8_t                  s_audio_ep_addr = 0x01;
static uint32_t                 s_isoc_total_transfers = 0;
static uint32_t                 s_audio_total_frames = 0;

static void isoc_transfer_cb(usb_transfer_t *transfer);
static void ctrl_transfer_cb(usb_transfer_t *transfer);

static void prepare_and_submit_isoc_transfer(usb_transfer_t *transfer)
{
    if (!transfer || !s_dev_handle || !s_connected || !s_audio_claimed) {
        return;
    }
    size_t buffer_offset = 0;
    for (int i = 0; i < transfer->num_isoc_packets; ++i) {
        uint16_t frames = p4_flx4_uac_packetizer_next_frames(&s_packetizer);
        size_t bytes = (size_t)frames * FLX4_UAC_CHANNELS * FLX4_UAC_BYTES_PER_SAMPLE;
        int16_t *dst = (int16_t *)(transfer->data_buffer + buffer_offset);
        portENTER_CRITICAL(&s_flx4_mux);
        (void)p4_flx4_audio_ring_read(&s_audio_ring, dst, frames, true);
        portEXIT_CRITICAL(&s_flx4_mux);
        transfer->isoc_packet_desc[i].num_bytes = (int)bytes;
        buffer_offset += bytes;
    }
    transfer->num_bytes = (int)buffer_offset;
    transfer->device_handle = s_dev_handle;
    transfer->bEndpointAddress = s_audio_ep_addr;
    transfer->callback = isoc_transfer_cb;
    esp_err_t err = usb_host_transfer_submit(transfer);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Isochronous transfer submit failed: %s", esp_err_to_name(err));
    } else {
        s_isoc_total_transfers++;
        if ((s_isoc_total_transfers % 1000) == 0) {
            ESP_LOGW(TAG, "FLX4 ISOC audio alive: %" PRIu32 " transfers, %" PRIu32 " frames pushed to ring",
                     s_isoc_total_transfers, s_audio_total_frames);
        }
    }
}

static void isoc_transfer_cb(usb_transfer_t *transfer)
{
    if (s_connected && s_dev_handle && s_audio_claimed) {
        prepare_and_submit_isoc_transfer(transfer);
    }
}

static void start_audio_config_sequence(void)
{
    if (!s_dev_handle || !s_client_handle || !s_audio_claimed) return;
    if (!s_ctrl_xfer) {
        esp_err_t err = usb_host_transfer_alloc(64, 0, &s_ctrl_xfer);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Alloc ctrl xfer failed: %s", esp_err_to_name(err));
            return;
        }
    }

    s_ctrl_step = 1;
    usb_setup_packet_t *setup = (usb_setup_packet_t *)s_ctrl_xfer->data_buffer;
    setup->bmRequestType = USB_BM_REQUEST_TYPE_DIR_OUT | USB_BM_REQUEST_TYPE_TYPE_STANDARD | USB_BM_REQUEST_TYPE_RECIP_INTERFACE; // 0x01
    setup->bRequest = USB_B_REQUEST_SET_INTERFACE; // 0x0B
    setup->wValue = 2; // Alt setting 2 (4-channel 16-bit PCM)
    setup->wIndex = 1; // Interface 1
    setup->wLength = 0;

    s_ctrl_xfer->device_handle = s_dev_handle;
    s_ctrl_xfer->bEndpointAddress = 0;
    s_ctrl_xfer->num_bytes = sizeof(usb_setup_packet_t);
    s_ctrl_xfer->callback = ctrl_transfer_cb;

    esp_err_t err = usb_host_transfer_submit_control(s_client_handle, s_ctrl_xfer);
    ESP_LOGW(TAG, "Submitted SET_INTERFACE (intf 1, alt 2): %s", esp_err_to_name(err));
}

static void ctrl_transfer_cb(usb_transfer_t *transfer)
{
    if (transfer->status != USB_TRANSFER_STATUS_COMPLETED) {
        ESP_LOGW(TAG, "Control transfer step %d failed with status %d", s_ctrl_step, transfer->status);
        return;
    }

    if (s_ctrl_step == 1) {
        s_ctrl_step = 2;
        usb_setup_packet_t *setup = (usb_setup_packet_t *)transfer->data_buffer;
        setup->bmRequestType = USB_BM_REQUEST_TYPE_DIR_OUT | USB_BM_REQUEST_TYPE_TYPE_CLASS | USB_BM_REQUEST_TYPE_RECIP_ENDPOINT; // 0x22
        setup->bRequest = 0x01; // UAC_SET_CUR
        setup->wValue = 0x0100; // SAMPLING_FREQ_CONTROL
        setup->wIndex = s_audio_ep_addr; // EP 0x01
        setup->wLength = 3;

        uint8_t *data = &transfer->data_buffer[sizeof(usb_setup_packet_t)];
        data[0] = 0x44; // 44100 Hz = 0x0000AC44
        data[1] = 0xAC;
        data[2] = 0x00;

        transfer->num_bytes = sizeof(usb_setup_packet_t) + 3;
        transfer->device_handle = s_dev_handle;
        transfer->bEndpointAddress = 0;
        transfer->callback = ctrl_transfer_cb;

        esp_err_t err = usb_host_transfer_submit_control(s_client_handle, transfer);
        ESP_LOGW(TAG, "Submitted SET_CUR (EP 0x%02x, 44100 Hz): %s", s_audio_ep_addr, esp_err_to_name(err));
    } else if (s_ctrl_step == 2) {
        s_ctrl_step = 0;
        ESP_LOGW(TAG, ">>> Pioneer DDJ-FLX4 Audio DAC INITIALIZED & READY! <<<");

        // Start Isochronous streaming!
        for (int k = 0; k < FLX4_ISOC_XFERS_NUM; ++k) {
            if (!s_isoc_xfers[k]) {
                (void)usb_host_transfer_alloc(FLX4_ISOC_MAX_PACKET_BYTES * FLX4_ISOC_PACKETS_PER_XFER,
                                              FLX4_ISOC_PACKETS_PER_XFER,
                                              &s_isoc_xfers[k]);
            }
            if (s_isoc_xfers[k]) {
                prepare_and_submit_isoc_transfer(s_isoc_xfers[k]);
            }
        }
        ESP_LOGW(TAG, "FLX4 Isochronous Audio streaming started on EP 0x%02x", s_audio_ep_addr);
        /* Initial allocation and ring priming are transition work. Raise the
         * client only after all periodic transfers are queued, so subsequent
         * UAC completion callbacks outrank the audio producer without letting
         * reconnect setup preempt an in-flight PCM5102A block. */
        vTaskPrioritySet(NULL, FLX4_USB_ACTIVE_TASK_PRIO);
    }
}

#define FLX4_RESAMPLE_INPUT_FRAMES  128u
#define FLX4_RESAMPLE_OUTPUT_FRAMES 129u

esp_err_t p4_flx4_host_write_audio(const int16_t *master_samples,
                                   const int16_t *hp_samples,
                                   size_t frame_count,
                                   uint32_t source_sample_rate)
{
    if ((!master_samples && !hp_samples) || frame_count == 0) return ESP_ERR_INVALID_ARG;
    if (source_sample_rate < FLX4_UAC_SAMPLE_RATE || source_sample_rate > 48000u) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!p4_flx4_host_is_connected()) return ESP_ERR_INVALID_STATE;

    if (s_resampler.source_rate != source_sample_rate ||
        s_resampler.target_rate != FLX4_UAC_SAMPLE_RATE ||
        s_resampler.channels != FLX4_UAC_CHANNELS) {
        if (!p4_flx4_uac_resampler_init(&s_resampler,
                                        source_sample_rate,
                                        FLX4_UAC_SAMPLE_RATE,
                                        FLX4_UAC_CHANNELS)) {
            return ESP_ERR_INVALID_ARG;
        }
    }

    int16_t input[FLX4_RESAMPLE_INPUT_FRAMES * FLX4_UAC_CHANNELS];
    int16_t output[FLX4_RESAMPLE_OUTPUT_FRAMES * FLX4_UAC_CHANNELS];
    bool dropped = false;
    while (frame_count > 0) {
        size_t chunk = frame_count > FLX4_RESAMPLE_INPUT_FRAMES
                           ? FLX4_RESAMPLE_INPUT_FRAMES
                           : frame_count;
        for (size_t i = 0; i < chunk; ++i) {
            int16_t m_l = master_samples ? (master_samples[i * 2 + 0] >> 2) : 0;
            int16_t m_r = master_samples ? (master_samples[i * 2 + 1] >> 2) : 0;
            int16_t h_l = hp_samples ? (hp_samples[i * 2 + 0] >> 2) : m_l;
            int16_t h_r = hp_samples ? (hp_samples[i * 2 + 1] >> 2) : m_r;

            input[i * 4 + 0] = m_l; // Ch 1: Master L (-12dB)
            input[i * 4 + 1] = m_r; // Ch 2: Master R (-12dB)
            input[i * 4 + 2] = h_l; // Ch 3: Headphones L (-12dB)
            input[i * 4 + 3] = h_r; // Ch 4: Headphones R (-12dB)
        }
        size_t output_frames = p4_flx4_uac_resampler_process(
            &s_resampler, input, chunk, output, FLX4_RESAMPLE_OUTPUT_FRAMES);
        if (output_frames > 0u) {
            portENTER_CRITICAL(&s_flx4_mux);
            if (!s_connected) {
                portEXIT_CRITICAL(&s_flx4_mux);
                return ESP_ERR_INVALID_STATE;
            }
            uint32_t overflow_before = s_audio_ring.overflow_frames;
            uint32_t accepted = p4_flx4_audio_ring_write_clocked(
                &s_audio_ring, output, (uint32_t)output_frames);
            bool overflowed = s_audio_ring.overflow_frames != overflow_before;
            portEXIT_CRITICAL(&s_flx4_mux);
            atomic_fetch_add_explicit(&s_audio_submitted_frames, accepted,
                                      memory_order_relaxed);
            if (overflowed) {
                dropped = true;
            }
        }

        s_audio_total_frames += (uint32_t)output_frames;
        if (master_samples) master_samples += chunk * 2;
        if (hp_samples) hp_samples += chunk * 2;
        frame_count -= chunk;
    }
    if (dropped) {
        atomic_fetch_add_explicit(&s_audio_dropped_blocks, 1u, memory_order_relaxed);
    }
    atomic_fetch_add_explicit(&s_audio_submitted_blocks, 1u, memory_order_relaxed);
    return ESP_OK;
}

esp_err_t p4_flx4_host_write_headphone_audio(const int16_t *samples,
                                             size_t frame_count,
                                             uint32_t source_sample_rate)
{
    return p4_flx4_host_write_audio(NULL, samples, frame_count,
                                    source_sample_rate);
}

void p4_flx4_host_get_audio_stats(p4_flx4_audio_stats_t *out_stats)
{
    if (!out_stats) return;
    out_stats->submitted_blocks = (uint32_t)atomic_load_explicit(
        &s_audio_submitted_blocks, memory_order_relaxed);
    out_stats->dropped_blocks = (uint32_t)atomic_load_explicit(
        &s_audio_dropped_blocks, memory_order_relaxed);
    out_stats->submitted_frames = (uint32_t)atomic_load_explicit(
        &s_audio_submitted_frames, memory_order_relaxed);
    portENTER_CRITICAL(&s_flx4_mux);
    out_stats->ring_queued_frames = s_audio_ring.queued_frames;
    out_stats->ring_capacity_frames = s_audio_ring.frame_capacity;
    out_stats->ring_high_water_frames = s_audio_ring.high_water_frames;
    out_stats->overflow_frames = s_audio_ring.overflow_frames;
    out_stats->underflow_frames = s_audio_ring.underflow_frames;
    out_stats->clock_trimmed_frames = s_audio_ring.clock_trimmed_frames;
    out_stats->clock_duplicated_frames = s_audio_ring.clock_duplicated_frames;
    portEXIT_CRITICAL(&s_flx4_mux);
}

static uint8_t                  s_claimed_ifaces[8];
static int                      s_num_claimed_ifaces = 0;
static usb_device_handle_t      s_cleanup_dev_handle = NULL;
static bool                     s_disconnect_cleanup_pending = false;
static uint32_t                 s_disconnect_cleanup_attempts = 0u;

static void try_cleanup_disconnected_device(void)
{
    if (!s_disconnect_cleanup_pending || !s_cleanup_dev_handle) {
        return;
    }

    s_disconnect_cleanup_attempts++;
    int retained = 0;
    for (int i = 0; i < s_num_claimed_ifaces; ++i) {
        const uint8_t interface_number = s_claimed_ifaces[i];
        const esp_err_t err = usb_host_interface_release(
            s_client_handle, s_cleanup_dev_handle, interface_number);
        if (err == ESP_OK || err == ESP_ERR_NOT_FOUND) {
            ESP_LOGI(TAG, "Released disconnected FLX4 interface %u",
                     (unsigned)interface_number);
            continue;
        }

        s_claimed_ifaces[retained++] = interface_number;
        if (s_disconnect_cleanup_attempts == 1u ||
            (s_disconnect_cleanup_attempts % 100u) == 0u) {
            ESP_LOGW(TAG, "FLX4 interface %u cleanup pending: %s",
                     (unsigned)interface_number, esp_err_to_name(err));
        }
    }
    s_num_claimed_ifaces = retained;
    if (retained != 0) {
        return;
    }

    const esp_err_t close_err = usb_host_device_close(
        s_client_handle, s_cleanup_dev_handle);
    if (close_err != ESP_OK && close_err != ESP_ERR_NOT_FOUND) {
        if (s_disconnect_cleanup_attempts == 1u ||
            (s_disconnect_cleanup_attempts % 100u) == 0u) {
            ESP_LOGW(TAG, "FLX4 device cleanup pending: %s",
                     esp_err_to_name(close_err));
        }
        return;
    }

    ESP_LOGI(TAG, "Disconnected FLX4 device handle released after %" PRIu32
                  " cleanup attempts", s_disconnect_cleanup_attempts);
    if (s_dev_handle == s_cleanup_dev_handle) {
        s_dev_handle = NULL;
        s_dev_addr = 0;
    }
    s_cleanup_dev_handle = NULL;
    s_disconnect_cleanup_pending = false;
    s_disconnect_cleanup_attempts = 0u;
}

static void flx4_client_event_cb(const usb_host_client_event_msg_t *event_msg, void *arg)
{
    (void)arg;
    if (!event_msg) return;

    if (event_msg->event == USB_HOST_CLIENT_EVENT_NEW_DEV) {
        uint8_t addr = event_msg->new_dev.address;
        ESP_LOGW(TAG, "New USB device detected at addr %d", addr);
        usb_device_handle_t dev = NULL;
        if (usb_host_device_open(s_client_handle, addr, &dev) == ESP_OK) {
            const usb_device_desc_t *desc = NULL;
            if (usb_host_get_device_descriptor(dev, &desc) == ESP_OK && desc) {
                ESP_LOGW(TAG, "USB Device descriptor: VID=0x%04X PID=0x%04X (addr %d)", desc->idVendor, desc->idProduct, addr);
                if (desc->idVendor == FLX4_USB_VID && desc->idProduct == FLX4_USB_PID) {
                    ESP_LOGW(TAG, ">>> Pioneer DDJ-FLX4 MATCHED at USB addr %d <<<", addr);
                    s_dev_handle = dev;
                    s_dev_addr = addr;
                    s_num_claimed_ifaces = 0;

                    const usb_config_desc_t *config_desc = NULL;
                    if (usb_host_get_active_config_descriptor(dev, &config_desc) == ESP_OK && config_desc) {
                        ESP_LOGW(TAG, "FLX4 Config Desc: total_len=%d, num_intf=%d", config_desc->wTotalLength, config_desc->bNumInterfaces);
                        for (int i = 0; i < config_desc->bNumInterfaces; ++i) {
                            for (int alt = 0; alt < 4; ++alt) {
                                int offset = 0;
                                const usb_intf_desc_t *intf = usb_parse_interface_descriptor(config_desc, i, alt, &offset);
                                if (!intf) break;
                                ESP_LOGW(TAG, "FLX4 Intf %d (alt %d): class=0x%02x subclass=0x%02x eps=%d",
                                         intf->bInterfaceNumber, intf->bAlternateSetting, intf->bInterfaceClass, intf->bInterfaceSubClass, intf->bNumEndpoints);

                                // Print raw descriptor bytes following this interface to inspect Audio Format Type
                                if (intf->bInterfaceClass == 0x01 && intf->bInterfaceSubClass == 0x02) {
                                    const uint8_t *raw = (const uint8_t *)intf;
                                    int remaining = config_desc->wTotalLength - (raw - (const uint8_t *)config_desc);
                                    if (remaining > 64) remaining = 64;
                                    char hexbuf[128] = {0};
                                    for (int b = 0; b < remaining && b < 32; ++b) {
                                        snprintf(&hexbuf[b*3], sizeof(hexbuf) - b*3, "%02x ", raw[b]);
                                    }
                                    ESP_LOGW(TAG, "  Audio Intf %d alt %d RAW: %s", intf->bInterfaceNumber, intf->bAlternateSetting, hexbuf);
                                }

                                for (int ep_idx = 0; ep_idx < intf->bNumEndpoints; ++ep_idx) {
                                    int ep_offset = offset;
                                    const usb_ep_desc_t *ep = usb_parse_endpoint_descriptor_by_index(intf, ep_idx, config_desc->wTotalLength, &ep_offset);
                                    if (!ep) continue;
                                    ESP_LOGW(TAG, "  EP[%d]: addr=0x%02x attr=0x%02x mps=%d",
                                             ep_idx, ep->bEndpointAddress, ep->bmAttributes, ep->wMaxPacketSize);
                                }

                                if (intf->bInterfaceClass == 0x01 && intf->bInterfaceSubClass == 0x02 && intf->bInterfaceNumber == 1 && intf->bAlternateSetting == 2) {
                                    esp_err_t claim_err = usb_host_interface_claim(s_client_handle, dev, intf->bInterfaceNumber, intf->bAlternateSetting);
                                    if (claim_err == ESP_OK) {
                                        ESP_LOGW(TAG, "Claimed FLX4 Audio Streaming Intf 1 (alt 2 - 4-ch 16-bit)");
                                        if (s_num_claimed_ifaces < (int)(sizeof(s_claimed_ifaces))) {
                                            s_claimed_ifaces[s_num_claimed_ifaces++] = intf->bInterfaceNumber;
                                        }
                                        s_audio_claimed = true;
                                        for (int ep_idx = 0; ep_idx < intf->bNumEndpoints; ++ep_idx) {
                                            int ep_offset = offset;
                                            const usb_ep_desc_t *ep = usb_parse_endpoint_descriptor_by_index(intf, ep_idx, config_desc->wTotalLength, &ep_offset);
                                            if (ep && !USB_EP_DESC_GET_EP_DIR(ep)) {
                                                s_audio_ep_addr = ep->bEndpointAddress;
                                            }
                                        }
                                    } else {
                                        ESP_LOGW(TAG, "Claim Audio Streaming intf 1 alt 2 failed: %s", esp_err_to_name(claim_err));
                                    }
                                }

                                if (intf->bInterfaceClass == 0x01 && intf->bInterfaceSubClass == 0x03) {
                                    esp_err_t claim_err = usb_host_interface_claim(s_client_handle, dev, intf->bInterfaceNumber, intf->bAlternateSetting);
                                    if (claim_err == ESP_OK) {
                                        ESP_LOGW(TAG, "Claimed FLX4 MIDI Streaming Intf %d (alt %d)", intf->bInterfaceNumber, intf->bAlternateSetting);
                                        if (s_num_claimed_ifaces < (int)(sizeof(s_claimed_ifaces))) {
                                            s_claimed_ifaces[s_num_claimed_ifaces++] = intf->bInterfaceNumber;
                                        }
                                    } else {
                                        ESP_LOGW(TAG, "Claim MIDI intf %d failed: %s", intf->bInterfaceNumber, esp_err_to_name(claim_err));
                                    }

                                    for (int ep_idx = 0; ep_idx < intf->bNumEndpoints; ++ep_idx) {
                                        int ep_offset = offset;
                                        const usb_ep_desc_t *ep = usb_parse_endpoint_descriptor_by_index(intf, ep_idx, config_desc->wTotalLength, &ep_offset);
                                        if (!ep) continue;
                                        if (USB_EP_DESC_GET_EP_DIR(ep)) {
                                            s_in_ep_addr = ep->bEndpointAddress;
                                            s_in_mps = ep->wMaxPacketSize;
                                        } else {
                                            s_out_ep_addr = ep->bEndpointAddress;
                                            s_out_mps = ep->wMaxPacketSize;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    portENTER_CRITICAL(&s_flx4_mux);
                    s_connected = true;
                    flx4_map_init(&s_map_state);
                    p4_flx4_uac_packetizer_init(&s_packetizer, FLX4_UAC_SAMPLE_RATE,
                                                FLX4_UAC_CHANNELS,
                                                FLX4_UAC_BYTES_PER_SAMPLE);
                    p4_flx4_audio_ring_reset(&s_audio_ring, FLX4_UAC_SAMPLE_RATE);
                    atomic_store_explicit(&s_audio_submitted_blocks, 0u, memory_order_relaxed);
                    atomic_store_explicit(&s_audio_dropped_blocks, 0u, memory_order_relaxed);
                    atomic_store_explicit(&s_audio_submitted_frames, 0u, memory_order_relaxed);
                    s_audio_total_frames = 0u;
                    portEXIT_CRITICAL(&s_flx4_mux);

                    s_out_inflight = false;
                    p4_flx4_midi_gate_start(&s_midi_gate);

                    // Allocate transfers if needed
                    if (!s_in_xfer) {
                        (void)usb_host_transfer_alloc(s_in_mps, 0, &s_in_xfer);
                    }
                    if (!s_out_xfer) {
                        (void)usb_host_transfer_alloc(s_out_mps, 0, &s_out_xfer);
                    }

                    if (s_in_xfer) {
                        s_in_xfer->device_handle = s_dev_handle;
                        s_in_xfer->bEndpointAddress = s_in_ep_addr;
                        s_in_xfer->num_bytes = s_in_mps;
                        s_in_xfer->callback = in_transfer_cb;
                        esp_err_t sub_err = usb_host_transfer_submit(s_in_xfer);
                        ESP_LOGW(TAG, "FLX4 MIDI IN transfer submitted (EP 0x%02x, mps=%d): %s",
                                 s_in_ep_addr, s_in_mps, esp_err_to_name(sub_err));
                    }

                    // Configure FLX4 USB Audio DAC hardware and start streaming
                    if (s_audio_claimed) {
                        start_audio_config_sequence();
                    }

                    // Inform deck_core and UI that FLX4 is connected so it forces a complete LED snapshot!
                    esp_err_t inject_rc = control_link_inject_semantic(CTRL_TYPE_STATE, CTRL_ID_FLX4_CONNECTION, CTRL_FLX4_CONNECTED);
                    ESP_LOGW(TAG, "FLX4 inject CONNECTED: %s (queue=%p)", esp_err_to_name(inject_rc), (void*)s_out_queue);

                    if (s_conn_cb) {
                        s_conn_cb(true, s_conn_cb_ctx);
                    }
                    return;
                }
            }
            (void)usb_host_device_close(s_client_handle, dev);
        }
    } else if (event_msg->event == USB_HOST_CLIENT_EVENT_DEV_GONE) {
        if (s_dev_handle && event_msg->dev_gone.dev_hdl == s_dev_handle) {
            /* The periodic endpoint is already gone, so no isochronous
             * deadline remains. Lower before any gate/queue/callback work; the
             * audio task must not be descheduled by disconnect publication. */
            vTaskPrioritySet(NULL, FLX4_USB_TRANSITION_TASK_PRIO);

            ESP_LOGW(TAG, "Pioneer DDJ-FLX4 disconnected");
            p4_flx4_midi_gate_stop(&s_midi_gate);
            if (s_out_queue) {
                xQueueReset(s_out_queue);
            }

            portENTER_CRITICAL(&s_flx4_mux);
            s_connected = false;
            s_audio_claimed = false;
            portEXIT_CRITICAL(&s_flx4_mux);

            s_out_inflight = false;
            s_cleanup_dev_handle = s_dev_handle;
            s_disconnect_cleanup_pending = true;
            s_disconnect_cleanup_attempts = 0u;

            (void)control_link_inject_semantic(CTRL_TYPE_STATE, CTRL_ID_FLX4_CONNECTION, CTRL_FLX4_DISCONNECTED);

            if (s_conn_cb) {
                s_conn_cb(false, s_conn_cb_ctx);
            }
        }
    }
}

static void flx4_midi_task(void *arg)
{
    (void)arg;
    while (1) {
        if (s_client_handle) {
            (void)usb_host_client_handle_events(s_client_handle, pdMS_TO_TICKS(10));
            /* DEV_GONE completion callbacks must run before claimed interfaces
             * can be released. Retrying here, outside the client callback,
             * lets canceled MIDI/UAC URBs retire before closing the device. */
            try_cleanup_disconnected_device();
            try_submit_next_out();
        } else {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

esp_err_t p4_flx4_host_init(void)
{
    if (s_client_handle) return ESP_OK;

    if (!s_out_queue) {
        s_out_queue = xQueueCreate(128, sizeof(p4_flx4_out_item_t));
    }
    control_link_set_led_sink(flx4_control_link_led_sink, NULL);
    p4_flx4_midi_gate_init(&s_midi_gate);
    p4_flx4_uac_packetizer_init(&s_packetizer, FLX4_UAC_SAMPLE_RATE, FLX4_UAC_CHANNELS, FLX4_UAC_BYTES_PER_SAMPLE);
    p4_flx4_audio_ring_init(&s_audio_ring, s_audio_storage, FLX4_AUDIO_RING_FRAMES, FLX4_UAC_CHANNELS, FLX4_UAC_SAMPLE_RATE);

    const usb_host_client_config_t client_cfg = {
        .is_synchronous = false,
        .max_num_event_msg = 10,
        .async = {
            .client_event_callback = flx4_client_event_cb,
            .callback_arg = NULL,
        },
    };

    esp_err_t err = ESP_FAIL;
    for (int retry = 0; retry < 20; ++retry) {
        err = usb_host_client_register(&client_cfg, &s_client_handle);
        if (err == ESP_OK) break;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "usb_host_client_register failed: %s", esp_err_to_name(err));
        return err;
    }

    if (xTaskCreatePinnedToCore(flx4_midi_task, "flx4_usb", FLX4_USB_TASK_STACK,
                                NULL, FLX4_USB_TRANSITION_TASK_PRIO,
                                &s_midi_task, 0) != pdPASS) {
        ESP_LOGE(TAG, "failed to create flx4_usb task");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "P4 FLX4 USB Host client (MIDI + UAC Audio) registered");
    return ESP_OK;
}

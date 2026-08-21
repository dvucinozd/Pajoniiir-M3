#include "p4_flx4_host.h"
#include "p4_flx4_map.h"
#include "p4_flx4_uac.h"
#include "p4_flx4_midi_gate.h"
#include "control_link.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "usb/usb_host.h"

#include <string.h>

static const char *TAG = "p4_flx4";

#define FLX4_MIDI_TASK_STACK  4096
#define FLX4_MIDI_TASK_PRIO   5

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
            ESP_LOGW(TAG, "FLX4 RAW PKT: %02x %02x %02x %02x", data[i], data[i+1], data[i+2], data[i+3]);
            flx4_midi_message_t msg;
            if (flx4_midi_parse_usb_packet(&data[i], &msg)) {
                flx4_control_event_t ev;
                portENTER_CRITICAL(&s_flx4_mux);
                bool translated = flx4_map_translate_message(&s_map_state, &msg, &ev);
                portEXIT_CRITICAL(&s_flx4_mux);
                if (translated) {
                    ESP_LOGW(TAG, "FLX4 EVENT RX: type=%d id=%d value=%d", ev.type, ev.id, ev.value);
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

static QueueHandle_t            s_out_queue     = NULL;
static volatile bool            s_out_inflight  = false;

static void out_transfer_cb(usb_transfer_t *transfer);

static void try_submit_next_out(void)
{
    if (s_out_inflight || !s_out_xfer || !s_dev_handle || !s_connected || !s_out_queue) {
        return;
    }
    uint8_t packet[4];
    if (xQueueReceive(s_out_queue, packet, 0) == pdTRUE) {
        memcpy(s_out_xfer->data_buffer, packet, 4);
        s_out_xfer->device_handle = s_dev_handle;
        s_out_xfer->bEndpointAddress = s_out_ep_addr;
        s_out_xfer->num_bytes = 4;
        s_out_xfer->callback = out_transfer_cb;
        s_out_inflight = true;
        esp_err_t err = usb_host_transfer_submit(s_out_xfer);
        if (err != ESP_OK) {
            s_out_inflight = false;
            ESP_LOGW(TAG, "Failed to submit OUT transfer: %s", esp_err_to_name(err));
        } else {
            ESP_LOGW(TAG, "FLX4 LED TX: %02x %02x %02x %02x", packet[0], packet[1], packet[2], packet[3]);
        }
    }
}

static void out_transfer_cb(usb_transfer_t *transfer)
{
    (void)transfer;
    s_out_inflight = false;
    try_submit_next_out();
}

esp_err_t p4_flx4_host_send_packet(const uint8_t packet[4])
{
    if (!packet || !s_connected || !s_out_queue) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xQueueSend(s_out_queue, packet, 0) != pdTRUE) {
        return ESP_ERR_NO_MEM;
    }

    if (s_client_handle) {
        (void)usb_host_client_unblock(s_client_handle);
    }
    try_submit_next_out();
    return ESP_OK;
}

esp_err_t p4_flx4_host_send_led(uint8_t led, uint8_t state, uint8_t deck)
{
    uint8_t packet[4] = {0};
    if (!flx4_led_midi_build_packet(led, state, deck, packet)) {
        return ESP_ERR_INVALID_ARG;
    }
    return p4_flx4_host_send_packet(packet);
}

#define FLX4_ISOC_XFERS_NUM        3
#define FLX4_ISOC_PACKETS_PER_XFER 4
#define FLX4_ISOC_MAX_PACKET_BYTES 384

static usb_transfer_t          *s_isoc_xfers[FLX4_ISOC_XFERS_NUM] = { NULL };
static bool                     s_audio_claimed = false;
static uint8_t                  s_audio_ep_addr = 0x01;
static uint32_t                 s_isoc_total_transfers = 0;
static uint32_t                 s_audio_total_frames = 0;

static void isoc_transfer_cb(usb_transfer_t *transfer);

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
            ESP_LOGI(TAG, "FLX4 ISOC audio alive: %" PRIu32 " transfers, %" PRIu32 " frames pushed to ring",
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

esp_err_t p4_flx4_host_write_audio(const int16_t *master_samples, const int16_t *hp_samples, size_t frame_count)
{
    if ((!master_samples && !hp_samples) || frame_count == 0) return ESP_ERR_INVALID_ARG;
    if (!p4_flx4_host_is_connected()) return ESP_ERR_INVALID_STATE;

    int16_t temp[128 * 4];
    while (frame_count > 0) {
        size_t chunk = frame_count > 128 ? 128 : frame_count;
        for (size_t i = 0; i < chunk; ++i) {
            int16_t m_l = master_samples ? master_samples[i * 2 + 0] : 0;
            int16_t m_r = master_samples ? master_samples[i * 2 + 1] : 0;
            int16_t h_l = hp_samples ? hp_samples[i * 2 + 0] : m_l;
            int16_t h_r = hp_samples ? hp_samples[i * 2 + 1] : m_r;

            temp[i * 4 + 0] = m_l; // Ch 1: Master L
            temp[i * 4 + 1] = m_r; // Ch 2: Master R
            temp[i * 4 + 2] = h_l; // Ch 3: Headphones L
            temp[i * 4 + 3] = h_r; // Ch 4: Headphones R
        }
        portENTER_CRITICAL(&s_flx4_mux);
        (void)p4_flx4_audio_ring_write(&s_audio_ring, temp, (uint32_t)chunk);
        portEXIT_CRITICAL(&s_flx4_mux);

        s_audio_total_frames += (uint32_t)chunk;
        if (master_samples) master_samples += chunk * 2;
        if (hp_samples) hp_samples += chunk * 2;
        frame_count -= chunk;
    }
    return ESP_OK;
}

esp_err_t p4_flx4_host_write_headphone_audio(const int16_t *samples, size_t frame_count)
{
    return p4_flx4_host_write_audio(NULL, samples, frame_count);
}

static uint8_t                  s_claimed_ifaces[8];
static int                      s_num_claimed_ifaces = 0;

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
                    p4_flx4_audio_ring_reset(&s_audio_ring, FLX4_UAC_SAMPLE_RATE);
                    portEXIT_CRITICAL(&s_flx4_mux);

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

                    // Start Isochronous Audio Streaming
                    if (s_audio_claimed) {
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
                        ESP_LOGW(TAG, "FLX4 Isochronous Audio streaming started on EP 0x%02x (4-channel 44.1 kHz 16-bit)", s_audio_ep_addr);
                    }

                    // Send test LEDs: Turn ON Play and Cue on Deck 1 and Deck 2!
                    (void)p4_flx4_host_send_led(LED_PLAY, 1, CTRL_DECK_1);
                    (void)p4_flx4_host_send_led(LED_CUE, 1, CTRL_DECK_1);
                    (void)p4_flx4_host_send_led(LED_PLAY, 1, CTRL_DECK_2);
                    (void)p4_flx4_host_send_led(LED_CUE, 1, CTRL_DECK_2);

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
            ESP_LOGW(TAG, "Pioneer DDJ-FLX4 disconnected");
            p4_flx4_midi_gate_stop(&s_midi_gate);

            portENTER_CRITICAL(&s_flx4_mux);
            s_connected = false;
            s_audio_claimed = false;
            portEXIT_CRITICAL(&s_flx4_mux);

            for (int i = 0; i < s_num_claimed_ifaces; ++i) {
                (void)usb_host_interface_release(s_client_handle, s_dev_handle, s_claimed_ifaces[i]);
            }
            s_num_claimed_ifaces = 0;

            (void)usb_host_device_close(s_client_handle, s_dev_handle);
            s_dev_handle = NULL;
            s_dev_addr = 0;

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
        s_out_queue = xQueueCreate(128, 4);
    }
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

    if (xTaskCreatePinnedToCore(flx4_midi_task, "flx4_midi", FLX4_MIDI_TASK_STACK,
                                NULL, FLX4_MIDI_TASK_PRIO, &s_midi_task, 0) != pdPASS) {
        ESP_LOGE(TAG, "failed to create flx4_midi task");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "P4 FLX4 USB Host client (MIDI + UAC Audio) registered");
    return ESP_OK;
}

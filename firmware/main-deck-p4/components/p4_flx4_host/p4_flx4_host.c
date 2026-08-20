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
#define FLX4_AUDIO_RING_FRAMES 2048

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
        // Resubmit transfer
        if (s_dev_handle && s_connected) {
            (void)usb_host_transfer_submit(transfer);
        }
    }
}

static void out_transfer_cb(usb_transfer_t *transfer)
{
    (void)transfer;
}

esp_err_t p4_flx4_host_send_packet(const uint8_t packet[4])
{
    if (!packet || !s_dev_handle || !s_connected || !s_out_xfer) {
        return ESP_ERR_INVALID_STATE;
    }

    uint32_t gen = 0;
    if (!p4_flx4_midi_gate_begin(&s_midi_gate, &gen)) {
        return ESP_ERR_INVALID_STATE;
    }

    memcpy(s_out_xfer->data_buffer, packet, 4);
    s_out_xfer->num_bytes = 4;
    s_out_xfer->device_handle = s_dev_handle;
    s_out_xfer->bEndpointAddress = s_out_ep_addr;
    s_out_xfer->callback = out_transfer_cb;
    s_out_xfer->context = NULL;

    esp_err_t err = usb_host_transfer_submit(s_out_xfer);
    p4_flx4_midi_gate_end(&s_midi_gate);
    return err;
}

esp_err_t p4_flx4_host_send_led(uint8_t led, uint8_t state, uint8_t deck)
{
    uint8_t packet[4];
    if (flx4_led_midi_build_packet(led, state, deck, packet)) {
        return p4_flx4_host_send_packet(packet);
    }
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t p4_flx4_host_write_headphone_audio(const int16_t *samples, size_t frame_count)
{
    if (!samples || frame_count == 0) return ESP_ERR_INVALID_ARG;
    if (!p4_flx4_host_is_connected()) return ESP_ERR_INVALID_STATE;

    portENTER_CRITICAL(&s_flx4_mux);
    (void)p4_flx4_audio_ring_write(&s_audio_ring, samples, (uint32_t)frame_count);
    portEXIT_CRITICAL(&s_flx4_mux);

    return ESP_OK;
}

static void flx4_client_event_cb(const usb_host_client_event_msg_t *event_msg, void *arg)
{
    (void)arg;
    if (!event_msg) return;

    if (event_msg->event == USB_HOST_CLIENT_EVENT_NEW_DEV) {
        uint8_t addr = event_msg->new_dev.address;
        usb_device_handle_t dev = NULL;
        if (usb_host_device_open(s_client_handle, addr, &dev) == ESP_OK) {
            const usb_device_desc_t *desc = NULL;
            if (usb_host_get_device_descriptor(dev, &desc) == ESP_OK && desc) {
                if (desc->idVendor == FLX4_USB_VID && desc->idProduct == FLX4_USB_PID) {
                    ESP_LOGI(TAG, "Pioneer DDJ-FLX4 detected at USB address %d", addr);
                    s_dev_handle = dev;
                    s_dev_addr = addr;

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
                        (void)usb_host_transfer_submit(s_in_xfer);
                    }

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
            ESP_LOGI(TAG, "Pioneer DDJ-FLX4 disconnected");
            p4_flx4_midi_gate_stop(&s_midi_gate);

            portENTER_CRITICAL(&s_flx4_mux);
            s_connected = false;
            portEXIT_CRITICAL(&s_flx4_mux);

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
            (void)usb_host_client_handle_events(s_client_handle, portMAX_DELAY);
        } else {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

esp_err_t p4_flx4_host_init(void)
{
    if (s_client_handle) return ESP_OK;

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

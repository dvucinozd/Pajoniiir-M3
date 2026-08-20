#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#define FLX4_USB_VID 0x2B73u
#define FLX4_USB_PID 0x0045u

typedef void (*p4_flx4_connection_cb_t)(bool connected, void *user_ctx);

esp_err_t p4_flx4_host_init(void);
bool      p4_flx4_host_is_connected(void);
esp_err_t p4_flx4_host_send_led(uint8_t led, uint8_t state, uint8_t deck);
esp_err_t p4_flx4_host_send_packet(const uint8_t packet[4]);
void      p4_flx4_host_set_connection_callback(p4_flx4_connection_cb_t cb, void *user_ctx);

// Direct audio streaming to FLX4 USB Headphone DAC (44.1 kHz stereo 16-bit)
esp_err_t p4_flx4_host_write_headphone_audio(const int16_t *samples, size_t frame_count);

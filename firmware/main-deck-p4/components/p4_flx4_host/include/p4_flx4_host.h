#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#define FLX4_USB_VID 0x2B73u
#define FLX4_USB_PID 0x0045u

typedef void (*p4_flx4_connection_cb_t)(bool connected, void *user_ctx);

typedef struct {
    uint32_t submitted_blocks;
    uint32_t dropped_blocks;
    uint32_t submitted_frames;
    uint32_t ring_queued_frames;
    uint32_t ring_capacity_frames;
    uint32_t ring_high_water_frames;
    uint32_t overflow_frames;
    uint32_t underflow_frames;
    uint32_t clock_trimmed_frames;
    uint32_t clock_duplicated_frames;
} p4_flx4_audio_stats_t;

esp_err_t p4_flx4_host_init(void);
bool      p4_flx4_host_is_connected(void);
esp_err_t p4_flx4_host_send_led(uint8_t led, uint8_t state, uint8_t deck);
esp_err_t p4_flx4_host_send_packet(const uint8_t packet[4]);
void      p4_flx4_host_set_connection_callback(p4_flx4_connection_cb_t cb, void *user_ctx);

// Direct audio streaming to FLX4 USB Audio DAC (44.1 kHz 4-channel 16-bit: Ch1/2 Master, Ch3/4 Headphones)
esp_err_t p4_flx4_host_write_audio(const int16_t *master_samples, const int16_t *hp_samples, size_t frame_count);
esp_err_t p4_flx4_host_write_headphone_audio(const int16_t *samples, size_t frame_count);
void      p4_flx4_host_get_audio_stats(p4_flx4_audio_stats_t *out_stats);

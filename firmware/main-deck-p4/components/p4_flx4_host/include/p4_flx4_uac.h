#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define FLX4_UAC_SAMPLE_RATE 44100u
#define FLX4_UAC_CHANNELS    2u
#define FLX4_UAC_BITS        16u
#define FLX4_UAC_BYTES_PER_SAMPLE 2u

// UAC1 Packetizer (handles 44.1 kHz fractional frames: 44 or 45 frames per 1 ms USB packet)
typedef struct {
    uint32_t sample_rate;
    uint8_t  channels;
    uint8_t  bytes_per_sample;
    uint32_t frame_accum;
} p4_flx4_uac_packetizer_t;

void     p4_flx4_uac_packetizer_init(p4_flx4_uac_packetizer_t *p, uint32_t sample_rate, uint8_t channels, uint8_t bytes_per_sample);
uint16_t p4_flx4_uac_packetizer_next_frames(p4_flx4_uac_packetizer_t *p);
size_t   p4_flx4_uac_packetizer_next_bytes(p4_flx4_uac_packetizer_t *p);

// Audio Ring Buffer for Isochronous streaming
typedef struct {
    int16_t *samples;
    uint32_t frame_capacity;
    uint8_t  channels;
    uint32_t sample_rate;
    uint32_t read_frame;
    uint32_t write_frame;
    uint32_t queued_frames;
    uint32_t generation;
} p4_flx4_audio_ring_t;

bool     p4_flx4_audio_ring_init(p4_flx4_audio_ring_t *ring, int16_t *storage, uint32_t frame_capacity, uint8_t channels, uint32_t sample_rate);
void     p4_flx4_audio_ring_reset(p4_flx4_audio_ring_t *ring, uint32_t sample_rate);
uint32_t p4_flx4_audio_ring_write(p4_flx4_audio_ring_t *ring, const int16_t *interleaved, uint32_t frames);
uint32_t p4_flx4_audio_ring_read(p4_flx4_audio_ring_t *ring, int16_t *interleaved, uint32_t frames, bool zero_fill);
uint32_t p4_flx4_audio_ring_queued(const p4_flx4_audio_ring_t *ring);
uint32_t p4_flx4_audio_ring_free(const p4_flx4_audio_ring_t *ring);

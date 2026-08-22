#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define FLX4_UAC_SAMPLE_RATE 44100u
#define FLX4_UAC_CHANNELS    4u
#define FLX4_UAC_BITS        16u
#define FLX4_UAC_BYTES_PER_SAMPLE 2u
#define FLX4_AUDIO_RING_FRAMES 2048u

// UAC1 Packetizer (handles 44.1 kHz fractional frames: 44 or 45 frames per 1 ms USB packet)
typedef struct {
    uint32_t sample_rate;
    uint8_t  channels;
    uint8_t  bytes_per_sample;
    uint32_t frame_accum;
} p4_flx4_uac_packetizer_t;

/* Stateful linear sample-rate conversion for the interleaved four-channel
 * FLX4 stream. Time is tracked as an exact rational so splitting one source
 * stream across arbitrary audio blocks cannot accumulate phase drift. */
typedef struct {
    uint32_t source_rate;
    uint32_t target_rate;
    uint64_t input_frames_seen;
    uint64_t next_output_time;
    uint8_t channels;
    bool has_previous;
    int16_t previous[8];
} p4_flx4_uac_resampler_t;

void     p4_flx4_uac_packetizer_init(p4_flx4_uac_packetizer_t *p, uint32_t sample_rate, uint8_t channels, uint8_t bytes_per_sample);
uint16_t p4_flx4_uac_packetizer_next_frames(p4_flx4_uac_packetizer_t *p);
size_t   p4_flx4_uac_packetizer_next_bytes(p4_flx4_uac_packetizer_t *p);

bool p4_flx4_uac_resampler_init(p4_flx4_uac_resampler_t *resampler,
                                uint32_t source_rate,
                                uint32_t target_rate,
                                uint8_t channels);
size_t p4_flx4_uac_resampler_output_bound(uint32_t source_rate,
                                          uint32_t target_rate,
                                          size_t input_frames);
size_t p4_flx4_uac_resampler_process(p4_flx4_uac_resampler_t *resampler,
                                     const int16_t *input,
                                     size_t input_frames,
                                     int16_t *output,
                                     size_t output_capacity_frames);

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
    uint32_t high_water_frames;
    uint32_t overflow_frames;
    uint32_t underflow_frames;
    uint32_t clock_trimmed_frames;
    uint32_t clock_duplicated_frames;
} p4_flx4_audio_ring_t;

bool     p4_flx4_audio_ring_init(p4_flx4_audio_ring_t *ring, int16_t *storage, uint32_t frame_capacity, uint8_t channels, uint32_t sample_rate);
void     p4_flx4_audio_ring_reset(p4_flx4_audio_ring_t *ring, uint32_t sample_rate);
uint32_t p4_flx4_audio_ring_write(p4_flx4_audio_ring_t *ring, const int16_t *interleaved, uint32_t frames);
uint32_t p4_flx4_audio_ring_write_clocked(p4_flx4_audio_ring_t *ring, const int16_t *interleaved, uint32_t frames);
uint32_t p4_flx4_audio_ring_read(p4_flx4_audio_ring_t *ring, int16_t *interleaved, uint32_t frames, bool zero_fill);
uint32_t p4_flx4_audio_ring_queued(const p4_flx4_audio_ring_t *ring);
uint32_t p4_flx4_audio_ring_free(const p4_flx4_audio_ring_t *ring);

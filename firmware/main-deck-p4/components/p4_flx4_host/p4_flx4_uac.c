#include "p4_flx4_uac.h"
#include <string.h>

void p4_flx4_uac_packetizer_init(p4_flx4_uac_packetizer_t *p,
                                uint32_t sample_rate,
                                uint8_t channels,
                                uint8_t bytes_per_sample)
{
    if (!p) return;
    p->sample_rate = sample_rate;
    p->channels = channels;
    p->bytes_per_sample = bytes_per_sample;
    p->frame_accum = 0u;
}

uint16_t p4_flx4_uac_packetizer_next_frames(p4_flx4_uac_packetizer_t *p)
{
    if (!p || p->sample_rate == 0u) return 0u;
    p->frame_accum += p->sample_rate;
    const uint16_t frames = (uint16_t)(p->frame_accum / 1000u);
    p->frame_accum -= (uint32_t)frames * 1000u;
    return frames;
}

size_t p4_flx4_uac_packetizer_next_bytes(p4_flx4_uac_packetizer_t *p)
{
    const uint16_t frames = p4_flx4_uac_packetizer_next_frames(p);
    if (!p) return 0u;
    return (size_t)frames * (size_t)p->channels * (size_t)p->bytes_per_sample;
}

bool p4_flx4_audio_ring_init(p4_flx4_audio_ring_t *ring,
                             int16_t *storage,
                             uint32_t frame_capacity,
                             uint8_t channels,
                             uint32_t sample_rate)
{
    if (!ring || !storage || frame_capacity == 0u || channels == 0u || channels > 8u || sample_rate == 0u) {
        return false;
    }
    memset(ring, 0, sizeof(*ring));
    ring->samples = storage;
    ring->frame_capacity = frame_capacity;
    ring->channels = channels;
    ring->sample_rate = sample_rate;
    ring->generation = 1u;
    return true;
}

void p4_flx4_audio_ring_reset(p4_flx4_audio_ring_t *ring, uint32_t sample_rate)
{
    if (!ring || !ring->samples || sample_rate == 0u) return;
    ring->read_frame = 0u;
    ring->write_frame = 0u;
    ring->queued_frames = 0u;
    ring->sample_rate = sample_rate;
    ring->generation++;
    ring->high_water_frames = 0u;
    ring->overflow_frames = 0u;
    ring->underflow_frames = 0u;
    ring->clock_trimmed_frames = 0u;
    ring->clock_duplicated_frames = 0u;
}

uint32_t p4_flx4_audio_ring_write(p4_flx4_audio_ring_t *ring,
                                  const int16_t *interleaved,
                                  uint32_t frames)
{
    if (!ring || !ring->samples || !interleaved || frames == 0u) return 0u;
    const uint32_t free_frames = p4_flx4_audio_ring_free(ring);
    const uint32_t accepted = frames < free_frames ? frames : free_frames;
    for (uint32_t frame = 0u; frame < accepted; ++frame) {
        const uint32_t dst_frame = (ring->write_frame + frame) % ring->frame_capacity;
        memcpy(&ring->samples[(size_t)dst_frame * ring->channels],
               &interleaved[(size_t)frame * ring->channels],
               (size_t)ring->channels * sizeof(int16_t));
    }
    ring->write_frame = (ring->write_frame + accepted) % ring->frame_capacity;
    ring->queued_frames += accepted;
    if (ring->queued_frames > ring->high_water_frames) {
        ring->high_water_frames = ring->queued_frames;
    }
    ring->overflow_frames += frames - accepted;
    return accepted;
}

uint32_t p4_flx4_audio_ring_write_clocked(p4_flx4_audio_ring_t *ring,
                                          const int16_t *interleaved,
                                          uint32_t frames)
{
    if (!ring || !ring->samples || !interleaved || frames == 0u) return 0u;

    /* The PCM5102A/I2S producer and FLX4 USB SOF consumer use independent
     * clocks. Keep a wide dead band around half-full and slip at most one
     * frame per producer block outside it. This turns an eventual 128-frame
     * overflow/underflow into sparse single-frame corrections. */
    const uint32_t low_water = (ring->frame_capacity * 3u) / 8u;
    const uint32_t high_water = (ring->frame_capacity * 5u) / 8u;
    const uint32_t free_frames = p4_flx4_audio_ring_free(ring);

    if (ring->queued_frames >= high_water && frames > 1u) {
        ring->clock_trimmed_frames++;
        return p4_flx4_audio_ring_write(ring, interleaved, frames - 1u);
    }

    const bool duplicate = ring->queued_frames <= low_water && free_frames > frames;
    uint32_t written = p4_flx4_audio_ring_write(ring, interleaved, frames);
    if (duplicate && written == frames) {
        const int16_t *last = &interleaved[(size_t)(frames - 1u) * ring->channels];
        if (p4_flx4_audio_ring_write(ring, last, 1u) == 1u) {
            ring->clock_duplicated_frames++;
            written++;
        }
    }
    return written;
}

uint32_t p4_flx4_audio_ring_read(p4_flx4_audio_ring_t *ring,
                                 int16_t *interleaved,
                                 uint32_t frames,
                                 bool zero_fill)
{
    if (!ring || !ring->samples || !interleaved || frames == 0u) return 0u;
    const uint32_t available = frames < ring->queued_frames ? frames : ring->queued_frames;
    for (uint32_t frame = 0u; frame < available; ++frame) {
        const uint32_t src_frame = (ring->read_frame + frame) % ring->frame_capacity;
        memcpy(&interleaved[(size_t)frame * ring->channels],
               &ring->samples[(size_t)src_frame * ring->channels],
               (size_t)ring->channels * sizeof(int16_t));
    }
    if (zero_fill && available < frames) {
        memset(&interleaved[(size_t)available * ring->channels], 0,
               (size_t)(frames - available) * ring->channels * sizeof(int16_t));
    }
    ring->read_frame = (ring->read_frame + available) % ring->frame_capacity;
    ring->queued_frames -= available;
    ring->underflow_frames += frames - available;
    return available;
}

uint32_t p4_flx4_audio_ring_queued(const p4_flx4_audio_ring_t *ring)
{
    return ring ? ring->queued_frames : 0u;
}

uint32_t p4_flx4_audio_ring_free(const p4_flx4_audio_ring_t *ring)
{
    return ring && ring->frame_capacity >= ring->queued_frames
               ? ring->frame_capacity - ring->queued_frames
               : 0u;
}

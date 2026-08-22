#include "p4_flx4_uac.h"
#include "test_support.h"

#include <string.h>

static void test_packetizer(void)
{
    p4_flx4_uac_packetizer_t packetizer;
    p4_flx4_uac_packetizer_init(&packetizer, 44100u, 4u, 2u);

    uint32_t total = 0u;
    uint32_t packets_44 = 0u;
    uint32_t packets_45 = 0u;
    for (unsigned i = 0u; i < 1000u; ++i) {
        uint16_t frames = p4_flx4_uac_packetizer_next_frames(&packetizer);
        CHECK(frames == 44u || frames == 45u);
        total += frames;
        packets_44 += frames == 44u;
        packets_45 += frames == 45u;
    }
    CHECK_EQ(total, 44100u);
    CHECK_EQ(packets_44, 900u);
    CHECK_EQ(packets_45, 100u);

    p4_flx4_uac_packetizer_init(&packetizer, 48000u, 4u, 2u);
    CHECK_EQ(p4_flx4_uac_packetizer_next_frames(&packetizer), 48u);
    CHECK_EQ(p4_flx4_uac_packetizer_next_bytes(&packetizer), 384u);
    CHECK_EQ(p4_flx4_uac_packetizer_next_frames(NULL), 0u);
    CHECK_EQ(p4_flx4_uac_packetizer_next_bytes(NULL), 0u);
    p4_flx4_uac_packetizer_init(&packetizer, 0u, 4u, 2u);
    CHECK_EQ(p4_flx4_uac_packetizer_next_bytes(&packetizer), 0u);
}

static void test_resampler_44100_passthrough(void)
{
    enum { FRAMES = 257, CHANNELS = 4 };
    int16_t input[FRAMES * CHANNELS];
    int16_t output[(FRAMES + 1) * CHANNELS];
    for (unsigned i = 0u; i < FRAMES * CHANNELS; ++i) {
        input[i] = (int16_t)((int)i * 13 - 6000);
    }
    memset(output, 0, sizeof(output));

    p4_flx4_uac_resampler_t resampler;
    CHECK(p4_flx4_uac_resampler_init(&resampler, 44100u, 44100u, CHANNELS));
    CHECK_EQ(p4_flx4_uac_resampler_output_bound(44100u, 44100u, FRAMES),
             FRAMES + 1u);
    CHECK_EQ(p4_flx4_uac_resampler_process(
                 &resampler, input, FRAMES, output, FRAMES + 1u),
             FRAMES);
    CHECK(memcmp(input, output, sizeof(input)) == 0);
}

static void fill_resampler_ramp(int16_t *frames,
                                size_t first_frame,
                                size_t frame_count)
{
    for (size_t frame = 0u; frame < frame_count; ++frame) {
        for (size_t channel = 0u; channel < 4u; ++channel) {
            frames[frame * 4u + channel] = (int16_t)(
                ((first_frame + frame) * 31u + channel * 7u) & 0x7fffu);
        }
    }
}

static void test_resampler_48000_ratio_and_block_continuity(void)
{
    enum { FRAMES = 480, FIRST = 127, CHANNELS = 4, OUTPUT_CAPACITY = 442 };
    int16_t input[FRAMES * CHANNELS];
    int16_t whole[OUTPUT_CAPACITY * CHANNELS];
    int16_t split[OUTPUT_CAPACITY * CHANNELS];
    fill_resampler_ramp(input, 0u, FRAMES);
    memset(whole, 0, sizeof(whole));
    memset(split, 0, sizeof(split));

    p4_flx4_uac_resampler_t whole_resampler;
    CHECK(p4_flx4_uac_resampler_init(
        &whole_resampler, 48000u, 44100u, CHANNELS));
    size_t whole_frames = p4_flx4_uac_resampler_process(
        &whole_resampler, input, FRAMES, whole, OUTPUT_CAPACITY);
    CHECK_EQ(whole_frames, 441u);

    p4_flx4_uac_resampler_t split_resampler;
    CHECK(p4_flx4_uac_resampler_init(
        &split_resampler, 48000u, 44100u, CHANNELS));
    size_t split_first = p4_flx4_uac_resampler_process(
        &split_resampler, input, FIRST, split, OUTPUT_CAPACITY);
    size_t split_second = p4_flx4_uac_resampler_process(
        &split_resampler,
        &input[FIRST * CHANNELS],
        FRAMES - FIRST,
        &split[split_first * CHANNELS],
        OUTPUT_CAPACITY - split_first);
    CHECK_EQ(split_first + split_second, whole_frames);
    CHECK(memcmp(whole, split,
                 whole_frames * CHANNELS * sizeof(*whole)) == 0);
}

static void test_resampler_48000_exact_one_second(void)
{
    enum { INPUT_CHUNK = 128, OUTPUT_CAPACITY = 129, CHANNELS = 4 };
    int16_t input[INPUT_CHUNK * CHANNELS];
    int16_t output[OUTPUT_CAPACITY * CHANNELS];
    p4_flx4_uac_resampler_t resampler;
    CHECK(p4_flx4_uac_resampler_init(&resampler, 48000u, 44100u, CHANNELS));

    size_t source_frame = 0u;
    size_t total_output = 0u;
    while (source_frame < 48000u) {
        size_t chunk = 48000u - source_frame;
        if (chunk > INPUT_CHUNK) chunk = INPUT_CHUNK;
        fill_resampler_ramp(input, source_frame, chunk);
        total_output += p4_flx4_uac_resampler_process(
            &resampler, input, chunk, output, OUTPUT_CAPACITY);
        source_frame += chunk;
    }
    CHECK_EQ(total_output, 44100u);
}

static void test_resampler_guards(void)
{
    p4_flx4_uac_resampler_t resampler;
    int16_t input[4u * 4u] = { 0 };
    int16_t output[6u * 4u] = { 0 };
    CHECK(!p4_flx4_uac_resampler_init(NULL, 48000u, 44100u, 4u));
    CHECK(!p4_flx4_uac_resampler_init(&resampler, 0u, 44100u, 4u));
    CHECK(!p4_flx4_uac_resampler_init(&resampler, 48000u, 0u, 4u));
    CHECK(!p4_flx4_uac_resampler_init(&resampler, 48000u, 44100u, 0u));
    CHECK(!p4_flx4_uac_resampler_init(&resampler, 48000u, 44100u, 9u));
    CHECK_EQ(p4_flx4_uac_resampler_output_bound(0u, 44100u, 4u), 0u);
    CHECK_EQ(p4_flx4_uac_resampler_output_bound(48000u, 0u, 4u), 0u);
    CHECK_EQ(p4_flx4_uac_resampler_output_bound(48000u, 44100u, 0u), 0u);

    CHECK(p4_flx4_uac_resampler_init(&resampler, 48000u, 44100u, 4u));
    size_t bound = p4_flx4_uac_resampler_output_bound(48000u, 44100u, 4u);
    CHECK(bound > 0u);
    CHECK_EQ(p4_flx4_uac_resampler_process(
                 &resampler, input, 4u, output, bound - 1u),
             0u);
    CHECK(!resampler.has_previous);
    CHECK_EQ(p4_flx4_uac_resampler_process(NULL, input, 4u, output, bound), 0u);
    CHECK_EQ(p4_flx4_uac_resampler_process(&resampler, NULL, 4u, output, bound), 0u);
    CHECK_EQ(p4_flx4_uac_resampler_process(&resampler, input, 0u, output, bound), 0u);
    CHECK_EQ(p4_flx4_uac_resampler_process(&resampler, input, 4u, NULL, bound), 0u);
}

static void expect_frame(const int16_t *samples,
                         unsigned frame,
                         int16_t a,
                         int16_t b,
                         int16_t c,
                         int16_t d)
{
    CHECK_EQ(samples[frame * 4u + 0u], a);
    CHECK_EQ(samples[frame * 4u + 1u], b);
    CHECK_EQ(samples[frame * 4u + 2u], c);
    CHECK_EQ(samples[frame * 4u + 3u], d);
}

static void test_audio_ring_fifo_wrap_and_overflow(void)
{
    p4_flx4_audio_ring_t ring;
    int16_t storage[4u * 4u] = { 0 };
    const int16_t first[3u * 4u] = {
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
    };
    const int16_t second[3u * 4u] = {
        13, 14, 15, 16,
        17, 18, 19, 20,
        21, 22, 23, 24,
    };
    int16_t output[5u * 4u];
    memset(output, 0x55, sizeof(output));

    CHECK(!p4_flx4_audio_ring_init(NULL, storage, 4u, 4u, 44100u));
    CHECK(!p4_flx4_audio_ring_init(&ring, NULL, 4u, 4u, 44100u));
    CHECK(!p4_flx4_audio_ring_init(&ring, storage, 0u, 4u, 44100u));
    CHECK(!p4_flx4_audio_ring_init(&ring, storage, 4u, 0u, 44100u));
    CHECK(!p4_flx4_audio_ring_init(&ring, storage, 4u, 9u, 44100u));
    CHECK(!p4_flx4_audio_ring_init(&ring, storage, 4u, 4u, 0u));
    CHECK(p4_flx4_audio_ring_init(&ring, storage, 4u, 4u, 44100u));
    CHECK_EQ(ring.generation, 1u);
    CHECK_EQ(p4_flx4_audio_ring_free(&ring), 4u);
    CHECK_EQ(p4_flx4_audio_ring_write(&ring, first, 3u), 3u);
    CHECK_EQ(p4_flx4_audio_ring_queued(&ring), 3u);
    CHECK_EQ(ring.high_water_frames, 3u);
    CHECK_EQ(p4_flx4_audio_ring_read(&ring, output, 2u, false), 2u);
    expect_frame(output, 0u, 1, 2, 3, 4);
    expect_frame(output, 1u, 5, 6, 7, 8);
    CHECK_EQ(p4_flx4_audio_ring_write(&ring, second, 3u), 3u);
    CHECK_EQ(p4_flx4_audio_ring_free(&ring), 0u);
    CHECK_EQ(p4_flx4_audio_ring_write(&ring, second, 1u), 0u);
    CHECK_EQ(ring.high_water_frames, 4u);
    CHECK_EQ(ring.overflow_frames, 1u);

    memset(output, 0x55, sizeof(output));
    CHECK_EQ(p4_flx4_audio_ring_read(&ring, output, 5u, true), 4u);
    expect_frame(output, 0u, 9, 10, 11, 12);
    expect_frame(output, 1u, 13, 14, 15, 16);
    expect_frame(output, 2u, 17, 18, 19, 20);
    expect_frame(output, 3u, 21, 22, 23, 24);
    expect_frame(output, 4u, 0, 0, 0, 0);
    CHECK_EQ(p4_flx4_audio_ring_queued(&ring), 0u);
    CHECK_EQ(p4_flx4_audio_ring_free(&ring), 4u);
    CHECK_EQ(ring.underflow_frames, 1u);

    CHECK_EQ(p4_flx4_audio_ring_write(&ring, first, 2u), 2u);
    p4_flx4_audio_ring_reset(&ring, 48000u);
    CHECK_EQ(ring.generation, 2u);
    CHECK_EQ(ring.sample_rate, 48000u);
    CHECK_EQ(p4_flx4_audio_ring_queued(&ring), 0u);
    CHECK_EQ(ring.read_frame, 0u);
    CHECK_EQ(ring.write_frame, 0u);
    CHECK_EQ(ring.high_water_frames, 0u);
    CHECK_EQ(ring.overflow_frames, 0u);
    CHECK_EQ(ring.underflow_frames, 0u);
    CHECK_EQ(ring.clock_trimmed_frames, 0u);
    CHECK_EQ(ring.clock_duplicated_frames, 0u);
}

static void test_audio_ring_clock_drift_correction(void)
{
    p4_flx4_audio_ring_t ring;
    int16_t storage[16u * 4u] = { 0 };
    const int16_t input[4u * 4u] = {
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16,
    };

    CHECK(p4_flx4_audio_ring_init(&ring, storage, 16u, 4u, 44100u));
    CHECK_EQ(p4_flx4_audio_ring_write_clocked(&ring, input, 4u), 5u);
    CHECK_EQ(p4_flx4_audio_ring_queued(&ring), 5u);
    CHECK_EQ(ring.clock_duplicated_frames, 1u);
    CHECK_EQ(ring.clock_trimmed_frames, 0u);

    CHECK_EQ(p4_flx4_audio_ring_write_clocked(&ring, input, 4u), 5u);
    CHECK_EQ(p4_flx4_audio_ring_queued(&ring), 10u);
    CHECK_EQ(ring.clock_duplicated_frames, 2u);

    CHECK_EQ(p4_flx4_audio_ring_write_clocked(&ring, input, 4u), 3u);
    CHECK_EQ(p4_flx4_audio_ring_queued(&ring), 13u);
    CHECK_EQ(ring.clock_trimmed_frames, 1u);
    CHECK_EQ(ring.overflow_frames, 0u);
    CHECK_EQ(ring.high_water_frames, 13u);

    CHECK_EQ(p4_flx4_audio_ring_write_clocked(NULL, input, 4u), 0u);
    CHECK_EQ(p4_flx4_audio_ring_write_clocked(&ring, NULL, 4u), 0u);
    CHECK_EQ(p4_flx4_audio_ring_write_clocked(&ring, input, 0u), 0u);
}

static void test_audio_ring_invalid_operations(void)
{
    p4_flx4_audio_ring_t ring = { 0 };
    int16_t sample[4] = { 1, 2, 3, 4 };
    CHECK_EQ(p4_flx4_audio_ring_write(NULL, sample, 1u), 0u);
    CHECK_EQ(p4_flx4_audio_ring_write(&ring, sample, 1u), 0u);
    CHECK_EQ(p4_flx4_audio_ring_read(NULL, sample, 1u, true), 0u);
    CHECK_EQ(p4_flx4_audio_ring_read(&ring, sample, 1u, true), 0u);
    CHECK_EQ(p4_flx4_audio_ring_queued(NULL), 0u);
    CHECK_EQ(p4_flx4_audio_ring_free(NULL), 0u);
}

int main(void)
{
    test_packetizer();
    test_resampler_44100_passthrough();
    test_resampler_48000_ratio_and_block_continuity();
    test_resampler_48000_exact_one_second();
    test_resampler_guards();
    test_audio_ring_fifo_wrap_and_overflow();
    test_audio_ring_clock_drift_correction();
    test_audio_ring_invalid_operations();
    test_report("p4_flx4_uac");
    return 0;
}

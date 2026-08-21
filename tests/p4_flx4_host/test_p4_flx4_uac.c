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
    CHECK_EQ(p4_flx4_audio_ring_read(&ring, output, 2u, false), 2u);
    expect_frame(output, 0u, 1, 2, 3, 4);
    expect_frame(output, 1u, 5, 6, 7, 8);
    CHECK_EQ(p4_flx4_audio_ring_write(&ring, second, 3u), 3u);
    CHECK_EQ(p4_flx4_audio_ring_free(&ring), 0u);
    CHECK_EQ(p4_flx4_audio_ring_write(&ring, second, 1u), 0u);

    memset(output, 0x55, sizeof(output));
    CHECK_EQ(p4_flx4_audio_ring_read(&ring, output, 5u, true), 4u);
    expect_frame(output, 0u, 9, 10, 11, 12);
    expect_frame(output, 1u, 13, 14, 15, 16);
    expect_frame(output, 2u, 17, 18, 19, 20);
    expect_frame(output, 3u, 21, 22, 23, 24);
    expect_frame(output, 4u, 0, 0, 0, 0);
    CHECK_EQ(p4_flx4_audio_ring_queued(&ring), 0u);
    CHECK_EQ(p4_flx4_audio_ring_free(&ring), 4u);

    CHECK_EQ(p4_flx4_audio_ring_write(&ring, first, 2u), 2u);
    p4_flx4_audio_ring_reset(&ring, 48000u);
    CHECK_EQ(ring.generation, 2u);
    CHECK_EQ(ring.sample_rate, 48000u);
    CHECK_EQ(p4_flx4_audio_ring_queued(&ring), 0u);
    CHECK_EQ(ring.read_frame, 0u);
    CHECK_EQ(ring.write_frame, 0u);
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
    test_audio_ring_fifo_wrap_and_overflow();
    test_audio_ring_invalid_operations();
    test_report("p4_flx4_uac");
    return 0;
}

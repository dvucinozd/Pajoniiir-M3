#include "p4_flx4_map.h"
#include "control_link.h"
#include "test_support.h"

#include <string.h>

static void expect_packet(uint8_t led,
                          uint8_t state,
                          uint8_t deck,
                          uint8_t cin,
                          uint8_t status,
                          uint8_t note,
                          uint8_t value)
{
    uint8_t packet[4] = { 0 };
    CHECK(flx4_led_midi_build_packet(led, state, deck, packet));
    CHECK_EQ(packet[0], cin);
    CHECK_EQ(packet[1], status);
    CHECK_EQ(packet[2], note);
    CHECK_EQ(packet[3], value);
}

static void test_transport_mode_and_global_leds(void)
{
    expect_packet(LED_PLAY, 1, CTRL_DECK_1, 0x09, 0x90, 0x0B, 0x7F);
    expect_packet(LED_CUE, 0, CTRL_DECK_2, 0x09, 0x91, 0x0C, 0x00);
    expect_packet(LED_PFL, 1, CTRL_DECK_2, 0x09, 0x91, 0x54, 0x7F);
    expect_packet(LED_SYNC, 1, CTRL_DECK_1, 0x09, 0x90, 0x58, 0x7F);
    expect_packet(LED_LOOP_IN, 1, CTRL_DECK_2, 0x09, 0x91, 0x10, 0x7F);
    expect_packet(LED_LOOP_OUT, 0, CTRL_DECK_1, 0x09, 0x90, 0x11, 0x00);
    expect_packet(LED_PAD_MODE_HOT_CUE, 1, CTRL_DECK_1, 0x09, 0x90, 0x1B, 0x7F);
    expect_packet(LED_PAD_MODE_PAD_FX2, 1, CTRL_DECK_2, 0x09, 0x91, 0x6B, 0x7F);
    expect_packet(LED_SMART_CFX, 1, CTRL_DECK_2, 0x09, 0x96, 0x00, 0x7F);
    expect_packet(LED_SMART_FADER, 0, CTRL_DECK_1, 0x09, 0x96, 0x01, 0x00);
    expect_packet(LED_MASTER_CUE, 1, CTRL_DECK_1, 0x09, 0x96, 0x63, 0x7F);
    expect_packet(LED_BEAT_FX_ON, 1, CTRL_DECK_1, 0x09, 0x94, 0x47, 0x7F);
    expect_packet(LED_BEAT_FX_ON, 1, CTRL_DECK_2, 0x09, 0x95, 0x47, 0x7F);
    expect_packet(LED_CENSOR, 1, CTRL_DECK_2, 0x09, 0x91, 0x0E, 0x7F);
    expect_packet(LED_CUE_SHIFT, 1, CTRL_DECK_1, 0x09, 0x90, 0x48, 0x7F);
    expect_packet(LED_LOOP_ADJUST_OUT, 0, CTRL_DECK_2, 0x09, 0x91, 0x4E, 0x00);
    expect_packet(LED_TRACK_LOAD_DECK1, 1, CTRL_DECK_2, 0x09, 0x9F, 0x00, 0x7F);
    expect_packet(LED_TRACK_LOAD_DECK2, 0, CTRL_DECK_1, 0x09, 0x9F, 0x01, 0x00);
}

static void test_pad_led_ranges(void)
{
    expect_packet(LED_BEAT_LOOP_PAD_1, 1, CTRL_DECK_1, 0x09, 0x97, 0x60, 0x7F);
    expect_packet(LED_BEAT_LOOP_PAD_8, 0, CTRL_DECK_2, 0x09, 0x99, 0x67, 0x00);
    expect_packet(LED_BEAT_JUMP_PAD_1, 1, CTRL_DECK_2, 0x09, 0x99, 0x20, 0x7F);
    expect_packet(LED_BEAT_JUMP_PAD_8, 0, CTRL_DECK_1, 0x09, 0x97, 0x27, 0x00);
    expect_packet(LED_BEAT_JUMP_SHIFT_HELPER_7, 1, CTRL_DECK_1, 0x09, 0x98, 0x26, 0x7F);
    expect_packet(LED_BEAT_JUMP_SHIFT_HELPER_8, 0, CTRL_DECK_2, 0x09, 0x9A, 0x27, 0x00);
    expect_packet(LED_HOT_CUE_PAD_1, 1, CTRL_DECK_1, 0x09, 0x97, 0x00, 0x7F);
    expect_packet(LED_HOT_CUE_PAD_8, 0, CTRL_DECK_2, 0x09, 0x99, 0x07, 0x00);
    expect_packet(LED_PAD_FX1_PAD_1, 1, CTRL_DECK_2, 0x09, 0x99, 0x10, 0x7F);
    expect_packet(LED_PAD_FX1_PAD_8, 0, CTRL_DECK_1, 0x09, 0x97, 0x17, 0x00);
    expect_packet(LED_PAD_FX2_PAD_1, 1, CTRL_DECK_1, 0x09, 0x97, 0x50, 0x7F);
    expect_packet(LED_PAD_FX2_PAD_8, 0, CTRL_DECK_2, 0x09, 0x99, 0x57, 0x00);
}

static void expect_shifted_mirror_packet(uint8_t led,
                                         uint8_t state,
                                         uint8_t deck,
                                         uint8_t status,
                                         uint8_t note,
                                         uint8_t value)
{
    uint8_t packet[4] = { 0 };
    CHECK(flx4_led_midi_build_shifted_mirror_packet(led, state, deck, packet));
    CHECK_EQ(packet[0], 0x09);
    CHECK_EQ(packet[1], status);
    CHECK_EQ(packet[2], note);
    CHECK_EQ(packet[3], value);
}

static void test_shifted_pad_led_mirrors(void)
{
    expect_shifted_mirror_packet(LED_HOT_CUE_PAD_1, 1, CTRL_DECK_1,
                                 0x98, 0x00, 0x7F);
    expect_shifted_mirror_packet(LED_HOT_CUE_PAD_8, 0, CTRL_DECK_2,
                                 0x9A, 0x07, 0x00);
    expect_shifted_mirror_packet(LED_PAD_FX1_PAD_1, 1, CTRL_DECK_2,
                                 0x9A, 0x10, 0x7F);
    expect_shifted_mirror_packet(LED_PAD_FX1_PAD_8, 0, CTRL_DECK_1,
                                 0x98, 0x17, 0x00);
    expect_shifted_mirror_packet(LED_PAD_FX2_PAD_1, 1, CTRL_DECK_1,
                                 0x98, 0x50, 0x7F);
    expect_shifted_mirror_packet(LED_PAD_FX2_PAD_8, 0, CTRL_DECK_2,
                                 0x9A, 0x57, 0x00);
    expect_shifted_mirror_packet(LED_BEAT_LOOP_PAD_1, 1, CTRL_DECK_2,
                                 0x9A, 0x60, 0x7F);
    expect_shifted_mirror_packet(LED_BEAT_LOOP_PAD_8, 0, CTRL_DECK_1,
                                 0x98, 0x67, 0x00);
    expect_shifted_mirror_packet(LED_BEAT_JUMP_PAD_1, 1, CTRL_DECK_1,
                                 0x98, 0x20, 0x7F);
    expect_shifted_mirror_packet(LED_BEAT_JUMP_PAD_6, 0, CTRL_DECK_2,
                                 0x9A, 0x25, 0x00);

    uint8_t packet[4] = { 0xAA, 0xAA, 0xAA, 0xAA };
    const uint8_t original[4] = { 0xAA, 0xAA, 0xAA, 0xAA };
    CHECK(!flx4_led_midi_build_shifted_mirror_packet(
        LED_BEAT_JUMP_PAD_7, 1, CTRL_DECK_1, packet));
    CHECK(!flx4_led_midi_build_shifted_mirror_packet(
        LED_PLAY, 1, CTRL_DECK_1, packet));
    CHECK(!flx4_led_midi_build_shifted_mirror_packet(
        LED_HOT_CUE_PAD_1, 1, CTRL_DECK_NONE, packet));
    CHECK(!flx4_led_midi_build_shifted_mirror_packet(
        LED_HOT_CUE_PAD_1, 1, CTRL_DECK_1, NULL));
    CHECK(memcmp(packet, original, sizeof(packet)) == 0);
}

static void test_vu_and_invalid_inputs(void)
{
    expect_packet(LED_VU_METER, 0x40, CTRL_DECK_1, 0x0B, 0xB0, 0x02, 0x40);
    expect_packet(LED_VU_METER, 0xFF, CTRL_DECK_2, 0x0B, 0xB1, 0x02, 0x7F);

    uint8_t packet[4] = { 0xAA, 0xAA, 0xAA, 0xAA };
    const uint8_t original[4] = { 0xAA, 0xAA, 0xAA, 0xAA };
    CHECK(!flx4_led_midi_build_packet(LED_PLAY, 1, CTRL_DECK_NONE, packet));
    CHECK(!flx4_led_midi_build_packet(0xFF, 1, CTRL_DECK_1, packet));
    CHECK(!flx4_led_midi_build_packet(LED_PLAY, 1, CTRL_DECK_1, NULL));
    CHECK(memcmp(packet, original, sizeof(packet)) == 0);
}

int main(void)
{
    test_transport_mode_and_global_leds();
    test_pad_led_ranges();
    test_shifted_pad_led_mirrors();
    test_vu_and_invalid_inputs();
    test_report("p4_flx4_led");
    return 0;
}

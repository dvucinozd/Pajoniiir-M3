#include "p4_flx4_map.h"
#include "control_link.h"
#include "test_support.h"

#include <string.h>

#define MSG(status_, data1_, data2_) (&(flx4_midi_message_t) { \
    .cable = 0u, .cin = 0x09u, .len = 3u,                       \
    .status = (status_), .data1 = (data1_), .data2 = (data2_)  \
})

static void expect_event(const flx4_control_event_t *event,
                         uint8_t type,
                         uint8_t id,
                         int16_t value)
{
    CHECK_EQ(event->type, type);
    CHECK_EQ(event->id, id);
    CHECK_EQ(event->value, value);
}

static void test_usb_packet_parser(void)
{
    flx4_midi_message_t message = { 0 };
    const uint8_t note_on[4] = { 0x29u, 0x91u, 0x0Cu, 0x7Fu };
    const uint8_t program_change[4] = { 0x0Cu, 0xC0u, 0x12u, 0xAAu };
    const uint8_t single_byte[4] = { 0x0Fu, 0xF8u, 0xAAu, 0xAAu };
    const uint8_t invalid[4] = { 0x00u, 0x90u, 0x0Bu, 0x7Fu };

    CHECK(!flx4_midi_parse_usb_packet(NULL, &message));
    CHECK(!flx4_midi_parse_usb_packet(note_on, NULL));
    CHECK(!flx4_midi_parse_usb_packet(invalid, &message));
    CHECK(flx4_midi_parse_usb_packet(note_on, &message));
    CHECK_EQ(message.cable, 2u);
    CHECK_EQ(message.cin, 9u);
    CHECK_EQ(message.len, 3u);
    CHECK_EQ(message.status, 0x91u);
    CHECK_EQ(message.data1, 0x0Cu);
    CHECK_EQ(message.data2, 0x7Fu);

    CHECK(flx4_midi_parse_usb_packet(program_change, &message));
    CHECK_EQ(message.len, 2u);
    CHECK_EQ(message.data1, 0x12u);
    CHECK_EQ(message.data2, 0u);

    CHECK(flx4_midi_parse_usb_packet(single_byte, &message));
    CHECK_EQ(message.len, 1u);
    CHECK_EQ(message.data1, 0u);
    CHECK_EQ(message.data2, 0u);
}

static void test_transport_and_global_buttons(void)
{
    flx4_map_state_t state;
    flx4_control_event_t event;
    flx4_map_init(&state);

    CHECK(flx4_map_translate_message(&state, MSG(0x90, 0x0B, 0x7F), &event));
    expect_event(&event, CTRL_TYPE_BUTTON, CTRL_ID_DECK1_PLAY, 1);
    CHECK(flx4_map_translate_message(&state, MSG(0x90, 0x0B, 0x00), &event));
    expect_event(&event, CTRL_TYPE_BUTTON, CTRL_ID_DECK1_PLAY, 0);
    CHECK(flx4_map_translate_message(&state, MSG(0x91, 0x0C, 0x7F), &event));
    expect_event(&event, CTRL_TYPE_BUTTON, CTRL_ID_DECK2_CUE, 1);
    CHECK(flx4_map_translate_message(&state, MSG(0x90, 0x54, 0x7F), &event));
    expect_event(&event, CTRL_TYPE_BUTTON, CTRL_ID_DECK1_PFL, 1);
    CHECK(flx4_map_translate_message(&state, MSG(0x91, 0x58, 0x7F), &event));
    expect_event(&event, CTRL_TYPE_BUTTON, CTRL_ID_DECK2_SYNC, 1);

    CHECK(flx4_map_translate_message(&state, MSG(0x96, 0x46, 0x7F), &event));
    expect_event(&event, CTRL_TYPE_BUTTON, CTRL_ID_LOAD_DECK1, 1);
    CHECK(flx4_map_translate_message(&state, MSG(0x96, 0x47, 0x7F), &event));
    expect_event(&event, CTRL_TYPE_BUTTON, CTRL_ID_LOAD_DECK2, 1);
    CHECK(flx4_map_translate_message(&state, MSG(0x96, 0x41, 0x7F), &event));
    expect_event(&event, CTRL_TYPE_BUTTON, CTRL_ID_BROWSE_PRESS, 1);
    CHECK(flx4_map_translate_message(&state, MSG(0x96, 0x00, 0x7F), &event));
    expect_event(&event, CTRL_TYPE_BUTTON, CTRL_ID_SMART_CFX, 1);
    CHECK(flx4_map_translate_message(&state, MSG(0x96, 0x01, 0x00), &event));
    expect_event(&event, CTRL_TYPE_BUTTON, CTRL_ID_SMART_FADER, 0);
    CHECK(flx4_map_translate_message(&state, MSG(0x96, 0x63, 0x7F), &event));
    expect_event(&event, CTRL_TYPE_BUTTON, CTRL_ID_MASTER_CUE, 1);
}

static void test_extended_deck_and_pad_actions(void)
{
    flx4_map_state_t state;
    flx4_control_event_t event;
    flx4_map_init(&state);

    CHECK(flx4_map_translate_message(&state, MSG(0x90, 0x0E, 0x7F), &event));
    expect_event(&event, CTRL_TYPE_BUTTON, CTRL_ID_DECK1_EXT_ACTION,
                 CTRL_DECK_EXT_VALUE(CTRL_DECK_EXT_ACTION_CENSOR, true));
    CHECK(flx4_map_translate_message(&state, MSG(0x91, 0x4C, 0x7F), &event));
    expect_event(&event, CTRL_TYPE_BUTTON, CTRL_ID_DECK2_EXT_ACTION,
                 CTRL_DECK_EXT_VALUE(CTRL_DECK_EXT_ACTION_LOOP_ADJUST_IN, true));
    CHECK(flx4_map_translate_message(&state, MSG(0x91, 0x6D, 0x7F), &event));
    expect_event(&event, CTRL_TYPE_BUTTON, CTRL_ID_DECK2_PAD_MODE_BEAT_LOOP, 1);

    CHECK(flx4_map_translate_message(&state, MSG(0x97, 0x03, 0x7F), &event));
    expect_event(&event, CTRL_TYPE_BUTTON, CTRL_ID_DECK1_PAD_ACTION,
                 CTRL_PAD_ACTION_VALUE(CTRL_PAD_MODE_HOT_CUE, 3, false, true));
    CHECK(flx4_map_translate_message(&state, MSG(0x9A, 0x27, 0x00), &event));
    expect_event(&event, CTRL_TYPE_BUTTON, CTRL_ID_DECK2_PAD_ACTION,
                 CTRL_PAD_ACTION_VALUE(CTRL_PAD_MODE_BEAT_JUMP, 7, true, false));
    CHECK(flx4_map_translate_message(&state, MSG(0x99, 0x52, 0x7F), &event));
    expect_event(&event, CTRL_TYPE_BUTTON, CTRL_ID_DECK2_PAD_ACTION,
                 CTRL_PAD_ACTION_VALUE(CTRL_PAD_MODE_PAD_FX2, 2, false, true));
    CHECK(!flx4_map_translate_message(&state, MSG(0x97, 0x30, 0x7F), &event));
}

static void test_relative_and_absolute_controls(void)
{
    flx4_map_state_t state;
    flx4_control_event_t event;
    flx4_map_init(&state);

    CHECK(flx4_map_translate_message(&state, MSG(0xB0, 0x22, 65), &event));
    expect_event(&event, CTRL_TYPE_ENCODER, CTRL_ID_DECK1_JOG_SCRATCH, 1);
    CHECK(flx4_map_translate_message(&state, MSG(0xB1, 0x23, 63), &event));
    expect_event(&event, CTRL_TYPE_ENCODER, CTRL_ID_DECK2_JOG_BEND, -1);
    CHECK(flx4_map_translate_message(&state, MSG(0xB6, 0x40, 0x7F), &event));
    expect_event(&event, CTRL_TYPE_ENCODER, CTRL_ID_BROWSE_DELTA, -1);
    CHECK(!flx4_map_translate_message(&state, MSG(0xB6, 0x40, 0x00), &event));

    CHECK(!flx4_map_translate_message(&state, MSG(0xB0, 0x00, 0x12), &event));
    CHECK(flx4_map_translate_message(&state, MSG(0xB0, 0x20, 0x34), &event));
    expect_event(&event, CTRL_TYPE_PITCH, CTRL_ID_DECK1_TEMPO,
                 (int16_t)((0x12u << 7) | 0x34u));
    CHECK(!flx4_map_translate_message(&state, MSG(0xB6, 0x1F, 0x20), &event));
    CHECK(flx4_map_translate_message(&state, MSG(0xB6, 0x3F, 0x01), &event));
    expect_event(&event, CTRL_TYPE_PITCH, CTRL_ID_CROSSFADER,
                 (int16_t)((0x20u << 7) | 0x01u));
    CHECK(!flx4_map_translate_message(&state, MSG(0xB6, 0x17, 0x40), &event));
    CHECK(flx4_map_translate_message(&state, MSG(0xB6, 0x37, 0x02), &event));
    expect_event(&event, CTRL_TYPE_PITCH, CTRL_ID_CH1_FILTER,
                 (int16_t)((0x40u << 7) | 0x02u));
}

static void test_beat_fx_target_state(void)
{
    flx4_map_state_t state;
    flx4_control_event_t event;
    flx4_map_init(&state);

    CHECK(!state.beat_fx_channel_valid);
    CHECK(!flx4_map_translate_message(&state, MSG(0x94, 0x10, 0x00), &event));
    CHECK(flx4_map_translate_message(&state, MSG(0x94, 0x10, 0x7F), &event));
    expect_event(&event, CTRL_TYPE_BUTTON, CTRL_ID_BEAT_FX_TARGET,
                 CTRL_BEAT_FX_TARGET_CH1);
    CHECK(state.beat_fx_channel_valid);
    CHECK(flx4_map_translate_message(&state, MSG(0x95, 0x11, 0x7F), &event));
    expect_event(&event, CTRL_TYPE_BUTTON, CTRL_ID_BEAT_FX_TARGET,
                 CTRL_BEAT_FX_TARGET_BOTH);
    CHECK(flx4_map_translate_message(&state, MSG(0x94, 0x10, 0x00), &event));
    expect_event(&event, CTRL_TYPE_BUTTON, CTRL_ID_BEAT_FX_TARGET,
                 CTRL_BEAT_FX_TARGET_CH2);
    CHECK(!flx4_map_translate_message(&state, MSG(0x95, 0x11, 0x00), &event));
    CHECK(!state.beat_fx_channel_valid);

    CHECK(flx4_map_translate_message(&state, MSG(0xB4, 0x02, 0x40), &event));
    expect_event(&event, CTRL_TYPE_PITCH, CTRL_ID_BEAT_FX_DEPTH, 64);
    CHECK(flx4_map_translate_message(&state, MSG(0x94, 0x47, 0x7F), &event));
    expect_event(&event, CTRL_TYPE_BUTTON, CTRL_ID_BEAT_FX_ON, 1);
}

static void test_invalid_and_unsupported_messages(void)
{
    flx4_map_state_t state;
    flx4_control_event_t event;
    flx4_midi_message_t short_message = { .len = 2u, .status = 0x90u };
    memset(&state, 0xA5, sizeof(state));
    flx4_map_init(&state);
    CHECK_EQ(state.tempo[0].msb, 0);
    CHECK(!state.tempo[0].msb_valid);

    CHECK(!flx4_map_translate_message(NULL, MSG(0x90, 0x0B, 0x7F), &event));
    CHECK(!flx4_map_translate_message(&state, NULL, &event));
    CHECK(!flx4_map_translate_message(&state, MSG(0x90, 0x0B, 0x7F), NULL));
    CHECK(!flx4_map_translate_message(&state, &short_message, &event));
    CHECK(!flx4_map_translate_message(&state, MSG(0x80, 0x0B, 0x40), &event));
    CHECK(!flx4_map_translate_message(&state, MSG(0x90, 0x6E, 0x7F), &event));
}

int main(void)
{
    test_usb_packet_parser();
    test_transport_and_global_buttons();
    test_extended_deck_and_pad_actions();
    test_relative_and_absolute_controls();
    test_beat_fx_target_state();
    test_invalid_and_unsupported_messages();
    test_report("p4_flx4_map");
    return 0;
}

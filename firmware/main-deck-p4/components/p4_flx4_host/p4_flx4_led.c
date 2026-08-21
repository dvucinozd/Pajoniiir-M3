#include "p4_flx4_map.h"
#include "control_link.h"

bool flx4_led_midi_builtin_authoritative(uint8_t led)
{
    return led == LED_TRACK_LOAD_DECK1 || led == LED_TRACK_LOAD_DECK2;
}

static bool note_for_led(uint8_t led, uint8_t *note)
{
    if (!note) return false;

    if (led >= LED_BEAT_LOOP_PAD_1 && led <= LED_BEAT_LOOP_PAD_8) {
        *note = (uint8_t)(0x60u + (led - LED_BEAT_LOOP_PAD_1));
        return true;
    }
    if (led >= LED_BEAT_JUMP_PAD_1 && led <= LED_BEAT_JUMP_PAD_8) {
        *note = (uint8_t)(0x20u + (led - LED_BEAT_JUMP_PAD_1));
        return true;
    }
    if (led >= LED_BEAT_JUMP_SHIFT_HELPER_7 && led <= LED_BEAT_JUMP_SHIFT_HELPER_8) {
        *note = (uint8_t)(0x26u + (led - LED_BEAT_JUMP_SHIFT_HELPER_7));
        return true;
    }
    if (led >= LED_HOT_CUE_PAD_1 && led <= LED_HOT_CUE_PAD_8) {
        *note = (uint8_t)(0x00u + (led - LED_HOT_CUE_PAD_1));
        return true;
    }
    if (led >= LED_PAD_FX1_PAD_1 && led <= LED_PAD_FX1_PAD_8) {
        *note = (uint8_t)(0x10u + (led - LED_PAD_FX1_PAD_1));
        return true;
    }
    if (led >= LED_PAD_FX2_PAD_1 && led <= LED_PAD_FX2_PAD_8) {
        *note = (uint8_t)(0x50u + (led - LED_PAD_FX2_PAD_1));
        return true;
    }

    switch (led) {
    case LED_PLAY:
        *note = 0x0B;
        return true;
    case LED_CUE:
        *note = 0x0C;
        return true;
    case LED_PFL:
        *note = 0x54;
        return true;
    case LED_SYNC:
        *note = 0x58;
        return true;
    case LED_LOOP_IN:
        *note = 0x10;
        return true;
    case LED_LOOP_OUT:
        *note = 0x11;
        return true;
    case LED_PAD_MODE_HOT_CUE:
        *note = 0x1B;
        return true;
    case LED_PAD_MODE_KEYBOARD:
        *note = 0x69;
        return true;
    case LED_PAD_MODE_PAD_FX1:
        *note = 0x1E;
        return true;
    case LED_PAD_MODE_PAD_FX2:
        *note = 0x6B;
        return true;
    case LED_PAD_MODE_BEAT_JUMP:
        *note = 0x20;
        return true;
    case LED_PAD_MODE_BEAT_LOOP:
        *note = 0x6D;
        return true;
    case LED_PAD_MODE_SAMPLER:
        *note = 0x22;
        return true;
    case LED_PAD_MODE_KEY_SHIFT:
        *note = 0x6F;
        return true;
    case LED_SMART_CFX:
        *note = 0x00;
        return true;
    case LED_SMART_FADER:
        *note = 0x01;
        return true;
    case LED_BEAT_FX_ON:
        *note = 0x47;
        return true;
    case LED_MASTER_CUE:
        *note = 0x63;
        return true;
    case LED_CENSOR:
        *note = 0x0E;
        return true;
    case LED_CUE_SHIFT:
        *note = 0x48;
        return true;
    case LED_LOOP_ADJUST_IN:
        *note = 0x4C;
        return true;
    case LED_LOOP_ADJUST_OUT:
        *note = 0x4E;
        return true;
    case LED_TRACK_LOAD_DECK1:
        *note = 0x00;
        return true;
    case LED_TRACK_LOAD_DECK2:
        *note = 0x01;
        return true;
    default:
        return false;
    }
}

static inline bool is_deck_2(uint8_t deck)
{
    return (deck == 1 || deck == CTRL_DECK_2);
}

static uint8_t note_status_for_deck(uint8_t deck, uint8_t led)
{
    if (led == LED_TRACK_LOAD_DECK1 || led == LED_TRACK_LOAD_DECK2) {
        return 0x9F;
    }
    if (led == LED_SMART_CFX || led == LED_SMART_FADER ||
        led == LED_BEAT_FX_ON || led == LED_MASTER_CUE) {
        return 0x96;
    }
    if ((led >= LED_HOT_CUE_PAD_1 && led <= LED_HOT_CUE_PAD_8) ||
        (led >= LED_PAD_FX1_PAD_1 && led <= LED_PAD_FX1_PAD_8) ||
        (led >= LED_PAD_FX2_PAD_1 && led <= LED_PAD_FX2_PAD_8) ||
        (led >= LED_BEAT_JUMP_PAD_1 && led <= LED_BEAT_JUMP_PAD_8) ||
        (led >= LED_BEAT_LOOP_PAD_1 && led <= LED_BEAT_LOOP_PAD_8) ||
        (led >= LED_BEAT_JUMP_SHIFT_HELPER_7 && led <= LED_BEAT_JUMP_SHIFT_HELPER_8)) {
        return is_deck_2(deck) ? 0x99 : 0x97;
    }
    return is_deck_2(deck) ? 0x91 : 0x90;
}

bool flx4_led_midi_build_packet(uint8_t led, uint8_t state, uint8_t deck, uint8_t packet[4])
{
    if (!packet) return false;

    if (led == LED_VU_METER) {
        packet[0] = 0x0B;  // Control Change
        packet[1] = is_deck_2(deck) ? 0xB1 : 0xB0;
        packet[2] = 0x02;  // VU meter CC
        packet[3] = state & 0x7F;
        return true;
    }

    uint8_t note = 0;
    if (!note_for_led(led, &note)) return false;

    packet[0] = 0x09;  // Note On
    packet[1] = note_status_for_deck(deck, led);
    packet[2] = note;
    packet[3] = (state == 0) ? 0x00 : 0x7F;
    return true;
}

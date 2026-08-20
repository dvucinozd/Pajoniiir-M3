#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
    bool     accepting;
    uint32_t active_producers;
    uint32_t generation;
} p4_flx4_midi_gate_t;

void p4_flx4_midi_gate_init(p4_flx4_midi_gate_t *gate);
void p4_flx4_midi_gate_start(p4_flx4_midi_gate_t *gate);
void p4_flx4_midi_gate_stop(p4_flx4_midi_gate_t *gate);
bool p4_flx4_midi_gate_begin(p4_flx4_midi_gate_t *gate, uint32_t *generation_out);
void p4_flx4_midi_gate_end(p4_flx4_midi_gate_t *gate);
bool p4_flx4_midi_gate_is_accepting(const p4_flx4_midi_gate_t *gate);

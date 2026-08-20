#include "p4_flx4_midi_gate.h"

void p4_flx4_midi_gate_init(p4_flx4_midi_gate_t *gate)
{
    if (!gate) return;
    __atomic_store_n(&gate->accepting, false, __ATOMIC_RELEASE);
    __atomic_store_n(&gate->active_producers, 0u, __ATOMIC_RELEASE);
    __atomic_store_n(&gate->generation, 0u, __ATOMIC_RELEASE);
}

void p4_flx4_midi_gate_start(p4_flx4_midi_gate_t *gate)
{
    if (!gate) return;
    (void)__atomic_add_fetch(&gate->generation, 1u, __ATOMIC_ACQ_REL);
    __atomic_store_n(&gate->accepting, true, __ATOMIC_RELEASE);
}

void p4_flx4_midi_gate_stop(p4_flx4_midi_gate_t *gate)
{
    if (!gate) return;
    __atomic_store_n(&gate->accepting, false, __ATOMIC_RELEASE);
}

bool p4_flx4_midi_gate_begin(p4_flx4_midi_gate_t *gate, uint32_t *generation_out)
{
    if (!gate || !__atomic_load_n(&gate->accepting, __ATOMIC_ACQUIRE)) {
        return false;
    }

    (void)__atomic_add_fetch(&gate->active_producers, 1u, __ATOMIC_ACQ_REL);
    if (!__atomic_load_n(&gate->accepting, __ATOMIC_ACQUIRE)) {
        (void)__atomic_sub_fetch(&gate->active_producers, 1u, __ATOMIC_ACQ_REL);
        return false;
    }
    if (generation_out) {
        *generation_out = __atomic_load_n(&gate->generation, __ATOMIC_ACQUIRE);
    }
    return true;
}

void p4_flx4_midi_gate_end(p4_flx4_midi_gate_t *gate)
{
    if (!gate) return;
    (void)__atomic_sub_fetch(&gate->active_producers, 1u, __ATOMIC_ACQ_REL);
}

bool p4_flx4_midi_gate_is_accepting(const p4_flx4_midi_gate_t *gate)
{
    return gate && __atomic_load_n(&gate->accepting, __ATOMIC_ACQUIRE);
}

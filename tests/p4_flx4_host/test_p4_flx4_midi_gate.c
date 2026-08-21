#include "p4_flx4_midi_gate.h"
#include "test_support.h"

#include <pthread.h>

typedef struct {
    p4_flx4_midi_gate_t *gate;
    unsigned attempts;
    unsigned accepted;
} producer_args_t;

static void *producer_thread(void *context)
{
    producer_args_t *args = context;
    for (unsigned i = 0u; i < args->attempts; ++i) {
        uint32_t generation = 0u;
        if (p4_flx4_midi_gate_begin(args->gate, &generation)) {
            if (generation != 0u) {
                args->accepted++;
            }
            p4_flx4_midi_gate_end(args->gate);
        }
    }
    return NULL;
}

static void test_gate_lifecycle(void)
{
    p4_flx4_midi_gate_t gate;
    uint32_t generation = 99u;
    p4_flx4_midi_gate_init(&gate);
    CHECK(!p4_flx4_midi_gate_is_accepting(&gate));
    CHECK_EQ(gate.active_producers, 0u);
    CHECK_EQ(gate.generation, 0u);
    CHECK(!p4_flx4_midi_gate_begin(&gate, &generation));
    CHECK_EQ(generation, 99u);

    p4_flx4_midi_gate_start(&gate);
    CHECK(p4_flx4_midi_gate_is_accepting(&gate));
    CHECK_EQ(gate.generation, 1u);
    CHECK(p4_flx4_midi_gate_begin(&gate, &generation));
    CHECK_EQ(generation, 1u);
    CHECK_EQ(gate.active_producers, 1u);
    CHECK(p4_flx4_midi_gate_accepts_generation(&gate, generation));

    p4_flx4_midi_gate_stop(&gate);
    CHECK(!p4_flx4_midi_gate_is_accepting(&gate));
    CHECK(!p4_flx4_midi_gate_accepts_generation(&gate, generation));
    CHECK(!p4_flx4_midi_gate_begin(&gate, NULL));
    CHECK_EQ(gate.active_producers, 1u);
    p4_flx4_midi_gate_end(&gate);
    CHECK_EQ(gate.active_producers, 0u);

    p4_flx4_midi_gate_start(&gate);
    CHECK_EQ(gate.generation, 2u);
    CHECK(!p4_flx4_midi_gate_accepts_generation(&gate, 1u));
    CHECK(p4_flx4_midi_gate_begin(&gate, &generation));
    CHECK_EQ(generation, 2u);
    CHECK(p4_flx4_midi_gate_accepts_generation(&gate, generation));
    p4_flx4_midi_gate_end(&gate);
    CHECK_EQ(gate.active_producers, 0u);

    p4_flx4_midi_gate_init(NULL);
    p4_flx4_midi_gate_start(NULL);
    p4_flx4_midi_gate_stop(NULL);
    p4_flx4_midi_gate_end(NULL);
    CHECK(!p4_flx4_midi_gate_begin(NULL, NULL));
    CHECK(!p4_flx4_midi_gate_is_accepting(NULL));
    CHECK(!p4_flx4_midi_gate_accepts_generation(NULL, 1u));
}

static void test_concurrent_producers_balance(void)
{
    enum { THREAD_COUNT = 4, ATTEMPTS = 20000 };
    p4_flx4_midi_gate_t gate;
    pthread_t threads[THREAD_COUNT];
    producer_args_t args[THREAD_COUNT];
    p4_flx4_midi_gate_init(&gate);
    p4_flx4_midi_gate_start(&gate);

    for (unsigned i = 0u; i < THREAD_COUNT; ++i) {
        args[i] = (producer_args_t) {
            .gate = &gate,
            .attempts = ATTEMPTS,
            .accepted = 0u,
        };
        CHECK_EQ(pthread_create(&threads[i], NULL, producer_thread, &args[i]), 0);
    }
    for (unsigned i = 0u; i < 2000u; ++i) {
        p4_flx4_midi_gate_stop(&gate);
        p4_flx4_midi_gate_start(&gate);
    }
    for (unsigned i = 0u; i < THREAD_COUNT; ++i) {
        CHECK_EQ(pthread_join(threads[i], NULL), 0);
        CHECK(args[i].accepted > 0u);
    }
    p4_flx4_midi_gate_stop(&gate);
    CHECK_EQ(gate.active_producers, 0u);
    CHECK_EQ(gate.generation, 2001u);
}

int main(void)
{
    test_gate_lifecycle();
    test_concurrent_producers_balance();
    test_report("p4_flx4_midi_gate");
    return 0;
}

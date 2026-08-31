#include "bsp_dsi_id_probe.h"

#include <assert.h>
#include <stdio.h>

typedef struct {
    uint8_t bytes[8];
    unsigned calls;
    int fail_at;
} fake_reader_t;

static fake_reader_t valid_reader(void)
{
    return (fake_reader_t){.bytes = {0xc1, 0x62, 0x11, 0x42, 0xc1, 0x62, 0x11, 0x42},
                           .fail_at = -1};
}

static bool read_byte(void *context, uint8_t reg, uint8_t *value)
{
    fake_reader_t *reader = context;
    unsigned call = reader->calls++;
    assert(call < 8);
    assert(reg == call % 4);
    *value = reader->bytes[call]; // Even plausible data on failure is rejected.
    return (int)call != reader->fail_at;
}

int main(void)
{
    unsigned cases = 0;
    uint8_t id[4];
    fake_reader_t reader = valid_reader();
    assert(bsp_dsi_id_probe(read_byte, &reader, id));
    assert(reader.calls == 8 && memcmp(id, reader.bytes, 4) == 0);
    ++cases;

    for (int failure = 0; failure < 8; ++failure) {
        reader = valid_reader();
        reader.fail_at = failure;
        assert(!bsp_dsi_id_probe(read_byte, &reader, id));
        assert(reader.calls == (unsigned)failure + 1);
        ++cases;
    }
    for (unsigned mismatch = 0; mismatch < 8; ++mismatch) {
        if (mismatch == 3) continue; // Any revision is valid if repeated.
        reader = valid_reader();
        reader.bytes[mismatch] ^= 1;
        assert(!bsp_dsi_id_probe(read_byte, &reader, id));
        assert(reader.calls == mismatch + 1);
        ++cases;
    }
    for (unsigned revision = 0; revision < 256; ++revision) {
        reader = valid_reader();
        reader.bytes[3] = reader.bytes[7] = (uint8_t)revision;
        assert(bsp_dsi_id_probe(read_byte, &reader, id));
        assert(reader.calls == 8 && id[3] == revision);
        ++cases;
    }
    reader = valid_reader();
    assert(!bsp_dsi_id_probe(read_byte, &reader, NULL));
    assert(reader.calls == 0);
    ++cases;
    memset(id, 0xff, sizeof(id));
    assert(!bsp_dsi_id_probe(NULL, &reader, id));
    const uint8_t zero[4] = {0};
    assert(reader.calls == 0 && memcmp(id, zero, 4) == 0);
    ++cases;
    printf("TESTS_RUN=%u\n", cases);
    return 0;
}

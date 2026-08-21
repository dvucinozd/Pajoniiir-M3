#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned g_tests_run;

#define CHECK(condition)                                                        \
    do {                                                                        \
        g_tests_run++;                                                          \
        if (!(condition)) {                                                     \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            exit(1);                                                            \
        }                                                                       \
    } while (0)

#define CHECK_EQ(actual, expected)                                              \
    do {                                                                        \
        int64_t actual_value = (int64_t)(actual);                               \
        int64_t expected_value = (int64_t)(expected);                           \
        g_tests_run++;                                                          \
        if (actual_value != expected_value) {                                   \
            fprintf(stderr,                                                     \
                    "FAIL %s:%d: %s=%lld expected %s=%lld\n",                  \
                    __FILE__, __LINE__, #actual, (long long)actual_value,       \
                    #expected, (long long)expected_value);                      \
            exit(1);                                                            \
        }                                                                       \
    } while (0)

static void test_report(const char *suite)
{
    printf("%s tests passed\nTESTS_RUN=%u\n", suite, g_tests_run);
}

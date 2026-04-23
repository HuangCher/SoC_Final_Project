#include "bloom.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#ifndef BATCH_BENCH_N
#define BATCH_BENCH_N 2000
#endif

static int tests_run = 0;
static int tests_failed = 0;

static uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec * 1000000000ull) + (uint64_t)ts.tv_nsec;
}

static void check_int(const char *name, int expected, int actual, uint64_t dt_ns) {
    tests_run++;
    if (expected != actual) {
        tests_failed++;
        if (dt_ns > 0u) {
            printf("[FAIL] %s: expected %d, got %d  (%llu ns)\n",
                   name, expected, actual, (unsigned long long)dt_ns);
        } else {
            printf("[FAIL] %s: expected %d, got %d\n", name, expected, actual);
        }
    } else {
        if (dt_ns > 0u) {
            printf("[PASS] %s  (%llu ns)\n", name, (unsigned long long)dt_ns);
        } else {
            printf("[PASS] %s\n", name);
        }
    }
}

static void check_u32_nonzero(const char *name, uint32_t value, uint64_t dt_ns) {
    tests_run++;
    if (value == 0) {
        tests_failed++;
        if (dt_ns > 0u) {
            printf("[FAIL] %s: value was 0  (%llu ns)\n", name, (unsigned long long)dt_ns);
        } else {
            printf("[FAIL] %s: value was 0\n", name);
        }
    } else {
        if (dt_ns > 0u) {
            printf("[PASS] %s  (%llu ns)\n", name, (unsigned long long)dt_ns);
        } else {
            printf("[PASS] %s\n", name);
        }
    }
}

int main(void) {
    bloom_sw_t bf;
    uint64_t t0, t1;
    int r;
    uint32_t apple1, apple2, banana;

    printf("Running Bloom filter software tests (pure SW, no accelerator)...\n\n");

    t0 = now_ns();
    bloom_sw_init(&bf);
    t1 = now_ns();
    printf("bloom_sw_init  (%llu ns)\n\n", (unsigned long long)(t1 - t0));

    t0 = now_ns();
    r = bloom_sw_query(&bf, 1234u);
    t1 = now_ns();
    check_int("query empty for key 1234", 0, r, t1 - t0);

    t0 = now_ns();
    r = bloom_sw_query(&bf, 9999u);
    t1 = now_ns();
    check_int("query empty for key 9999", 0, r, t1 - t0);

    t0 = now_ns();
    bloom_sw_insert(&bf, 1234u);
    t1 = now_ns();
    printf("insert key 1234  (%llu ns)\n", (unsigned long long)(t1 - t0));

    t0 = now_ns();
    r = bloom_sw_query(&bf, 1234u);
    t1 = now_ns();
    check_int("insert/query same key 1234", 1, r, t1 - t0);

    t0 = now_ns();
    bloom_sw_insert(&bf, 1234u);
    t1 = now_ns();
    printf("insert key 1234 (repeat 1)  (%llu ns)\n", (unsigned long long)(t1 - t0));

    t0 = now_ns();
    bloom_sw_insert(&bf, 1234u);
    t1 = now_ns();
    printf("insert key 1234 (repeat 2)  (%llu ns)\n", (unsigned long long)(t1 - t0));

    t0 = now_ns();
    r = bloom_sw_query(&bf, 1234u);
    t1 = now_ns();
    check_int("repeated insert/query 1234", 1, r, t1 - t0);

    t0 = now_ns();
    bloom_sw_clear(&bf);
    t1 = now_ns();
    printf("bloom_sw_clear  (%llu ns)\n", (unsigned long long)(t1 - t0));

    t0 = now_ns();
    r = bloom_sw_query(&bf, 1234u);
    t1 = now_ns();
    check_int("clear resets key 1234", 0, r, t1 - t0);

    t0 = now_ns();
    bloom_sw_insert(&bf, 111u);
    t1 = now_ns();
    printf("insert key 111  (%llu ns)\n", (unsigned long long)(t1 - t0));

    t0 = now_ns();
    bloom_sw_insert(&bf, 222u);
    t1 = now_ns();
    printf("insert key 222  (%llu ns)\n", (unsigned long long)(t1 - t0));

    t0 = now_ns();
    bloom_sw_insert(&bf, 333u);
    t1 = now_ns();
    printf("insert key 333  (%llu ns)\n", (unsigned long long)(t1 - t0));

    t0 = now_ns();
    r = bloom_sw_query(&bf, 111u);
    t1 = now_ns();
    check_int("query inserted key 111", 1, r, t1 - t0);

    t0 = now_ns();
    r = bloom_sw_query(&bf, 222u);
    t1 = now_ns();
    check_int("query inserted key 222", 1, r, t1 - t0);

    t0 = now_ns();
    r = bloom_sw_query(&bf, 333u);
    t1 = now_ns();
    check_int("query inserted key 333", 1, r, t1 - t0);

    t0 = now_ns();
    apple1 = bloom_sw_hash_string("apple");
    t1 = now_ns();
    printf("hash_string(\"apple\")  (%llu ns)\n", (unsigned long long)(t1 - t0));

    t0 = now_ns();
    apple2 = bloom_sw_hash_string("apple");
    t1 = now_ns();
    printf("hash_string(\"apple\") again  (%llu ns)\n", (unsigned long long)(t1 - t0));

    t0 = now_ns();
    banana = bloom_sw_hash_string("banana");
    t1 = now_ns();
    printf("hash_string(\"banana\")  (%llu ns)\n", (unsigned long long)(t1 - t0));

    check_int("same string hashes the same", 1, apple1 == apple2, 0u);
    check_int("different strings hash differently", 1, apple1 != banana, 0u);
    check_u32_nonzero("apple hash nonzero", apple1, 0u);

    t0 = now_ns();
    bloom_sw_clear(&bf);
    t1 = now_ns();
    printf("bloom_sw_clear (before string tests)  (%llu ns)\n",
           (unsigned long long)(t1 - t0));

    t0 = now_ns();
    bloom_sw_insert(&bf, apple1);
    t1 = now_ns();
    printf("insert hashed apple  (%llu ns)\n", (unsigned long long)(t1 - t0));

    t0 = now_ns();
    r = bloom_sw_query(&bf, apple1);
    t1 = now_ns();
    check_int("query inserted string apple", 1, r, t1 - t0);

    t0 = now_ns();
    r = bloom_sw_query(&bf, banana);
    t1 = now_ns();
    check_int("query non-inserted string banana maybe absent", 0, r, t1 - t0);

    printf(
        "\n---- Batch: %d keys (deterministic; compare with hw_smoke) ----\n",
        BATCH_BENCH_N);
    {
        uint32_t *batch_keys = (uint32_t *)malloc(
            (size_t)BATCH_BENCH_N * sizeof(uint32_t));
        if (!batch_keys) {
            perror("malloc batch_keys");
        } else {
            int j;
            for (j = 0; j < BATCH_BENCH_N; j++) {
                batch_keys[j] =
                    0x1000u * (uint32_t)j ^ 0xA5A5A5A5u;
            }

            bloom_sw_clear(&bf);
            t0 = now_ns();
            for (j = 0; j < BATCH_BENCH_N; j++) {
                bloom_sw_insert(&bf, batch_keys[j]);
            }
            t1 = now_ns();

            uint64_t sw_ins_ns = t1 - t0;
            volatile int sw_hits = 0;
            t0 = now_ns();
            for (j = 0; j < BATCH_BENCH_N; j++) {
                sw_hits += bloom_sw_query(&bf, batch_keys[j]);
            }
            t1 = now_ns();
            uint64_t sw_q_ns = t1 - t0;

            printf(
                "batch: insert  total=%llu ns  per_key_avg=%llu ns\n",
                (unsigned long long)sw_ins_ns,
                (unsigned long long)(sw_ins_ns / (uint64_t)BATCH_BENCH_N));
            printf(
                "batch: query   total=%llu ns  per_key_avg=%llu ns  hits=%d\n",
                (unsigned long long)sw_q_ns,
                (unsigned long long)(sw_q_ns / (uint64_t)BATCH_BENCH_N),
                (int)sw_hits);
            free(batch_keys);
        }
    }

    printf("\nTests run:    %d\n", tests_run);
    printf("Tests failed: %d\n", tests_failed);

    if (tests_failed == 0) {
        printf("All software Bloom filter tests passed.\n");
        return 0;
    }

    printf("Some tests failed.\n");
    return 1;
}

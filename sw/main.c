#include "bloom.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec * 1000000000ull) + (uint64_t)ts.tv_nsec;
}

static uint32_t xorshift32(uint32_t *state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static void print_help(void) {
    printf("Commands:\n");
    printf("  insert <word>\n");
    printf("  query <word>\n");
    printf("  clear\n");
    printf("  bench <count>\n");
    printf("  status\n");
    printf("  help\n");
    printf("  exit\n");
}

static void run_benchmark(bloom_sw_t *sw, bloom_hw_t *hw, int count) {
    if (count <= 0) {
        printf("bench count must be > 0\n");
        return;
    }

    uint32_t *keys = (uint32_t *)malloc((size_t)count * sizeof(uint32_t));
    if (!keys) {
        perror("malloc");
        return;
    }

    uint32_t seed = 0x12345678u;
    for (int i = 0; i < count; i++) {
        keys[i] = xorshift32(&seed);
    }

    bloom_sw_clear(sw);
    if (bloom_hw_clear(hw) != 0) {
        printf("Hardware clear failed before benchmark\n");
        free(keys);
        return;
    }

    uint64_t sw_insert_start = now_ns();
    for (int i = 0; i < count; i++) {
        bloom_sw_insert(sw, keys[i]);
    }
    uint64_t sw_insert_end = now_ns();

    uint64_t hw_insert_start = now_ns();
    for (int i = 0; i < count; i++) {
        if (bloom_hw_insert(hw, keys[i]) != 0) {
            printf("Hardware insert failed at i=%d\n", i);
            free(keys);
            return;
        }
    }
    uint64_t hw_insert_end = now_ns();

    volatile int sw_hits = 0;
    uint64_t sw_query_start = now_ns();
    for (int i = 0; i < count; i++) {
        sw_hits += bloom_sw_query(sw, keys[i]);
    }
    uint64_t sw_query_end = now_ns();

    volatile int hw_hits = 0;
    uint64_t hw_query_start = now_ns();
    for (int i = 0; i < count; i++) {
        int present = 0;
        if (bloom_hw_query(hw, keys[i], &present) != 0) {
            printf("Hardware query failed at i=%d\n", i);
            free(keys);
            return;
        }
        hw_hits += present;
    }
    uint64_t hw_query_end = now_ns();

    uint64_t sw_insert_ns = sw_insert_end - sw_insert_start;
    uint64_t hw_insert_ns = hw_insert_end - hw_insert_start;
    uint64_t sw_query_ns  = sw_query_end - sw_query_start;
    uint64_t hw_query_ns  = hw_query_end - hw_query_start;

    printf("\nBenchmark results for %d keys\n", count);
    printf("-----------------------------------\n");
    printf("SW insert total: %llu ns\n", (unsigned long long)sw_insert_ns);
    printf("HW insert total: %llu ns\n", (unsigned long long)hw_insert_ns);
    if (hw_insert_ns != 0) {
        printf("Insert speedup:  %.2fx\n", (double)sw_insert_ns / (double)hw_insert_ns);
    }

    printf("SW query total:  %llu ns\n", (unsigned long long)sw_query_ns);
    printf("HW query total:  %llu ns\n", (unsigned long long)hw_query_ns);
    if (hw_query_ns != 0) {
        printf("Query speedup:   %.2fx\n", (double)sw_query_ns / (double)hw_query_ns);
    }

    printf("SW hits: %d\n", sw_hits);
    printf("HW hits: %d\n", hw_hits);
    printf("-----------------------------------\n\n");

    free(keys);
}

int main(void) {
    char line[256];
    bloom_sw_t sw_filter;
    bloom_hw_t hw;

    bloom_sw_init(&sw_filter);

    if (bloom_hw_open(&hw, BLOOM_BASE_ADDR) != 0) {
        fprintf(stderr, "Failed to map hardware at 0x%08X\n", BLOOM_BASE_ADDR);
        fprintf(stderr, "Check AXI base address and /dev/mem permissions.\n");
        return 1;
    }

    printf("Bloom accelerator CLI\n");
    printf("Mapped at 0x%08X\n", BLOOM_BASE_ADDR);
    print_help();

    while (1) {
        char cmd[32] = {0};
        char arg[128] = {0};

        printf("> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }

        int fields = sscanf(line, "%31s %127s", cmd, arg);
        if (fields <= 0) {
            continue;
        }

        if (strcmp(cmd, "insert") == 0) {
            if (fields < 2) {
                printf("Usage: insert <word>\n");
                continue;
            }

            uint32_t key = bloom_sw_hash_string(arg);

            bloom_sw_insert(&sw_filter, key);
            if (bloom_hw_insert(&hw, key) != 0) {
                printf("Hardware insert failed\n");
                continue;
            }

            printf("%s inserted (key=0x%08X)\n", arg, key);
        }
        else if (strcmp(cmd, "query") == 0) {
            if (fields < 2) {
                printf("Usage: query <word>\n");
                continue;
            }

            uint32_t key = bloom_sw_hash_string(arg);
            int sw_present = bloom_sw_query(&sw_filter, key);
            int hw_present = 0;

            if (bloom_hw_query(&hw, key, &hw_present) != 0) {
                printf("Hardware query failed\n");
                continue;
            }

            printf("%s -> HW: %s | SW: %s (key=0x%08X)\n",
                   arg,
                   hw_present ? "maybe present" : "definitely not present",
                   sw_present ? "maybe present" : "definitely not present",
                   key);

            if (hw_present != sw_present) {
                printf("WARNING: SW/HW mismatch detected\n");
            }
        }
        else if (strcmp(cmd, "clear") == 0) {
            bloom_sw_clear(&sw_filter);
            if (bloom_hw_clear(&hw) != 0) {
                printf("Hardware clear failed\n");
                continue;
            }
            printf("Bloom filter cleared\n");
        }
        else if (strcmp(cmd, "bench") == 0) {
            if (fields < 2) {
                printf("Usage: bench <count>\n");
                continue;
            }

            run_benchmark(&sw_filter, &hw, atoi(arg));
        }
        else if (strcmp(cmd, "status") == 0) {
            uint32_t status = bloom_hw_read_status(&hw);
            uint32_t result = bloom_hw_read_result(&hw);

            printf("STATUS = 0x%08X [DONE=%u BUSY=%u]\n",
                   status,
                   (status >> 0) & 1u,
                   (status >> 1) & 1u);

            printf("RESULT = 0x%08X [bit0=%u]\n",
                   result,
                   result & 1u);
        }
        else if (strcmp(cmd, "help") == 0) {
            print_help();
        }
        else if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
            break;
        }
        else {
            printf("Unknown command: %s\n", cmd);
            print_help();
        }
    }

    bloom_hw_close(&hw);
    return 0;
}
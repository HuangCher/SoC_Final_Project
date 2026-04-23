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
     bloom_hw_t hw;
     uint64_t t0, t1;
     int r;
     int present;
     uint32_t apple1, apple2, banana;
 
     printf("Running Bloom filter hardware tests (AXI accelerator, MMIO)...\n\n");
 
     t0 = now_ns();
     if (bloom_hw_open(&hw, BLOOM_BASE_ADDR) != 0) {
         printf("[FAIL] bloom_hw_open (mmap /dev/mem) at 0x%08X\n", BLOOM_BASE_ADDR);
         return 1;
     }
     if (bloom_hw_clear(&hw) != 0) {
         printf("[FAIL] bloom_hw_clear after open\n");
         bloom_hw_close(&hw);
         return 1;
     }
     t1 = now_ns();
     printf("bloom_hw_init (open + clear to empty)  (%llu ns)\n\n",
            (unsigned long long)(t1 - t0));
 
     t0 = now_ns();
     if (bloom_hw_query(&hw, 1234u, &present) != 0) {
         printf("[FAIL] bloom_hw_query API for key 1234\n");
         bloom_hw_close(&hw);
         return 1;
     }
     r = present;
     t1 = now_ns();
     check_int("query empty for key 1234", 0, r, t1 - t0);
 
     t0 = now_ns();
     if (bloom_hw_query(&hw, 9999u, &present) != 0) {
         printf("[FAIL] bloom_hw_query API for key 9999\n");
         bloom_hw_close(&hw);
         return 1;
     }
     r = present;
     t1 = now_ns();
     check_int("query empty for key 9999", 0, r, t1 - t0);
 
     t0 = now_ns();
     if (bloom_hw_insert(&hw, 1234u) != 0) {
         printf("[FAIL] bloom_hw_insert 1234\n");
         bloom_hw_close(&hw);
         return 1;
     }
     t1 = now_ns();
     printf("insert key 1234  (%llu ns)\n", (unsigned long long)(t1 - t0));
 
     t0 = now_ns();
     if (bloom_hw_query(&hw, 1234u, &present) != 0) {
         printf("[FAIL] bloom_hw_query after insert 1234\n");
         bloom_hw_close(&hw);
         return 1;
     }
     r = present;
     t1 = now_ns();
     check_int("insert/query same key 1234", 1, r, t1 - t0);
 
     t0 = now_ns();
     if (bloom_hw_insert(&hw, 1234u) != 0) {
         printf("[FAIL] bloom_hw_insert 1234 repeat 1\n");
         bloom_hw_close(&hw);
         return 1;
     }
     t1 = now_ns();
     printf("insert key 1234 (repeat 1)  (%llu ns)\n", (unsigned long long)(t1 - t0));
 
     t0 = now_ns();
     if (bloom_hw_insert(&hw, 1234u) != 0) {
         printf("[FAIL] bloom_hw_insert 1234 repeat 2\n");
         bloom_hw_close(&hw);
         return 1;
     }
     t1 = now_ns();
     printf("insert key 1234 (repeat 2)  (%llu ns)\n", (unsigned long long)(t1 - t0));
 
     t0 = now_ns();
     if (bloom_hw_query(&hw, 1234u, &present) != 0) {
         printf("[FAIL] bloom_hw_query repeated 1234\n");
         bloom_hw_close(&hw);
         return 1;
     }
     r = present;
     t1 = now_ns();
     check_int("repeated insert/query 1234", 1, r, t1 - t0);
 
     t0 = now_ns();
     if (bloom_hw_clear(&hw) != 0) {
         printf("[FAIL] bloom_hw_clear\n");
         bloom_hw_close(&hw);
         return 1;
     }
     t1 = now_ns();
     printf("bloom_hw_clear  (%llu ns)\n", (unsigned long long)(t1 - t0));
 
     t0 = now_ns();
     if (bloom_hw_query(&hw, 1234u, &present) != 0) {
         printf("[FAIL] bloom_hw_query after clear\n");
         bloom_hw_close(&hw);
         return 1;
     }
     r = present;
     t1 = now_ns();
     check_int("clear resets key 1234", 0, r, t1 - t0);
 
     t0 = now_ns();
     if (bloom_hw_insert(&hw, 111u) != 0) {
         printf("[FAIL] bloom_hw_insert 111\n");
         bloom_hw_close(&hw);
         return 1;
     }
     t1 = now_ns();
     printf("insert key 111  (%llu ns)\n", (unsigned long long)(t1 - t0));
 
     t0 = now_ns();
     if (bloom_hw_insert(&hw, 222u) != 0) {
         printf("[FAIL] bloom_hw_insert 222\n");
         bloom_hw_close(&hw);
         return 1;
     }
     t1 = now_ns();
     printf("insert key 222  (%llu ns)\n", (unsigned long long)(t1 - t0));
 
     t0 = now_ns();
     if (bloom_hw_insert(&hw, 333u) != 0) {
         printf("[FAIL] bloom_hw_insert 333\n");
         bloom_hw_close(&hw);
         return 1;
     }
     t1 = now_ns();
     printf("insert key 333  (%llu ns)\n", (unsigned long long)(t1 - t0));
 
     t0 = now_ns();
     if (bloom_hw_query(&hw, 111u, &present) != 0) {
         printf("[FAIL] bloom_hw_query 111\n");
         bloom_hw_close(&hw);
         return 1;
     }
     r = present;
     t1 = now_ns();
     check_int("query inserted key 111", 1, r, t1 - t0);
 
     t0 = now_ns();
     if (bloom_hw_query(&hw, 222u, &present) != 0) {
         printf("[FAIL] bloom_hw_query 222\n");
         bloom_hw_close(&hw);
         return 1;
     }
     r = present;
     t1 = now_ns();
     check_int("query inserted key 222", 1, r, t1 - t0);
 
     t0 = now_ns();
     if (bloom_hw_query(&hw, 333u, &present) != 0) {
         printf("[FAIL] bloom_hw_query 333\n");
         bloom_hw_close(&hw);
         return 1;
     }
     r = present;
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
     if (bloom_hw_clear(&hw) != 0) {
         printf("[FAIL] bloom_hw_clear before string tests\n");
         bloom_hw_close(&hw);
         return 1;
     }
     t1 = now_ns();
     printf("bloom_hw_clear (before string tests)  (%llu ns)\n",
            (unsigned long long)(t1 - t0));
 
     t0 = now_ns();
     if (bloom_hw_insert(&hw, apple1) != 0) {
         printf("[FAIL] bloom_hw_insert hashed apple\n");
         bloom_hw_close(&hw);
         return 1;
     }
     t1 = now_ns();
     printf("insert hashed apple  (%llu ns)\n", (unsigned long long)(t1 - t0));
 
     t0 = now_ns();
     if (bloom_hw_query(&hw, apple1, &present) != 0) {
         printf("[FAIL] bloom_hw_query apple\n");
         bloom_hw_close(&hw);
         return 1;
     }
     r = present;
     t1 = now_ns();
     check_int("query inserted string apple", 1, r, t1 - t0);
 
     t0 = now_ns();
     if (bloom_hw_query(&hw, banana, &present) != 0) {
         printf("[FAIL] bloom_hw_query banana\n");
         bloom_hw_close(&hw);
         return 1;
     }
     r = present;
     t1 = now_ns();
     check_int("query non-inserted string banana maybe absent", 0, r, t1 - t0);
 
     printf(
         "\n---- Batch: %d keys (deterministic; compare with test_bloom) ----\n",
         BATCH_BENCH_N);
     {
         uint32_t *batch_keys = (uint32_t *)malloc(
             (size_t)BATCH_BENCH_N * sizeof(uint32_t));
         if (!batch_keys) {
             perror("malloc batch_keys");
             tests_failed++;
         } else {
             int j;
             for (j = 0; j < BATCH_BENCH_N; j++) {
                 batch_keys[j] =
                     0x1000u * (uint32_t)j ^ 0xA5A5A5A5u;
             }
 
             if (bloom_hw_clear(&hw) != 0) {
                 printf("[FAIL] bloom_hw_clear before batch\n");
                 free(batch_keys);
                 bloom_hw_close(&hw);
                 return 1;
             }
 
             t0 = now_ns();
             for (j = 0; j < BATCH_BENCH_N; j++) {
                 if (bloom_hw_insert(&hw, batch_keys[j]) != 0) {
                     printf("[FAIL] bloom_hw_insert batch at j=%d\n", j);
                     free(batch_keys);
                     tests_failed++;
                     bloom_hw_close(&hw);
                     return 1;
                 }
             }
             t1 = now_ns();
             uint64_t hw_ins_ns = t1 - t0;
 
             int hw_hits = 0;
             t0 = now_ns();
             for (j = 0; j < BATCH_BENCH_N; j++) {
                 if (bloom_hw_query(&hw, batch_keys[j], &present) != 0) {
                     printf("[FAIL] bloom_hw_query batch at j=%d\n", j);
                     free(batch_keys);
                     tests_failed++;
                     bloom_hw_close(&hw);
                     return 1;
                 }
                 hw_hits += present;
             }
             t1 = now_ns();
             uint64_t hw_q_ns = t1 - t0;
 
             printf(
                 "batch: insert  total=%llu ns  per_key_avg=%llu ns\n",
                 (unsigned long long)hw_ins_ns,
                 (unsigned long long)(hw_ins_ns / (uint64_t)BATCH_BENCH_N));
             printf(
                 "batch: query   total=%llu ns  per_key_avg=%llu ns  hits=%d\n",
                 (unsigned long long)hw_q_ns,
                 (unsigned long long)(hw_q_ns / (uint64_t)BATCH_BENCH_N),
                 hw_hits);
             free(batch_keys);
         }
     }
 
     bloom_hw_close(&hw);
 
     printf("\nTests run:    %d\n", tests_run);
     printf("Tests failed: %d\n", tests_failed);
 
     if (tests_failed == 0) {
         printf("All hardware Bloom filter tests passed.\n");
         return 0;
     }
 
     printf("Some tests failed.\n");
     return 1;
 }
 
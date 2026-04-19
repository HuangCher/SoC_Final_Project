#include "bloom.h"
#include <stdio.h>
#include <stdint.h>

static int tests_run = 0;
static int tests_failed = 0;

static void check_int(const char *name, int expected, int actual) {
    tests_run++;
    if (expected != actual) {
        tests_failed++;
        printf("[FAIL] %s: expected %d, got %d\n", name, expected, actual);
    } else {
        printf("[PASS] %s\n", name);
    }
}

static void check_u32_nonzero(const char *name, uint32_t value) {
    tests_run++;
    if (value == 0) {
        tests_failed++;
        printf("[FAIL] %s: value was 0\n", name);
    } else {
        printf("[PASS] %s\n", name);
    }
}

int main(void) {
    bloom_sw_t bf;

    printf("Running Bloom filter software tests...\n\n");

    /* Test 1: init should clear everything */
    bloom_sw_init(&bf);
    check_int("query empty for key 1234", 0, bloom_sw_query(&bf, 1234u));
    check_int("query empty for key 9999", 0, bloom_sw_query(&bf, 9999u));

    /* Test 2: insert then query same key */
    bloom_sw_insert(&bf, 1234u);
    check_int("insert/query same key 1234", 1, bloom_sw_query(&bf, 1234u));

    /* Test 3: repeated insert should still work */
    bloom_sw_insert(&bf, 1234u);
    bloom_sw_insert(&bf, 1234u);
    check_int("repeated insert/query 1234", 1, bloom_sw_query(&bf, 1234u));

    /* Test 4: clear should reset filter */
    bloom_sw_clear(&bf);
    check_int("clear resets key 1234", 0, bloom_sw_query(&bf, 1234u));

    /* Test 5: insert multiple keys */
    bloom_sw_insert(&bf, 111u);
    bloom_sw_insert(&bf, 222u);
    bloom_sw_insert(&bf, 333u);

    check_int("query inserted key 111", 1, bloom_sw_query(&bf, 111u));
    check_int("query inserted key 222", 1, bloom_sw_query(&bf, 222u));
    check_int("query inserted key 333", 1, bloom_sw_query(&bf, 333u));

    /* Test 6: string hashing should be stable/nonzero */
    uint32_t apple1 = bloom_sw_hash_string("apple");
    uint32_t apple2 = bloom_sw_hash_string("apple");
    uint32_t banana = bloom_sw_hash_string("banana");

    check_int("same string hashes the same", 1, apple1 == apple2);
    check_int("different strings hash differently", 1, apple1 != banana);
    check_u32_nonzero("apple hash nonzero", apple1);

    /* Test 7: string insert/query */
    bloom_sw_clear(&bf);
    bloom_sw_insert(&bf, apple1);
    check_int("query inserted string apple", 1, bloom_sw_query(&bf, apple1));
    check_int("query non-inserted string banana maybe absent",
              0, bloom_sw_query(&bf, banana));

    printf("\nTests run:    %d\n", tests_run);
    printf("Tests failed: %d\n", tests_failed);

    if (tests_failed == 0) {
        printf("All software Bloom filter tests passed.\n");
        return 0;
    } else {
        printf("Some tests failed.\n");
        return 1;
    }
}
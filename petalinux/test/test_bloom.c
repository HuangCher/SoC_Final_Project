#include <stdio.h>
#include <assert.h>
#include "../bloom_driver.h"

int main() {
    bloom_init(0x44A00000u);

    printf("Running tests...\n");

    // Insert some values
    bloom_insert(10);
    bloom_insert(20);

    // Test inserted values
    assert(bloom_query(10) == 1);
    assert(bloom_query(20) == 1);

    // Test non-inserted value
    assert(bloom_query(99) == 0);

    printf("All tests passed!\n");

    return 0;
}
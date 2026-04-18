#ifndef BLOOM_DRIVER_H
#define BLOOM_DRIVER_H

#include <stdint.h>

void bloom_init(uint32_t base_addr);
void bloom_insert(uint32_t key);
int bloom_query(uint32_t key);
void bloom_cleanup();

#endif
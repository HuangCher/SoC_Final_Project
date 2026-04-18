#ifndef MOCK_BLOOM_H
#define MOCK_BLOOM_H

#include <stdint.h>

void mock_init();
void mock_write_key(uint32_t key);
void mock_write_control(uint32_t ctrl);
uint32_t mock_read_result();
uint32_t mock_read_status();

#endif
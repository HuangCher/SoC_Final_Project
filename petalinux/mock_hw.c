/*
- mock hardware interface for testing bloom filter driver without actual hardware
- simulates register reads/writes and a simple bloom filter behavior in software
*/

#include "mock_hw.h"
#include <stdio.h>

#define KEY_IN   0
#define CONTROL  1
#define RESULT   2
#define STATUS   3

// register values
static uint32_t regs[4];

// simple storage
static uint32_t mock_set[1024];

void mock_init() {
    for (int i = 0; i < 4; i++) {
        regs[i] = 0;
    }
}

void mock_write_key(uint32_t key) {
    regs[KEY_IN] = key;
}

void mock_write_control(uint32_t ctrl) {
    regs[CONTROL] = ctrl;

    // INSERT
    if (ctrl == 0x3) {
        mock_set[regs[KEY_IN] % 1024] = 1;
        regs[STATUS] = 0x1;
    }

    // QUERY
    else if (ctrl == 0x5) {
        regs[RESULT] = mock_set[regs[KEY_IN] % 1024];
        regs[STATUS] = 0x1;
    }
}

uint32_t mock_read_result() {
    return regs[RESULT];
}

uint32_t mock_read_status() {
    return regs[STATUS];
}
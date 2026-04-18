/*
- driver to interface with the bloom filter hardware accelerator
- contains functions to initialize the hardware, insert keys, 
  query keys, and clean up resources 
*/

#include "bloom_driver.h"
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

// reg offsets
#define KEY_IN   0x00
#define CONTROL  0x04
#define RESULT   0x08
#define STATUS   0x0C

#define START  0x1
#define INSERT 0x2
#define QUERY  0x4

#define CMD_INSERT (START | INSERT)  // 0x3
#define CMD_QUERY  (START | QUERY)   // 0x5

#define BUSY_MASK 0x2
#define DONE_MASK 0x1

volatile uint32_t *base;
int fd;

void bloom_init(uint32_t base_addr) {
    // open /dev/mem and mmap the hardware registers
    fd = open("/dev/mem", O_RDWR | O_SYNC);

    // error handling    
    if (fd < 0) {
        perror("Failed to open /dev/mem");
        return;
    }

    // map 4KB of memory starting at base_addr
    // mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
    base = (volatile uint32_t *) mmap(
        NULL,
        4096, // 4KB
        PROT_READ | PROT_WRITE, // allow read/write
        MAP_SHARED,
        fd,
        base_addr
    );

    // error handling
    if (base == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return;
    }
}

// wait until hardware finishes
static void wait_done() {
    while (base[STATUS/4] & BUSY_MASK);
}

void bloom_insert(uint32_t key) {
    wait_done();

    base[KEY_IN/4] = key;
    base[CONTROL/4] = CMD_INSERT;

    wait_done();
}

int bloom_query(uint32_t key) {
    wait_done();

    base[KEY_IN/4] = key;
    base[CONTROL/4] = CMD_QUERY;

    wait_done();

    return base[RESULT/4] & 0x1;
}

void bloom_cleanup() {
    munmap((void *)base, 4096);
    close(fd);
}
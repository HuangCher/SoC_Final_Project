#include "bloom.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>

static inline void reg_write32(volatile uint8_t *base, uint32_t offset, uint32_t value) {
    *(volatile uint32_t *)(base + offset) = value;
}

static inline uint32_t reg_read32(volatile uint8_t *base, uint32_t offset) {
    return *(volatile uint32_t *)(base + offset);
}

int bloom_hw_open(bloom_hw_t *hw, uint32_t base_addr) {
    if (!hw) return -1;

    hw->fd = -1;
    hw->base = NULL;
    hw->map_size = BLOOM_MAP_SIZE;
    hw->phys_addr = base_addr;

    hw->fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (hw->fd < 0) {
        fprintf(stderr, "open(/dev/mem) failed: %s\n", strerror(errno));
        return -1;
    }

    hw->base = (volatile uint8_t *)mmap(
        NULL,
        hw->map_size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        hw->fd,
        hw->phys_addr
    );

    if (hw->base == MAP_FAILED) {
        fprintf(stderr, "mmap failed: %s\n", strerror(errno));
        close(hw->fd);
        hw->fd = -1;
        hw->base = NULL;
        return -1;
    }

    return 0;
}

void bloom_hw_close(bloom_hw_t *hw) {
    if (!hw) return;

    if (hw->base && hw->base != MAP_FAILED) {
        munmap((void *)hw->base, hw->map_size);
        hw->base = NULL;
    }

    if (hw->fd >= 0) {
        close(hw->fd);
        hw->fd = -1;
    }
}

uint32_t bloom_hw_read_status(bloom_hw_t *hw) {
    if (!hw || !hw->base) return 0;
    return reg_read32(hw->base, REG_STATUS);
}

uint32_t bloom_hw_read_result(bloom_hw_t *hw) {
    if (!hw || !hw->base) return 0;
    return reg_read32(hw->base, REG_RESULT) & 0x1u;
}

static int bloom_hw_wait_not_busy(bloom_hw_t *hw, uint32_t timeout_iters) {
    if (!hw || !hw->base) return -1;

    while (timeout_iters--) {
        uint32_t status = bloom_hw_read_status(hw);
        if ((status & STATUS_BUSY_MASK) == 0u) {
            return 0;
        }
    }

    fprintf(stderr, "Timeout waiting for BUSY=0\n");
    return -1;
}

static int bloom_hw_wait_done(bloom_hw_t *hw, uint32_t timeout_iters) {
    if (!hw || !hw->base) return -1;

    while (timeout_iters--) {
        uint32_t status = bloom_hw_read_status(hw);
        if (status & STATUS_DONE_MASK) {
            return 0;
        }
    }

    fprintf(stderr, "Timeout waiting for DONE=1\n");
    return -1;
}

int bloom_hw_insert(bloom_hw_t *hw, uint32_t key) {
    if (!hw || !hw->base) return -1;

    if (bloom_hw_wait_not_busy(hw, 1000000u) != 0) return -1;

    reg_write32(hw->base, REG_KEY_IN, key);
    reg_write32(hw->base, REG_CONTROL, CMD_INSERT);

    if (bloom_hw_wait_done(hw, 1000000u) != 0) return -1;

    return 0;
}

int bloom_hw_query(bloom_hw_t *hw, uint32_t key, int *maybe_present) {
    if (!hw || !hw->base || !maybe_present) return -1;

    if (bloom_hw_wait_not_busy(hw, 1000000u) != 0) return -1;

    reg_write32(hw->base, REG_KEY_IN, key);
    reg_write32(hw->base, REG_CONTROL, CMD_QUERY);

    if (bloom_hw_wait_done(hw, 1000000u) != 0) return -1;

    *maybe_present = (int)(bloom_hw_read_result(hw) & 0x1u);
    return 0;
}

int bloom_hw_clear(bloom_hw_t *hw) {
    if (!hw || !hw->base) return -1;

    if (bloom_hw_wait_not_busy(hw, 1000000u) != 0) return -1;

    reg_write32(hw->base, REG_CONTROL, CMD_CLEAR);

    if (bloom_hw_wait_done(hw, 1000000u) != 0) return -1;

    return 0;
}
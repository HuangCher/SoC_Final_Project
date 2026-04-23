#ifndef BLOOM_H
#define BLOOM_H

#include <stdint.h>
#include <stddef.h>


#define BLOOM_SW_BITS   1024u
#define BLOOM_SW_WORDS  (BLOOM_SW_BITS / 32u)

typedef struct {
    uint32_t bits[BLOOM_SW_WORDS];
} bloom_sw_t;

// Hardware MMIO settings
#define BLOOM_MAP_SIZE      0x1000u

#ifndef BLOOM_BASE_ADDR
#define BLOOM_BASE_ADDR     0x44A00000u
#endif

// Register offsets
#define REG_KEY_IN          0x00u
#define REG_CONTROL         0x04u
#define REG_RESULT          0x08u
#define REG_STATUS          0x0Cu

// CONTROL command values
#define CMD_INSERT          0x3u   // START INSERT
#define CMD_QUERY           0x5u   // START QUERY
#define CMD_CLEAR           0x9u   // START CLEAR

// STATUS bits
#define STATUS_DONE_MASK    (1u << 0)
#define STATUS_BUSY_MASK    (1u << 1)

typedef struct {
    int fd;
    volatile uint8_t *base;
    size_t map_size;
    uint32_t phys_addr;
} bloom_hw_t;

// Software Bloom filter API
void bloom_sw_init(bloom_sw_t *bf);
void bloom_sw_clear(bloom_sw_t *bf);
void bloom_sw_insert(bloom_sw_t *bf, uint32_t key);
int  bloom_sw_query(const bloom_sw_t *bf, uint32_t key);
uint32_t bloom_sw_hash_string(const char *s);

// Hardware Bloom filter API
int      bloom_hw_open(bloom_hw_t *hw, uint32_t base_addr);
void     bloom_hw_close(bloom_hw_t *hw);
int      bloom_hw_insert(bloom_hw_t *hw, uint32_t key);
int      bloom_hw_query(bloom_hw_t *hw, uint32_t key, int *maybe_present);
int      bloom_hw_clear(bloom_hw_t *hw);
uint32_t bloom_hw_read_status(bloom_hw_t *hw);
uint32_t bloom_hw_read_result(bloom_hw_t *hw);

#endif
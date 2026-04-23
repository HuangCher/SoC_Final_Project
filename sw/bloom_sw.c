#include "bloom.h"
#include <string.h>

static inline uint32_t bloom_hash1(uint32_t key) {
    return key ^ (key >> 16);
}

static inline uint32_t bloom_hash2(uint32_t key) {
    return key * 0x045D9F3Bu;
}

static inline uint32_t bloom_hash3(uint32_t key) {
    return key + (key << 6) + (key >> 2);
}

static inline uint32_t bloom_index(uint32_t hash) {
    return hash & 0x3FFu;
}

static inline void bloom_set_bit(bloom_sw_t *bf, uint32_t idx) {
    bf->bits[idx >> 5] |= (1u << (idx & 31u));
}

static inline uint32_t bloom_get_bit(const bloom_sw_t *bf, uint32_t idx) {
    return (bf->bits[idx >> 5] >> (idx & 31u)) & 1u;
}

void bloom_sw_init(bloom_sw_t *bf) {
    if (!bf) return;
    memset(bf->bits, 0, sizeof(bf->bits));
}

void bloom_sw_clear(bloom_sw_t *bf) {
    if (!bf) return;
    memset(bf->bits, 0, sizeof(bf->bits));
}

void bloom_sw_insert(bloom_sw_t *bf, uint32_t key) {
    if (!bf) return;

    uint32_t i1 = bloom_index(bloom_hash1(key));
    uint32_t i2 = bloom_index(bloom_hash2(key));
    uint32_t i3 = bloom_index(bloom_hash3(key));

    bloom_set_bit(bf, i1);
    bloom_set_bit(bf, i2);
    bloom_set_bit(bf, i3);
}

int bloom_sw_query(const bloom_sw_t *bf, uint32_t key) {
    if (!bf) return 0;

    uint32_t i1 = bloom_index(bloom_hash1(key));
    uint32_t i2 = bloom_index(bloom_hash2(key));
    uint32_t i3 = bloom_index(bloom_hash3(key));

    return bloom_get_bit(bf, i1) &&
           bloom_get_bit(bf, i2) &&
           bloom_get_bit(bf, i3);
}

uint32_t bloom_sw_hash_string(const char *s) {
    const uint32_t FNV_OFFSET = 2166136261u;
    const uint32_t FNV_PRIME  = 16777619u;

    if (!s) return 0;

    uint32_t hash = FNV_OFFSET;
    while (*s) {
        hash ^= (uint8_t)(*s++);
        hash *= FNV_PRIME;
    }
    return hash;
}
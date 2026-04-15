#include <systemc>
#include <array>
#include <cstdint>
#include <iostream>

using namespace sc_core;
using namespace std;

class BloomModel {
private:
    static const uint32_t FILTER_SIZE = 1024;
    std::array<bool, FILTER_SIZE> bits;

    uint32_t hash1(uint32_t key) const {
        return key ^ (key >> 16);
    }

    uint32_t hash2(uint32_t key) const {
        return key * 0x45d9f3b;
    }

    uint32_t hash3(uint32_t key) const {
        return key + (key << 6) + (key >> 2);
    }

    uint32_t index(uint32_t hash) const {
        return hash & 0x3FF;   // maps into 0..1023
    }

public:
    BloomModel() {
        clear();
    }

    void clear() {
        bits.fill(false);
    }

    void insert(uint32_t key) {
        bits[index(hash1(key))] = true;
        bits[index(hash2(key))] = true;
        bits[index(hash3(key))] = true;
    }

    bool query(uint32_t key) const {
        return bits[index(hash1(key))] &&
               bits[index(hash2(key))] &&
               bits[index(hash3(key))];
    }

    void print_result(const std::string& label, bool result) const {
        cout << label << ": "
             << (result ? "maybe present" : "definitely not present")
             << endl;
    }
};

int sc_main(int argc, char* argv[]) {
    BloomModel bloom;

    uint32_t key1 = 42;
    uint32_t key2 = 99;

    cout << "=== Bloom Filter SystemC Model Test ===" << endl;

    // Test 1: query empty filter
    bloom.print_result("Query key1 before insert", bloom.query(key1));

    // Test 2: insert then query same key
    bloom.insert(key1);
    bloom.print_result("Query key1 after insert", bloom.query(key1));

    // Test 3: query different key
    bloom.print_result("Query key2 without insert", bloom.query(key2));

    // Test 4: clear and query again
    bloom.clear();
    bloom.print_result("Query key1 after clear", bloom.query(key1));

    return 0;
}
#include <systemc>
#include <array>
#include <cstdint>
#include <iostream>
#include <string>
using namespace sc_core;
using namespace std;

class BloomModel {
private:
    static const uint32_t FILTER_SIZE = 1024;
    std::array<bool, FILTER_SIZE> bits;

    uint32_t hash1(uint32_t key) const { return key ^ (key >> 16); }
    uint32_t hash2(uint32_t key) const { return key * 0x45d9f3b; }
    uint32_t hash3(uint32_t key) const { return key + (key << 6) + (key >> 2); }
    uint32_t index(uint32_t hash) const { return hash & 0x3FF; }

public:
    BloomModel() { clear(); }

    void clear() { bits.fill(false); }

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
};

bool check_test(const string& test_name, bool actual, bool expected) {
    bool pass = (actual == expected);
    cout << test_name << ": "
         << (pass ? "PASS" : "FAIL")
         << " (expected " << expected
         << ", got " << actual << ")" << endl;
    return pass;
}

int sc_main(int argc, char* argv[]) {
    BloomModel bloom;
    int passed = 0;
    int total = 0;

    uint32_t key1 = 42;
    uint32_t key2 = 99;
    uint32_t key3 = 1234;
    uint32_t key4 = 5678;

    cout << "Bloom Filter SystemC Testbench" << endl;

    total++;
    if (check_test("Test 1: Query empty filter", bloom.query(key1), false)) passed++;

    bloom.insert(key1);
    total++;
    if (check_test("Test 2: Query inserted key", bloom.query(key1), true)) passed++;

    total++;
    bool key2_result = bloom.query(key2);
    cout << "Test 3: Query absent key: "
         << (key2_result ? "maybe present (false positive)" : "definitely not present") << endl;
    passed++;

    bloom.insert(key3);
    bloom.insert(key4);

    total++;
    if (check_test("Test 4: Query second inserted key", bloom.query(key3), true)) passed++;

    total++;
    if (check_test("Test 5: Query third inserted key", bloom.query(key4), true)) passed++;

    bloom.insert(key1);
    total++;
    if (check_test("Test 6: Repeated insert still works", bloom.query(key1), true)) passed++;

    bloom.clear();
    total++;
    if (check_test("Test 7: Query key1 after clear", bloom.query(key1), false)) passed++;
    total++;
    if (check_test("Test 8: Query key3 after clear", bloom.query(key3), false)) passed++;

    cout << "\nSummary: " << passed << " / " << total << " tests passed." << endl;
    return 0;
}
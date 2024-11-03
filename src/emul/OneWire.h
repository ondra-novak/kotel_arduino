#pragma once
#include <cstdint>


class OneWire {
public:

    OneWire() = default;
    OneWire(int) {};
    //nothing implemented


    void begin(int) {}
    bool search(uint8_t *newAddr, bool search_mode = true );
    void reset_search();
    static uint8_t crc8(const uint8_t *addr, int count);
    bool reset();
    void select(const uint8_t *addr);
    void write(uint8_t);
    int read();
};


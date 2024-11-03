#pragma once
#include <cstdint>

namespace Matrix {
    void init();
    void set_pixel(uint8_t x, uint8_t y, bool val);
    bool get_pixel(uint8_t x, uint8_t y);
    void clear();
}

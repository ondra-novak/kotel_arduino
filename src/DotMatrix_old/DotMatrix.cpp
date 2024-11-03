#include "../DotMatrix_old/DotMatrix.h"

#include <Arduino_LED_Matrix.h>

namespace Matrix {


ArduinoLEDMatrix matrix;

void init() {
    matrix.begin();
}

void set_pixel(uint8_t x, uint8_t y, bool val) {
    if (x < 8 && y < 12) {
        int pinindex = y + 12* x;
        auto ofs = pinindex >> 8;
        auto &v = framebuffer[ofs];
        if (val) {
            v |= (1 <<  (pinindex & 0x7));
        } else {
            v &= ~(1 <<  (pinindex & 0x7));
        }
    }
}

bool get_pixel(uint8_t x, uint8_t y) {
    if (x < 8 && y < 12) {
        int pinindex = y + 12* x;
        auto ofs = pinindex >> 8;
        auto &v = framebuffer[ofs];
        return (v & (1 << (pinindex & 0x7))) != 0;
    }
}

void clear() {
    std::fill(std::begin(framebuffer),std::end(framebuffer), 0);
}



}

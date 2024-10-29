#include "DotMatrix.h"

#include <algorithm>
#include <iterator>
namespace Matrix {

uint8_t framebuffer[12];
bool frame_changed = false;


void init() {
}

void set_pixel(uint8_t x, uint8_t y, bool val) {
    if (x < 8 && y < 12) {
        int pinindex = y + 12* x;
        auto ofs = pinindex >> 3;
        auto v = framebuffer[ofs];
        auto w = v;

        if (val) {
            w |= (1 <<  (pinindex & 0x7));
        } else {
            w &= ~(1 <<  (pinindex & 0x7));
        }
        if (w != v) {
            framebuffer[ofs] = w;
            frame_changed = true;
        }
    }
}

bool get_pixel(uint8_t x, uint8_t y) {
    if (x < 8 && y < 12) {
        int pinindex = y + 12* x;
        auto ofs = pinindex >> 3;
        auto &v = framebuffer[ofs];
        return (v & (1 << (pinindex & 0x7))) != 0;
    }
    return false;
}


void clear() {
    std::fill(std::begin(framebuffer),std::end(framebuffer), 0);
    frame_changed = true;
}



}

#pragma once
#include <cstdint>

namespace Matrix {

void render_character_6x4(uint8_t x, uint8_t y, int ascii_char);

enum class Icon {
    wifi_failed = 0,
    wifi_success,
    tray_open

};

void render_icon(uint8_t x, uint8_t y, Icon icon);

}



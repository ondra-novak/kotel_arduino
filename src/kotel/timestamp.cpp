#include "timestamp.h"

static uint32_t time_offset;

uint32_t get_current_time() {
    return static_cast<uint32_t>(get_current_timestamp()/1000)+time_offset;
}
void set_current_time(uint32_t t) {
    time_offset = 0;
    auto z = get_current_time();
    time_offset = t - z;
}

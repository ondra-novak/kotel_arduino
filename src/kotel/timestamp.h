#pragma once

#include "Arduino.h"

///Local timestamp - time from start in milliseconds no overflow

using TimeStampMs = uint64_t;

inline TimeStampMs get_current_timestamp() {
    static unsigned long last_milli = 0;
    static TimeStampMs last_ms = 0;
    unsigned long n = millis();
    auto diff = n - last_ms;
    last_milli += diff;
    last_ms = n;
    return last_milli;
}

constexpr int32_t from_seconds(int seconds) {
    return static_cast<int64_t>(seconds * 1000);
}

constexpr int32_t from_minutes(int minutes) {
    return static_cast<int64_t>(minutes * 60000);
}

constexpr TimeStampMs max_timestamp = ~TimeStampMs{};


uint32_t get_current_time();
void set_current_time(uint32_t t);



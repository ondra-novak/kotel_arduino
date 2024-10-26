#pragma once

#include "api/Common.h"

///Local timestamp - time from start in milliseconds no overflow

using TimeStampMs = uint64_t;

inline TimeStampMs get_current_timestamp() {
    static unsigned long last_milli = 0;
    static TimeStampMs last_ms = 0;
    unsigned long n = millis();
    auto diff = n - last_ms;
    last_milli += diff;
    return last_milli;


}


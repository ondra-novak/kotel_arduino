#include "timestamp.h"

static uint32_t time_offset = 0;
static bool time_synced = false;

uint32_t get_current_time() {
    return static_cast<uint32_t>(get_current_timestamp()/1000)+time_offset;
}
void set_current_time(uint32_t t) {
    time_offset = 0;
    auto z = get_current_time();
    time_offset = t - z;
    time_synced = true;
}

bool is_time_synced() {
    return time_synced;
}

#include <stdio.h>
#include <time.h>

void day_to_date(long days, char *buf, size_t buflen) {
    time_t t = (time_t)days * 86400;
    struct tm tm = *gmtime(&t);
    strftime(buf, buflen, "%Y-%m-%d", &tm);
}

#pragma once

#include "timestamp.h"

namespace kotel {

class IScheduler {
public:
    virtual ~IScheduler() = default;
    virtual void reschedule() = 0;
};


class AbstractTimedTask {
public:
    static constexpr TimeStampMs disabled_task = static_cast<TimeStampMs>(-1);
    virtual TimeStampMs get_scheduled_time() const = 0;
    virtual void run(TimeStampMs cur_time) = 0;
    virtual ~AbstractTimedTask() = default;

    unsigned long _run_time = 0;
};


}

#pragma once

#include "timestamp.h"

namespace kotel {



class ITimedTask {
public:
    static constexpr TimeStampMs disabled_task = static_cast<TimeStampMs>(-1);
    virtual TimeStampMs get_scheduled_time() const = 0;
    virtual void run(TimeStampMs cur_time) = 0;
    virtual ~ITimedTask() = default;
};


}

#pragma once

#include "timed_task.h"

namespace kotel {


class Podavac : public ITimedTask {
public:
    virtual void run(TimeStampMs ) override {}
    virtual TimeStampMs get_scheduled_time() const override {return 0;}

};


}


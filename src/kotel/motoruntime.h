#pragma once
#include "timed_task.h"

namespace kotel {

class Controller;

class MotoRunTime: public AbstractTimedTask {
public:


    MotoRunTime(Controller &cntr): _cntr(cntr) {}

    virtual TimeStampMs get_scheduled_time() const override;
    virtual void run(TimeStampMs cur_time) override;

protected:

    TimeStampMs _next_call = 0;
    TimeStampMs _flush_time = max_timestamp;
    Controller &_cntr;

};

}

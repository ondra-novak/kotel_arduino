#pragma once

#include "timed_task.h"

namespace kotel {


class Cerpadlo : public AbstractTimedTask { // @suppress("Miss copy constructor or assignment operator")
public:
    virtual void run(TimeStampMs ) override {}
    virtual TimeStampMs get_scheduled_time() const override {return max_timestamp;}

};


}


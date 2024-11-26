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
    virtual const char *name() const = 0;

    unsigned long _run_time = 0;
};

template<typename X, TimeStampMs (X::*task)(TimeStampMs cur_time), const char *&_name>
class TimedTaskMethod: public AbstractTimedTask {
public:

    TimedTaskMethod(X *object):_object(object) {}
    virtual TimeStampMs get_scheduled_time() const override {
        return _next_call;
    }
    virtual void run(TimeStampMs cur_time) override {
        _next_call = cur_time + ((*_object).*task)(cur_time);
    }
    void wake_up(IScheduler &sch) {
        _next_call = 0;
        sch.reschedule();
    }
    virtual const char *name() const  override{
        return _name;
    }


protected:
    X *_object;
    TimeStampMs _next_call = 0;
};


}

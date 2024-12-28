#pragma once

#include "timestamp.h"

namespace kotel {

class IScheduler {
public:
    virtual ~IScheduler() = default;
    virtual void reschedule() = 0;
};


class AbstractTask {
public:
    static constexpr TimeStampMs disabled_task = static_cast<TimeStampMs>(-1);
    virtual void run(TimeStampMs cur_time) = 0;
    virtual ~AbstractTask() = default;

    unsigned long _run_time = 0;

    void resume_at(IScheduler &sch, TimeStampMs at) {
        if (at != _scheduled_time) {
           _scheduled_time = at;
           sch.reschedule();
        }
    }
    void stop(IScheduler &sch) {
        resume_at(sch, disabled_task);
    }
    TimeStampMs get_scheduled_time() const {return _scheduled_time;}

    void resume(TimeStampMs cur_time) {
        _scheduled_time = disabled_task;
        auto start = millis();
        run(cur_time);
        auto util = millis() - start;;
        auto rt = _run_time;
        if (util > rt) {
            _run_time = util;
        } else if (util < rt) {
            _run_time -= (rt - util)/10;
        }
    }
protected:
    TimeStampMs _scheduled_time = 0;
    void resume_at(TimeStampMs at) {
        _scheduled_time = at;
    }
    void stop() {
        _scheduled_time = disabled_task;
    }

};

template<typename X, TimeStampMs (X::*task)(TimeStampMs cur_time)>
class TaskMethod: public AbstractTask { // @suppress("Miss copy constructor or assignment operator")
public:

    TaskMethod(X *object):_object(object) {}
    virtual void run(TimeStampMs cur_time) override {
        auto next_call = cur_time + ((*_object).*task)(cur_time);
        resume_at(next_call);
    }
    void wake_up(IScheduler &sch) {
        resume_at(sch, 0);
    }


protected:
    X *_object;
    TimeStampMs _next_call = 0;
};


}

#pragma once
#include "timed_task.h"
#include "constants.h"
#include "nonv_storage.h"
namespace kotel {

class Feeder: public AbstractTimedTask {
public:




    Feeder(Storage &storage):_storage(storage) {}

    void begin() {
        set_active(false);
    }

    virtual TimeStampMs get_scheduled_time() const override {
        return _stop_time;
    }
    virtual void run(TimeStampMs cur_time) override {
        if (_stop_time <= cur_time) {
            set_active(false);
            _stop_time = max_timestamp;
        }
    }
    void stop() {
        set_active(false);
        _stop_time = max_timestamp;
    }

    void keep_running(IScheduler &sch, TimeStampMs until) {
        _stop_time = until;
        if (!_active) {
            set_active(true);
            sch.reschedule();
        }
    }

    bool is_active() const {return _active;}


protected:
    Storage &_storage;
    bool _active = true;
    TimeStampMs _stop_time = 0;

    bool set_active(bool a) {
        if (_active != a) {
            if (a) {
                ++_storage.cntr1.feeder_start_count;
            }
            pinMode(pin_out_feeder_on, a?active_feeder:inactive_feeder);
            _active = a;
            return true;
        } else {
            return false;
        }
    }
};



}

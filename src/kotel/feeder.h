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
        return _next_call;
    }
    virtual void run(TimeStampMs cur_time) override {
        if (_cycle) {
            set_active(!_active);
            if (_active) {
                _next_call = cur_time + from_seconds(_storage.config.feeder_on_sec);
            } else {
                _next_call = cur_time + from_seconds(_storage.config.feeder_off_sec);
            }


        } else {
            set_active( false);
            _next_call = max_timestamp;
        }
    }

    void stop() {
        set_active(false);
        _cycle = false;
    }

    void one_shot(IScheduler &sch, unsigned int seconds_to_run) {
        set_active(true);
        _next_call = get_current_timestamp() + from_seconds(seconds_to_run);
        sch.reschedule();
    }

    void keep_running(IScheduler &sch) {
        if (!is_cycling()) begin_cycle(sch, false);
    }

    void begin_cycle(IScheduler &sch, bool after_attenuation) {
        if (after_attenuation) {
            one_shot(sch, _storage.config.feeder_first_on_sec);
        } else {
            one_shot(sch, _storage.config.feeder_on_sec);
        }
        _cycle = true;
    }


    bool is_active() const {return _active;}
    bool is_cycling() const {return _cycle;}

protected:
    Storage &_storage;
    bool _active = false;
    bool _cycle = false;
    TimeStampMs _next_call = 0;

    bool set_active(bool a) {
        if (_active != a) {
            if (a) {
                ++_storage.cntr1.feeder_start_count;
            }
            digitalWrite(pin_out_feeder_on, a?active_feeder:inactive_feeder);
            _active = a;
            return true;
        } else {
            return false;
        }
    }
};



}

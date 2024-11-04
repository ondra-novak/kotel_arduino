#pragma once
#include "timed_task.h"
#include "constants.h"
#include "nonv_storage.h"
namespace kotel {

class Feeder: public AbstractTimedTask {
public:


    enum Mode {
        stop,
        timed_run,
        cycle,
        cycle_after_atten
    };


    Feeder(Storage &storage):_storage(storage) {}


    virtual TimeStampMs get_scheduled_time() const override {
        return _next_call;
    }
    virtual void run(TimeStampMs cur_time) override {
        if (_cur_mode == cycle ) {

            set_active(cur_time, !_active);
            if (_active) {
                _next_call = cur_time + from_seconds(_storage.config.feeder_on_sec);
            } else {
                _next_call = cur_time + from_seconds(_storage.config.feeder_off_sec);
            }


        } else {
            set_active(cur_time, false);
            _next_call = max_timestamp;
        }
    }


    void set_mode(IScheduler &sch, Mode mode) {
        auto now = get_current_timestamp();
        if (mode != _cur_mode) {
            switch (mode) {
                case stop: set_active(now, false);break;
                case timed_run: if (set_active(now, true)) {
                                    _next_call = now + from_minutes(2);
                                    sch.reschedule();
                                }
                                break;

                case cycle: if (set_active(now, true)) {
                                    _next_call = now + from_seconds(_storage.config.feeder_on_sec);
                                    sch.reschedule();
                            }
                            break;
                case cycle_after_atten:
                            if (set_active(now, true)) {
                                _next_call = now + from_seconds(_storage.config.feeder_first_on_sec);
                                sch.reschedule();
                                mode = cycle;
                            }
                            break;
            }
            _cur_mode = mode;
        }
    }

    bool is_active() const {return _active;}

protected:
    Storage &_storage;
    Mode _cur_mode;
    bool _active = false;
    TimeStampMs _next_call = 0;
    TimeStampMs _start_time = 0;

    bool set_active(TimeStampMs cur_time, bool a) {
        if (_active != a) {
            if (a) {
                _start_time = cur_time;
                ++_storage.cntr1.feeder_start_count;
                _storage.save();
            } else {
                _storage.tray.feeder_time += cur_time - _start_time;
                _storage.save();
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

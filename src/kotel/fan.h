#pragma once
#include "timed_task.h"

#include "nonv_storage.h"
namespace kotel {

class Fan: public AbstractTimedTask {
public:

    enum Mode {
        stop,
        running,
        run_away
    };

    Fan(Storage &stor):_stor(stor),_mode(stop) {}

    virtual TimeStampMs get_scheduled_time() const override {
        return _next_call;
    }
    virtual void run(TimeStampMs cur_time) override {
        if (_mode == stop) {
            _next_call = max_timestamp;
            set_active(false);
        } else {
            TimeStampMs plen = static_cast<TimeStampMs>(_stor.config.fan_pulse)*20;
            _speed = static_cast<unsigned int>(_stor.config.fan_power_pct);
            if (_mode == run_away) {
                if (cur_time > _decay_begin) {
                    if (cur_time < _decay_end) {
                        _speed = ((_decay_end - cur_time)
                                * static_cast<unsigned int>(_stor.config.fan_power_pct))
                                        /(_decay_end - _decay_begin);
                    } else {
                        _speed = 0;
                    }
                    if (_speed < 1) {
                        set_active(false);
                        _mode =stop;
                        return;
                    }
                }
            }

            TimeStampMs gap = plen * 100 / _speed - plen;

            if (gap == 0) {
                set_active(true);
            } else if (_pulse) {
                set_active(false);
                _next_call = cur_time + gap;
            } else {
                set_active(true);
                _next_call = cur_time + plen;
            }
        }
    }

    void set_mode(IScheduler &sch, Mode mode) {
        auto now = get_current_timestamp();
        switch (mode) {
            default:
            case stop:
                _mode = stop;
                break;
            case running:
                if (_mode != running) {
                    _mode = running;
                    _next_call = now;
                    sch.reschedule();
                    ++_stor.cntr1.fan_start_count;
                }
                break;
            case run_away:
                if (_mode == running) {
                    _mode = run_away;
                    _decay_begin = now+from_seconds(_stor.config.feeder_off_sec);
                    _decay_end = _decay_begin+from_seconds(_stor.config.fan_rundown_sec);
                }
                break;
        }
    }

    int get_speed() const {
        if (_mode == stop) return 0;
        return _speed;
    }

    bool is_active() const {
        return _mode != stop;
    }

protected:

    TimeStampMs _next_call = 0;
    TimeStampMs _decay_begin = 0;
    TimeStampMs _decay_end = 0;
    unsigned int _speed;
    Storage &_stor;
    bool _pulse = false;
    Mode _mode;

    void set_active(bool p) {
        if (p != _pulse) {
            _pulse = p;
            digitalWrite(pin_out_fan_on, p?active_fan:inactive_fan);
        }
    }



};

}


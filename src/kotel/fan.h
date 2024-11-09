#pragma once
#include "timed_task.h"

#include "nonv_storage.h"
namespace kotel {

class Fan: public AbstractTimedTask {
public:



    Fan(Storage &stor):_stor(stor) {}


    void begin() {
        set_active(false);
    }

    void stop() {
        _speed = 0;
    }


    void rundown(unsigned int wait_time, unsigned int rundown_time) {
        _rundown_begin_speed = _speed;
        _rundown_begin_time = get_current_timestamp()+from_seconds(wait_time);
        _rundown_time = rundown_time;
    }

    void rundown() {
        rundown(_stor.config.feeder_off_sec, _stor.config.fan_rundown_sec);
    }

    void set_speed(uint8_t speed) {
        _speed = speed;
    }

    void one_shot(IScheduler &sch, unsigned int time) {
        one_shot(sch, _stor.config.fan_power_pct, time);
    }

    void one_shot(IScheduler &sch, uint8_t speed, unsigned int time) {
        _speed = speed;
        _rundown_begin_speed = _speed;
        _rundown_begin_time = get_current_timestamp()+ from_seconds(time);
        _rundown_time = 0;
        if (_next_call == max_timestamp) {
            _next_call = 0;
            sch.reschedule();
        }

    }

    void start(IScheduler &sch, uint8_t speed) {
        _speed = speed;
        _rundown_begin_time = max_timestamp;
        if (_next_call == max_timestamp) {
            _next_call = get_current_timestamp();
            sch.reschedule();
        }
    }

    void keep_running(IScheduler &sch) {
        start(sch);
    }

    void start(IScheduler &sch) {
        start(sch, _stor.config.fan_power_pct);
    }


    virtual TimeStampMs get_scheduled_time() const override {
        return _next_call;
    }
    virtual void run(TimeStampMs cur_time) override {
        if (_rundown_begin_time < cur_time) {
            TimeStampMs end = _rundown_begin_time + from_seconds(_rundown_time);
            if (end < cur_time) {
                _speed = 0;
            } else {
                _speed = _rundown_begin_speed * (end - cur_time )/(end - _rundown_begin_time);
            }
        }
        if (_speed == 0) {
            _next_call = max_timestamp;
            set_active(false);
            return;
        }
        TimeStampMs plen = static_cast<TimeStampMs>(_stor.config.fan_pulse)*20;
        TimeStampMs gap = plen * 100 / _speed - plen;
        if (_pulse && gap > 0) {
            set_active(false);
            _next_call =  cur_time + gap;
        } else {
            set_active(true);
            _next_call =  cur_time + plen;
        }
    }

    int get_speed() const {
        return _speed;
    }

    bool is_active() const {
        return _speed > 0;
    }

protected:

    Storage &_stor;
    bool _pulse = false;
    uint8_t _speed = 0;
    uint8_t _rundown_begin_speed = 0;
    TimeStampMs _rundown_begin_time = ~max_timestamp;
    TimeStampMs _next_call = 0;
    unsigned long _rundown_time = 0;


    void set_active(bool p) {
        if (p != _pulse) {
            _pulse = p;
            digitalWrite(pin_out_fan_on, p?active_fan:inactive_fan);
        }
    }



};

}


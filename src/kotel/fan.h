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

    void keep_running(IScheduler &sch, TimeStampMs until) {
        _stop_time = until;
        if (!_running) {
            _running = true;
            _next_call = 0;
            sch.reschedule();
            ++_stor.cntr1.fan_start_count;
        }
    }


    void stop() {
        _running = false;
    }

    void set_speed(uint8_t speed) {
        _speed = speed;
    }


    int get_current_speed() const {
        return _running?_speed:0;
    }

    int get_speed() const {
        return _speed;
    }


    bool is_active() const {
        return _running;
    }

    virtual TimeStampMs get_scheduled_time() const override {return _next_call;}
    virtual void run(TimeStampMs cur_time) override {
        if (_stop_time <= cur_time || !_running) {
            set_active(false);
            _running = false;
            _next_call = max_timestamp;
            return;
        }
        auto pln = static_cast<unsigned int>(_stor.config.fan_pulse_count);
        auto spd = std::max<int>(std::min<int>(_speed, 100),1);
        pln = std::max<unsigned int>(11, pln * spd / 100);
        if (_pulse) {
            auto total = pln * 100 / spd;
            auto rest = total - pln;
            if (rest > 0)  {
                _next_call = cur_time+rest;
                set_active(!_pulse);
            } else {
                _next_call = cur_time+pln;
            }
        } else {
            set_active(!_pulse);
            _next_call = cur_time+pln;
        }
    }

protected:

    Storage &_stor;
    bool _pulse = true;
    bool _running = false;
    uint8_t _speed = 100;
    TimeStampMs _stop_time = 0;
    TimeStampMs _next_call = 0;


    void set_active(bool p) {
        if (p != _pulse) {
            _pulse = p;
            pinMode(pin_out_fan_on, p?active_fan: inactive_fan);
        }
    }



};

}


#pragma once
#include "nonv_storage.h"
#include "task.h"
#include <cmath>
namespace kotel {

class Fan: public AbstractTask {
public:

// (\ln(1/(-e^{t}\cdot x+e^{t}+x))+t)/t

    Fan(Storage &stor):_stor(stor) {}


    void begin() {
        set_active(false);
    }

    void keep_running(TimeStampMs until) {
        _stop_time = until;
        if (!_running) {
            _running = true;
            resume_at(0);
            ++_stor.cntr.fan_start_count;
        }
    }


    void stop() {
        _running = false;
    }

    void set_speed(uint8_t speed) {
        _speed = speed;
        float s = speed * 0.01;
        float t = _stor.config.fan_nonlinear_correction.value *0.1f;
        // (ln(1/(e^t * (1-s) + s)) + t)/t
        _adj_speed = (std::log(1.0f/(std::exp(t)*(1-s))+s)+t)/t;
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

    virtual void run(TimeStampMs cur_time) override {
        if (_stop_time <= cur_time || !_running) {
            set_active(false);
            _running = false;
            AbstractTask::stop();
            return;
        }
        unsigned int interval = _stor.config.fan_pulse_interval;
        auto pln = std::clamp(
                static_cast<unsigned int>(interval * _adj_speed),
                        11U, interval);

        if (_pulse) {
            unsigned int  rest = interval - pln;
            if (rest > 0)  {
                resume_at(cur_time + rest);
                set_active(!_pulse);
            } else {
                resume_at(cur_time+pln);
            }
        } else {
            set_active(!_pulse);
            resume_at(cur_time+pln);
        }
    }

    bool is_pulse() const {
        return _pulse;
    }

protected:

    Storage &_stor;
    bool _pulse = true;
    bool _running = false;
    uint8_t _speed = 100;
    float _adj_speed = 1000;
    TimeStampMs _stop_time = 0;



    void set_active(bool p) {
        if (p != _pulse) {
            _pulse = p;
            pinMode(pin_out_fan_on, p?active_fan: inactive_fan);
        }
    }



};

}


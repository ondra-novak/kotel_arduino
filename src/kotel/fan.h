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
        float adj_speed = (std::log(1.0f/(std::exp(t)*(1-s)+s))+t)/t;
        auto on = 8 + static_cast<unsigned int>(20.0/(adj_speed*100));
        auto off = static_cast<unsigned int>(on/adj_speed - on);
        auto new_off = off * 10;
        auto new_on = on * 10;
        if (new_off != _off_ms || new_on != _on_ms) {
            resume_at(0);
        }
        _off_ms = new_off ;
        _on_ms = new_on;

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
        if (_pulse && _off_ms > 0) {
            resume_at(cur_time + _off_ms);
            set_active(false);
        } else {
            set_active(true);
            resume_at(cur_time+_on_ms);
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
    unsigned int _on_ms;
    unsigned int _off_ms;
    TimeStampMs _stop_time = 0;



    void set_active(bool p) {
        if (p != _pulse) {
            _pulse = p;
            pinMode(pin_out_fan_on, p?active_fan: inactive_fan);
        }
    }



};

}


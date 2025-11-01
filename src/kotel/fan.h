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
        float adj_speed = std::pow(s, t);   //power-law
        _pulse_mode =adj_speed< 0.10;
        float on = _pulse_mode?70.0:8.0;
        float off = on/adj_speed - on;
        auto new_off = static_cast<unsigned int>(off) * 10;
        auto new_on = static_cast<unsigned int>(on) * 10;
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
    int get_speed_for_display() const {
        if (!_running) return 0;
        if (_pulse_mode) return _pulse?3:0;
        if ((_off_ms>>1) > _on_ms) return 1;
        if (_off_ms > _on_ms) return 2;
        return 3;
    }
    bool is_active_for_display() const {
        return _pulse_mode?_pulse:_running;
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
    bool _pulse_mode = false;
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


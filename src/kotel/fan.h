#pragma once
#include "nonv_storage.h"
#include "task.h"
namespace kotel {

class Fan: public AbstractTask {
public:



    Fan(Storage &stor):_stor(stor) {}


    void begin() {
        set_active(false);
    }

    void keep_running(TimeStampMs until) {
        _stop_time = until;
        if (!_running) {
            _running = true;
            resume_at(0);
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

    virtual void run(TimeStampMs cur_time) override {
        if (_stop_time <= cur_time || !_running) {
            set_active(false);
            _running = false;
            AbstractTask::stop();
            return;
        }
        auto pln = static_cast<unsigned int>(_stor.config.fan_pulse_count);
        auto spd = std::max<int>(std::min<int>(_speed, 100),1);
        pln = std::max<unsigned int>(11, pln * spd / 100);
        if (_pulse) {
            auto total = pln * 100 / spd;
            auto rest = total - pln;
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
    TimeStampMs _stop_time = 0;



    void set_active(bool p) {
        if (p != _pulse) {
            _pulse = p;
            pinMode(pin_out_fan_on, p?active_fan: inactive_fan);
        }
    }



};

}


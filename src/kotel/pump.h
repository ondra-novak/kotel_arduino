#pragma once
#include "nonv_storage.h"

#include "timestamp.h"
namespace kotel {

class Pump: public AbstractTimedTask {
public:
    Pump(Storage &stor):_stor(stor) {}

    void set_active(bool a) {
        if (a != _active) {
            _active = a;
            auto now = get_current_timestamp();
            if (a) {
                ++_stor.cntr1.pump_start_count;
                _start_time  = now;
                _stor.save();
            }
            else {
                _stor.utlz.pump_time += now - _start_time;
                _stor.save();
            }
            digitalWrite(pin_out_pump_on, a?active_pump:inactive_pump);
        }
    }


    bool is_active() const {
        return _active;
    }

    virtual TimeStampMs get_scheduled_time() const override {
        return _next_call;
    }
    virtual void run(TimeStampMs cur_time) override {
        _next_call = _next_call + 1000;
        _stor.utlz.pump_time += cur_time - _start_time;
        _start_time = cur_time;
    }


protected:
    bool _active = false;
    TimeStampMs _start_time = 0;
    TimeStampMs _next_call = 0;
    Storage &_stor;

};

}


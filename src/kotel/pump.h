#pragma once
#include "nonv_storage.h"

#include "timestamp.h"
namespace kotel {

class Pump {
public:
    Pump(Storage &stor):_stor(stor) {}

    void begin() {
        set_active(false);
    }

    void set_active(bool a) {
        if (a != _active) {
            _active = a;
            if (a) {
                ++_stor.cntr1.pump_start_count;
                pinMode(pin_out_pump_on, active_pump);
            } else {
                _stor.save();
                pinMode(pin_out_pump_on, inactive_pump);
            }
        }
    }


    bool is_active() const {
        return _active;
    }



protected:
    bool _active = true;
    Storage &_stor;

};

}


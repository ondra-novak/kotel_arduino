#pragma once
#include "task.h"
#include "constants.h"
#include "nonv_storage.h"
namespace kotel {

class Feeder: public AbstractTask {
public:


    Feeder(Storage &storage):_storage(storage) {}

    void begin() {
        set_active(false);
    }

    virtual void run(TimeStampMs ) override {
        set_active(false);
    }
    void stop() {
        set_active(false);
    }

    void keep_running(TimeStampMs until) {
        set_active(true);
        resume_at(until);
    }

    bool is_active() const {return _active;}


protected:
    Storage &_storage;
    bool _active = true;

    bool set_active(bool a) {
        if (_active != a) {
            if (a) {
                ++_storage.cntr1.feeder_start_count;
            }
            pinMode(pin_out_feeder_on, a?active_feeder:inactive_feeder);
            _active = a;
            return true;
        } else {
            return false;
        }
    }
};



}

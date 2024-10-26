#include "feeder.h"
#include "constants.h"

namespace kotel {

Feeder::Feeder(Storage &storage):_storage(storage) {
}

void Feeder::start() {
    change_state(true);
}

TimeStampMs Feeder::get_scheduled_time() const {
    if (_emergency || _tray_open) return max_timestamp;
    if (_storage.config.operation_mode) {
        if (_att) return max_timestamp;
        if (_state) {
            if (_was_att) {
                return _change_time + from_seconds(_storage.config.feeder_first_on_sec);
            } else {
                return _change_time + from_seconds(_storage.config.feeder_on_sec);
            }
        } else {
            return _change_time + from_seconds(_storage.config.feeder_off_sec);
        }
    } else {
        if (_state) {
            return _change_time + from_minutes(2);
        } else {
            return max_timestamp;
        }
    }
}

void Feeder::run(TimeStampMs) {
    if (_emergency || _tray_open || _att) return;
    change_state(!_state);
}

void Feeder::config_changed(IScheduler &sch) {
    sch.reschedule();
}

void Feeder::manual_control(IScheduler &sch, bool on) {
    if (!_storage.config.operation_mode) {
        if (on) {
            change_state(true);
        } else {
            change_state(false);
        }
        sch.reschedule();
    }
}

void Feeder::attenuation(IScheduler &sch, bool on) {
    if (_storage.config.operation_mode) {
        if (on) {
            change_state(false);
            _att = true;
            _was_att = true;
        } else {
            change_state(true);
            _att = false;
        }
    }
    sch.reschedule();
}

void Feeder::emergency_stop(IScheduler &sch, bool on) {
    if (on) {
        change_state(false);
        _emergency = true;
    } else {
        _emergency = false;
    }
    sch.reschedule();
}

void Feeder::tray_open(IScheduler &sch, bool on) {
    if (on) {
        _tray_open = true;
        change_state(false);
    } else {
        _tray_open = false;
    }
    sch.reschedule();
}

void Feeder::change_state(bool new_state) {
    if (new_state && _tray_open) new_state = false;
    if (new_state != _state) {

        auto now = get_current_timestamp();
        if (_state) {
            _storage.tray.feeder_time += now - _change_time;
            if (_storage.tray.feeder_time > _storage.tray.tray_empty_time
                    && (_storage.tray.tray_open_time + (_storage.config.default_bag_count * _storage.tray.bag_consump_time > _storage.tray.tray_empty_time)/2)) {
                _storage.tray.tray_empty_time = _storage.tray.tray_open_time + (_storage.config.default_bag_count * _storage.tray.bag_consump_time > _storage.tray.tray_empty_time);
            }
            _storage.update(_storage.tray);
        } else {
            ++_storage.cntr1.feeder_start_count;
            _storage.update(_storage.cntr1);
            _was_att = false;
        }
        _change_time = get_current_timestamp();
        _state = new_state;
        set_active(_state);
    }

}

void Feeder::set_active(bool active) {
    digitalWrite(pin_out_feeder_on, active?LOW:HIGH);   //ssr is active on LOW
}




}

#include "motoruntime.h"
#include "controller.h"

namespace kotel {

TimeStampMs MotoRunTime::get_scheduled_time() const {
    return _next_call;
}

void MotoRunTime::run(TimeStampMs cur_time) {
    _next_call = cur_time + 1000;
    bool fd = _cntr.is_feeder_on();
    bool pm = _cntr.is_pump_on();
    bool fa = _cntr.is_fan_on();
    bool at = _cntr.is_attenuation();
    Storage &stor = _cntr.get_storage();
    ++stor.utlz.active_time;
    if (fd) ++stor.tray.feeder_time;
    if (pm) ++stor.utlz.pump_time;
    if (fa) ++stor.utlz.fan_time;
    if (at) ++stor.utlz.attent_time;



    bool anything_running = fd || pm || fa ;

    if (anything_running && _flush_time == max_timestamp) {
        _flush_time = cur_time+60000;
    } else if (cur_time >= _flush_time) {
        stor.save();
        if (!anything_running) _flush_time = max_timestamp;
        else _flush_time = cur_time+60000;
    }
}

}

#pragma once

#include "pins.h"

namespace kotel {


class Sensors {
public:

    bool tray_open;
    bool feeder_overheat;



    void read_sensors() {
        feeder_overheat = sensor_motor_temp.read() == motor_overheat_level;
        tray_open = sensor_tray_open.read() == tray_open_level;
    }

};


}

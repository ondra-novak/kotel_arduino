#pragma once

#include "constants.h"

namespace kotel {


class Sensors {
public:

    bool tray_open;
    bool feeder_overheat;

    void read_sensors() {
        feeder_overheat = digitalRead(pin_in_motor_temp) == motor_overheat_level;
        tray_open = digitalRead(pin_in_tray) == tray_open_level;
    }

};


}

#pragma once

#include "constants.h"

namespace kotel {


class Sensors {
public:

    bool tray_open;
    bool motor_temp_monitor;

    void read_sensors() {
        motor_temp_monitor = digitalRead(pin_in_motor_temp) == HIGH;
        tray_open = digitalRead(pin_in_tray) == HIGH;
    }


};


}

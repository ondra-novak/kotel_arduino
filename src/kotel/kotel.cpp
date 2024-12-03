#include "kotel.h"
#include "nonv_storage.h"
#include <r4eeprom.h>

#include "controller.h"

namespace kotel {

Controller controller;

void setup() {
    Serial.begin(115200);
    pinMode(pin_in_motor_temp, INPUT_PULLUP);
    pinMode(pin_in_tray, INPUT_PULLUP);
    pinMode(pin_out_fan_on, OUTPUT);
    pinMode(pin_out_feeder_on, OUTPUT);
    pinMode(pin_out_pump_on, OUTPUT);
    controller.begin();

}

void loop() {
    controller.run();
}



}

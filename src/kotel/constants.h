#pragma once

#include "Arduino.h"

namespace kotel {


//--+---+---+---+---+---+---+---+---+---+---+
//   10   9   8       7   6   5   4   3   2
//--+---+---+---+---+---+---+---+---+---+---+
//    |   |   |        |          |   |
//    |   |   |        |          |   \------ tray
//    |   |   |        |          \---------- motor temperature
//    |   |   |        \--------------------- teplomery
//    |   |   \-------------------------------podavac
//    |   \-----------------------------------ventilator
//    \---------------------------------------cerpadlo




constexpr int pin_out_feeder_on = 10;    //ssr/rele podavace
constexpr int pin_out_fan_on =  9;      //ssr ventilatoru
constexpr int pin_out_pump_on = 8;      //ssr cerpadla
constexpr int pin_in_one_wire = 7;      //teplomery




constexpr int pin_in_tray = 3;          //cidlo otevrene nasypky
constexpr int pin_in_motor_temp = 4;    //teplotni cidlo motoru



constexpr int active_feeder = OUTPUT;
constexpr int inactive_feeder = INPUT_PULLUP;
constexpr int active_pump = OUTPUT;
constexpr int inactive_pump = INPUT_PULLUP;
constexpr int active_fan = OUTPUT;
constexpr int inactive_fan = INPUT_PULLUP;
constexpr int tray_open_level = LOW;
constexpr int motor_overheat_level = HIGH;

constexpr int display_data_pin = A2;
constexpr int display_sel_pin = A1;
constexpr int display_clk_pin = A0;

constexpr int keyboard_pin = A5;

}



#pragma once

namespace kotel {


//--+---+---+---+---+---+---+---+---+---+---+
//   10   9   8       7   6   5   4   3   2
//--+---+---+---+---+---+---+---+---+---+---+
//    |   |   |        |              |   |
//    |   |   |        |              |   \-- tray
//    |   |   |        |              \------ motor temperature
//    |   |   |        \--------------------- teplomery
//    |   |   \-------------------------------podavac
//    |   \-----------------------------------ventilator
//    \---------------------------------------cerpadlo




constexpr int pin_out_feeder_on = 8;    //ssr/rele podavace
constexpr int pin_out_fan_on =  9;      //ssr ventilatoru
constexpr int pin_out_pump_on = 10;      //ssr cerpadla
constexpr int pin_in_one_wire = 7;      //teplomery




constexpr int pin_in_tray = 2;          //cidlo otevrene nasypky
constexpr int pin_in_motor_temp = 3;    //teplotni cidlo motoru



constexpr int active_feeder = LOW;
constexpr int inactive_feeder = HIGH;
constexpr int active_pump = LOW;
constexpr int inactive_pump = HIGH;
constexpr int active_fan = LOW;
constexpr int inactive_fan = HIGH;
constexpr int tray_open_level = LOW;
constexpr int motor_overheat_level = HIGH;

}



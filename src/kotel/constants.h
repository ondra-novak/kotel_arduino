#pragma once

namespace kotel {


constexpr int pin_out_feeder_on = 7;    //ssr/rele podavace
constexpr int pin_out_fan_on = 8;       //ssr ventilatoru
constexpr int pin_out_pump_on = 9;      //ssr cerpadla
constexpr int pin_in_tray = 4;          //cidlo otevrene nasypky
constexpr int pin_in_motor_temp = 6;    //teplotni cidlo motoru
constexpr int pin_in_one_wire = 2;      //teplomery

constexpr int active_feeder = LOW;
constexpr int inactive_feeder = HIGH;
constexpr int active_pump = LOW;
constexpr int inactive_pump = HIGH;
constexpr int active_fan = LOW;
constexpr int inactive_fan = HIGH;

}

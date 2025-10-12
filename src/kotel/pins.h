#pragma once
#ifndef SRC_KOTEL_PINS_H_
#define SRC_KOTEL_PINS_H_

#include "pinread.h"
#include "constants.h"

namespace kotel {

inline PinReader<pin_in_tray, 200> sensor_tray_open;
inline PinReader<pin_in_motor_temp, 20> sensor_motor_temp;


}





#endif /* SRC_KOTEL_PINS_H_ */

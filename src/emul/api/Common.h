#pragma once

#include <stdint.h>
#include <utility>
#include "Stream.h"
unsigned long millis();


#define HIGH 1
#define LOW 0

void digitalWrite(int pin, int mode);
int digitalRead(int pin);


#define PSTR(x) (x)

#pragma once

#include <stdint.h>
#include <utility>
#include "Stream.h"
unsigned long millis();


#define HIGH 1
#define LOW 0
#define INPUT 1
#define OUTPUT 2
#define INPUT_PULLUP 3
#define INPUT_PULLDOWN 4
#define LED_BUILTIN 13
#define A0 100
#define A1 101
#define A2 102
#define A3 103
#define A4 104
#define A5 105


void digitalWrite(int pin, int mode);
void digitalWrite(int pin, int mode);
int digitalRead(int pin);
int analogRead(int pin);
void pinMode(int pin, int mode);
inline void delay(int) {}

#define PSTR(x) (x)


class UART : public Stream{
  public:
    void begin(unsigned long);
    void begin(unsigned long, uint16_t config);
    void end();
    int available(void);
    int peek(void);
    int read(void);
    void flush(void);
    size_t write(uint8_t c);
    size_t write(uint8_t* c, size_t len);
    size_t write_raw(uint8_t* c, size_t len);
    using Print::write;
    operator bool(); // { return true; }
};

extern UART Serial;




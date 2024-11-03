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



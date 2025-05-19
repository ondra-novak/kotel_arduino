#include "Matrix_MAX7219.h"
#include <Arduino.h>


namespace Matrix_MAX7219 {


void Driver::begin(byte data_pin, byte cs_pin, byte clk_pin)
{
    _data_pin = data_pin;
    _clk_pin = clk_pin;
    _cs_pin = cs_pin;
    begin();


}

void Driver::begin() const {
    disable_pin(_data_pin);
    disable_pin(_clk_pin);
    disable_pin(_cs_pin);

}

void Driver::enable_pin(byte pin)  const {
    pinMode(pin, OUTPUT);
}
void Driver::disable_pin(byte pin)  const{
    pinMode(pin, INPUT_PULLUP);
}

bool Driver::check_pin(byte pin)  const{
    return digitalRead(pin) != LOW;
}

void Driver::set_pin_high(byte pin)  const{
    digitalWrite(pin, HIGH);
}

void Driver::set_pin_low(byte pin)  const{
    digitalWrite(pin, LOW);
}


bool Driver::start_packet()  const{
#ifdef DRIVER_SERIAL_DEBUG
    Serial.println("Start packet");
#endif
    if (!check_pin(_clk_pin) || !check_pin(_cs_pin) || !check_pin(_data_pin)) {
#ifdef DRIVER_SERIAL_DEBUG
        Serial.println("Start packet failed");
#endif
        return false;
    }

    enable_pin(_data_pin);
    enable_pin(_cs_pin);
    set_pin_low(_cs_pin);
    enable_pin(_clk_pin);
    set_pin_low(_clk_pin);
    return true;
}

void Driver::transfer_byte(byte data)  const{
        for (byte i = 0x80 ; i ; i = i >> 1) {
            set_pin_low(_clk_pin);
            set_pin_level(_data_pin, (data & i) != 0);
            set_pin_high(_clk_pin);
        }
}


void Driver::send_command(Operation op, byte data)  const{
#ifdef DRIVER_SERIAL_DEBUG
    Serial.print(static_cast<byte>(op), HEX);
    Serial.print(',');
    Serial.println(data, HEX);
#endif
    transfer_byte(static_cast<byte>(op));
    transfer_byte(data);
}



void Driver::commit_packet()  const{
#ifdef DRIVER_SERIAL_DEBUG
    Serial.println("Commit packet");
#endif
    disable_pin(_cs_pin);
    disable_pin(_clk_pin);
    disable_pin(_data_pin);

}

void Driver::set_pin_level(byte pin, bool level)  const{
    if (level) set_pin_high(pin); else set_pin_low(pin);
}



}

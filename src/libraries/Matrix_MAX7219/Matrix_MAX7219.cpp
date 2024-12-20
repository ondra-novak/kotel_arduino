#include "Matrix_MAX7219.h"
#include <api/Common.h>


namespace Matrix_MAX7219 {

void MatrixControlBase::begin(byte data_pin, byte clk_pin, byte cs_pin)
{
    _data_pin = data_pin;
    _clk_pin = clk_pin;
    _cs_pin = cs_pin;


    disable_pin(_data_pin);
    disable_pin(_clk_pin);
    disable_pin(_cs_pin);
}

void MatrixControlBase::enable_pin(byte pin) {
    pinMode(pin, INPUT_PULLUP);
}
void MatrixControlBase::disable_pin(byte pin) {
    pinMode(pin, INPUT_PULLUP);
}

bool MatrixControlBase::check_pin(byte pin) {
    return digitalRead(pin) != LOW;
}

void MatrixControlBase::set_pin_high(byte pin) {
    pinMode(pin, INPUT_PULLUP);
}

void MatrixControlBase::set_pin_low(byte pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}


bool MatrixControlBase::start_packet() {

    if (!check_pin(_clk_pin) || !check_pin(_cs_pin) || !check_pin(_data_pin)) {
        return false;
    }

    enable_pin(_data_pin);
    enable_pin(_cs_pin);
    set_pin_low(_cs_pin);
    enable_pin(_clk_pin);
    set_pin_low(_clk_pin);
    return true;
}

template<bool reverse_data>
void MatrixControlBase::transfer_byte(byte data) {
    if constexpr(reverse_data) {
        for (byte i = 0x01 ; i ; i = i << 1) {
            set_pin_low(_clk_pin);
            set_pin_level(_data_pin, (data & i) != 0);
            set_pin_high(_clk_pin);
        }
    } else {
        for (byte i = 0x80 ; i ; i = i >> 1) {
            set_pin_low(_clk_pin);
            set_pin_level(_data_pin, (data & i) != 0);
            set_pin_high(_clk_pin);
        }
    }
}
/*
template void MatrixControlBase::transfer<true>(byte);
template void MatrixControlBase::transfer<false>(byte);
*/


template<bool reversed>
void MatrixControlBase::send_command(byte op, byte data) {
    transfer_byte<false>(op);
    transfer_byte<reversed>(data);
}

template void MatrixControlBase::send_command<true>(byte, byte);
template void MatrixControlBase::send_command<false>(byte, byte);


void MatrixControlBase::commit_packet() {
    disable_pin(_cs_pin);

    disable_pin(_clk_pin);

    disable_pin(_data_pin);

}

void MatrixControlBase::set_pin_level(byte pin, bool level) {
    if (level) set_pin_high(pin); else set_pin_low(pin);
}



}

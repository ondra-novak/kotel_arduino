#include "../libraries/Matrix_MAX7219/Matrix_MAX7219.h"
#include "api/Common.h"
#include "simul_matrix.h"

SimulMatrixMAX7219<4> _instance4;


namespace Matrix_MAX7219 {





void MatrixControlBase::begin(byte data_pin, byte clk_pin, byte cs_pin)
{
    _data_pin = data_pin;
    _clk_pin = clk_pin;
    _cs_pin = cs_pin;

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

    return true;
}

template<bool reverse_data>
void MatrixControlBase::transfer_byte(byte data) {
    if constexpr(reverse_data) {
        byte x = 0;
        for (byte i = 0x01 ; i ; i = i << 1) {
            x<<=1;
            if ((data & i)) x |= 0x1;
        }
        data = x;
    }
    _instance4.transfer(data);
}

template<bool reversed>
void MatrixControlBase::send_command(byte op, byte data) {
    transfer_byte<false>(op);
    transfer_byte<reversed>(data);
}

template void MatrixControlBase::send_command<true>(byte, byte);
template void MatrixControlBase::send_command<false>(byte, byte);


void MatrixControlBase::commit_packet() {
    _instance4.activate();
}

void MatrixControlBase::set_pin_level(byte pin, bool level) {
    if (level) set_pin_high(pin); else set_pin_low(pin);
}



}

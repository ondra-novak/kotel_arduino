#include "../libraries/Matrix_MAX7219/Matrix_MAX7219.h"
#include "api/Common.h"
#include "simul_matrix.h"




namespace Matrix_MAX7219 {


void Driver::begin() const {}

void Driver::begin(byte data_pin, byte clk_pin, byte cs_pin)
{
    _data_pin = data_pin;
    _clk_pin = clk_pin;
    _cs_pin = cs_pin;
}

void Driver::enable_pin(byte pin) const{
    pinMode(pin, INPUT_PULLUP);
}
void Driver::disable_pin(byte pin) const {
    pinMode(pin, INPUT_PULLUP);
}

bool Driver::check_pin(byte pin) const {
    return digitalRead(pin) != LOW;
}

void Driver::set_pin_high(byte pin) const {
    pinMode(pin, INPUT_PULLUP);
}

void Driver::set_pin_low(byte pin) const {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}


bool Driver::start_packet() const {
    return true;
}

void Driver::transfer_byte(byte data) const {
    SimulMatrixMAX7219_Abstract::current_instance->transfer(data);
}

void Driver::send_command(Operation op, byte data) const {
    transfer_byte(static_cast<byte>(op));
    transfer_byte(data);
}



void Driver::commit_packet() const {
    SimulMatrixMAX7219_Abstract::current_instance->activate();
}

void Driver::set_pin_level(byte pin, bool level) const {
    if (level) set_pin_high(pin); else set_pin_low(pin);
}



}

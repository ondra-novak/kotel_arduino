#include "OneWire.h"
#if defined(ARDUINO_MINIMA) || defined(ARDUINO_UNOWIFIR4)

#include <Arduino.h>

#endif

void OneWire::begin(uint8_t pin)
{
    this->_pin = pin;
    init_pin();
}

bool OneWire::reset(void)
{
    if (!wait_for_release()) return false;
    //initiate reset
    //low 480, high 70
    hold_low_for(param_H,param_I);
    //sample for 70
    if (!wait_for(param_I, true)) return false;
    //wait for 410
    wait_for(param_J);
    //this is ok
    return true;
}


bool OneWire::write_bit(uint8_t v)
{
    //if bus is low, wait for release
    if (!wait_for_release()) return false;

    if (v) {
        //low 6, high 64
        disable_interrupt();
        hold_low_for(param_A, 0);
        enable_interrupt();
        wait_for(param_B);
    } else {
        //low 60, high 10
        hold_low_for(param_C, param_D);
    }
    return true;
}

bool OneWire::read_bit(bool &v)
{
    //if bus is low, wait for release
    if (!wait_for_release()) return false;
    disable_interrupt();
    //low 6, high 9
    hold_low_for(param_A, param_E);
    //sample and wait for 55
    auto tp = get_timepoint(param_F);
    v = !wait_for(param_F, true);
    wait_until(tp);
    enable_interrupt();
    return true;
}

bool OneWire::write_internal(uint8_t v) {

    for (uint8_t bitMask = 0x01; bitMask; bitMask <<= 1) {
        if (!OneWire::write_bit(bitMask & v)) return false;
    }
    return true;
}
bool OneWire::write(uint8_t v) {
    return write_internal(v);
}

bool OneWire::write_bytes(const uint8_t *buf, uint16_t count) {
  for (uint16_t i = 0 ; i < count ; i++) {
      if (!write_internal(buf[i])) return false;
  }
  return true;
}

bool OneWire::read_internal(uint8_t &v) {
    v = 0;
    for (uint8_t bitMask = 0x01; bitMask; bitMask <<= 1) {
        bool st;
        if ( !OneWire::read_bit(st)) return false;
        if (st) v |= bitMask;
    }
    return true;
}

bool OneWire::read(uint8_t &byte) {
    return read_internal(byte);
}

bool OneWire::read_bytes(uint8_t *buf, uint16_t count) {
  for (uint16_t i = 0 ; i < count ; i++)
     if (!read_internal(buf[i])) return false;
  return true;
}

bool OneWire::select(const Address &rom) {
    return select(rom.data);
}
bool OneWire::select(const uint8_t *rom)
{
    if (write(0x55)) {
        for (unsigned int i = 0; i < 8; i++) {
            if (!write(rom[i])) return false;
        }
        return true;
    }
    return false;
}
bool OneWire::select_all()
{
    return write(0xCC);           // Skip ROM
}

OneWire::SearchState OneWire::search_begin() {
    return {};
}

OneWire::SearchState OneWire::search_begin(uint8_t family_code) {
    OneWire::SearchState st;
    st.ROM_NO[0] = family_code;
    for (uint8_t i = 1; i < 8; i++) st.ROM_NO[i] = 0;
    st.LastDiscrepancy = 64;
    st.LastFamilyDiscrepancy = 0;
    st.LastDeviceFlag = false;
    return st;
}

//

//
bool OneWire::search(SearchState &state, Address &newAddr, bool alert_only ) {
    return search(state, newAddr.data, alert_only);
}
bool OneWire::search(SearchState &state, uint8_t *newAddr, bool alert_only )
{
   uint8_t id_bit_number = 1;
   uint8_t last_zero = 0;
   uint8_t rom_byte_number = 0;
   uint8_t rom_byte_mask = 1;
   bool search_result = false;
   bool id_bit = true;
   bool cmp_id_bit = true;

   if (state.LastDeviceFlag) return false;

    if (!reset() || !write(alert_only?0xEC:0xF0)) {
        return false;
    }

    // loop to do the search
    do {
        // read a bit and its complement
        if (!read_bit(id_bit) || !read_bit(cmp_id_bit)) return false;

        // check for no devices on 1-wire
        if (id_bit && cmp_id_bit) {
            break;
        } else {
            bool search_direction;
            // all devices coupled have 0 or 1
            if (id_bit != cmp_id_bit) {
                search_direction = id_bit;  // bit write value for search
            } else {
                // if this discrepancy if before the Last Discrepancy
                // on a previous next then pick the same as last time
                if (id_bit_number < state.LastDiscrepancy) {
                    search_direction =
                            ((state.ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
                } else {
                    // if equal to last pick 1, if not then pick 0
                    search_direction = (id_bit_number == state.LastDiscrepancy);
                }
                // if 0 was picked then record its position in LastZero
                if (!search_direction) {
                    last_zero = id_bit_number;

                    // check for Last discrepancy in family
                    if (last_zero < 9)
                        state.LastFamilyDiscrepancy = last_zero;
                }
            }

            // set or clear the bit in the ROM byte rom_byte_number
            // with mask rom_byte_mask
            if (search_direction)
                state.ROM_NO[rom_byte_number] |= rom_byte_mask;
            else
                state.ROM_NO[rom_byte_number] &= ~rom_byte_mask;

            // serial number search direction write bit
            if (!write_bit(search_direction)) return false;

            // increment the byte counter id_bit_number
            // and shift the mask rom_byte_mask
            id_bit_number++;
            rom_byte_mask <<= 1;

            // if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
            if (rom_byte_mask == 0) {
                rom_byte_number++;
                rom_byte_mask = 1;
            }
        }
    } while (rom_byte_number < 8);  // loop until through all ROM bytes 0-7

    // if the search was successful then
    if (!(id_bit_number < 65)) {
        // search successful so set LastDiscrepancy,LastDeviceFlag,search_result
        state.LastDiscrepancy = last_zero;

        // check for last device
        if (state.LastDiscrepancy == 0) {
            state.LastDeviceFlag = true;
        }
        search_result = true;
    }
    if (!state.ROM_NO[0]) {
        search_result = false;
    } else {
        for (int i = 0; i < 8; ++i) newAddr[i] = state.ROM_NO[i];
    }
    return  search_result;
}


//
// Compute a Dallas Semiconductor 8 bit CRC directly.
// this is much slower, but a little smaller, than the lookup table.
//
uint8_t OneWire::crc8(const uint8_t *addr, uint8_t len)
{
	uint8_t crc = 0;

	while (len--) {
#if defined(__AVR__)
		crc = _crc_ibutton_update(crc, *addr++);
#else
		uint8_t inbyte = *addr++;
		for (uint8_t i = 8; i; i--) {
			uint8_t mix = (crc ^ inbyte) & 0x01;
			crc >>= 1;
			if (mix) crc ^= 0x8C;
			inbyte >>= 1;
		}
#endif
	}
	return crc;
}

bool OneWire::check_crc16(const uint8_t* input, uint16_t len, const uint8_t* inverted_crc, uint16_t crc)
{
    crc = ~crc16(input, len, crc);
    return (crc & 0xFF) == inverted_crc[0] && (crc >> 8) == inverted_crc[1];
}

uint16_t OneWire::crc16(const uint8_t* input, uint16_t len, uint16_t crc)
{
#if defined(__AVR__)
    for (uint16_t i = 0 ; i < len ; i++) {
        crc = _crc16_update(crc, input[i]);
    }
#else
    static const uint8_t oddparity[16] =
        { 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 };

    for (uint16_t i = 0 ; i < len ; i++) {
      // Even though we're just copying a byte from the input,
      // we'll be doing 16-bit computation with it.
      uint16_t cdata = input[i];
      cdata = (cdata ^ crc) & 0xff;
      crc >>= 8;

      if (oddparity[cdata & 0x0F] ^ oddparity[cdata >> 4])
          crc ^= 0xC001;

      cdata <<= 6;
      crc ^= cdata;
      cdata <<= 1;
      crc ^= cdata;
    }
#endif
    return crc;
}

#if defined(ARDUINO_MINIMA) || defined(ARDUINO_UNOWIFIR4)


void OneWire::init_pin() {
    digitalWrite(_pin, LOW);
    pinMode(_pin, _pull_up?INPUT_PULLUP:INPUT);
}

void OneWire::release_pin() {
    pinMode(_pin, _pull_up?INPUT_PULLUP:INPUT);
}

void OneWire::hold_low_pin() {
    pinMode(_pin, OUTPUT);
}

bool OneWire::read_pin() {
    return digitalRead(_pin);
}


unsigned long OneWire::get_current_time() {
    return micros();
}

void OneWire::enable_interrupt() {
    interrupts();
}


void OneWire::disable_interrupt() {
    noInterrupts();
}

#endif



OneWire::MicroType OneWire::get_timepoint(unsigned long micro) {
    return update_timepoint(get_current_time(), micro);
}

OneWire::MicroType OneWire::update_timepoint(OneWire::MicroType tp, unsigned long micro) {
    return tp + micro;
}
void OneWire::wait_until(unsigned long tp) {
   while ((get_current_time() - tp) & micro_msb);
}

template<typename Fn>
bool OneWire::wait_until(unsigned long tp, Fn &&fn) {
   while ((get_current_time() - tp) & micro_msb) {
       if (fn()) return true;
   }
   return false;
}

void OneWire::wait_for(unsigned long micro) {
    wait_until(get_timepoint(micro));
}

bool OneWire::wait_for(unsigned long micro, bool pin_value) {
    return wait_until(get_timepoint(micro), [&]{
        return read_pin() != pin_value;
    });
}

void OneWire::hold_low_for(unsigned long hold_ms, unsigned long stabilize_ms) {
    auto tp = get_timepoint(hold_ms);
    //hold pin low
    hold_low_pin();
    //wait given microseconds
    wait_until(tp);
    //update timepoint
    tp = update_timepoint(tp, stabilize_ms);
    //release pin
    release_pin();
    //wait 5us to stabilize bus
    wait_until(tp);
}

bool OneWire::wait_for_release() {
    //bus is released when is HIGH
    if (!wait_for(500, false)) return false;
    //wait 5 uS to ensure, that pullup resistor settled bus
    wait_for(param_D);
    return true;
}

void OneWire::enable_pullup(bool pullup) {
    _pull_up = pullup;
}



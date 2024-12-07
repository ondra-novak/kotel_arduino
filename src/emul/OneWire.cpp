#include "OneWire.h"


void OneWire::begin(uint8_t pin)
{
    this->_pin = pin;
    init_pin();
}


bool OneWire::enable_power(bool power) {
    if (power) {
        _parasite_power = true;
        power_pin();
    } else {
        _parasite_power = false;
        init_pin();
    }
}

// Perform the onewire reset function.  We will wait up to 250uS for
// the bus to come high, if it doesn't then it is broken or shorted
// and we return a 0;
//
// Returns 1 if a device asserted a presence pulse, 0 otherwise.
//
bool OneWire::reset(void)
{
    if (!wait_for_release()) return false;
    //initiate reset
    hold_low_pin();
    //wait 480 for reset
    wait_for(480);
    //release the pin
    release_pin();
    //wait up 500 uS to presence
    if (!wait_for(500, true)) return false;
    //this is ok
    return true;
}

bool OneWire::write_bit(uint8_t v)
{
    if (!wait_for_release()) return false;
    if (v) {
        hold_low_pin();
        wait_for(10);
        release_pin();
        wait_for(55);
    } else {
        hold_low_pin();
        wait_for(65);
        release_pin();
        wait_for(5);
    }
    return true;
}

bool OneWire::read_bit(bool &v)
{
    if (!wait_for_release()) return false;
    hold_low_pin();
    wait_for(3);
    release_pin();
    v =  !wait_for(53, true);
    return true;
}

bool OneWire::write_internal(uint8_t v) {

    for (uint8_t bitMask = 0x01; bitMask; bitMask <<= 1) {
        if (!OneWire::write_bit(bitMask & v)) return false;
    }
    return true;
}
bool OneWire::write(uint8_t v) {
    return write_internal(v) && power_pin();
}

bool OneWire::write_bytes(const uint8_t *buf, uint16_t count) {
  for (uint16_t i = 0 ; i < count ; i++) {
      if (!write_internal(buf[i])) return false;
  }
  return power_pin();
}

bool OneWire::read_internal(uint8_t &v) {
    if (!wait_for_release()) return false;
    v = 0;
    for (uint8_t bitMask = 0x01; bitMask; bitMask <<= 1) {
        bool st;
        if ( !OneWire::read_bit(st)) return false;
        if (st) v |= bitMask;
    }
    return true;
}

bool OneWire::read(uint8_t &byte) {
    return read_internal(byte) && power_pin();
}

bool OneWire::read_bytes(uint8_t *buf, uint16_t count) {
  for (uint16_t i = 0 ; i < count ; i++)
     if (!read_internal(buf[i])) return false;
  return power_pin();
}

bool OneWire::select(const Address &rom)
{
    if (write(0x55)) {
        for (unsigned int i = 0; i < 8; i++) {
            if (!write(rom.data[i])) return false;
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
    st.ROM_NO.data[0] = family_code;
    for (uint8_t i = 1; i < 8; i++) st.ROM_NO.data[i] = 0;
    st.LastDiscrepancy = 64;
    st.LastFamilyDiscrepancy = 0;
    st.LastDeviceFlag = false;
}

//
// Perform a search. If this function returns a '1' then it has
// enumerated the next device and you may retrieve the ROM from the
// OneWire::address variable. If there are no devices, no further
// devices, or something horrible happens in the middle of the
// enumeration then a 0 is returned.  If a new device is found then
// its address is copied to newAddr.  Use OneWire::reset_search() to
// start over.
//
// --- Replaced by the one from the Dallas Semiconductor web site ---
//--------------------------------------------------------------------------
// Perform the 1-Wire Search Algorithm on the 1-Wire bus using the existing
// search state.
// Return TRUE  : device found, ROM number in ROM_NO buffer
//        FALSE : device not found, end of search
//
bool OneWire::search(SearchState &state, Address &newAddr, bool alert_only )
{
   uint8_t id_bit_number;
   uint8_t last_zero, rom_byte_number;
   bool search_result;
   bool id_bit = true;
   bool cmp_id_bit = true;

   unsigned char rom_byte_mask;

   // initialize for search
   id_bit_number = 1;
   last_zero = 0;
   rom_byte_number = 0;
   rom_byte_mask = 1;
   search_result = false;

   if (state.LastDeviceFlag) return false;

    if (!reset() || !write(alert_only?0xEC:0xF0)) {
        return false;
    }

    // loop to do the search
    do {
        // read a bit and its complement
        if (!read_bit(id_bit) || read_bit(cmp_id_bit)) return false;

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
                            ((state.ROM_NO.data[rom_byte_number] & rom_byte_mask) > 0);
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
                state.ROM_NO.data[rom_byte_number] |= rom_byte_mask;
            else
                state.ROM_NO.data[rom_byte_number] &= ~rom_byte_mask;

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
    if (!state.ROM_NO.data[0]) {
        search_result = false;
    } else {
        newAddr = state.ROM_NO;
    }
    return power_pin() && search_result;
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
    pinMode(_pin, _pull_up?INPUT_PULLUP:INPUT);
    _powered = false;
}

void OneWire::release_pin() {
    pinMode(_pin, _pull_up?INPUT_PULLUP:INPUT);
}

void OneWire::hold_low_pin() {
    if (_powered) {
        digitalWrite(_pin, LOW);
        _powered = false;
    } else {
        pinMode(_pin, OUTPUT);
    }
}

bool OneWire::read_pin() {
    return digitalRead(_pin);
}

inline unsigned long cur_micro() {
    return micros();
}

bool OneWire::power_pin() {
    if (!wait_for_release()) return false;
    init_pin();
    digitalWrite(_pin, HIGH);
    pinMode(_pin, OUTPUT)
    _powered = true;
    return true;
}

#else
inline unsigned long cur_micro() {
    return 0;
}
#endif


using MicroType = decltype(cur_micro());
constexpr MicroType micro_msb = MicroType(1) << (sizeof(MicroType) * 8 - 1);

void OneWire::wait_for(unsigned long micro) {
    auto n = cur_micro();
    auto m = n + micro;
    while ((n - m) & micro_msb) {
        n = cur_micro();
    }
}

bool OneWire::wait_for(unsigned long micro, bool pin_value) {
    auto n = cur_micro();
    auto m = n + micro;
    while ((n - m) & micro_msb ) {
        if (read_pin() != pin_value) return true;
        n = cur_micro();
    }
    return false;

}

bool OneWire::wait_for_release() {
    return wait_for(500, false);
}

void OneWire::enable_pullup(bool pullup) {
    _pull_up = pullup;
}

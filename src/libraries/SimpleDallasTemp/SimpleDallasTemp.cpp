#include "SimpleDallasTemp.h"

#include "../OneWire/OneWire.h"

constexpr uint8_t DS18S20MODEL = 0x10;
constexpr uint8_t DS18B20MODEL = 0x28;
constexpr uint8_t DS1822MODEL  = 0x22;
constexpr uint8_t DS1825MODEL  = 0x3B;
constexpr uint8_t DS28EA00MODEL = 0x42;


// Scratchpad locations
constexpr int TEMP_LSB        =0;
constexpr int TEMP_MSB        =1;
constexpr int HIGH_ALARM_TEMP =2;
constexpr int LOW_ALARM_TEMP  =3;
constexpr int CONFIGURATION   =4;
constexpr int INTERNAL_BYTE   =5;
constexpr int COUNT_REMAIN    =6;
constexpr int COUNT_PER_C     =7;
constexpr int SCRATCHPAD_CRC  =8;

// DSROM FIELDS
constexpr int DSROM_FAMILY    =0;
constexpr int DSROM_CRC       =7;

// Device resolution
constexpr int TEMP_9_BIT = 0x1F; //  9 bit
constexpr int TEMP_10_BIT = 0x3F; // 10 bit
constexpr int TEMP_11_BIT = 0x5F; // 11 bit
constexpr int TEMP_12_BIT = 0x7F; // 12 bit

SimpleDallasTemp::SimpleDallasTemp(OneWire &one):_wire(one) {

}

bool SimpleDallasTemp::is_valid_address(const Address &addr) {
    if (_wire.crc8(addr.data(), 7) == addr[DSROM_CRC]) {
        switch (addr[DSROM_FAMILY]) {
            default:
                return false;

            case DS18S20MODEL:
            case DS18B20MODEL:
            case DS1822MODEL:
            case DS1825MODEL:
            case DS28EA00MODEL:
                return true;
        }
    }
    return false;

}

bool SimpleDallasTemp::request_temp(const Address &addr) {
    return _wire.reset() &&
           _wire.select(addr.data()) &&
           _wire.write(0x44);
}

bool SimpleDallasTemp::request_temp() {
    return _wire.reset() &&
           _wire.select_all() &&
           _wire.write(0x44);

}


SimpleDallasTemp::Status
    SimpleDallasTemp::calculateTemperature(const uint8_t* deviceAddress,
                                    const uint8_t* scratchPad, int32_t &result) {

    int32_t fpTemperature = 0;

    // looking thru the spec sheets of all supported devices, bit 15 is always the signing bit
    // Detected if signed
    int32_t neg = 0x0;
    if (scratchPad[TEMP_MSB] & 0x80)
        neg = 0xFFF80000;

    // detect MAX31850
    // The temp range on a MAX31850 can far exceed other models, causing an overrun @ 256C
    // Based on the spec sheets for the MAX31850, bit 7 is always 1
    // Whereas the DS1825 bit 7 is always 0
    // DS1825   - https://datasheets.maximintegrated.com/en/ds/DS1825.pdf
    // MAX31850 - https://datasheets.maximintegrated.com/en/ds/MAX31850-MAX31851.pdf

    if (deviceAddress[DSROM_FAMILY] == DS1825MODEL && (scratchPad[CONFIGURATION] & 0x80)) {
        if (scratchPad[TEMP_LSB] & 1) { // Fault Detected
            if (scratchPad[HIGH_ALARM_TEMP] & 1) {
                return Status::fault_open;
            }
            else if (scratchPad[HIGH_ALARM_TEMP] >> 1 & 1) {
                return Status::fault_shortgnd;
            }
            else if (scratchPad[HIGH_ALARM_TEMP] >> 2 & 1) {
                return Status::fault_shortvdd;
            }
            else {
                return Status::fault_disconnected;
            }
        }
        // We must mask out bit 1 (reserved) and 0 (fault) on TEMP_LSB
        fpTemperature = (static_cast<int32_t> (scratchPad[TEMP_MSB]) << 11)
                        | (static_cast<int32_t>( scratchPad[TEMP_LSB] & 0xFC) << 3)
                        | neg;
    } else {
        fpTemperature = (static_cast<int32_t>(scratchPad[TEMP_MSB]) << 11)
                        | (static_cast<int32_t>(scratchPad[TEMP_LSB]) << 3)
                        | neg;
    }

    /*
     DS1820 and DS18S20 have a 9-bit temperature register.

     Resolutions greater than 9-bit can be calculated using the data from
     the temperature, and COUNT REMAIN and COUNT PER °C registers in the
     scratchpad.  The resolution of the calculation depends on the model.

     While the COUNT PER °C register is hard-wired to 16 (10h) in a
     DS18S20, it changes with temperature in DS1820.

     After reading the scratchpad, the TEMP_READ value is obtained by
     truncating the 0.5°C bit (bit 0) from the temperature data. The
     extended resolution temperature can then be calculated using the
     following equation:

                                      COUNT_PER_C - COUNT_REMAIN
     TEMPERATURE = TEMP_READ - 0.25 + --------------------------
                                             COUNT_PER_C

     Hagai Shatz simplified this to integer arithmetic for a 12 bits
     value for a DS18S20, and James Cameron added legacy DS1820 support.

     See - http://myarduinotoy.blogspot.co.uk/2013/02/12bit-result-from-ds18s20.html
     */

    if ((deviceAddress[DSROM_FAMILY] == DS18S20MODEL) && (scratchPad[COUNT_PER_C] != 0)) {
        fpTemperature = (((fpTemperature & 0xfff0) << 3) - 32
                        + (((scratchPad[COUNT_PER_C] - scratchPad[COUNT_REMAIN]) << 7)
                           / scratchPad[COUNT_PER_C])) | neg;
    }

    result = fpTemperature;
    return Status::ok;
}

std::optional<int32_t> SimpleDallasTemp::read_temp_raw(const Address &addr) {
    bool b =  _wire.reset();
    if (b) {
        uint8_t data[12];
        if (!_wire.select(addr.data()) || !_wire.write(0xBE)) return {};
        for ( int i = 0; i < 9; i++) {
            if (!_wire.read(data[i])) return false;
        }
        if ( _wire.crc8( data, 8) == data[8]) {
            int32_t result;
            _last_status = calculateTemperature(addr.data(), data, result);
            if (_last_status == Status::ok) return result;
        } else {
            _last_status = Status::fault_crc;
        }
    } else {
        _last_status = Status::fault_not_present;
    }
    return {};
}


std::optional<float> SimpleDallasTemp::read_temp_celsius(const Address &addr) {
    auto r = read_temp_raw(addr);
    if (r) {
        return (float) *r * (1.0f / 128.0f);
    } else {
        return {};
    }
}



std::optional<int32_t> SimpleDallasTemp::async_read_temp_raw(AsyncState &st) {
    if (st.st != Status::ok) return {};
    if (OneWire::crc8(st.buffer, 8) == st.buffer[8]) {
        int32_t result;
        st.st = calculateTemperature(st.addr.data(), st.buffer, result);
        if (st.st == Status::ok) return result;
    } else {
        st.st = Status::fault_crc;
    }
    return {};
}

std::optional<float> SimpleDallasTemp::async_read_temp_celsius(AsyncState &st) {
    auto r = async_read_temp_raw(st);
    if (r) {
        return (float) *r * (1.0f / 128.0f);
    } else {
        return {};
    }
}

void SimpleDallasTemp::async_request_temp(AsyncState &st, const Address &addr) {
    st.phase = 0;
    if (_wire.reset()) {
        st.state = AsyncCommand::request_temp_addr;
        st.st = Status::ok;
        st.addr = addr;
    } else {
        st.state = AsyncCommand::done;
        st.st = Status::fault_not_present;
    }
}

void SimpleDallasTemp::async_request_temp(AsyncState &st) {
    st.phase = 0;
    if (_wire.reset()) {
        st.state = AsyncCommand::request_temp_global;
        st.st = Status::ok;
    } else {
        st.state = AsyncCommand::done;
        st.st = Status::fault_not_present;
    }

}

void SimpleDallasTemp::async_read_temp(AsyncState &st, const Address &addr) {
    st.phase = 0;
    st.addr = addr;
    if (_wire.reset()) {
        st.state = AsyncCommand::read_temp;
        st.st = Status::ok;
    } else {
        st.state = AsyncCommand::done;
        st.st = Status::fault_not_present;
    }
}

bool SimpleDallasTemp::async_cycle(AsyncState &st) {
    auto com_error = [&]{
        st.state = AsyncCommand::done;
        st.st = Status::fault_shortgnd;
        return true;
    };
    switch(st.state) {
        case AsyncCommand::done: return true;
        case AsyncCommand::read_temp:
            if (st.phase == 0) {
                if (!_wire.select(st.addr.data())) return com_error();
            } else if (st.phase == 1) {
                if (!_wire.write(0xBE)) return com_error();
            } else if (static_cast<std::size_t>(st.phase - 2) < sizeof(st.buffer)){
                if (!_wire.read(st.buffer[st.phase - 2])) return com_error();
            } else {
                st.state = AsyncCommand::done;
                return true;
            }
            ++st.phase;
            break;
        case AsyncCommand::request_temp_addr:
            if (st.phase == 0) {
                if (!_wire.select(st.addr.data())) return com_error();
            } else if (st.phase == 1) {
                if (!_wire.write(0x44)) return com_error();
            } else {
                st.state = AsyncCommand::done;
                return true;
            }
            ++st.phase;
            break;
        case AsyncCommand::request_temp_global:
            if (st.phase == 0) {
                if (!_wire.select_all()) return com_error();
            } else if (st.phase == 1) {
                if (!_wire.write(0x44)) return com_error();
            } else {
                st.state = AsyncCommand::done;
                return true;
            }
            ++st.phase;
            break;
    }
    return false;

}

void SimpleDallasTemp::enum_devices_cb(EnumCallback &cb) {
    auto st = _wire.search_begin();
    Address addr;
    while (_wire.search(st, addr.data())) {
        if (is_valid_address(addr) && !cb(addr)) break;
    }
}



#pragma once

#include <stdint.h>

class OneWire {
public:
    OneWire() = default;

    ///Address structure
    struct Address {
        uint8_t data[8] = {};
    };

    ///Search state - to store state during search
    struct SearchState {
        Address ROM_NO = {};
        uint8_t LastDiscrepancy = 0;
        uint8_t LastFamilyDiscrepancy = 0;
        bool LastDeviceFlag = false;
    };

    ///initialize and assign pin
    /**
     * @param pin pin number
     */
    void begin(uint8_t pin);

    ///Enable parasite power
    /**
     * Set true to begin powering the line by activate HIGH value. The power
     * is kept when the bus is not used.

     * @param power true to enable power, false to disable power.
     * @retval true power active
     * @retval false failed to active power, the bus is shorted!
     */
    bool enable_power(bool power);

    ///Enable pullup resistor
    /**
     * @param pullup true to enable pullup, false to disable
     *
     * @note this enable setting to INPUT_PULLUP when bus is released. Otherwise it
     * is set to INPUT (high impedance)
     */
    void enable_pullup(bool ) {}


    ///reset and check presence
    /**
     * @retval true device is present
     * @retval false reset failed, no device is present or bus is shorted
     *
     * You need to call reset to begin data exchange
     */
    bool reset(void);
    ///select device to communicate
    /**
     * @param rom rom address
     * @retval true success
     * @retval false failure, bus is shorted
     */
    bool select(const Address &rom);

    ///select all devices
    /**
     * @retval true success
     * @retval false failure, bus is shorted
     */
    bool select_all(void);

    ///write byte
    /**
     * @param v byte to write
     * @retval true success
     * @retval false failure, bus is shorted
     */
    bool write(uint8_t v);

    ///write bytes
    /**
     * @param buf buffer
     * @param count buffer size
     * @retval true success
     * @retval false failure, bus is shorted
     */
    bool write_bytes(const uint8_t *buf, uint16_t count);

    ///read byte
    /**
     * @param byte reference to variable which receives byte
     * @retval true success
     * @retval false failure, bus is shorted
     */
    bool read(uint8_t &byte);

    ///read bytes
    /**
     * @param buf buffer
     * @param count buffer size
     * @retval true success
     * @retval false failure, bus is shorted
     */
    bool read_bytes(uint8_t *buf, uint16_t count);

    ///begin search
    /**
     * @return search state object
     */
    static SearchState search_begin();
    ///begin search for family code
    /**
     *
     * @param family_code
     * @return search state object
     */
    static SearchState search_begin(uint8_t family_code);
    ///perform search
    /**
     * @param state search state
     * @param addr reference to found address
     * @param alert_only search for alerts only
     * @retval true found
     * @retval false search is finished, or error (shorted bus)
     */
    bool search(SearchState &state, Address &addr, bool alert_only = false);

    static uint8_t crc8(const uint8_t *addr, uint8_t len);
    static bool check_crc16(const uint8_t* input, uint16_t len, const uint8_t* inverted_crc, uint16_t crc = 0);
    static uint16_t crc16(const uint8_t* input, uint16_t len, uint16_t crc = 0);

protected:
    uint8_t _pin = 0;
    bool _powered = false;
    bool _parasite_power = false;
    bool _pull_up = false;

    ///write one bit
    /**
     * @param v if v is zero, then zero is written otherwise one is written
     * @retval true success
     * @retval false failure, bus is shorted
     *
     * @note this function doesn't recover the power
     */
    bool write_bit(uint8_t v);
    ///read one bit
    /**
     *
     * @param v variable receives bit
     * @retval true success
     * @retval false bus is shorted
     */
    bool read_bit(bool &v);

    ///set pin to initial state (PULLUP)
    void init_pin();
    ///release pin
    void release_pin();
    void hold_low_pin();
    bool read_pin();
    bool power_pin();
    void wait_for(unsigned long micro);
    bool wait_for(unsigned long micro, bool pin_value);
    bool wait_for_release();

    bool write_internal(uint8_t v);
    bool read_internal(uint8_t &v);
};




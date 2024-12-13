#pragma once

#include <stdint.h>

class OneWire {
public:

    static constexpr unsigned int param_A = 6;
    static constexpr unsigned int param_B = 64;
    static constexpr unsigned int param_C = 60;
    static constexpr unsigned int param_D = 10;
    static constexpr unsigned int param_E = 9;
    static constexpr unsigned int param_F = 55;
    static constexpr unsigned int param_H = 480;
    static constexpr unsigned int param_I = 70;
    static constexpr unsigned int param_J = 410;

    OneWire() = default;

    ///Address structure
    struct Address {
        uint8_t data[8] = {};
    };

    ///Search state - to store state during search
    struct SearchState {
        uint8_t ROM_NO[8] = {};
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
    void enable_pullup(bool pullup);


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

    bool select(const uint8_t *rom);

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

    bool search(SearchState &state, uint8_t *addr, bool alert_only = false);


    static uint8_t crc8(const uint8_t *addr, uint8_t len);
    static bool check_crc16(const uint8_t* input, uint16_t len, const uint8_t* inverted_crc, uint16_t crc = 0);
    static uint16_t crc16(const uint8_t* input, uint16_t len, uint16_t crc = 0);

protected:
    uint8_t _pin = 0;
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

    using MicroType = unsigned long;
    static constexpr MicroType micro_msb = MicroType(1) << (sizeof(MicroType) * 8 - 1);

    ///put pin to low, wait given microseconds, release it and wait
    /**
     * @param hold_us hold microseconds
     * @param stabilize_us delay to stabilize bus (wait for pull up resistor)
     */
    void hold_low_for(unsigned long hold_us, unsigned long stabilize_us);

    ///wait until bus is released
    /**
     * Waits up to 500uS while bus is LOW. Then wait 8uS before returns. If the
     * bus is not relesed in time, return sfalse
     * @return true bus is free
     * @return false bus is occupied
     */
    bool wait_for_release();


    ///get timepoint for waiting
    /**
     * @param micro micro seconds after current time
     * @return timepoint usable for wait_until
     */
    static MicroType get_timepoint(unsigned long micro);
    ///wait until timepoint is reached
    static void wait_until(unsigned long tp);
    ///repeat call of function until time is reached
    /**
     * @param tp timepoint to reach
     * @param fn function to call in cycle. The function must return true to interrupt waiting
     * @retval true function returned true
     * @retval false function returned false for whole period
     */
    template<typename Fn>
    static bool wait_until(unsigned long tp, Fn &&fn);
    ///update timepoint
    static unsigned long update_timepoint(unsigned long tp, unsigned long micro);
    ///wait for given
    static void wait_for(unsigned long micro);
    ///wait for given microseconds while pin value equals given argument
    /**
     * @param micro microseconds to wait
     * @param pin_value test value
     * @retval true pin changed
     * @retval false timeout
     */
    bool wait_for(unsigned long micro, bool pin_value);

    bool write_internal(uint8_t v);
    bool read_internal(uint8_t &v);

    //----------- MUST BE IMPLEMENTED ON THE TARGET PLATFORM

    ///set pin to initial state
    void init_pin();
    ///hold pin at low value
    void hold_low_pin();
    ///release pin, set it to high impedance
    void release_pin();
    ///read pin value
    /**
     * @retval true pin is in high impedance
     * @retval false pin is low
     */
    bool read_pin();

    ///get current microsecond time.
    static MicroType get_current_time();
    ///enable interrupts
    static void enable_interrupt();
    ///disable interrupts
    static void disable_interrupt();

    //----------- END OF PLATFORM DEPEND IMPLEMENTATION

};




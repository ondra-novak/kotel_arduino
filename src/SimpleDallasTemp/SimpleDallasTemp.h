#pragma once


#include <array>
#include <cstdint>
#include <optional>

class OneWire;

class SimpleDallasTemp {
public:

    enum class Status {
        ok,
        fault_open,
        fault_shortgnd,
        fault_shortvdd,
        fault_disconnected,
        fault_not_present,
        fault_crc

    };

    SimpleDallasTemp(OneWire &one);

    using Address = std::array<uint8_t, 8>;

    template<typename Fn>
    void enum_devices(Fn &&fn) {
        static_assert(std::is_invocable_r_v<bool, Fn, Address>);
        wire_reset_search();
        Address addr;
        while (wire_search(addr)) {
            if (is_valid_address(addr) && !fn(addr)) break;
        }
    }
    bool is_valid_address(const Address &addr);
    unsigned long request_temp(const Address &addr);
    unsigned long request_temp();   //global

    std::optional<int32_t> read_temp_raw(const Address &addr);
    std::optional<float> read_temp_celsius(const Address &addr);
    Status get_last_error() const {return _last_status;}
protected:
    OneWire &_wire;
    Status _last_status = Status::ok;

    static Status calculateTemperature(const uint8_t* deviceAddress,
                                    const uint8_t* scratchPad, int32_t &result) ;

    //we need this to hide that there is no wire in emulation
    void wire_reset_search();
    //we need this to hide that there is no wire in emulation
    bool wire_search(Address &addr);
};



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

    enum class AsyncCommand {
        request_temp_global,
        request_temp_addr,
        read_temp,
        done
    };

    using Address = std::array<uint8_t, 8>;

    struct AsyncState {
        AsyncCommand state = AsyncCommand::done;
        uint8_t phase = 0;
        Address addr = {};
        uint8_t buffer[9] = {};
        Status st = {};
    };

    SimpleDallasTemp(OneWire &one);


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

    ///Initiate asynchronous request_temp
    /**
     * @param st asynchronous state
     * @param addr address to read
     * @note you need repeatedly call async_process in loop()
     */
    void async_request_temp(AsyncState &st, const Address &addr);
    ///Initiate asynchronous request_temp (global)
    /**
     * @param st asynchronous state
     * @note you need repeatedly call async_process in loop()
     */
    void async_request_temp(AsyncState &st);
    ///Initiate asynchronous read temperature
    /**
     * @param st asynchronous state
     * @param addr address
     */
    void async_read_temp(AsyncState &st, const Address &addr);
    ///Perform one step of asynchronous operation
    /**
     * @param st asynchronous state
     * @retval true operation is done
     * @retval false still in progress
     */
    bool async_cycle(AsyncState &st);
    ///retrieve last error of async operation
    static Status async_get_last_error(const AsyncState &st) {return st.st;}
    ///retrieve raw temperature of last operation
    static std::optional<int32_t> async_read_temp_raw(AsyncState &st);
    ///retrieve celsius temperature of last operation
    static std::optional<float> async_read_temp_celsius(AsyncState &st);


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



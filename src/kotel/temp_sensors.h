#pragma once

#include "constants.h"
#include "nonv_storage.h"
#include "linreg.h"
#include <SimpleDallasTemp.h>
#include <OneWire.h>

#include "combined_container.h"

namespace kotel {

class TempSensors: public AbstractTimedTask {
public:

    enum class State {
        start,
        write_request,
        read_temp1,
        read_temp2,
        wait
    };

    TempSensors(Storage &stor):_stor(stor)
        ,_temp_reader(_wire) {}

    void begin() {
        _wire.begin(pin_in_one_wire);
    }

    virtual TimeStampMs get_scheduled_time() const override {
        return _next_read_time;
    }

    virtual void run(TimeStampMs cur_time) override {
        std::optional<float> rdtmp;


        switch (_state) {
            case State::start:
                _next_measure_time = cur_time + 2000;
                _next_read_time = cur_time+1;
                _state = State::write_request;
                break;
            case State::write_request:
                _temp_reader.request_temp();
                _state = State::read_temp1;
                _next_read_time = cur_time + 200;
                break;
            case State::read_temp1:
                _input.read(_temp_reader, _stor.temp.input_temp);
                _state = State::read_temp2;
                _next_read_time = cur_time + 1;
                break;
            case State::read_temp2:
                _output.read(_temp_reader, _stor.temp.output_temp);
                _state = State::wait;
                _next_read_time = cur_time + 1;
                break;
            default:
                _next_read_time = _next_measure_time;
                _next_measure_time += 2000;
                _state = State::write_request;



        }

    }

    auto &get_controller() {
        return _temp_reader;
    }

    SimpleDallasTemp::Status get_input_status() const {
        return _input._status;
    }

     std::optional<float> get_input_temp() const {
        return _input._value;
    }

    SimpleDallasTemp::Status get_output_status() const {
        return _output._status;
    }

    std::optional<float> get_output_temp() const {
        return _output._value;
    }

    unsigned int get_read_count() const {
        return _read_count;
    }

    bool is_reading() const {
        return  _state != State::write_request;
    }

    float get_input_ampl() const {
        return _input.extrapolate(static_cast<int>(_stor.config.input_temp_ampl)*10);
    }

    float get_output_ampl() const {
        return _output.extrapolate(static_cast<int>(_stor.config.output_temp_ampl)*10);
    }


protected:

    static constexpr unsigned int history_count = 128;



    struct TempDeviceState {
        std::optional<float> _value = {};
        SimpleDallasTemp::Status _status = {};
        unsigned int _wrpos = history_count - 1;
        float _history[history_count] = {};
        bool _first_value = true;
        mutable bool _dirty = true;
        mutable LinReg _lnr;

        void read(SimpleDallasTemp &reader, const SimpleDallasTemp::Address &addr) {
            _value = reader.read_temp_celsius(addr);
            _status = reader.get_last_error();
            auto newpos = (_wrpos + 1) % history_count;
            if (_value.has_value()) {
                if (_first_value) {
                    _first_value = false;
                    for (unsigned int i = 0; i < history_count; ++i) {
                        _history[i] = *_value;
                    }
                } else {
                    _history[newpos] = *_value;
                }
            } else {
                _history[newpos] = _history[_wrpos];
            }
            _wrpos = newpos;
            _dirty = true;
        }

        float extrapolate(int x) const {
            x+=history_count;
            if (_dirty) {
                auto beg = (_wrpos+1) % history_count;
                std::basic_string_view<float> s1(_history+beg, history_count-beg);
                std::basic_string_view<float> s2(_history, beg);
                CombinedContainers<std::basic_string_view<float>,std::basic_string_view<float> > cont(s1,s2);

                _lnr = LinReg(cont.begin(), cont.end());
                _dirty = false;
            }
            return _lnr(x);
        }



    };

    State _state = State::start;
    Storage &_stor;
    OneWire _wire;
    SimpleDallasTemp _temp_reader;
    TempDeviceState _input;
    TempDeviceState _output;
    TimeStampMs _next_read_time = 0;
    TimeStampMs _next_measure_time = 0;
    unsigned int _read_count = 0;

    float calc_change(float n, float o) const {
        return (n - o) * (2.0/_stor.temp.trend_smooth) + o;
    }
};

}


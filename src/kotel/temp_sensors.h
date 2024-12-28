#pragma once

#include "constants.h"
#include "nonv_storage.h"
#include "linreg.h"
#include <SimpleDallasTemp.h>
#include <OneWire.h>

#include "combined_container.h"

namespace kotel {

class TempSensors: public AbstractTask {
public:

    static constexpr unsigned int measure_interval = 10000;

    enum class State {
        start,
        write_request,
        write_request_done,
        read_temp1,
        read_temp1_done,
        read_temp2,
        read_temp2_done,
        wait
    };

    TempSensors(Storage &stor):_stor(stor)
        ,_temp_reader(_wire) {}

    void begin() {
        _wire.enable_pullup(true);
        _wire.begin(pin_in_one_wire);

    }


    virtual void run(TimeStampMs cur_time) override {
        if (_simulated) {
            _input.set_value(_input._value, SimpleDallasTemp::Status::ok);
            _output.set_value(_output._value, SimpleDallasTemp::Status::ok);
            resume_at(cur_time + measure_interval);
            return;
        }


        switch (_state) {
            case State::start:
                _next_measure_time = cur_time + measure_interval;
                _state = State::write_request;
                resume_at(cur_time+1);
                break;
            case State::write_request:
                _temp_reader.async_request_temp(_temp_async_state);
                _state = State::write_request_done;
                resume_at(0);
                break;
            case State::write_request_done:
                if (_temp_reader.async_cycle(_temp_async_state)) {
                    resume_at(cur_time + 200);
                    _state = State::read_temp1;
                } else {
                    resume_at(0);
                }
                break;
            case State::read_temp1:
                _temp_reader.async_read_temp(_temp_async_state, _stor.temp.input_temp);
                _state = State::read_temp1_done;
                resume_at(0);
                break;
            case State::read_temp1_done:
                if (_temp_reader.async_cycle(_temp_async_state)) {
                    _input.read(_temp_async_state);
                    _state = State::read_temp2;
                    resume_at(cur_time + 1);
                } else {
                    resume_at(0);
                }
                break;
            case State::read_temp2:
                _temp_reader.async_read_temp(_temp_async_state, _stor.temp.output_temp);
                _state = State::read_temp2_done;
                resume_at(0);
                break;
            case State::read_temp2_done:
                if (_temp_reader.async_cycle(_temp_async_state)) {
                    _output.read(_temp_async_state);
                    _state = State::wait;
                    resume_at(cur_time + 1);
                } else {
                    resume_at(0);
                }
                break;
            default:
                resume_at(_next_measure_time);
                _next_measure_time += measure_interval;
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

    bool is_reading() const {
        return  _state != State::write_request;
    }

    float get_input_ampl() const {
        return _input.extrapolate(static_cast<int>(_stor.config.input_min_temp_samples));
    }

    float get_output_ampl() const {
        return _output.extrapolate(static_cast<int>(_stor.config.output_max_temp_samples));
    }



    void simulate_temperature(float input, float output) {
        _simulated = true;
        _input.set_value(input, SimpleDallasTemp::Status::ok);
        _output.set_value(output, SimpleDallasTemp::Status::ok);
    }

    void disable_simulated_temperature() {
        _simulated = false;
    }

    bool is_simulated() const {
        return _simulated;
    }

protected:

    static constexpr unsigned int max_history_count = 100;



    struct TempDeviceState {
        std::optional<float> _value = {};
        SimpleDallasTemp::Status _status = {};
        unsigned int _wrpos = max_history_count - 1;
        float _history[max_history_count] = {};
        bool _first_value = true;

        void read(SimpleDallasTemp::AsyncState &st) {
            set_value(SimpleDallasTemp::async_read_temp_celsius(st),
                        SimpleDallasTemp::async_get_last_error(st));
        }

        void set_value(std::optional<float> value, SimpleDallasTemp::Status st) {
            _value = value;
            _status = st;
            auto newpos = (_wrpos + 1) % max_history_count;
            if (_value.has_value()) {
                if (_first_value) {
                    _first_value = false;
                    for (unsigned int i = 0; i < max_history_count; ++i) {
                        _history[i] = *_value;
                    }
                } else {
                    _history[newpos] = *_value;
                }
            } else {
                _history[newpos] = _history[_wrpos];
            }
            _wrpos = newpos;
        }

        float extrapolate(unsigned int x) const {
            if (x > max_history_count) x = max_history_count;
            if (x < 2) return _history[_wrpos];
            auto skip = max_history_count - x;
            auto beg = (_wrpos+1) % max_history_count;
            std::basic_string_view<float> s1(_history+beg, max_history_count-beg);
            std::basic_string_view<float> s2(_history, beg);
            CombinedContainers<std::basic_string_view<float>,std::basic_string_view<float> > cont(s1,s2);
            auto b = cont.begin();
            auto e = cont.end();
            std::advance(b, skip);

            LinReg lnr(b, e);
            return lnr(2*x);
        }



    };

    State _state = State::start;
    Storage &_stor;
    OneWire _wire;
    SimpleDallasTemp _temp_reader;
    SimpleDallasTemp::AsyncState _temp_async_state;
    TempDeviceState _input;
    TempDeviceState _output;
    TimeStampMs _next_measure_time = 0;
    bool _simulated = false;


};

}


#pragma once

#include "constants.h"
#include "nonv_storage.h"
#include <SimpleDallasTemp.h>
#include <OneWire.h>


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
                _next_read_time = cur_time+1;
                _state = State::write_request;
                break;
            case State::write_request:
                _temp_reader.request_temp();
                _state = State::read_temp1;
                _next_read_time = cur_time + 200;
                break;
            case State::read_temp1:
                rdtmp = _temp_reader.read_temp_celsius(_stor.temp.input_temp);
                _input_status = _temp_reader.get_last_error();
                if (rdtmp.has_value()) {
                    if (_input_temp.has_value()) {
                        _input_change = calc_change(*rdtmp, *_input_temp);
                        _input_temp = rdtmp;
                    } else {
                        _input_temp = rdtmp;
                    }
                }
                _state = State::read_temp2;
                _next_read_time = cur_time + 1;
                break;
            case State::read_temp2:
                rdtmp = _temp_reader.read_temp_celsius(_stor.temp.output_temp);
                _output_status = _temp_reader.get_last_error();
                if (rdtmp.has_value()) {
                    if (_output_temp.has_value()) {
                        _output_change = calc_change(*rdtmp, *_output_temp);
                        _output_temp = rdtmp;
                    } else {
                        _output_temp = rdtmp;
                    }
                }
                _state = State::wait;
                _next_read_time = cur_time + 1;
                break;
            default:
                _next_read_time = cur_time + 1000;
                _state = State::write_request;



        }

    }

    auto &get_controller() {
        return _temp_reader;
    }

    SimpleDallasTemp::Status get_input_status() const {
        return _input_status;
    }

     std::optional<float> get_input_temp() const {
        return _input_temp;
    }

    SimpleDallasTemp::Status get_output_status() const {
        return _output_status;
    }

    std::optional<float> get_output_temp() const {
        return _output_temp;
    }

    unsigned int get_read_count() const {
        return _read_count;
    }

    bool is_reading() const {
        return  _state != State::wait;
    }

    bool is_emergency_temp() const {
        return !_output_temp.has_value() || *_output_temp >= static_cast<float>(_stor.config.output_max_temp);
    }

    bool is_attent_temp() const {
        if (_output_temp.has_value() && _input_temp.has_value()) {
            auto tmin = static_cast<float>(_stor.config.input_min_temp);
            auto tmax = static_cast<float>(_stor.config.output_max_temp);
            if (*_output_temp >= tmax || *_input_temp>=tmax) return true;
            if (*_input_temp <= tmin) return false;
            float cur_center = ((*_output_temp) + (*_input_temp)) / 2.0;
            float optimal_center = (tmin + tmax)/2.0;
            return (cur_center >= optimal_center);
        }
        return true;
    }

    bool is_start_temp() const {
        if (_output_temp.has_value() && _input_temp.has_value()) {
            auto tmin = static_cast<float>(_stor.config.input_min_temp);
            auto tmax = static_cast<float>(_stor.config.output_max_temp);
            if (*_output_temp >= tmax) return false;
            return (*_input_temp <= tmin);
        }
        return false;
    }




protected:

    State _state = State::start;
    Storage &_stor;
    OneWire _wire;
    SimpleDallasTemp _temp_reader;
    std::optional<float> _input_temp = {};
    std::optional<float> _output_temp = {};
    float _input_change = 0;
    float _output_change = 0;
    SimpleDallasTemp::Status _input_status = {};
    SimpleDallasTemp::Status _output_status = {};
    TimeStampMs _next_read_time = 0;
    unsigned int _read_count = 0;

    float calc_change(float n, float o) const {
        return (n - o) * (2.0/_stor.temp.trend_smooth) + o;
    }
};

}


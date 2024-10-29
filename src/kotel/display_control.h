#pragma once
#include "timed_task.h"
#include "nonv_storage.h"
#include "display.h"
#include "sensors.h"

namespace kotel {



class DisplayControl: public AbstractTimedTask {
public:

    DisplayControl(const Storage &stor, const WiFiMonitor &wifi_mon,
            const TempSensors &temp, const Sensors &sensors)
        :_stor(stor)
        ,_wifi_mon(wifi_mon)
        ,_temp(temp)
        ,_sensors(sensors) {}

    virtual TimeStampMs get_scheduled_time() const override {
        return _next_change;

    }
    virtual void run(TimeStampMs cur_time) override {
        _next_change = cur_time + 2000;
        _alternate_state = !_alternate_state;
        bool wifi_connected = _wifi_mon.is_connected();
        if (_sensors.tray_open) {
            Matrix::render_icon(0,6,Matrix::Icon::tray_open);
        } else if (_old_wifi_state != wifi_connected && wifi_connected) {
            Matrix::render_icon(0,6,Matrix::Icon::wifi_success);;
        } else if (_alternate_state) {
            if (!wifi_connected) {
                Matrix::render_icon(0,6,Matrix::Icon::wifi_failed);;
            } else {
                print_temp(6, _temp.get_input_temp());
            }
        } else {
            print_temp(6, _temp.get_input_temp());
        }
        print_temp(0, _temp.get_output_temp());
        _old_wifi_state = wifi_connected;
    }



protected:
    const Storage &_stor;
    const WiFiMonitor &_wifi_mon;
    const TempSensors &_temp;
    const Sensors &_sensors;
    bool _old_wifi_state = false;
    bool _alternate_state = false;
    TimeStampMs _next_change = 0;

    void print_temp(std::uint8_t line, std::optional<float> numb) {
        if (numb.has_value()) {

            int v = static_cast<int>(*numb+0.5f);
            if (v > 99) {
                Matrix::render_character_6x4(0, line, '1');
                Matrix::render_character_6x4(4, line, '+');
            } else if (v < -9) {
                Matrix::render_character_6x4(0, line, '-');
                Matrix::render_character_6x4(4, line, '9');
            } else {
                Matrix::render_character_6x4(0, line, (v / 10) + '0');
                Matrix::render_character_6x4(4, line, (v % 10) + '0');
            }

        } else {
            Matrix::render_character_6x4(0, line, '-');
            Matrix::render_character_6x4(4, line, '-');
        }
    }

};


}


#pragma once

#include "timed_task.h"
#include "nonv_storage.h"
#include "sensors.h"
#include "icons.h"

#include <DotMatrix.h>

namespace kotel {




class DisplayControl: public AbstractTimedTask {
public:

    using FrameBuffer = DotMatrix::FrameBuffer<8,12,DotMatrix::Format::monochrome_1bit>;
    using Driver = DotMatrix::Driver<FrameBuffer, DotMatrix::Orientation::portrait>;
    static constexpr Driver dot_driver = {};

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
            icon_tray_open.draw(frame_buffer, 0, 6);
        } else if (_old_wifi_state != wifi_connected && wifi_connected) {
            icon_network_ok.draw(frame_buffer, 0, 6);
        } else if (_alternate_state) {
            if (!wifi_connected) {
                icon_no_network.draw(frame_buffer, 0, 6);
            } else {
                print_temp(6, _temp.get_input_temp());
            }
        } else {
            print_temp(6, _temp.get_input_temp());
        }
        print_temp(0, _temp.get_output_temp());
        _old_wifi_state = wifi_connected;
    }

    void begin() {
        DotMatrix::enable_auto_drive(dot_driver, display_state, frame_buffer);
    }

public:
    FrameBuffer frame_buffer;
    DotMatrix::State display_state;

protected:
    const Storage &_stor;
    const WiFiMonitor &_wifi_mon;
    const TempSensors &_temp;
    const Sensors &_sensors;
    bool _old_wifi_state = false;
    bool _alternate_state = false;
    TimeStampMs _next_change = 0;

    void print_temp(std::uint8_t line, std::optional<float> numb) {
        char txtbuf[3] = {};
        using TR = DotMatrix::TextRender<>;
        if (numb.has_value()) {

            int v = static_cast<int>(*numb+0.5f);
            if (v > 99) {
                txtbuf[0] = '1';
                txtbuf[1] = '#';
            } else if (v < -9) {
                txtbuf[0] = '-';
                txtbuf[1] = '9';
            } else {
                txtbuf[0] = v/10+'0';
                txtbuf[1] = v%10+'0';
            }

        } else {
            txtbuf[0] = '-';
            txtbuf[1] = '-';
        }

        TR::render_text(frame_buffer, DotMatrix::font_5x3, 0, line, {txtbuf,2});
    }

};


}



#pragma once

#include "timed_task.h"
#include "nonv_storage.h"
#include "sensors.h"
#include "icons.h"

#include <DotMatrix.h>

namespace kotel {


class Controller;

class DisplayControl: public AbstractTimedTask {
public:

    using FrameBuffer = DotMatrix::FrameBuffer<8,12,DotMatrix::Format::monochrome_1bit>;
    using Driver = DotMatrix::Driver<FrameBuffer, DotMatrix::Orientation::portrait>;
    static constexpr Driver dot_driver = {};

    static constexpr unsigned int first_line = 0;
    static constexpr unsigned int second_line = 0;



    DisplayControl(const Controller &cntr):_cntr(cntr) {}

    void begin() {
        DotMatrix::enable_auto_drive(dot_driver, display_state, frame_buffer);
    }
   
    virtual TimeStampMs get_scheduled_time() const override;
    virtual void run(TimeStampMs cur_time) override;
    
    /* {
        _next_change = cur_time + 1000;
        if (_ec) {
            print_error(second_line,_ec);
        } else {
            if ()
        }
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
    */


public:
    FrameBuffer frame_buffer;
    DotMatrix::State display_state;

protected:
    const Controller &_cntr;

    TimeStampMs _next_change = 0;
    using TR = DotMatrix::TextRender<>;

    void print_temp(std::uint8_t line, std::optional<float> numb) {
        char txtbuf[3] = {};
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

    void print_error(uint8_t line, ErrorCode code) {
        char txt[] = "E?";
        txt[1] = '0' + static_cast<uint8_t>(code);
        TR::render_text(frame_buffer, DotMatrix::font_5x3, 0, line, {txt,2});
    }
    void main_temp();

};


}



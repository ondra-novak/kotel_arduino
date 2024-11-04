#include "display_control.h"
#include "controller.h"

namespace kotel {



    TimeStampMs DisplayControl::get_scheduled_time() const
    {
        return _next_change;
    }

    void DisplayControl::run(TimeStampMs cur_time)
    {
        _next_change = cur_time + 1000;
        if (_cntr.is_stop()) {
            TR::draw_text(frame_buffer,DotMatrix::font_5x3, 0, first_line, "ST");
            TR::draw_text(frame_buffer,DotMatrix::font_5x3, 0, second_line, "OP");
            return;
        }
        if (_cntr.is_manual()) {
            main_temp();
            TR::draw_text(frame_buffer, DotMatrix::font5x3, 0, second_line, "RU");
            return;
        }
        if (_cntr.is_tray_open()) {
            icon_tray_open.draw(frame_buffer, 0, 6);

        }

    }
}
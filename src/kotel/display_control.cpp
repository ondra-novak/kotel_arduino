#include "display_control.h"
#include "controller.h"

namespace kotel {



    TimeStampMs DisplayControl::get_scheduled_time() const {
    return _next_change;
}

void DisplayControl::run(TimeStampMs cur_time) {
    _next_change = cur_time + 125;
    ++_anim_phase;
    bool f = _cntr.is_feeder_on();
    auto f1 = f?(_anim_phase>>1) & 1:0;
    auto f2 = f?(1-f1):0;
    frame_buffer.set_pixel(6,5,f1);
    frame_buffer.set_pixel(7,6,f1);
    frame_buffer.set_pixel(6,6,f2);
    frame_buffer.set_pixel(7,5,f2);

    if (_alternate_enable && (_anim_phase & 0x7) == 0x7) {
        frame_buffer.draw_box(0, second_line, 7, 12, 0);
    }
    if (_anim_phase & 0x7) return;
    _alternate = !_alternate;
    _alternate_enable = false;
    if (_alternate) {
        if (_cntr.is_wifi()) {
            if (!_wifi_shown) {
                icon_network_ok.draw(frame_buffer, 0, second_line);
                _wifi_shown = true;
                return;
            }
        } else {
            _wifi_shown = false;
            _alternate_enable = true;
            icon_no_network.draw(frame_buffer, 0, second_line);
            return;
        }
    } else if (!_cntr.is_wifi()) {
        _alternate_enable = true;
    }
    if (_cntr.is_stop()) {
        TR::render_text(frame_buffer, DotMatrix::font_5x3, 0, first_line, "ST");
        TR::render_text(frame_buffer, DotMatrix::font_5x3, 0, second_line,
                "OP");
        return;
    }
    if (_cntr.is_manual()) {
        main_temp();
        TR::render_text(frame_buffer, DotMatrix::font_5x3, 0, second_line,
                "RU");
        return;
    }
    if (_cntr.is_tray_open()) {
        frame_buffer.clear(0);
        icon_tray_open.draw(frame_buffer, 0, 0);
        return ;
    }
    auto rmn = _cntr.calc_tray_remain();
    if (rmn <= 4 && rmn >= 0) {
        _alternate_enable = true;
        if (_alternate) {
            main_temp();
            char buff[2];
            buff[0] = rmn + '0';
            buff[1] = 'P';
            TR::render_text(frame_buffer, DotMatrix::font_5x3, 0, second_line, {buff,2});
            return;
        }
    }
    main_temp();
    print_temp(second_line, _cntr.get_input_temp());
}

void DisplayControl::print_temp(std::uint8_t line, std::optional<float> numb) {
    char txtbuf[3] = { };
    if (numb.has_value()) {

        int v = static_cast<int>(*numb + 0.5f);
        if (v > 99) {
            txtbuf[0] = '1';
            txtbuf[1] = '#';
        } else if (v < -9) {
            txtbuf[0] = '-';
            txtbuf[1] = '9';
        } else {
            txtbuf[0] = v / 10 + '0';
            txtbuf[1] = v % 10 + '0';
        }

    } else {
        txtbuf[0] = '-';
        txtbuf[1] = '-';
    }

    TR::render_text(frame_buffer, DotMatrix::font_5x3, 0, line, { txtbuf, 2 });
}

void DisplayControl::main_temp() {
    print_temp(first_line, _cntr.get_output_temp());
}

}

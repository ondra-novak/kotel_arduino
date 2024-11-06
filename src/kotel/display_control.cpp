#include "display_control.h"
#include "controller.h"

namespace kotel {



    TimeStampMs DisplayControl::get_scheduled_time() const {
    return _next_change;
}

constexpr DotMatrix::Bitmap<5,6> fann_anim[] = {
        "  X  "
        "  X  "
        "XXXXX"
        "  X  "
        "  X  "
        "     ",
        " X   "
        " X XX"
        "  X  "
        "XX X "
        "   X "
        "     ",
        "X   X"
        " X X "
        "  X  "
        " X X "
        "X   X"
        "     ",
        "   X "
        "X X  "
        " XXX "
        "  X X"
        " X   "
        "     ",
};

constexpr DotMatrix::Bitmap<3,6> feed_anim[] = {
        " xx"
        " x "
        " x "
        " xx"
        " x "
        " x ",
        " x "
        " x "
        " xx"
        " x "
        " x "
        " xx",
        " x "
        " xx"
        " x "
        " x "
        " xx"
        " x ",
};


void DisplayControl::run(TimeStampMs cur_time) {

    if (!_scroll_text.empty()) {
        TR::render_text(frame_buffer, DotMatrix::font_5x3, 8-_scroll_text_pos, 4, _scroll_text);
        _scroll_text_pos++;
        if (_scroll_text_pos > (_scroll_text.size()+3)*4) {
            _scroll_text.clear();
            _scroll_text_pos = 0;
        }
        _next_change = cur_time+100;
        return;
    }
    _next_change = cur_time + 125;
    ++_anim_phase;

    auto phase_pos = _anim_phase & 0x7;
    bool alternate = !(_anim_phase & 0x8);

    if (!_cntr.is_attenuation() && (_cntr.is_feeder_on() || _cntr.is_fan_on())) {
        if (_cntr.is_feeder_on()) {
            auto p = _anim_phase%3;
            feed_anim[p].draw(frame_buffer, 5,second_line);
        } else {
            frame_buffer.draw_box(5, second_line, 7, 12,0);
        }

        if (_cntr.is_fan_on()) {
            auto p = _anim_phase%4;
            fann_anim[p].draw(frame_buffer, 0,second_line);
        } else {
            frame_buffer.draw_box(0, second_line, 5, 12,0);
        }
        main_temp();
        return;
    }

    if (!_cntr.is_wifi()) {
        if (phase_pos == 0) {
            frame_buffer.draw_box(0,second_line,7,11,0);
            return;
        } else if (alternate) {
            _show_wifi_on = 20;
            icon_no_network.draw(frame_buffer, 0, second_line);
            main_temp();
            return;
        }
    } else {
        if (_show_wifi_on) {
            icon_network_ok.draw(frame_buffer, 0, second_line);
            --_show_wifi_on;
            main_temp();
        }
    }

    if (_cntr.is_stop() || _cntr.is_manual()) {
        if (alternate && !_cntr.is_wifi_used()) {
                char buff[100];
                auto ip = WiFi.localIP();
                sprintf(buff,"http://%s", ip.toString().c_str());
                begin_scroll(buff);
        }
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
        if (phase_pos == 0) {
            frame_buffer.draw_box(0,second_line,7,11,0);
            return;
        }
        if (alternate) {
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

void DisplayControl::begin_scroll(const std::string_view text) {
    _scroll_text = text;
    _scroll_text_pos = 0;
    _anim_phase = 0;
    frame_buffer.clear();
}

}

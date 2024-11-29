#include "display_control.h"
#include "controller.h"

namespace kotel {



    TimeStampMs DisplayControl::get_scheduled_time() const {
    return _next_change;
}

constexpr DotMatrix::Bitmap<5,6> fann_anim[] =  {
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

constexpr DotMatrix::Bitmap<8,6> startup[] =  {
        "  xx    "
        " xx     "
        "        "
        "        "
        "     xx "
        "    xx  ",
        "        "
        " xx     "
        " x    x "
        " x    x "
        "     xx "
        "        ",
        "        "
        "     xx "
        " x    x "
        " x    x "
        " xx     "
        "        ",
        "    xx  "
        "     xx "
        "        "
        "        "
        " xx     "
        "  xx    ",
        "  xxxx  "
        "        "
        "        "
        "        "
        "        "
        "  xxxx  "
};

constexpr DotMatrix::Bitmap<8,6> tray_open_anim[] =  {
        "        "
        "        "
        "        "
        "  xxxxxx"
        " xxxxxx "
        "  xxxxx ",
        "        "
        "  xx    "
        "    xx  "
        "      xx"
        " xxxxxx "
        "  xxxxx ",
        "    x   "
        "     x  "
        "      x "
        "       x"
        " xxxxxx "
        "  xxxxx ",
};

constexpr DotMatrix::Bitmap<3,6> feed_anim[] =  {
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

    if (cur_time < _next_change) return;
    if (!_scroll_text.empty()) {
        TR::render_text(frame_buffer, DotMatrix::font_5x3, 8-_scroll_text_pos, 4, _scroll_text);
        _scroll_text_pos++;
        if (_scroll_text_pos > (_scroll_text.size()+3)*4) {
            _scroll_text.clear();
            _scroll_text_pos = 0;
        }
        _next_change = cur_time+50;
        return;
    }
    _next_change = cur_time + 125;
    ++_anim_phase;

    auto phase_pos = _anim_phase & 0x7;
    bool alternate = !(_anim_phase & 0x8);
    frame_buffer.draw_box(0, 6, 8, 7, 0);

    if (!_cntr.is_wifi()) {
        main_temp();
        if (phase_pos == 0) {
            frame_buffer.draw_box(0,second_line,7,11,0);
            return;
        } else if (alternate) {
            _show_wifi_on = 20;
            icon_no_network.draw(frame_buffer, 0, second_line);
            return;
        }
    } else {
        if (_show_wifi_on) {
            icon_network_ok.draw(frame_buffer, 0, second_line);
            --_show_wifi_on;
            main_temp();
            if (_show_wifi_on == 0) {
                display_url();
            }
            return;
        }
    }

    if (_cntr.is_tray_open()) {
        main_temp();
        auto p = _anim_phase%8;
        constexpr int frames[] = {0,1,2,2,2,2,1,0};
        tray_open_anim[frames[p]].draw(frame_buffer, 0,6);
        return ;
    }


    if ((_cntr.is_feeder_on() || _cntr.is_fan_on())) {
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




    auto md = _cntr.get_drive_mode();

    if (md != Controller::DriveMode::automatic) {
        if (_anim_phase>50 && !_cntr.is_wifi_used()) {
            display_url();
                return;
        }
    }

    if (md == Controller::DriveMode::stop) {
        TR::render_text(frame_buffer, DotMatrix::font_5x3, 0, first_line, "ST");
        TR::render_text(frame_buffer, DotMatrix::font_5x3, 0, second_line,
                "OP");
        return;
    }
    if (md == Controller::DriveMode::manual) {
        main_temp();
        TR::render_text(frame_buffer, DotMatrix::font_5x3, 0, second_line,
                "RU");
        return;
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
    auto md = _cntr.get_drive_mode();
    if (md == Controller::DriveMode::init) {
        auto p = _anim_phase%5;
        startup[p].draw(frame_buffer, 0,0);
        return;
    }
    print_temp(first_line, _cntr.get_output_temp());
}

void DisplayControl::begin_scroll(const std::string_view text) {
    _scroll_text = text;
    _scroll_text_pos = 0;
    _anim_phase = 0;
    frame_buffer.clear();
}

void DisplayControl::display_code(IScheduler &sch, std::array<char, 4> code) {
    begin_scroll("");
    constexpr std::pair<uint8_t, uint8_t> xy[] = {{0,0},{4,0},{0,second_line},{4,second_line}};
    int p = 0;
    for (char c: code) {
        TR::render_character(frame_buffer, DotMatrix::font_5x3, xy[p].first, xy[p].second, c);
        ++p;
    }
    _next_change = get_current_timestamp()+10000;
    sch.reschedule();
}

void DisplayControl::display_url() {
    char buff[100];
    auto ip = WiFi.localIP();
    sprintf(buff,"http://%s", ip.toString().c_str());
    begin_scroll(buff);
}

}

#include "display_control.h"
#include "controller.h"
#include <fonts/font_6p.h>
#include <fonts/font_5x3.h>
#include <api/itoa.h>
#include <WifiTCP.h>
#include "version.h"

#include <cstring>
namespace kotel {


constexpr Matrix_MAX7219::Bitmap<6,2> feeder_anim[] = {
        "@  @  "
        "@@@@@@",
        " @  @ "
        "@@@@@@",
        "  @  @"
        "@@@@@@",
};

constexpr Matrix_MAX7219::Bitmap<2,2> fan_anim[] = {
        "@ "
        "@ ",
        "@@"
        "  ",
        " @"
        " @",
        "  "
        "@@"
};

constexpr Matrix_MAX7219::Bitmap<5,2> wifi_icon = {
        "@ @ @"
        " @ @ ",
};

constexpr Matrix_MAX7219::Bitmap<5,2> wifi_ap_icon = {
        "   @ "
        "  @@@",
};


constexpr Matrix_MAX7219::Bitmap<7,8> tray_fill_icon[11] = {
        "@     @"
        "@     @"
        "@     @"
        "@     @"
        "@     @"
        "@     @"
        "@     @"
        " @@@@@ ",

        "@     @"
        "@     @"
        "@     @"
        "@     @"
        "@     @"
        "@     @"
        "@ @ @ @"
        " @@@@@ ",

        "@     @"
        "@     @"
        "@     @"
        "@     @"
        "@     @"
        "@  @  @"
        "@ @ @ @"
        " @@@@@ ",

        "@     @"
        "@     @"
        "@     @"
        "@     @"
        "@     @"
        "@@ @ @@"
        "@ @ @ @"
        " @@@@@ ",

        "@     @"
        "@     @"
        "@     @"
        "@     @"
        "@ @ @ @"
        "@@ @ @@"
        "@ @ @ @"
        " @@@@@ ",

        "@     @"
        "@     @"
        "@     @"
        "@  @  @"
        "@ @ @ @"
        "@@ @ @@"
        "@ @ @ @"
        " @@@@@ ",

        "@     @"
        "@     @"
        "@     @"
        "@@ @ @@"
        "@ @ @ @"
        "@@ @ @@"
        "@ @ @ @"
        " @@@@@ ",

        "@     @"
        "@     @"
        "@ @ @ @"
        "@@ @ @@"
        "@ @ @ @"
        "@@ @ @@"
        "@ @ @ @"
        " @@@@@ ",

        "@     @"
        "@  @  @"
        "@ @ @ @"
        "@@ @ @@"
        "@ @ @ @"
        "@@ @ @@"
        "@ @ @ @"
        " @@@@@ ",

        "@     @"
        "@@ @ @@"
        "@ @ @ @"
        "@@ @ @@"
        "@ @ @ @"
        "@@ @ @@"
        "@ @ @ @"
        " @@@@@ ",

        "@ @ @ @"
        "@@ @ @@"
        "@ @ @ @"
        "@@ @ @@"
        "@ @ @ @"
        "@@ @ @@"
        "@ @ @ @"
        " @@@@@ ",
};

constexpr Matrix_MAX7219::Bitmap<5,5> full_power_anim_frames[] = {
        " @@@@"
        "  @ @"
        " @ @@"
        "@ @ @"
        " @   ",
        " @@@@"
        "  @ @"
        " @ @@"
        "@ @ @"
        "@@   ",
        " @@@@"
        "  @ @"
        " @ @@"
        "@@@ @"
        " @   ",
        " @@@@"
        "  @ @"
        " @@@@"
        "@ @ @"
        " @   ",
        " @@@@"
        "  @@@"
        " @ @@"
        "@ @ @"
        " @   ",
};

constexpr int full_power_anim[] = {0,1,2,3,4};


constexpr Matrix_MAX7219::Bitmap<5,5> low_power_anim_frames[] = {
        "     "
        "@@   "
        "@ @ @"
        "   @@"
        "     ",
        "     "
        "@@@  "
        "  @  "
        "  @@@"
        "     ",
        "     "
        " @@@ "
        "  @  "
        " @@@ "
        "     ",
        "     "
        "  @@@"
        "  @  "
        "@@@  "
        "     ",
        "     "
        "   @@"
        "@ @ @"
        "@@   "
        "     ",
        "     "
        "@   @"
        "@ @ @"
        "@   @"
        "     ",
};

constexpr int low_power_anim[] = {0,1,2,3,4,5};

constexpr Matrix_MAX7219::Bitmap<7,6> tray_open_anim_frames[] =  {
        "       "
        "       "
        "       "
        " xxxxxx"
        "xxxxxx "
        " xxxxx ",

        "       "
        " xx    "
        "   xx  "
        "     xx"
        "xxxxxx "
        " xxxxx ",

        "   x   "
        "    x  "
        "     x "
        "      x"
        "xxxxxx "
        " xxxxx ",
};

constexpr int tray_open_anim[] = {0,0,1,1,2,2,2,2,2,1,1,0,0};

constexpr Matrix_MAX7219::Bitmap<5,5> atten_anim_frames[] = {
        " @   "
        "@ @ @"
        " @ @@"
        "  @ @"
        " @@@@",

        "@@   "
        "@ @ @"
        " @ @@"
        "  @ @"
        " @@@@",

        " @   "
        "@@@ @"
        " @ @@"
        "  @ @"
        " @@@@",

        " @   "
        "@ @ @"
        " @@@@"
        "  @ @"
        " @@@@",

        " @   "
        "@ @ @"
        " @ @@"
        "  @@@"
        " @@@@",
};

constexpr int atten_anim[] = {0,0,0,0,0,0,1,2,3,4};


constexpr Matrix_MAX7219::Bitmap<5,5> motor_overheat_icon = {
        "@   @"
        "@@ @@"
        "@ @ @"
        "@   @"
        "@   @"
};




constexpr Matrix_MAX7219::Bitmap<4,8> celsius_icon={
        " @  "
        "    "
        "  @@"
        " @  "
        " @  "
        "  @@"
        "    "
        "    "
};


void DisplayControl::tray_icon() {
    const Storage &storage = _cntr.get_storage();
    uint32_t fill = storage.tray.calc_tray_fill();
    uint32_t fill_max = storage.config.tray_kg;
    uint32_t fill_pct = fill_max?(fill * 10 + (fill_max >> 1)) / fill_max:0;
    const auto &bmp = tray_fill_icon[fill_pct];
    frame_buffer.put_image( { 0, 0 }, bmp);
}

void DisplayControl::run(TimeStampMs cur_time) {
    if (_pause_display_sec) {
        resume_at(cur_time + from_seconds(_pause_display_sec));
        _pause_display_sec = 0;
        return;
    }
    resume_at( cur_time+100);
    frame_buffer.clear();

    if (cur_time < _scroll_end) {
        draw_scroll(cur_time);
        display.display(frame_buffer, 0, 0);
        return ;
    }

    ++frame;

    if (cur_time > _ipaddr_show_next) {
        _ipaddr_show_next = max_timestamp;
        if (!_cntr.is_wifi_used()) {
            auto s = _cntr.get_local_ip().toString();
            scroll_text({s.c_str(), s.length()});
            return ;
        }
    }

    if (_cntr.is_tray_open()) {
        show_tray_state();
    } else {
        tray_icon();
        drive_mode_anim(frame);
        draw_feeder_anim(frame);
        temperatures_anim(frame);
        draw_fan_anim(frame);
        draw_pump_anim(frame);
    }
    draw_wifi_state(cur_time);
    if (!(frame & 0x63)) begin();
    display.display(frame_buffer, 0, 0);




}

void DisplayControl::display_code(std::array<char, 4> code) {
    frame_buffer.clear();
    int pos = 0;
    for (const auto &x: code) {
        auto fc = Matrix_MAX7219::font_6p.get_face(x);
        if (fc) {
            frame_buffer.put_image({8*pos+2,1}, *fc);
        }
        ++pos;
    }
    display.display(frame_buffer, 0, 0);
    _pause_display_sec = 30;
}

template<typename T, int n>
constexpr int countof(const T (&)[n]) {
    return n;
}

void DisplayControl::drive_mode_anim(unsigned int anim_pos) {
    if (_cntr.is_tray_open()) {
        frame_buffer.put_image({8,0}, tray_open_anim_frames[tray_open_anim[anim_pos % countof(tray_open_anim)]]);
    } else {
        switch (_cntr.get_drive_mode()) {
            case Controller::DriveMode::init:
                break;
            case Controller::DriveMode::manual:
                TR::textout(frame_buffer,Matrix_MAX7219::font_5x3, {8,0}, "RU");break;
            default:
                if ((anim_pos >> 3) & 1) {
                    TR::textout(frame_buffer,Matrix_MAX7219::font_5x3, {0,0}, "STOP");
                } else if (_cntr.is_feeder_overheat()) {
                    frame_buffer.put_image({9,0}, motor_overheat_icon);
                }
                break;
            case Controller::DriveMode::automatic: {
                switch (_cntr.get_auto_mode()) {
                    case Controller::AutoMode::fullpower:
                        frame_buffer.put_image({9,0}, full_power_anim_frames[full_power_anim[anim_pos % countof(full_power_anim)]]);
                        break;
                    case Controller::AutoMode::lowpower:
                        frame_buffer.put_image({9,0}, low_power_anim_frames[low_power_anim[anim_pos % countof(low_power_anim)]]);
                        break;
                    default:
                    case Controller::AutoMode::off:
                        frame_buffer.put_image({9,0}, atten_anim_frames[atten_anim[anim_pos % countof(atten_anim)]]);
                        break;
                }

            }

        }
    }
}

void DisplayControl::temperatures_anim(unsigned int ) {
    auto input = _cntr.get_input_temp();
    auto output = _cntr.get_output_temp();
    char input_temp_str[2];
    char output_temp_str[2];

    auto temp_to_str = [](std::optional<float> val, char *buff) {
        if (!val.has_value()) {
            buff[0] = '-';
            buff[1] = '-';
        } else {
            int z = static_cast<int>(*val);
            if (z > 99) z = 99;
            buff[0] = z / 10+'0';
            buff[1] = z % 10+'0';
        }
    };

    temp_to_str(input, input_temp_str);
    temp_to_str(output, output_temp_str);
    TR::textout(frame_buffer, Matrix_MAX7219::font_5x3, {16,0}, output_temp_str, output_temp_str+2);
    TR::textout(frame_buffer, Matrix_MAX7219::font_5x3, {25,0}, input_temp_str, input_temp_str+2);
}

void DisplayControl::draw_feeder_anim(unsigned int gframe) {
    if (_cntr.is_feeder_on()) {
        auto frame = gframe % 3;
        frame_buffer.put_image({8,6}, feeder_anim[frame]);
    }
}

void DisplayControl::draw_fan_anim(unsigned int gframe) {
    if (_cntr.is_fan_on()) {
        auto frame = gframe % 4;
        frame_buffer.put_image({16,6}, fan_anim[frame]);
    }
}

void DisplayControl::draw_pump_anim(unsigned int) {
    if (_cntr.is_pump_on()) {
        frame_buffer.draw_box(21, 6, 22, 7,true);
    }
}

void DisplayControl::draw_wifi_state( TimeStampMs cur_time) {
    if (_cntr.is_wifi()) {
        bool topen = _cntr.is_tray_open();
        frame_buffer.put_image({27,6}, _cntr.is_wifi_ap()?wifi_ap_icon:wifi_icon);
        if (_ipaddr_show_next == max_timestamp
           && !_cntr.is_wifi_used()
           && (!topen && _tray_opened)) {
            _ipaddr_show_next = cur_time + from_seconds(5);
        }
        bool client = _cntr.get_last_net_activity() + 10000 > cur_time;
        if (client && (frame & 0x1)) {
            auto a = _cntr.get_net_activity_counter();
            if (a != _last_net_activity) {
                _last_net_activity = a;
                client = false;
            }
        }

        frame_buffer.set_pixel(25,7,client);
        _tray_opened = topen;
    } else {
        _tray_opened = true;
    }
}

void DisplayControl::display_version() {
    char c[9];
    snprintf(c,9,"v 1.%d",project_version);
    frame_buffer.clear();
    TR::textout(frame_buffer, Matrix_MAX7219::font_6p, {0,1}, c);
    display.display(frame_buffer, 0, 0);
    _pause_display_sec = 5;
}

void DisplayControl::draw_scroll(TimeStampMs cur_time) {
    int to_end = static_cast<int>((_scroll_end - cur_time)/50);
    TR::textout(frame_buffer, Matrix_MAX7219::font_5x3p, {-31+to_end,1}, _scroll_text.begin(), _scroll_text.end());
    resume_at(cur_time+50);
}

void DisplayControl::scroll_text(const std::string_view &text) {
    unsigned int len = 0;
    for (char c: text) {
        len = len + Matrix_MAX7219::font_5x3p.get_face_width(c);
    }
    _scroll_text = text;
    _scroll_end = get_current_timestamp()+(len+40) * 50;

}

void DisplayControl::begin() {
    display.begin();
    display.set_intensity(_cntr.get_storage().config.display_intensity);
}

void DisplayControl::display_init_pattern() {
    for (int j = 0; j < 2; j++) {
        for (int i = j; i < 8; i+=3) frame_buffer.draw_line(0, i, 31, i, true);
        display.display(frame_buffer);
        delay(200);
    }
}

void DisplayControl::show_tray_state() {
    frame_buffer.put_image({0,1}, tray_open_anim_frames[tray_open_anim[frame % countof(tray_open_anim)]]);
    if ((frame & 0x3) == 0) return;
    auto ct = _cntr.get_cur_tray_change();
    char buff[10];
    int x;
    if (ct._full) {
        x = 8;
        std::strcpy(buff, "MAX");
    }
    else {
        x = 12;
        int z = ct._change;
        if (z < 0) {
            buff[0] = '-';
            z = -z;
        } else {
            buff[0] = '+';
        }
        buff[0] = ct._change < 0?'-':'+';
        itoa(z, buff+1, 10);
    }
    TR::textout(frame_buffer, Matrix_MAX7219::font_6p, {x,1}, buff, buff+strlen(buff));
}

}

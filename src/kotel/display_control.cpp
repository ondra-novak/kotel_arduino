#include "display_control.h"
#include "controller.h"
#include <fonts/font_6p.h>
#include <fonts/font_5x3.h>
#include "version.h"

namespace kotel {

constexpr Matrix_MAX7219::Bitmap<5,5> arrow_up_icon = {
        "  @  "
        " @@@ "
        "@ @ @"
        "  @  "
        "  @  "

};

constexpr Matrix_MAX7219::Bitmap<5,5> arrow_right_icon = {
        "  @  "
        "   @ "
        "@@@@@"
        "   @ "
        "  @  "

};

constexpr Matrix_MAX7219::Bitmap<5,5> arrow_bottom_icon = {
        "  @  "
        "  @  "
        "@ @ @"
        " @@@ "
        "  @  "

};

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
        " @",
        " @"
        "@ "
};

constexpr Matrix_MAX7219::Bitmap<5,2> wifi_icon = {
        "@ @ @"
        " @ @ ",
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

constexpr Matrix_MAX7219::Bitmap<7,8> tray_open_anim_frames[] =  {
        "       "
        "       "
        "       "
        "       "
        "       "
        " xxxxxx"
        "xxxxxx "
        " xxxxx ",

        "       "
        "       "
        "       "
        " xx    "
        "   xx  "
        "     xx"
        "xxxxxx "
        " xxxxx ",

        "       "
        "       "
        "   x   "
        "    x  "
        "     x "
        "      x"
        "xxxxxx "
        " xxxxx ",
};

constexpr int tray_open_anim[] = {0,1,2,2,2,2,2,1};

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


constexpr Matrix_MAX7219::Bitmap<7,8> mode_init_icon = {
        "@@@@@@@"
        "@     @"
        " @@@@@ "
        "  @@@  "
        " @   @ "
        "@  @  @"
        "@ @ @ @"
        "@@@@@@@"

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
    uint32_t fill_pct = (fill * 10 + (fill_max >> 1)) / fill_max;
    const auto &bmp = tray_fill_icon[fill_pct];
    frame_buffer.put_image( { 0, 0 }, bmp);
}

void DisplayControl::run(TimeStampMs cur_time) {
    _next_change = cur_time+100;

    frame_buffer.clear();
    tray_icon();
    drive_mode_anim(cur_time);
    draw_feeder_anim(cur_time);
    temperatures_anim(cur_time);
    draw_fan_anim(cur_time);
    draw_pump_anim(cur_time);
    draw_wifi_state(cur_time);
    display.display(frame_buffer, 0, 0);



}

void DisplayControl::display_code(IScheduler &sch, std::array<char, 4> code) {
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
    _next_change = get_current_timestamp()+ from_minutes(1);
    sch.reschedule();
}

template<typename T, int n>
constexpr int countof(const T (&)[n]) {
    return n;
}

void DisplayControl::drive_mode_anim(TimeStampMs cur_time) {
    int anim_pos = static_cast<int>(cur_time/100);
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

void DisplayControl::temperatures_anim(TimeStampMs ) {
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
/*    int ofs = static_cast<int>((cur_time/100) & 0x3F);
    int y = ofs & 0x1F;
    if (y > 6) y = 6;
    if (ofs > 0x1F) {
        y = 6-y;
    }
    int r1 =  Matrix_MAX7219::font_6p.get_face_width(output_temp_str[1]);
    int r2 =  Matrix_MAX7219::font_6p.get_face_width(input_temp_str[1]);
    TR::textout(frame_buffer, Matrix_MAX7219::font_6p, {20,-y}, output_temp_str+0, output_temp_str+1);
    TR::textout(frame_buffer, Matrix_MAX7219::font_6p, {29-r1,-y}, output_temp_str+1, output_temp_str+2);
    TR::textout(frame_buffer, Matrix_MAX7219::font_6p, {20,8-y}, input_temp_str+0, input_temp_str+1);
    TR::textout(frame_buffer, Matrix_MAX7219::font_6p, {29-r2,8-y}, input_temp_str+1, input_temp_str+2);
    frame_buffer.put_image({28,-y}, celsius_icon);
    frame_buffer.put_image({28,8-y}, celsius_icon);*/
    TR::textout(frame_buffer, Matrix_MAX7219::font_5x3, {16,0}, output_temp_str, output_temp_str+2);
    TR::textout(frame_buffer, Matrix_MAX7219::font_5x3, {25,0}, input_temp_str, input_temp_str+2);
}

void DisplayControl::draw_feeder_anim(TimeStampMs cur_time) {
    if (_cntr.is_feeder_on()) {
        auto frame = static_cast<int>((cur_time/100) % 3);
        frame_buffer.put_image({8,6}, feeder_anim[frame]);
    }
}

void DisplayControl::draw_fan_anim(TimeStampMs cur_time) {
    if (_cntr.is_fan_on()) {
        auto frame = static_cast<int>((cur_time/100) % 2);
        frame_buffer.put_image({16,6}, fan_anim[frame]);
    }
}

void DisplayControl::draw_pump_anim(TimeStampMs) {
    if (_cntr.is_pump_on()) {
        frame_buffer.draw_box(21, 6, 22, 7,true);
    }
}

void DisplayControl::draw_wifi_state(TimeStampMs ) {
    if (_cntr.is_wifi()) {
        frame_buffer.put_image({27,6}, wifi_icon);
    }
}

void DisplayControl::display_version() {
    char c[9]="VER:";
    snprintf(c+4,4,"%d",project_version);
    TR::textout(frame_buffer, Matrix_MAX7219::font_5x3, {1,2}, c);
    _next_change = get_current_timestamp()+from_seconds(10);
    display.display(frame_buffer, 0, 0);
}

}

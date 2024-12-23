#include "display_control.h"
#include "controller.h"
#include <fonts/font_6p.h>

namespace kotel {

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

constexpr Matrix_MAX7219::Bitmap<8,8> full_power_anim_frames[] = {
        "    @@@@"
        "     @@@"
        "    @@@@"
        "   @@@ @"
        "  @@@   "
        "   @    "
        "        "
        "        ",
        "        "
        "   @@@@ "
        "    @@@ "
        "   @@@@ "
        "  @@@ @ "
        " @@@    "
        "  @     "
        "        ",
        "        "
        "        "
        "  @@@@  "
        "   @@@  "
        "  @@@@  "
        " @@@ @  "
        "@@@     "
        " @      ",
        "        "
        "        "
        "        "
        " @@@@   "
        "  @@@   "
        " @@@@   "
        "@@@ @   "
        "@@      ",
};

constexpr int full_power_anim[] = {3,3,2,2,1,0,1,2,3,3};


constexpr Matrix_MAX7219::Bitmap<8,8> low_power_anim_frames[] = {
        "        "
        "        "
        "        "
        "        "
        "@@@@@@@@"
        "        "
        "        "
        "        ",
        "        "
        "        "
        "        "
        "        "
        " @@@@@@@"
        "        "
        "        "
        "        ",
        "        "
        "        "
        "        "
        "        "
        "@ @@@@@@"
        "        "
        "        "
        "        ",
        "        "
        "        "
        "        "
        "@       "
        " @ @@@@@"
        "@       "
        "        "
        "        ",
        "        "
        "        "
        "@       "
        " @      "
        "  @ @@@@"
        " @      "
        "@       "
        "        ",
        "        "
        "        "
        " @      "
        "  @     "
        "@  @ @@@"
        "  @     "
        " @      "
        "        ",
        "        "
        "        "
        "  @     "
        "   @    "
        "@@  @ @@"
        "   @    "
        "  @     "
        "        ",
        "        "
        "        "
        "   @    "
        "    @   "
        "@@@  @ @"
        "    @   "
        "   @    "
        "        ",
        "        "
        "        "
        "    @   "
        "     @  "
        "@@@@  @ "
        "     @  "
        "    @   "
        "        ",
        "        "
        "        "
        "     @  "
        "      @ "
        "@@@@@  @"
        "      @ "
        "     @  "
        "        ",
        "        "
        "        "
        "      @ "
        "       @"
        "@@@@@@  "
        "       @"
        "      @ "
        "        ",
        "        "
        "        "
        "       @"
        "        "
        "@@@@@@@ "
        "        "
        "       @"
        "        ",
};

constexpr int low_power_anim[] = {0,0,0,0,0,0,1,2,3,4,5,6,7,8,9,10,11,0,0,0,0,0,0,0,0,0};


constexpr Matrix_MAX7219::Bitmap<8,8> atten_anim_frames[] = {
        "        "
        "        "
        "   @    "
        "  @ @  @"
        "   @ @ @"
        "    @ @@"
        "     @ @"
        "   @@@@@",
        "        "
        "        "
        "   @    "
        "  @@@  @"
        "   @ @ @"
        "    @ @@"
        "     @ @"
        "   @@@@@",
        "        "
        "        "
        "   @    "
        "  @ @  @"
        "   @@@ @"
        "    @ @@"
        "     @ @"
        "   @@@@@",
        "        "
        "        "
        "   @    "
        "  @ @  @"
        "   @ @ @"
        "    @@@@"
        "     @ @"
        "   @@@@@",
        "        "
        "        "
        "   @    "
        "  @ @  @"
        "   @ @ @"
        "    @ @@"
        "     @@@"
        "   @@@@@",
};

constexpr int atten_anim[] = {0,0,0,0,0,0,1,2,3,4};



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

constexpr Matrix_MAX7219::Bitmap<7,8> mode_manual_icon={
        "       "
        "@@  @ @"
        "@ @ @ @"
        "@ @ @ @"
        "@@  @ @"
        "@ @ @ @"
        "@ @  @@"
        "       "
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
    temperatures_anim(cur_time);
    display.display(frame_buffer, 0, 0);



}

void DisplayControl::display_code(IScheduler &sch, std::array<char, 4> code) {
    frame_buffer.clear();
    int pos = 0;
    for (const auto &x: code) {
        auto fc = Matrix_MAX7219::font_6p.get_face(x);
        if (fc) {
            frame_buffer.put_image({pos,0}, *fc);
        }
        ++pos;
    }
    display.display(frame_buffer, 0, 0);
    _next_change = from_minutes(1);
    sch.reschedule();
}

template<typename T, int n>
constexpr int countof(const T (&)[n]) {
    return n;
}

void DisplayControl::drive_mode_anim(TimeStampMs cur_time) {
    int anim_pos = static_cast<int>(cur_time/100);
    switch (_cntr.get_drive_mode()) {
        case Controller::DriveMode::init:
            frame_buffer.put_image({8,0}, mode_init_icon);break;
        case Controller::DriveMode::manual:
            frame_buffer.put_image({8,0}, mode_manual_icon);break;
        default:
            frame_buffer.draw_box(0, 0, 31, 0,false);
            TR::textout(frame_buffer,Matrix_MAX7219::font_6p, {0,1}, "STOP");break;
        case Controller::DriveMode::automatic: {
            switch (_cntr.get_auto_mode()) {
                case Controller::AutoMode::fullpower:
                    frame_buffer.put_image({8,0}, full_power_anim_frames[full_power_anim[anim_pos % countof(full_power_anim)]]);
                    break;
                case Controller::AutoMode::lowpower:
                    frame_buffer.put_image({8,0}, low_power_anim_frames[low_power_anim[anim_pos % countof(low_power_anim)]]);
                    break;
                default:
                case Controller::AutoMode::off:
                    frame_buffer.put_image({8,0}, atten_anim_frames[atten_anim[anim_pos % countof(atten_anim)]]);
                    break;
            }

        }

    }
}

void DisplayControl::temperatures_anim(TimeStampMs cur_time) {
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
    int ofs = static_cast<int>((cur_time/100) & 0x3F);
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
    frame_buffer.put_image({28,8-y}, celsius_icon);
}

}

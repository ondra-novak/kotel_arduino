#pragma once

#include "nonv_storage.h"
#include "sensors.h"
#include "task.h"


#include <Matrix_MAX7219.h>
#include <optional>
#include <api/IPAddress.h>

namespace kotel {


class Controller;

class DisplayControl: public AbstractTask {
public:

    using FrameBuffer = Matrix_MAX7219::Bitmap<32,8>;
    using Driver = Matrix_MAX7219::MatrixDriver<4,1,Matrix_MAX7219::ModuleOrder::right_to_left,Matrix_MAX7219::Transform::none>;
    using TR = Matrix_MAX7219::TextOutputDef<>;
    static constexpr Driver display = {display_data_pin,display_sel_pin,display_clk_pin};

    DisplayControl(const Controller &cntr):_cntr(cntr) {}

    void begin();

    void display_code(std::array<char, 4> code);
    void display_version();
    void display_init_pattern();
    void scroll_text(const std::string_view &text);

    virtual void run(TimeStampMs cur_time) override;


public:
    FrameBuffer frame_buffer;

protected:

    const Controller &_cntr;
    TimeStampMs _scroll_end = 0;
    TimeStampMs _ipaddr_show_next = max_timestamp;
    bool _tray_opened = true;
    uint8_t _pause_display_sec = 0;
    uint8_t _last_net_activity = 0;
    std::string _scroll_text = {};
    //unsigned int _scroll_text_len = 0;
    unsigned int frame = 0;

    void tray_icon();
    void drive_mode_anim(unsigned int frame);
    void temperatures_anim(unsigned int frame);
    void draw_feeder_anim(unsigned int frame);
    void draw_fan_anim(unsigned int frame);
    void draw_pump_anim(unsigned int frame);
    void draw_wifi_state(TimeStampMs cur_time);
    void draw_scroll(TimeStampMs cur_time);

    void show_tray_state();


};


}



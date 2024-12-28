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
    static constexpr Driver display = {A2,A1,A0};


    DisplayControl(const Controller &cntr):_cntr(cntr) {}

    void begin();

    void display_code(IScheduler &sch, std::array<char, 4> code);
    void display_version();
    void scroll_text(const std::string_view &text);

    virtual void run(TimeStampMs cur_time) override;


public:
    FrameBuffer frame_buffer;

protected:

    const Controller &_cntr;
    TimeStampMs _scroll_end = 0;
    TimeStampMs _ipaddr_show_next = max_timestamp;
    bool _tray_opened = true;
    std::string _scroll_text = {};
    unsigned int _scroll_text_len = 0;

    void tray_icon();
    void drive_mode_anim(TimeStampMs cur_time);
    void temperatures_anim(TimeStampMs cur_time);
    void draw_feeder_anim(TimeStampMs cur_time);
    void draw_fan_anim(TimeStampMs cur_time);
    void draw_pump_anim(TimeStampMs cur_time);
    void draw_wifi_state(TimeStampMs cur_time);
    void draw_scroll(TimeStampMs cur_time);



};


}



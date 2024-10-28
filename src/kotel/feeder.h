#pragma once
#include "timed_task.h"

#include "nonv_storage.h"
namespace kotel {

class Feeder: public AbstractTimedTask {
public:

    Feeder(Storage &storage);

    void start();

    virtual TimeStampMs get_scheduled_time() const override;
    virtual void run(TimeStampMs cur_time) override;

    ///zmena configu
    void config_changed(IScheduler &sch);
    ///rucni ovladani (nefunguje kdyz je zapnuta automatika)
    void manual_control(IScheduler &sch, bool on);
    ///aktivace utlumu (vypne podavac a vypne casovani) - nefunguje rucnim
    void attenuation(IScheduler &sch, bool on);
    ///aktivace emergency stop -> vypne i manualni rezim
    void emergency_stop(IScheduler &sch, bool on);
    ///when tray is opened (stop temporarily)
    void tray_open(IScheduler &sch, bool on);


protected:
    Storage &_storage;
    bool _att = false;
    bool _emergency = false;
    bool _tray_open = false;
    bool _state = false;
    bool _was_att = false;
    TimeStampMs _change_time = 0;

    void change_state(bool new_state);


    void set_active(bool active);
};



}

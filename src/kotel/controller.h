#pragma once
#include "feeder.h"

#include "ventilator.h"

#include "cerpadlo.h"

#include "scheduler.h"
#include "temp_sensors.h"

namespace kotel {

//controller

class Controller {
public:

    Controller();
    void begin();
    void run();

    void config_out(Stream &s);
    void stats_out(Stream &s);
    bool config_update(std::string_view body, std::string_view &&failed_field = {});
    void list_onewire_sensors(Stream &s);

protected:
    Storage _storage;
    Feeder _feeder;
    Ventilator _fan;
    Cerpadlo _pump;
    TempSensors _temp_sensors;
    Scheduler<4> _scheduler;

};

}

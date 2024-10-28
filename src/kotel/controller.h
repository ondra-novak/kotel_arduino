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
    void config_update(std::string_view body);

protected:
    Storage _storage;
    Feeder _feeder;
    Ventilator _fan;
    Cerpadlo _pump;
    Scheduler<3> _scheduler;

};

}

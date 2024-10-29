#pragma once
#include "feeder.h"
#include "fan.h"




#include "scheduler.h"
#include "temp_sensors.h"
#include "wifi_mon.h"
#include "display_control.h"
#include "sensors.h"

namespace kotel {

//controller

class Controller {
public:

    Controller();
    void begin();
    void run();

    void config_out(Stream &s);
    void stats_out(Stream &s);
    void status_out(Stream &s);
    bool config_update(std::string_view body, std::string_view &&failed_field = {});
    void list_onewire_sensors(Stream &s);

    void set_operation_mode(uint8_t mode);

protected:
    Sensors _sensors;

    Storage _storage;
    Feeder _feeder;
    Fan _fan;
    TempSensors _temp_sensors;
    WiFiMonitor _wifi_mon;
    DisplayControl _display;
    Scheduler<5> _scheduler;

    bool _pump_active = false;
    bool _attenuation = false;


    void control_pump();
    void run_manual_mode();
    void run_auto_mode();
    void run_other_mode();
    void run_stop_mode();

};

}

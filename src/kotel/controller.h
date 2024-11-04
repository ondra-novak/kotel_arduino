#pragma once
#include "feeder.h"
#include "fan.h"




#include "scheduler.h"
#include "temp_sensors.h"
#include "wifi_mon.h"
#include "display_control.h"
#include "sensors.h"

#include "pump.h"
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


    void clear_error();


    const DisplayControl &get_display() const {return _display;}

    bool is_stop() const {return _is_stop;}
    bool is_manual() const {return _storage.config.operation_mode == 0;}
    bool is_tray_open() const {return _sensors.tray_open;}

protected:
    Sensors _sensors;

    bool _auto_stop_disabled = false;
    bool _is_stop = false;

    Storage _storage;
    Feeder _feeder;
    Fan _fan;
    Pump _pump;
    TempSensors _temp_sensors;
    WiFiMonitor _wifi_mon;
    DisplayControl _display;
    Scheduler<6> _scheduler;



    bool _attenuation = false;


    void control_pump();
    void run_manual_mode();
    void run_auto_mode();
    void run_other_mode();
    void run_stop_mode();

};

}

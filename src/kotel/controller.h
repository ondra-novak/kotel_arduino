#pragma once
#include "feeder.h"
#include "fan.h"




#include "scheduler.h"
#include "temp_sensors.h"
#include "wifi_mon.h"
#include "display_control.h"
#include "sensors.h"

#include "pump.h"

#include "motoruntime.h"
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
    bool is_wifi() const {return _wifi_mon.is_connected();}
    auto get_input_temp() const {return _temp_sensors.get_input_temp();}
    auto get_output_temp() const {return _temp_sensors.get_output_temp();}
    bool is_feeder_on() const {return _feeder.is_active();}
    bool is_pump_on() const {return _pump.is_active();}
    bool is_fan_on() const {return _fan.is_active();}
    bool is_attenuation() const {return _attenuation;}
    int calc_tray_remain() const;
    Storage &get_storage() {return _storage;}
    void set_wifi_used() {_wifi_used = true;}
    bool is_wifi_used() const {return _wifi_used;}

protected:
    Sensors _sensors;

    bool _auto_stop_disabled = false;
    bool _is_stop = false;
    bool _wifi_used = false;

    Storage _storage;
    Feeder _feeder;
    Fan _fan;
    Pump _pump;
    TempSensors _temp_sensors;
    WiFiMonitor _wifi_mon;
    DisplayControl _display;
    MotoRunTime _motoruntime;
    Scheduler<6> _scheduler;



    bool _attenuation = false;


    void control_pump();
    void run_manual_mode();
    void run_auto_mode();
    void run_other_mode();
    void run_stop_mode();

};

}

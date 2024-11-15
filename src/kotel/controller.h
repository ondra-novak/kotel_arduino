#pragma once
#include "feeder.h"
#include "fan.h"




#include "scheduler.h"
#include "temp_sensors.h"
#include "wifi_mon.h"
#include "display_control.h"
#include "sensors.h"

#include "pump.h"


#include "http_server.h"
#include "http_utils.h"


namespace kotel {

//controller

class Controller {
public:

    enum class DriveMode {
        unknown,
        manual,
        automatic,
        stop,
        other
    };

    enum class AutoMode {
        active,     //aktivne se topi
        attenuation,    //utlum
        renewal         //obnova plamene
    };


    using MyHttpServer = HttpServer<4096,32>;

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

    bool is_tray_open() const {return _sensors.tray_open;}
    bool is_wifi() const;
    auto get_input_temp() const {return _temp_sensors.get_input_temp();}
    auto get_output_temp() const {return _temp_sensors.get_output_temp();}
    bool is_feeder_on() const {return _feeder.is_active();}
    bool is_pump_on() const {return _pump.is_active();}
    bool is_fan_on() const {return _fan.is_active();}
    bool is_attenuation() const {return _auto_mode == AutoMode::attenuation;}
    int calc_tray_remain() const;
    Storage &get_storage() {return _storage;}
    void set_wifi_used() {_wifi_used = true;}
    bool is_wifi_used() const {return _wifi_used;}
    DriveMode get_drive_mode() const {return _cur_mode;}

    void out_form_config(MyHttpServer::Request &req);

    struct ManualControlStruct {
        uint16_t _feeder_time = 0;
        uint16_t _fan_time = 0;
        uint16_t _fan_speed = 0;
        uint16_t _force_pump = 0;
    };

    ///for manual control, this must be called repeatedly
    bool manual_control(const ManualControlStruct &cntr);
    bool manual_control(std::string_view body, std::string_view &&error_field);


    TimeStampMs run_http(TimeStampMs tmsp);

    TimeStampMs update_motorhours(TimeStampMs  now);


protected:
    Sensors _sensors;

    bool _auto_stop_disabled = false;
    bool _is_stop = false;
    bool _wifi_used = false;
    bool _force_pump = false;

    DriveMode _cur_mode = DriveMode::unknown;
    AutoMode _auto_mode = AutoMode::active;
    TimeStampMs _auto_mode_change = 0;
    TimeStampMs _flush_time = 0;

    Storage _storage;
    Feeder _feeder;
    Fan _fan;
    Pump _pump;
    TempSensors _temp_sensors;
    DisplayControl _display;
    TimedTaskMethod<Controller, &Controller::update_motorhours> _motoruntime;
    Scheduler<5> _scheduler;
    MyHttpServer _server;
    std::optional<WiFiClient> _list_temp_async;




    void control_pump();
    void run_manual_mode();
    void run_auto_mode();
    void run_other_mode();
    void run_stop_mode();
    void init_wifi();

    void handle_server(MyHttpServer::Request &req);
    void send_file(MyHttpServer::Request &req, std::string_view content_type, std::string_view file_name);

    bool set_fuel(std::string_view req, std::string_view &&error);
};

}

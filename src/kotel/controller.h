#pragma once
#include "feeder.h"
#include "fan.h"
#include "scheduler.h"
#include "temp_sensors.h"
#include "display_control.h"
#include "sensors.h"
#include "pump.h"
#include "http_server.h"
#include "http_utils.h"
#include <WDT.h>

#include "ntp.h"
#include "simple_dns.h"

namespace kotel {

//controller

class Controller {
public:

    enum class DriveMode {
        unknown,
        manual,
        automatic,
        stop,
        init,
        other
    };

    enum class AutoMode {
        fullpower,
        lowpower,
        off,
        notset
    };


    using MyHttpServer = HttpServer<4096,32>;

    Controller();
    void begin();
    void run();

    void config_out(Stream &s);
    void status_out(Stream &s);
    bool config_update(std::string_view body, std::string_view &&failed_field = {});
    void list_onewire_sensors(Stream &s);


    const DisplayControl &get_display() const {return _display;}

    bool is_tray_open() const {return _sensors.tray_open;}
    bool is_wifi() const;
    auto get_input_temp() const {return _temp_sensors.get_input_temp();}
    auto get_output_temp() const {return _temp_sensors.get_output_temp();}
    bool is_feeder_on() const {return _feeder.is_active();}
    bool is_pump_on() const {return _pump.is_active();}
    bool is_fan_on() const {return _fan.is_active();}
    Storage &get_storage() {return _storage;}
    void set_wifi_used() {_wifi_used = true;}
    bool is_wifi_used() const {return _wifi_used;}
    DriveMode get_drive_mode() const {return _cur_mode;}
    const IPAddress &get_my_ip() const {return _my_ip;}

    struct ManualControlStruct {
        uint8_t _feeder_time = 0;
        uint8_t _fan_time = 0;
        uint8_t _fan_speed = 0;
        uint8_t _force_pump = 0xFF;
    };


struct SetFuelParams {
    int16_t kgchg = 0;
    int8_t kalib = 0;
    int8_t absnow = 0;
    int8_t full = 0;
};

    ///for manual control, this must be called repeatedly
    bool manual_control(const ManualControlStruct &cntr);
    bool manual_control(std::string_view body, std::string_view &&error_field);



    TimeStampMs update_motorhours(TimeStampMs  now);

    //simulation
    bool enable_temperature_simulation(std::string_view command);
    void disable_temperature_simulation();

    void factory_reset();
protected:

    TimeStampMs auto_drive_cycle(TimeStampMs cur_time);
    TimeStampMs wifi_mon(TimeStampMs  cur_time);
    TimeStampMs run_server(TimeStampMs cur_time);
    TimeStampMs read_serial(TimeStampMs cur_time);
    TimeStampMs refresh_wdt(TimeStampMs cur_time);
protected:


    Sensors _sensors;

    bool _wifi_used = false;
    bool _wifi_connected = false;
    bool _force_pump = false;
    bool _was_tray_open = false;
    int16_t _wifi_rssi = -100;

    DriveMode _cur_mode = DriveMode::init;
    AutoMode _auto_mode = AutoMode::fullpower;
    TimeStampMs _flush_time = 0;
    TimeStampMs _time_resync = 0;
    TimeStampMs _last_net_activity = max_timestamp;
    TimeStampMs _start_mode_until = 0;
    NTPClient _ntp;
    IPAddress _ntp_addr;
    SimpleDNS _sdns;



    Storage _storage;
    Feeder _feeder;
    Fan _fan;
    Pump _pump;
    TempSensors _temp_sensors;
    DisplayControl _display;
    TimedTaskMethod<Controller, &Controller::update_motorhours> _motoruntime;
    TimedTaskMethod<Controller, &Controller::auto_drive_cycle> _auto_drive_cycle;
    TimedTaskMethod<Controller, &Controller::wifi_mon> _wifi_mon;
    TimedTaskMethod<Controller, &Controller::run_server> _run_server;
    TimedTaskMethod<Controller, &Controller::read_serial> _read_serial;
    TimedTaskMethod<Controller, &Controller::refresh_wdt> _refresh_wdt;
    Scheduler<10> _scheduler;
    MyHttpServer _server;
    std::optional<TCPClient> _list_temp_async;
    StringStream<1024> static_buff;
    std::array<char, 4> _last_code;
    IPAddress _my_ip;

    enum class WsReqCmd {
        file_config = 0,
        file_tray = 1,
        file_util1 = 2,
        file_cntrs1 = 3,
        file_util2 = 4,
        file_tempsensor = 5,
        file_wifi_ssid = 6,
        file_wifi_pwd = 7,
        file_wifi_net = 8,
        file_cntrs2 = 9,

        control_status = 'c',
        set_fuel = 'f',
        get_config = 'C',
        set_config = 'S',
        failed_config = 'F',
        get_stats = 'T',
        ping = 'p',
        enum_tasks = '#',
        generate_code = 'G',
        unpair_all ='U',
        reset = '!',
        clear_stats = '0'


    };

    void control_pump();
    void run_manual_mode();
    void run_auto_mode();
    void run_other_mode();
    void run_stop_mode();
    void run_init_mode();
    void init_wifi();

    void handle_server(MyHttpServer::Request &req);
    void handle_ws_request(MyHttpServer::Request &req);
    void send_file(MyHttpServer::Request &req, std::string_view content_type, std::string_view file_name);

    bool set_fuel(const SetFuelParams &sfp);
    void status_out_ws(Stream &s);
    std::string_view get_task_name(const AbstractTimedTask *task);
    bool is_overheat() const;
    void generate_otp_code();
    std::array<char, 20> generate_token_random_code();
    std::array<char, 40> generate_signed_token(std::string_view random);
    void generate_pair_secret();
    void gen_and_print_token();
    //void update_time();
    void init_serial_log();

};

}

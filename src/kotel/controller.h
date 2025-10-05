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

#include "network_control.h"

#include "keyboard.h"
namespace kotel {




//controller

class Controller {
public:

    static constexpr int stop_btn_start_interval_ms = 2000;
    static constexpr int default_btn_release_interval_ms = 100;
    static constexpr int default_btn_press_interval_ms = 100;
    static constexpr int negative_tray_btn_press_interval_ms = 1000;
    static constexpr int manual_run_interval_ms = 120000;
    static constexpr int feeder_min_press_interval_ms = 150;
    static constexpr int fan_speed_change_step = 20;
    static constexpr int day_length_seconds = 24*60*60;

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

    struct TrayChange {
        bool _full;
        int _change;
    };

    using MyHttpServer = NetworkControl::MyHttpServer;

    Controller();
    void begin();
    void run();

    void config_out(Stream &s);
    void status_out(Stream &s);
    void stats_out(Stream &s);
    bool config_update(std::string_view body, std::string_view &&failed_field = {});
    void list_onewire_sensors(Stream &s);


    const DisplayControl &get_display() const {return _display;}

    bool is_safe_for_blocking() const;
    bool is_tray_open() const {return _sensors.tray_open;}
    bool is_wifi() const;
    bool is_wifi_ap() const;
    auto get_net_activity_counter() const {return _network.get_server().get_activity_counter();}
    TimeStampMs get_last_net_activity() const {return _last_net_activity;}
    auto get_input_temp() const {return _temp_sensors.get_input_temp();}
    auto get_output_temp() const {return _temp_sensors.get_output_temp();}
    bool is_feeder_on() const {return _feeder.is_active();}
    bool is_pump_on() const {return _pump.is_active();}
    int get_fan_level() const {return _fan.get_speed_for_display();}
    bool is_fan_on() const {return _fan.is_active();}
    bool is_feeder_overheat() const {return _sensors.feeder_overheat;}
    Storage &get_storage() {return _storage;}
    const Storage &get_storage() const {return _storage;}
    void set_wifi_used() {_wifi_used = true;}
    bool is_wifi_used() const {return _wifi_used;}
    IPAddress get_local_ip() const;
    DriveMode get_drive_mode() const {return _cur_mode;}
    AutoMode get_auto_mode() const {return _auto_mode;}
    const IPAddress &get_my_ip() const {return _my_ip;}
    TrayChange get_cur_tray_change() const {return _cur_tray_change;}

    struct ManualControlStruct {
        uint32_t _control_id;
        uint8_t _feeder_time = 0;
        uint8_t _fan_time = 0;
        uint8_t _fan_speed = 0;
        uint8_t _force_pump = 0xFF;
        static ManualControlStruct from(std::string_view str);
    };

    struct StatusOut {
        uint32_t cur_time;
        uint32_t control_id;
        int16_t temp_output_value;
        int16_t temp_output_amp_value;
        int16_t temp_input_value;
        int16_t temp_input_amp_value;
        int16_t rssi;
        uint8_t tray_fill_pct;
        uint8_t temp_sim;
        uint8_t temp_input_status;
        uint8_t temp_output_status;
        uint8_t mode;
        uint8_t automode;
        uint8_t try_open;
        uint8_t motor_temp_ok;
        uint8_t pump;
        uint8_t feeder;
        uint8_t fan;
    };

    struct HistoryRequest {
        uint16_t day_from = 0;
        uint16_t day_to = 0;
        static HistoryRequest from(std::string_view str);
    };

    StatusOut status_out() const;

    void history_out(const HistoryRequest &req, Print &out);


    void handle_server(MyHttpServer::Request &req);


struct SetFuelParams {
    int16_t kgchg = 0;
    int8_t kalib = 0;
    int8_t absnow = 0;
    int8_t full = 0;
};

    ///for manual control, this must be called repeatedly
    bool manual_control(const ManualControlStruct &cntr);



    TimeStampMs update_motorhours(TimeStampMs now);
    TimeStampMs run_keyboard(TimeStampMs now);


    //simulation
    bool enable_temperature_simulation(std::string_view command);
    void disable_temperature_simulation();

    void factory_reset();


protected:

    TimeStampMs auto_drive_cycle(TimeStampMs cur_time);
    TimeStampMs read_serial(TimeStampMs cur_time);
    TimeStampMs refresh_wdt(TimeStampMs cur_time);
    TimeStampMs daily_log(TimeStampMs cur_time);
protected:


    Sensors _sensors;

    bool _wifi_used = false;
    bool _force_pump = false;
    bool _was_tray_open = false;
    bool _keyboard_connected = false;
    bool _fan_step_down = false;
    bool _ap_scan = false;

    DriveMode _cur_mode = DriveMode::init;
    AutoMode _auto_mode = AutoMode::fullpower;
    TimeStampMs _flush_time = 0;
    TimeStampMs _time_resync = 0;
    TimeStampMs _last_net_activity = max_timestamp;
    TimeStampMs _start_mode_until = 0;



    Storage _storage;
    Feeder _feeder;
    Fan _fan;
    Pump _pump;
    TempSensors _temp_sensors;
    DisplayControl _display;
    TaskMethod<Controller, &Controller::update_motorhours> _motoruntime;
    TaskMethod<Controller, &Controller::auto_drive_cycle> _auto_drive_cycle;
    TaskMethod<Controller, &Controller::read_serial> _read_serial;
    TaskMethod<Controller, &Controller::refresh_wdt> _refresh_wdt;
    TaskMethod<Controller, &Controller::run_keyboard> _keyboard_scanner;
    TaskMethod<Controller, &Controller::daily_log> _daily_log;
    NetworkControl _network;
    Scheduler<11> _scheduler;
    std::optional<TCPClient> _list_temp_async;
    StringStream<1024> static_buff;
    std::array<char, 4> _last_code;
    std::vector<WiFiUtils::AccessPoint> _aplist;
    IPAddress _my_ip;
    MyKeyboard::State _kbdstate;
    TrayChange _cur_tray_change;
    std::uint32_t _man_mode_control_id = 1;

    enum class WsReqCmd {

        set_fuel = 'f',
        get_config = 'C',
        set_config = 'S',
        get_stats = 'T',
        ping = 'p',
        generate_code = 'G',
        unpair_all ='U',
        reset = '!',
        clear_stats = '0',
        reset_fuel = 'r',
        monitor_cycle = 'm',
        history = 'h'

    };

    void control_pump();
    void run_manual_mode();
    void run_auto_mode();
    void run_other_mode();
    void run_stop_mode();
    void run_init_mode();

    void handle_ws_request(MyHttpServer::Request &req);
    void send_file(MyHttpServer::Request &req, std::string_view content_type, std::string_view file_name);

    bool is_overheat() const;
    void generate_otp_code();
    std::array<char, 20> generate_token_random_code();
    std::array<char, 40> generate_signed_token(std::string_view random);
    void generate_pair_secret();
    void gen_and_print_token();
    //void update_time();
    void init_serial_log();

#ifdef EMULATOR
    void prepare_history_mockup();
#endif


};

}

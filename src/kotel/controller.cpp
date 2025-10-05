
#include "controller.h"
#include "http_utils.h"
#include "web_page.h"
#include "stringstream.h"
#include "serial.h"
#include "version.h"
#include <SoftwareATSE.h>
#include "controller_config.h"

#include "sha1.h"
#ifdef EMULATOR
#include <fstream>
extern std::string www_path;
#endif

#define ENABLE_VDT 1

#ifdef EMULATOR
#include <random>
// Funkce pro generování mock dat spotřeby
template <typename OutFn>
void generateConsumptionData(
    OutFn out,
    int days = 360,           // počet dnů
    int baseValue = 20,       // výchozí hodnota
    int maxDelta = 1,         // maximální odchylka nahoru/dolů
    unsigned seed = std::random_device{}() // seed pro RNG
) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(-maxDelta, maxDelta);

    int value = baseValue;
    for (int i = 0; i < days; ++i) {
        out(value);
        int delta = dist(rng);
        value = std::max(0, value + delta); // nesmí být záporné
    }
}
#endif


namespace kotel {




Controller::Controller()
        :_feeder(_storage)
        ,_fan(_storage)
        ,_pump(_storage)
        ,_temp_sensors(_storage)
        ,_display(*this)
        ,_motoruntime(this)
        ,_auto_drive_cycle(this)
        ,_read_serial(this)
        ,_refresh_wdt(this)
        ,_keyboard_scanner(this)
        ,_daily_log(this)
        ,_network(*this)
        ,_scheduler({&_feeder, &_fan, &_temp_sensors,  &_display,
            &_motoruntime, &_auto_drive_cycle, &_network,
            &_read_serial, &_refresh_wdt, &_keyboard_scanner, &_daily_log})
{
}

void Controller::begin() {
    _storage.begin();
    init_serial_log();
    _display.begin();
    _display.display_version();
    _fan.begin();
    _feeder.begin();
    _pump.begin();
    _temp_sensors.begin();
    _display.display_init_progress(1);
    _network.begin();

    if (_storage.pair_secret_need_init) {
        generate_pair_secret();
    }
    _display.display_init_progress(3);
    kbdcntr.begin();
    _keyboard_connected = true;
    for (int i = 0; i < 100 && _keyboard_connected; ++i) {
        kbdcntr.read(_kbdstate);
        if (_kbdstate.last_level != 0) _keyboard_connected = false;
        delay(1);
    }
    print(Serial, "Keyboard: ", _keyboard_connected?"connected":"not detected","\r\n");
    _storage.cntr.restart_count++;
    _display.display_init_progress(5);
#if ENABLE_VDT
    Serial.print("Init WDT: ");
    Serial.println(WDT.begin(5589));
#endif
    _display.display_init_progress(7);

}

static inline bool defined_and_above(const std::optional<float> &val, float cmp) {
    return val.has_value() && *val > cmp;
}

void Controller::run() {
    _sensors.read_sensors();
    auto prev_mode = _cur_mode;
    control_pump();
    if (_sensors.tray_open) {
        _storage.tray.feeder_time_open_tray = _storage.runtm.feeder;
        _was_tray_open = true;
        _feeder.stop();
        _fan.stop();
    } else {
        if (_was_tray_open) {
            _was_tray_open = false;
            _storage.cntr.tray_open_count++;
            if (_cur_tray_change._full) {
                _storage.set_max_fill();
            } else if (_cur_tray_change._change) {
                _storage.add_fuel(_cur_tray_change._change*_storage.config.bag_kg);
            }
            _cur_tray_change = {};
            _storage.save();
        }
        if (_sensors.feeder_overheat || is_overheat()) {
            run_stop_mode();
        } else if (_storage.config.operation_mode == 0) {
            run_manual_mode();
        } else if (_storage.config.operation_mode == 1) {
            run_auto_mode();
        } else {
            run_other_mode();
        }
    }

    if (prev_mode != _cur_mode) {
        _feeder.stop();
        _fan.stop();
    }
    if (_list_temp_async && !_temp_sensors.is_reading() && is_safe_for_blocking()) {
        list_onewire_sensors(*_list_temp_async);
        _list_temp_async->stop();
        _list_temp_async.reset();
    }
    if (_ap_scan) {
        _ap_scan = false;
        _fan.stop();
        _feeder.stop();
        WiFiUtils::Scanner scn;
        scn.begin();
        auto tm = get_current_timestamp();
        auto stop = tm + 10000;
        while (!scn.is_ready() && stop > tm) {
            delay(1);
            WDT.refresh();
            tm = get_current_timestamp();
        }
        _aplist = scn.get_result();
        _network.restore_wifi_mode();
        return;
    }

    _scheduler.run();
    _storage.commit();

}

static constexpr std::pair<const char *, uint8_t Profile::*> profile_table[] = {
        {"burnout",&Profile::burnout_sec},
        {"fanpw",&Profile::fanpw},
        {"fueling",&Profile::fueling_sec},
};

static constexpr std::pair<const char *, uint8_t Config::*> config_table[] ={
        {"tout", &Config::output_max_temp},
        {"tfull", &Config::input_temp_full_power},
        {"tlow", &Config::input_temp_low_power},
        {"touts", &Config::output_temp_samples},
        {"tins", &Config::input_temp_samples},
        {"tlsf", &Config::lowerst_safe_temp},
        {"m", &Config::operation_mode},
        {"fanpc", &Config::fan_pulse_interval},
        {"tpump",&Config::lowerst_safe_temp},
        {"srlog",&Config::serial_log_out},
        {"bgkg",&Config::bag_kg},
        {"traykg",&Config::tray_kg},
        {"dspli",&Config::display_intensity},
};

static constexpr std::pair<const char *, OneDecimalValue Config::*> config_table_2[] ={
        {"hval",&Config::heat_value},
        {"fannc", &Config::fan_nonlinear_correction},
};
static constexpr std::pair<const char *, SimpleDallasTemp::Address TempSensor::*> tempsensor_table_1[] ={
        {"tsinaddr", &TempSensor::input_temp},
        {"tsoutaddr", &TempSensor::output_temp},
};
static constexpr std::pair<const char *, int8_t TempSensor::*> tempsensor_calibration_table[] =  {
        {"tsincalib60", &TempSensor::input_calib_60},
        {"tsoutcalib60", &TempSensor::output_calib_60},
};

static constexpr std::pair<const char *, uint32_t Tray::*> tray_table[] ={
        {"fd.Et", &Tray::feeder_time_accum},
        {"fuel.Em", &Tray::fuel_kg_accum},
        {"fd.tefp", &Tray::feeder_time_enter_full_power},
        {"fd.open", &Tray::feeder_time_open_tray},
        {"fd.lastf",&Tray::feeder_time_last_fill},
};

static constexpr std::pair<const char *, uint16_t Tray::*> tray_table_2[] ={
        {"fd.speed", &Tray::feeder_speed}
};

static constexpr std::pair<const char *, int16_t Tray::*> tray_table_3[] ={
        {"fd.initf", &Tray::initial_fill_adj}
};

static constexpr std::pair<const char *, TextSector WiFi_SSID::*> wifi_ssid_table[] ={
        {"wifi.ssid", &WiFi_SSID::ssid},
};
static constexpr std::pair<const char *, PasswordSector WiFi_Password::*> wifi_password_table[] ={
        {"wifi.password", &WiFi_Password::password},
};
static constexpr std::pair<const char *, PasswordSector WiFi_Password::*> pair_sectet_table[] ={
        {"pair_secret", &WiFi_Password::password},
};

static constexpr std::pair<const char *, IPAddr WiFi_NetSettings::*> wifi_netcfg_table[] ={
        {"net.ip", &WiFi_NetSettings::ipaddr},
        {"net.dns", &WiFi_NetSettings::dns},
        {"net.gateway", &WiFi_NetSettings::gateway},
        {"net.netmask", &WiFi_NetSettings::netmask},
};

static constexpr std::pair<const char *, uint8_t Controller::ManualControlStruct::*> man_control_table_1[] ={
        {"fdt",&Controller::ManualControlStruct::_feeder_time},
        {"fnt",&Controller::ManualControlStruct::_fan_time},
        {"fns",&Controller::ManualControlStruct::_fan_speed},
        {"fcp",&Controller::ManualControlStruct::_force_pump},
};
static constexpr std::pair<const char *, uint32_t Controller::ManualControlStruct::*> man_control_table_2[] ={
        {"ctr",&Controller::ManualControlStruct::_control_id},
};

static constexpr std::pair<const char *, uint32_t Controller::StatusOut::*> status_table_1[] ={
        {"time",&Controller::StatusOut::cur_time},
        {"cntr",&Controller::StatusOut::control_id}
};
static constexpr std::pair<const char *, int16_t Controller::StatusOut::*> status_table_2[] ={
        {"to",&Controller::StatusOut::temp_output_value},
        {"tao",&Controller::StatusOut::temp_output_amp_value},
        {"ti",&Controller::StatusOut::temp_input_value},
        {"tai",&Controller::StatusOut::temp_input_amp_value},
        {"rssi",&Controller::StatusOut::rssi}

};
static constexpr std::pair<const char *, uint8_t Controller::StatusOut::*> status_table_3[] ={
        {"tfp",&Controller::StatusOut::tray_fill_pct},
        {"tsim",&Controller::StatusOut::temp_sim},
        {"tis",&Controller::StatusOut::temp_input_status},
        {"tos",&Controller::StatusOut::temp_output_status},
        {"m",&Controller::StatusOut::mode},
        {"am",&Controller::StatusOut::automode},
        {"tro",&Controller::StatusOut::try_open},
        {"mto",&Controller::StatusOut::motor_temp_ok},
        {"p",&Controller::StatusOut::pump},
        {"fd",&Controller::StatusOut::feeder},
        {"fn",&Controller::StatusOut::fan}
};

static constexpr std::pair<const char *, uint16_t Controller::HistoryRequest::*> history_table[] = {
        {"from", &Controller::HistoryRequest::day_from},
        {"to", &Controller::HistoryRequest::day_to},
};
static constexpr std::pair<const char *, uint32_t Runtime::*> stats_table_runtime[] ={
        {"rt.feeder", &Runtime::feeder},
        {"rt.fan", &Runtime::fan},
        {"rt.feeder_low", &Runtime::feeder_low},
        {"rt.full_power", &Runtime::full_power},
        {"rt.low_power", &Runtime::low_power},
        {"rt.cooling", &Runtime::cooling},
        {"rt.stop_time", &Runtime::stop_time}
};
static constexpr std::pair<const char *, uint16_t Counters::*> stats_table_counters[] ={
        {"cnt.feeder_overheat",&Counters::feeder_overheat_count},
        {"cnt.tray_open",&Counters::tray_open_count},
        {"cnt.restart",&Counters::restart_count},
        {"cnt.overheat",&Counters::overheat_count},
        {"cnt.therm_failure",&Counters::therm_failure_count},
        {"cnt.fan_start",&Counters::fan_start_count},
        {"cnt.feeder_start",&Counters::feeder_start_count},
        {"cnt.stop",&Counters::stop_count},
        {"cnt.manual_control",&Counters::manual_control_count}
};
static constexpr std::pair<const char *, uint32_t Tray::*> stats_table_tray[] ={
        {"tray.feeder_time_accum",&Tray::feeder_time_accum},
        {"tray.fuel_kg_accum",&Tray::fuel_kg_accum},
        {"tray.feeder_time_enter_full_power",&Tray::feeder_time_enter_full_power},
        {"tray.feeder_time_open_tray",&Tray::feeder_time_open_tray},
        {"tray.feeder_time_last_fill",&Tray::feeder_time_last_fill},
};
static constexpr std::pair<const char *, uint16_t Tray::*> stats_table_tray2[] ={
        {"tray.feeder_speed", &Tray::feeder_speed}
};
static constexpr std::pair<const char *, int16_t Tray::*> stats_table_tray3[] ={
        {"tray.initial_fill_adj", &Tray::initial_fill_adj}
};

static int16_t encode_temp(std::optional<float> v) {
    if (v.has_value()) {
        return static_cast<int16_t>(*v*10.0);
    } else {
        return static_cast<int16_t>(0x8000);
    }
}

Controller::ManualControlStruct Controller::ManualControlStruct::from(std::string_view str) {
    ManualControlStruct out;
    update_settings_fd(out, str, man_control_table_1, man_control_table_2);
    return out;
}
Controller::HistoryRequest Controller::HistoryRequest::from(std::string_view str) {
    HistoryRequest out;
    update_settings_fd(out, str, history_table);
    return out;
}


Controller::StatusOut Controller::status_out() const {
    return {
            get_current_time(),
            _man_mode_control_id,
            encode_temp(_temp_sensors.get_output_temp()),
            encode_temp(_temp_sensors.get_output_ampl()),
            encode_temp(_temp_sensors.get_input_temp()),
            encode_temp(_temp_sensors.get_input_ampl()),
            static_cast<int16_t>(_network.get_rssi()),
            static_cast<uint8_t>(_storage.calc_remaining_fuel_pct()),
            static_cast<uint8_t>(_temp_sensors.is_simulated()?1:0),
            static_cast<uint8_t>(_temp_sensors.get_output_status()),
            static_cast<uint8_t>(_temp_sensors.get_input_status()),
            static_cast<uint8_t>(_cur_mode),
            static_cast<uint8_t>(_auto_mode),
            static_cast<uint8_t>(_sensors.tray_open?1:0),
            static_cast<uint8_t>(_sensors.feeder_overheat?1:0),
            static_cast<uint8_t>(_pump.is_active()?1:0),
            static_cast<uint8_t>(_feeder.is_active()?1:0),
            static_cast<uint8_t>(_fan.get_current_speed()),
   };
}

void Controller::config_out(Stream &s) {

    print_table(s, config_table, _storage.config);
    print_table(s, config_table_2, _storage.config);
    print_table(s, profile_table, _storage.config.full_power, "full.");
    print_table(s, profile_table, _storage.config.low_power, "low.");
    print_table(s, tempsensor_table_1, _storage.temp);
    print_table(s, tempsensor_calibration_table, _storage.temp);
    print_table(s, wifi_ssid_table, _storage.wifi_ssid);
    print_table(s, wifi_password_table, _storage.wifi_password);
    print_table(s, wifi_netcfg_table, _storage.wifi_config);
    print_table(s, tray_table, _storage.tray);
    print_table(s, tray_table_2, _storage.tray);
    print_table(s, tray_table_3, _storage.tray);
    print(s,"ver=",project_version,"\n");
}


bool Controller::config_update(std::string_view body, std::string_view &&failed_field) {

    do {
        auto value = split(body, "&");
        auto key = trim(split(value, "="));
        value = trim(value);
        bool ok = update_settings(config_table, _storage.config, key, value)
                || update_settings(config_table_2, _storage.config, key, value)
                || update_settings(profile_table, _storage.config.full_power, key, value, "full.")
                || update_settings(profile_table, _storage.config.low_power, key, value, "low.")
                || update_settings(tempsensor_table_1, _storage.temp, key, value)
                || update_settings(tempsensor_calibration_table, _storage.temp, key, value)
                || update_settings(wifi_ssid_table, _storage.wifi_ssid, key, value)
                || update_settings(wifi_password_table, _storage.wifi_password, key, value)
                || update_settings(wifi_netcfg_table, _storage.wifi_config, key, value)
                || update_settings(pair_sectet_table, _storage.pair_secret, key, value)
                || update_settings(tray_table_2, _storage.tray, key, value);

        if (!ok) {
            failed_field = key;
            return false;
        }

    } while (!body.empty());
    _storage.save();
    _display.begin();
    return true;

}



void Controller::list_onewire_sensors(Stream &s) {
    auto cntr = _temp_sensors.get_controller();
    cntr.request_temp();
    delay(200);
    cntr.enum_devices([&](const auto &addr){
        print_data(s, addr);
        s.print('=');
        auto tmp = cntr.read_temp_celsius(addr);
        if (tmp) print_data(s, *tmp);
        s.println();
        return true;
    });
}

void Controller::status_out(Stream &s) {
    StatusOut st = status_out();
    print_table(s, status_table_1, st);
    print_table(s, status_table_2, st);
    print_table(s, status_table_3, st);
}

void Controller::stats_out(Stream &s) {
    print_table(s, stats_table_runtime, _storage.runtm);
    print_table(s, stats_table_counters, _storage.cntr);
    print_table(s, stats_table_tray, _storage.tray);
    print_table(s, stats_table_tray2, _storage.tray);
    print_table(s, stats_table_tray3, _storage.tray);
    print(s,"tray.remain=",_storage.calc_remaining_fuel(),"\n");
    print(s,"eeprom_errors=", _storage.get_eeprom().get_crc_error_counter(),"\n");
}

void Controller::control_pump() {
//    auto t_input = _temp_sensors.get_input_temp();
    _pump.set_active(_storage.config.operation_mode == static_cast<uint8_t>(OperationMode::automatic)
                || _force_pump );
}

void Controller::run_manual_mode() {
    if (_cur_mode != DriveMode::manual) {
        ++_storage.cntr.manual_control_count;
    }
    _cur_mode = DriveMode::manual;
    _auto_mode = AutoMode::notset;
    _start_mode_until = get_current_timestamp() + from_minutes(120);

}

void Controller::run_auto_mode() {

    bool start_mode = _start_mode_until > get_current_timestamp();

    auto t_input = _temp_sensors.get_input_temp();
    auto t_output = _temp_sensors.get_output_temp();
    if (t_input.has_value()
       && t_output.has_value()
       && t_input.value() <= _storage.config.output_max_temp
       && t_output.value() <= _storage.config.output_max_temp
       && ((   t_input.value() >= _storage.config.lowerst_safe_temp
            && t_output.value() >= _storage.config.lowerst_safe_temp)
            || start_mode)) {


        if (_cur_mode == DriveMode::init) {
            if (get_current_timestamp() > 6000) {
                _cur_mode = DriveMode::unknown;
            }
        } else if (_cur_mode != DriveMode::automatic)  {
            _cur_mode = DriveMode::automatic;
            _auto_drive_cycle.wake_up();
        }
    } else {
        run_stop_mode();
    }
}

void Controller::run_other_mode() {
    _cur_mode = DriveMode::other;

}

void Controller::run_stop_mode() {
    if (_cur_mode != DriveMode::stop) {
        ++_storage.cntr.stop_count;
        if (!_temp_sensors.get_output_temp()) {
            ++_storage.cntr.therm_failure_count;
        } else if (is_overheat()) {
            ++_storage.cntr.overheat_count;
        }
        if (_sensors.feeder_overheat) {
            ++_storage.cntr.feeder_overheat_count;
        }
        _storage.save();
    }
    _cur_mode = DriveMode::stop;
    _auto_mode = AutoMode::notset;
}




bool Controller::is_wifi() const {
    return _network.is_connected();
}
bool Controller::is_wifi_ap() const {
    return _network.is_ap_mode();
}
IPAddress Controller::get_local_ip() const {
    return _network.get_local_ip();
}

void Controller::history_out(const HistoryRequest &req, Print &out) {
    for (uint16_t i = req.day_from; i < req.day_to; ++i) {
        auto snp = _storage.read_day_stats(i);
        char day[20];
        day_to_date(i, day, sizeof(day));
        print(out, day, ",", snp.feeder, ",",
               snp.feeder_low, ",", snp.fuel_kg, ",",
               snp.overheat_count,",", snp.therm_fail_count,",",
               snp.restart_count,",", snp.manual_count,"\n");
    }

}


#ifdef EMULATOR



void Controller::send_file(MyHttpServer::Request &req, std::string_view content_type, std::string_view file_name) {
    std::string path = www_path;
    path.append(file_name);
    std::ifstream f(path);
    if (!f) {
            _network.get_server().error_response(req, 404, {}, {}, path);
    } else {
        _network.get_server().send_simple_header(req, content_type, -1);
        int i = f.get();
        while (i != EOF) {
            req.client->print(static_cast<char>(i));
            i = f.get();
        }
    }
    req.client->stop();
}
#endif

struct SimulInfo {
    float input = 0;
    float output = 0;
};

constexpr std::pair<const char *, float SimulInfo::*> simulate_temp_table[] = {
        {"input", &SimulInfo::input},
        {"output", &SimulInfo::output},
};



void Controller::handle_server(MyHttpServer::Request &req) {

    using Ctx = MyHttpServer::ContentType;

    auto &_server = _network.get_server();

    set_wifi_used();
    _last_net_activity = get_current_timestamp();
    if (req.request_line.method == HttpMethod::WS) {
        handle_ws_request(req);
        return;
    } else if (req.request_line.path == "/api/scan_temp" && req.request_line.method == HttpMethod::POST) {
        if (_list_temp_async.has_value()) {
            _server.error_response(req, 503, "Service unavailable" , {}, {});
        } else {
            _server.send_simple_header(req, Ctx::text);
            _list_temp_async.emplace(std::move(*req.client));
            return;
        }
    } else if (req.request_line.path == "/api/scan_wifi") {
        if (req.request_line.method == HttpMethod::POST) {
            _server.error_response(req, 202, {});
            _ap_scan = true;
        } else if (req.request_line.method == HttpMethod::GET) {
            static_buff.clear();
            for (const auto &item: _aplist) {
                print(static_buff, item.rssi,",",
                        item.encryption_mode,",",
                        item.ssid,"\n");
            }
            _server.send_file(req,Ctx::text,static_buff.get_text(), false);
        } else {
            _server.error_response(req,400,{});
        }
    } else if (req.request_line.path == "/api/code" && req.request_line.method == HttpMethod::POST) {
        if (req.body.empty()) {
            generate_otp_code();
            _display.display_code( _last_code);
            _server.error_response(req, 202, {});
        } else if (std::string_view(_last_code.data(), _last_code.size()) == req.body) {
            static_buff.clear();
            gen_and_print_token();
            _server.send_file_async(req, Ctx::text, static_buff.get_text(), false);
            std::fill(_last_code.begin(), _last_code.end(),0);
        } else {
            _server.error_response(req, 409, "Conflict", {}, "code doesn't match");
        }

    } else if (req.request_line.path.substr(0,7) == "/api/ws" && req.request_line.method == HttpMethod::GET) {
        uint16_t code = 0;
        auto q = req.request_line.path.substr(7);
        if (q.substr(0,7) != "?token=") {
            code = 4020;
        } else {
            q = q.substr(7);
            auto tkn = generate_signed_token(q);
            if (std::string_view(tkn.data(), tkn.size()) != q) {
                code = 4090;
            }
        }
        auto iter = std::find_if(req.headers, req.headers+req.headers_count, [&](const auto &hdr){
            return icmp(hdr.first,"Sec-WebSocket-Key");
        });
        if (iter != req.headers+req.headers_count) {
            auto accp = ws::calculate_ws_accept(iter->second);
            _server.send_header(req,{
                {"Upgrade","websocket"},
                {"Connection","upgrade"},
                {"Sec-WebSocket-Accept",accp}
            },101,"Switching protocols");
            if (code) {
                _server.send_ws_message(req, ws::Message{"",ws::Type::connClose, code});
            } else {
                 return;
            }
        } else {
            _server.error_response(req,400,{});
        }
    } else if (req.request_line.path == "/api/history" && req.request_line.method == HttpMethod::GET) {
        uint16_t last_day = _storage.snapshot.day_number+1;
        uint16_t first_day = last_day - graph_files * eeprom_sector_size;
        _server.send_simple_header(req, "text/csv", -1,false,"");
        print(*req.client, "Datum,Podavac,Podavac uspoarne, Plneni, Prehrati, Chyba teplomeru, Restart, Rucni rezim\n");
        history_out({first_day, last_day}, *req.client);
    } else if (req.request_line.path == "/") {
        _server.send_file_async(req, HttpServerBase::ContentType::html, embedded_index_html, true, embedded_index_html_etag);
#ifdef EMULATOR
    } else {
        std::string_view ext = req.request_line.path.substr(req.request_line.path.find('.')+1);
        std::string_view ctx;
        if (ext == "html") ctx = Ctx::html;
        else if (ext == "js") ctx = Ctx::javascript;
        else if (ext == "css") ctx = Ctx::css;
        else ctx = Ctx::text;
        send_file(req, ctx, req.request_line.path);
    }
#else
    } else {
        _server.error_response(req, 404, "Not found");
    }
#endif
    req.client->stop();
}




bool Controller::manual_control(const ManualControlStruct &cntr) {
    if (_cur_mode != DriveMode::manual || _sensors.tray_open) return false;
    if (cntr._control_id < _man_mode_control_id ) return false;
    _man_mode_control_id = cntr._control_id;
    if (cntr._fan_speed != 0xFF) {
        _fan.set_speed(cntr._fan_speed);
    }
    if (cntr._fan_time != 0xFF) {
        if (cntr._fan_time != 0) {
           _fan.keep_running(get_current_timestamp()+from_seconds(cntr._fan_time));
        } else {
            _fan.stop();
        }
    }

    if (cntr._feeder_time != 0xFF) {
        if (cntr._feeder_time != 0) {
            _feeder.keep_running( get_current_timestamp()+from_seconds(cntr._feeder_time));
        } else {
            _feeder.stop();
        }
    }
    if (cntr._force_pump != 0xFF) {
        _force_pump = cntr._force_pump != 0;
        control_pump();
    }
    return true;
}


TimeStampMs Controller::update_motorhours(TimeStampMs now) {
    bool fd = is_feeder_on();
    bool fa = is_fan_on();
    Storage &stor = get_storage();
    if (fd) ++stor.runtm.feeder;
    if (fa) ++stor.runtm.fan;
    if (_cur_mode == DriveMode::automatic) {
        switch (_auto_mode) {
            case AutoMode::fullpower: ++stor.runtm.full_power;break;
            case AutoMode::lowpower: ++stor.runtm.low_power;
                                if (fd) ++stor.runtm.feeder_low;
                                break;
            case AutoMode::off:     ++stor.runtm.cooling;break;
            default:break;
        }
    } else if (_cur_mode == DriveMode::stop) {
        ++stor.runtm.stop_time;
    }

    if (now >= _flush_time) {
        stor.save();
        _flush_time = now+60000;
    }
    return 1000;
}



void Controller::factory_reset() {
    _storage.get_eeprom().erase_all();
    while (true) {
        delay(1);
    }
}

TimeStampMs Controller::auto_drive_cycle(TimeStampMs cur_time) {
    //determine next control mode
    auto t_input = _temp_sensors.get_input_ampl();
    if (_cur_mode != DriveMode::automatic) {
        return from_minutes(60);
    }
    if (_sensors.tray_open) {
        return 1000;
    }

    AutoMode prev_mode = _auto_mode;


    if (t_input < _storage.config.input_temp_full_power) {
        _auto_mode = AutoMode::fullpower;
    } else if (t_input < _storage.config.input_temp_low_power) {
        _auto_mode = AutoMode::lowpower;
    } else {
        _auto_mode = AutoMode::off;
    }

    const Profile *p;

    switch (_auto_mode) {
        default:
        case AutoMode::off:
            _fan.stop();
            _feeder.stop();
            return TempSensors::measure_interval;
        case AutoMode::fullpower:
            p = &_storage.config.full_power;
            break;
        case AutoMode::lowpower:
            p = &_storage.config.low_power;
            break;
    }

    auto cycle_interval = from_seconds(p->burnout_sec)+from_seconds(p->fueling_sec);
    bool fan_active = _fan.is_active();
    if (prev_mode == _auto_mode || !fan_active)
        _fan.set_speed(p->fanpw);
    _fan.keep_running(cur_time + cycle_interval+1000);
     auto t = from_seconds(p->fueling_sec);




    if (fan_active) {
        _feeder.keep_running( cur_time+t);
        return cycle_interval;
    } else {
        return cycle_interval/2;
    }
}



void Controller::handle_ws_request(MyHttpServer::Request &req)
{
    static_buff.clear();

    auto &_server = _network.get_server();

    auto msg = req.body;
    if (msg.empty()) return;
    WsReqCmd cmd = static_cast<WsReqCmd>(msg[0]);
    msg = msg.substr(1);
    static_buff.write(static_cast<char>(cmd));
    switch (cmd)
    {
    case WsReqCmd::set_fuel: {
        int32_t amount;
        if (parse_to_field_int(amount, msg)) {
            _storage.add_fuel(amount);
        } else {
            static_buff.print("Parse error: number");
        }
    }
    break;
    case WsReqCmd::get_config: {
        config_out(static_buff);
    }
    break;
    case WsReqCmd::set_config: {
        std::string_view failed_field;
        if (!config_update(msg, std::move(failed_field))) {
            print(static_buff, failed_field);
        }
    }
    break;
    case WsReqCmd::generate_code:
        generate_otp_code();
        static_buff.write(_last_code.data(), _last_code.size());
        break;
    case WsReqCmd::unpair_all:
        generate_pair_secret();
        gen_and_print_token();
        break;
    case WsReqCmd::monitor_cycle: {
        auto m = ManualControlStruct::from(msg);
        manual_control(m);
        status_out(static_buff);
        break;
    }
    case WsReqCmd::history: {
        auto hr = HistoryRequest::from(msg);
        history_out(hr, static_buff);
        break;
    }
    case WsReqCmd::get_stats:
        stats_out(static_buff);
        break;
    case WsReqCmd::reset_fuel:
         _storage.reset_fuel_stats();
         break;
    case WsReqCmd::reset:
        _server.send_ws_message(req, ws::Message{static_buff.get_text(), ws::Type::text});
        delay(10000);   //delay more than 5 sec invokes WDT
        return;
    default:
        break;
    }
    _server.send_ws_message(req, ws::Message{static_buff.get_text(), ws::Type::text});
}




TimeStampMs Controller::read_serial(TimeStampMs) {
    while (handle_serial(*this));
    return 250;
}



bool Controller::enable_temperature_simulation(std::string_view b) {
    SimulInfo sinfo;
    if (!update_settings_fd(sinfo, b, simulate_temp_table)) {
       return false;
    }
    if (sinfo.input>0 && sinfo.output > 0) {
        _temp_sensors.simulate_temperature(sinfo.input,sinfo.output);
        return true;
    } else {
        return false;
    }

}

void Controller::disable_temperature_simulation() {
    _temp_sensors.disable_simulated_temperature();
}

bool Controller::is_overheat() const {
    return defined_and_above(_temp_sensors.get_output_temp(),_storage.config.output_max_temp)
         || defined_and_above(_temp_sensors.get_input_temp(),_storage.config.output_max_temp);
}

void Controller::generate_otp_code() {
    constexpr char characters[] = "ABCDEFHIJKLOPQRSTUXYZ";
    constexpr auto count_chars = sizeof(characters)-1;
    SATSE.begin();
    SATSE.random(reinterpret_cast<unsigned char *>(_last_code.data()), _last_code.size());
    SATSE.end();
    for (char &c: _last_code) c = characters[static_cast<unsigned char>(c) % count_chars];

}

std::array<char, 20> Controller::generate_token_random_code() {
    unsigned char c[15];
    SATSE.begin();
    SATSE.random(c, 15);
    SATSE.end();
    std::array<char, 20> out;
    base64url.encode(std::begin(c), std::end(c), out.begin());
    return out;
}

std::array<char, 40> Controller::generate_signed_token(std::string_view random) {
    std::array<char, 40> out;
    std::array<char, 20> tmp;
    std::fill(tmp.begin(), tmp.end(), 'A');
    std::fill(out.begin(), out.end(), 'A');
    random = random.substr(0, 20);
    std::copy(random.begin(), random.end(), tmp.begin());
    auto iter = tmp.begin();
    for (unsigned int c: _storage.pair_secret.password.text) {*iter = *iter ^ c; ++iter;}
    auto digest = SHA1(std::string_view(tmp.data(), 20)).final();
    std::array<char, 30> digest_base64;
    std::copy(random.begin(), random.end(), out.begin());
    base64url.encode(digest.begin(), digest.end(), digest_base64.begin());
    std::string_view digestw (digest_base64.data(), digest_base64.size());
    digestw = digestw.substr(0,20);
    std::copy(digestw.begin(), digestw.end(), out.begin()+20);
    return out;
}

void Controller::generate_pair_secret() {
    SATSE.begin();
    SATSE.random(reinterpret_cast<unsigned char *>(_storage.pair_secret.password.text), sizeof(_storage.pair_secret.password.text));
    SATSE.end();
    _storage.save();
}

void Controller::gen_and_print_token() {
    auto rnd = generate_token_random_code();
    auto tkn = generate_signed_token({rnd.data(), rnd.size()});
    print(static_buff, "token=", std::string_view(tkn.data(), tkn.size()),"\n");
}

TimeStampMs Controller::refresh_wdt(TimeStampMs) {
#if ENABLE_VDT
    WDT.refresh();
#endif
    return 100;
}

void Controller::run_init_mode() {
    if (get_current_timestamp() > 6000) {
        _cur_mode = DriveMode::unknown;
    }
}

bool Controller::is_safe_for_blocking() const {
    return _fan.get_current_speed() > 90 || !_fan.is_pulse();
}

void Controller::init_serial_log() {
    if (_storage.config.serial_log_out) {
#ifdef _MODEM_WIFIS3_H_
        delay(5000);    //wait for connect serial line
        modem.debug(Serial, 100);
#endif
        Serial.println("Debug mode on");
    }
}

TimeStampMs Controller::run_keyboard(TimeStampMs cur_time) {
    if (!_keyboard_connected) return 1000;
    kbdcntr.read(_kbdstate);
    if (_sensors.tray_open) {
        auto &up = _kbdstate.get_state(key_code_up);
        auto &down = _kbdstate.get_state(key_code_down);
        auto &full = _kbdstate.get_state(key_code_full);
        bool at_zero = !_cur_tray_change._full && _cur_tray_change._change == 0;
        int maxbg = 255/_storage.config.bag_kg;
        if (up.stabilize(default_btn_press_interval_ms) && up.pressed()) {
            if (_cur_tray_change._full) {
                _cur_tray_change._full = false;
            } else {
                _cur_tray_change._change = std::min<int>(maxbg, _cur_tray_change._change+1);
            }
        } else if (down.stabilize(at_zero?negative_tray_btn_press_interval_ms:default_btn_press_interval_ms) && down.pressed()) {
            if (_cur_tray_change._full) {
                _cur_tray_change._full = false;
            } else {
                _cur_tray_change._change = std::max<int>(-maxbg, _cur_tray_change._change-1);
            }
        } else if (full.stabilize(default_btn_press_interval_ms) && full.pressed()) {
            _cur_tray_change._full = !_cur_tray_change._full;
        }
    } else {
        auto &stop_btn = _kbdstate.get_state(key_code_stop);
        if (stop_btn.pressed()) {
            if (stop_btn.stabilize(stop_btn_start_interval_ms)) {
                _storage.config.operation_mode = 1;
                stop_btn.set_user_state();
                _storage.save();
                ++_man_mode_control_id;
            }
        } else {
            if (stop_btn.stabilize(default_btn_release_interval_ms)) {
                if (!stop_btn.test_and_reset_user_state()) {
                    _storage.config.operation_mode = 0;
                    _feeder.stop();
                    _fan.stop();
                    _storage.save();
                    ++_man_mode_control_id;
                }
            }
        }

        if (_cur_mode == DriveMode::manual) {
            auto &feeder_btn = _kbdstate.get_state(key_code_feeder);
            if (feeder_btn.pressed()) {
                if (feeder_btn.stable()) {
                    auto sch = cur_time+150;
                    if (_feeder.get_scheduled_time() < sch) {
                        _feeder.keep_running(sch);
                        ++_man_mode_control_id;
                    }
                }
                if (feeder_btn.stabilize(feeder_min_press_interval_ms)) {
                    _feeder.keep_running(cur_time+1000);
                    feeder_btn.set_user_state();
                    ++_man_mode_control_id;
                }
            }
            auto &fan_btn = _kbdstate.get_state(key_code_fan);
            if (fan_btn.stabilize(default_btn_release_interval_ms) && fan_btn.pressed()) {
                if (_fan.is_active()) {
                    ++_man_mode_control_id;
                    int spd = _fan.get_speed();
                    if (spd >= 100) _fan_step_down = true;
                    if (spd <= fan_speed_change_step) _fan_step_down = false;
                    if (_fan_step_down) {
                        spd -= fan_speed_change_step;
                        if (spd < fan_speed_change_step) spd = fan_speed_change_step;
                    } else {
                        spd += fan_speed_change_step;
                        if (spd > 100) spd = 100;
                    }
                    _fan.set_speed(spd);
                }
                _fan.keep_running(cur_time+manual_run_interval_ms);
                ++_man_mode_control_id;
            }

        }

    }
    return 2;
}

TimeStampMs Controller::daily_log(TimeStampMs ) {
    if (!is_time_synced()) return 1000;
#ifdef EMULATOR
    prepare_history_mockup();
#endif
    //-1 - because day is recorded at beginning of new day
    auto dnum = get_current_time()/day_length_seconds-1;
    _storage.record_day_stats(static_cast<uint16_t>(dnum));
    return std::min(60000,day_length_seconds *1000);
}

#ifdef EMULATOR
void Controller::prepare_history_mockup() {
    if (_storage.snapshot.day_number == 0) {
        auto dnum = get_current_time()/day_length_seconds-1;
        std::mt19937 rng(2);
        int feeder = 70;
        int feeder_low = 10;
        int fill = 0;
        for (int i = 0; i < 360; ++i) {
            _storage.manage_day_stats(i-360+dnum+1, [&](int pos, Storage::AllDailyData &dd){
                dd.feeder._days[pos] = feeder;
                dd.feeder_low._days[pos] = std::min(feeder_low, feeder);
                std::uniform_int_distribution<int> dist(-10, 10);
                std::uniform_int_distribution<int> fdist(150, 225);
                std::uniform_int_distribution<int> f0dist(5, 10);
                std::uniform_int_distribution<int> errd(0,40);
                std::uniform_int_distribution<int> errv(0, 255);
                feeder = std::clamp(feeder + dist(rng)/3, 5, 60);
                feeder_low = std::clamp(feeder_low + dist(rng)/3, 5, 60);
                if (f0dist(rng) < fill) {
                    dd.fuel._days[pos] = fdist(rng);
                    fill=0;
                } else {
                    dd.fuel._days[pos] = 0;
                    ++fill;
                }
                if (errd(rng) == 0) {
                    dd.errors._days[pos] = errv(rng);
                }
                if (errd(rng) == 0) {
                    dd.controls._days[pos] = errv(rng);
                }
                return true;
            });
        }
        _storage.snapshot.day_number = dnum;
    }
}
#endif


}

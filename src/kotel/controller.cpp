
#include "controller.h"
#include "http_utils.h"
#include "web_page.h"
#include "stringstream.h"
#include "serial.h"
#include <SoftwareATSE.h>

#include "sha1.h"
#ifdef EMULATOR
#include <fstream>
extern std::string www_path;
#endif

#define ENABLE_VDT 1

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
        ,_network(*this)
        ,_scheduler({&_feeder, &_fan, &_temp_sensors,  &_display,
            &_motoruntime, &_auto_drive_cycle, &_network,
            &_read_serial, &_refresh_wdt, &_keyboard_scanner})
{

}

void Controller::begin() {
    _storage.begin();
    init_serial_log();
    _display.begin();
    _display.display_init_pattern();
    _display.display_version();
    _fan.begin();
    _feeder.begin();
    _pump.begin();
    _temp_sensors.begin();
    _network.begin();
    kbdcntr.begin();
    _keyboard_connected = true;
    for (int i = 0; i < 100 && _keyboard_connected; ++i) {
        kbdcntr.read(_kbdstate);
        if (_kbdstate.last_level != 0) _keyboard_connected = false;
        delay(1);
    }
    print(Serial, "Keyboard: ", _keyboard_connected?"connected":"not detected","\r\n");
    _storage.cntr1.restart_count++;
    if (_storage.pair_secret_need_init) {
        generate_pair_secret();
    }
#if ENABLE_VDT
    Serial.print("Init WDT: ");
    Serial.println(WDT.begin(5589));
#endif

}

static inline bool defined_and_above(const std::optional<float> &val, float cmp) {
    return val.has_value() && *val > cmp;
}

void Controller::run() {
    _sensors.read_sensors();
    auto prev_mode = _cur_mode;
    bool start_mode = _start_mode_until > get_current_timestamp();
    control_pump();
    if (_sensors.tray_open) {
        _storage.tray.tray_open_time = _storage.tray.feeder_time;
        _was_tray_open = true;
        _feeder.stop();
        _fan.stop();
    } else {
        if (_was_tray_open) {
            _was_tray_open = false;
            _storage.cntr1.tray_open_count++;
            if (_cur_tray_change._full) {
                _storage.tray.set_max_fill(_storage.tray.feeder_time, _storage.config.tray_kg);
            } else if (_cur_tray_change._change) {
                _storage.tray.update_tray_fill(_storage.tray.feeder_time, _cur_tray_change._change*_storage.config.bag_kg);
            }
            _cur_tray_change = {};
            _storage.save();
        }
        if (_sensors.feeder_overheat || is_overheat()) {
            run_stop_mode();
        } else if (_storage.config.operation_mode == 0) {
            run_manual_mode();
        } else if (_storage.config.operation_mode == 1) {
            if (_cur_mode == DriveMode::init) {
                run_init_mode();
            } else if ( (!_temp_sensors.get_output_temp().has_value()) //we don't have output temperature
                || (_temp_sensors.get_output_temp().has_value() //output temperature is less than input temperature
                        && _temp_sensors.get_input_temp().has_value()
                        && *_temp_sensors.get_input_temp() < _storage.config.pump_start_temp
                        && !start_mode)
                || (_temp_sensors.get_output_temp().has_value() //output temp is too low
                        && *_temp_sensors.get_output_temp() < _storage.config.pump_start_temp
                        && !start_mode)){
                run_stop_mode();
            } else {
                run_auto_mode();
            }
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
    _scheduler.run();
    _storage.commit();

}

static constexpr std::pair<const char *, uint8_t Profile::*> profile_table[] ={
        {"burnout",&Profile::burnout_sec},
        {"fanpw",&Profile::fanpw},
        {"fueling",&Profile::fueling_sec},
};

static constexpr std::pair<const char *, uint8_t Config::*> config_table[] ={
        {"tout", &Config::output_max_temp},
        {"tin", &Config::input_min_temp},
        {"touts", &Config::output_max_temp_samples},
        {"tins", &Config::input_min_temp_samples},
        {"m", &Config::operation_mode},
        {"fanpc", &Config::fan_pulse_count},
        {"tpump",&Config::pump_start_temp},
        {"srlog",&Config::serial_log_out},
        {"bgkg",&Config::bag_kg},
        {"traykg",&Config::tray_kg},
        {"dspli",&Config::display_intensity},
};

static constexpr std::pair<const char *, HeatValue Config::*> config_table_2[] ={
        {"hval",&Config::heat_value},
};

template<typename X>
void print_data(Stream &s, const X &data) {
    s.print(data);
}


template<>
void print_data(Stream &s, const HeatValue &data) {
    s.print(static_cast<float>(data.heatvalue)*0.1f);
}


template<>
void print_data(Stream &s, const SimpleDallasTemp::Address &data) {
    if (data[0]<16) s.print('0');
    s.print(data[0], HEX);
    for (unsigned int i = 1; i < data.size();++i) {
        s.print('-');
        if (data[i]<16) s.print('0');
        s.print(data[i],HEX);
    }
}

template<>
void print_data(Stream &s, const IPAddr &data) {
    s.print(data.ip[0]);
    for (unsigned int i = 1; i < 4; ++i) {
        s.print('.');
        s.print(data.ip[i]);
    }
}

template<>
void print_data(Stream &s, const TextSector &data) {
    auto txt = data.get();
    s.write(txt.data(), txt.size());
}

template<>
void print_data(Stream &s, const PasswordSector &data) {
    auto txt = data.get();
    for (std::size_t i = 0; i < txt.size(); ++i) s.print('*');
}

template<>
void print_data(Stream &s, const std::optional<float> &data) {
    if (data.has_value()) s.print(*data);
}


template<typename T>
void print_data_line(Stream &s, const char *name, const T &object) {
    s.print(name);
    s.print('=');
    print_data(s,object);
    s.println();

}

template<typename Table, typename Object>
void print_table(Stream &s, const Table &table, const Object &object, std::string_view prefix = {}) {
    for (const auto &[k,ptr]: table) {
        if (!prefix.empty()) s.write(prefix.data(), prefix.size());
        print_data_line(s, k, object.*ptr);
    }

}
static constexpr std::pair<const char *, SimpleDallasTemp::Address TempSensor::*> tempsensor_table_1[] ={
        {"tsinaddr", &TempSensor::input_temp},
        {"tsoutaddr", &TempSensor::output_temp},
};



static constexpr std::pair<const char *, uint32_t Tray::*> tray_table[] ={
        {"feed.t", &Tray::feeder_time},
        {"tray.ot", &Tray::tray_open_time},
        {"tray.ft", &Tray::tray_fill_time}
};

static constexpr std::pair<const char *, uint16_t Tray::*> tray_table_2[] ={
        {"tray.tfkg", &Tray::tray_fill_kg},
        {"tray.f1kgt", &Tray::feeder_1kg_time}
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

static constexpr std::pair<const char *, Tray Storage::*> tray_control_table[] ={
        {"tray.tfkg", &Storage::tray},
};
static constexpr std::pair<const char *, uint16_t Tray::*> tray_control_table_2[] ={
        {"tray.f1kgt", &Tray::feeder_1kg_time}
};


template <typename T>
struct member_pointer_type;

template <typename X, typename Y>
struct member_pointer_type<X Y::*> {
    using type = X;
};

template <typename T>
using member_pointer_type_t = typename member_pointer_type<T>::type;


template<typename Table>
void print_table_format(Stream &s, Table &t) {
    using T = member_pointer_type_t<decltype(t[0].second)>;
    bool comma = false;
    for (const auto &[k,ptr]: t) {
        if (comma) s.print(','); else comma = true;
        s.print('"');
        s.print(k);
        s.print("\":\"");
        if constexpr(std::is_same_v<T, uint8_t>) {
            s.print("uint8");
        } else if constexpr(std::is_same_v<T, uint16_t>) {
            s.print("uint16");
        } else if constexpr(std::is_same_v<T, uint32_t>) {
            s.print("uint32");
        } else if constexpr(std::is_same_v<T, TextSector>) {
            s.print("text");
        } else if constexpr(std::is_same_v<T, PasswordSector>) {
            s.print("password");
        } else if constexpr(std::is_same_v<T, IPAddr>) {
            s.print("ipv4");
        } else if constexpr(std::is_same_v<T, SimpleDallasTemp::Address>) {
            s.print("onewire_address");
        } else {
            static_assert(std::is_same_v<T, bool>, "Undefined type");
        }
        s.print('"');
    }

}

void Controller::config_out(Stream &s) {

    print_table(s, config_table, _storage.config);
    print_table(s, config_table_2, _storage.config);
    print_table(s, profile_table, _storage.config.full_power, "full.");
    print_table(s, profile_table, _storage.config.low_power, "low.");
    print_table(s, tempsensor_table_1, _storage.temp);
    print_table(s, wifi_ssid_table, _storage.wifi_ssid);
    print_table(s, wifi_password_table, _storage.wifi_password);
    print_table(s, wifi_netcfg_table, _storage.wifi_config);
    print_table(s, tray_table_2, _storage.tray);
}


template<typename UINT>
bool parse_to_field_uint(UINT &fld, std::string_view value) {
    uint32_t v = 0;
    constexpr uint32_t max = ~static_cast<UINT>(0);
    for (char c: value) {
        if (c < '0' || c > '9') return false;
        v = v * 10 + (c - '0');
        if (v > max) return false;
    }
    fld  = static_cast<UINT>(v);
    return true;
}

bool parse_to_field(int8_t &fld, std::string_view value) {
    uint8_t x;
    if (!value.empty() && value.front() == '-') {
        if (!parse_to_field_uint(x, value.substr(1))) return false;
        fld = -static_cast<int8_t>(x);
        return true;
    } else {
        if (!parse_to_field_uint(x,value)) return false;
        fld = static_cast<int8_t>(x);
        return true;
    }

}

bool parse_to_field(uint8_t &fld, std::string_view value) {
    return parse_to_field_uint(fld,value);
}
bool parse_to_field(uint16_t &fld, std::string_view value) {
    return parse_to_field_uint(fld,value);
}
bool parse_to_field(uint32_t &fld, std::string_view value) {
    return parse_to_field_uint(fld,value);
}

bool parse_to_field(float &fld, std::string_view value) {
    char buff[21];
    char *c;
    value = value.substr(0,20);
    *std::copy(value.begin(), value.end(), buff) = 0;
    fld = std::strtof(buff,&c);
    return c > buff;
}

bool parse_to_field(SimpleDallasTemp::Address &fld, std::string_view value) {
    for (auto &x: fld) {
        auto part = split(value,"-");
        if (part.empty()) return false;
        int v = 0;
        for (char c: part) {
            v = v * 16;
            if (c >= '0' && c <= '9') v += (c-'0');
            else if (c >= 'A' && c <= 'F') v += (c-'A'+10);
            else if (c >= 'a' && c <= 'f') v += (c-'a'+10);
            else return false;
            if (v > 0xFF) return false;
        }
        x = static_cast<uint8_t>(v);
    }
    return (value.empty());
}
bool parse_to_field(IPAddr &fld, std::string_view value) {
    for (auto &x:fld.ip) {
        auto part = split(value,".");
        if (part.empty()) return false;
        if (!parse_to_field(x, part)) return false;
    }
    return true;
}

bool parse_to_field(TextSector &fld, std::string_view value) {
    fld.set_url_dec(value);
    return true;
}

bool parse_to_field(Tray &fld, std::string_view value) {
    if (!parse_to_field(fld.tray_fill_kg, value)) return false;
    fld.tray_fill_time = fld.tray_open_time;
    return true;
}

bool parse_to_field(HeatValue &fld, std::string_view value) {
    float f;
    if (!parse_to_field(f, value)) return false;
    fld.heatvalue = static_cast<uint8_t>(f * 10.0f);
    return true;
}


template<typename Table, typename Config>
bool update_settings(const Table &table, Config &config, std::string_view key, std::string_view value, std::string_view prefix = {}) {
    if (!prefix.empty()) {
        if (key.substr(0,prefix.size()) == prefix) {
            key = key.substr(prefix.size());
        } else {
            return false;
        }
    }
    for (const auto &[k,ptr]: table) {
        if (k == key) {
            return parse_to_field(config.*ptr, value);
        }
    }
    return false;
}

template<typename Table, typename Config>
bool update_settings_kv(const Table &table, Config &config, std::string_view keyvalue) {
    auto key = split(keyvalue,"=");
    auto value = keyvalue;
    return update_settings(table, config, trim(key), trim(value));
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
                || update_settings(wifi_ssid_table, _storage.wifi_ssid, key, value)
                || update_settings(wifi_password_table, _storage.wifi_password, key, value)
                || update_settings(wifi_netcfg_table, _storage.wifi_config, key, value)
                || update_settings(pair_sectet_table, _storage.pair_secret, key, value)
                || update_settings(tray_control_table, _storage, key, value)
                || update_settings(tray_control_table_2, _storage.tray, key, value);
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
    print_data_line(s,"mode", static_cast<int>(_cur_mode));
    print_data_line(s,"auto_mode", static_cast<int>(_auto_mode));
    print_data_line(s,"temp.output.value", _temp_sensors.get_output_temp());
    print_data_line(s,"temp.output.status", static_cast<int>(_temp_sensors.get_output_status()));
    print_data_line(s,"temp.output.ampl", _temp_sensors.get_output_ampl());
    print_data_line(s,"temp.input.value", _temp_sensors.get_input_temp());
    print_data_line(s,"temp.input.status", static_cast<int>(_temp_sensors.get_input_status()));
    print_data_line(s,"temp.input.ampl", _temp_sensors.get_input_ampl());
    print_data_line(s,"temp.sim", _temp_sensors.is_simulated()?1:0);
    print_data_line(s,"tray_open", _sensors.tray_open);
    print_data_line(s,"motor_temp_ok", !_sensors.feeder_overheat);
    print_data_line(s,"pump", _pump.is_active());
    print_data_line(s,"feeder", _feeder.is_active());
    print_data_line(s,"fan", _fan.get_current_speed());
    print_data_line(s,"network.ip", WiFi.localIP());
    print_data_line(s,"network.ssid", WiFi.SSID());
    print_data_line(s,"network.signal",WiFi.RSSI());


}

void Controller::control_pump() {
    if (_storage.config.operation_mode == 0 && _force_pump) {
        _pump.set_active(true);
        return;
    }
    auto t = _temp_sensors.get_output_ampl();
    _pump.set_active(t > _storage.config.pump_start_temp);
}

void Controller::run_manual_mode() {
    _cur_mode = DriveMode::manual;
    _auto_mode = AutoMode::notset;
    _start_mode_until = get_current_timestamp() + from_minutes(60);

}

void Controller::run_auto_mode() {

    if (_cur_mode != DriveMode::automatic)  {
        _cur_mode = DriveMode::automatic;
        _auto_drive_cycle.wake_up();
    }
}

void Controller::run_other_mode() {
    _cur_mode = DriveMode::other;

}

void Controller::run_stop_mode() {
    if (_cur_mode != DriveMode::stop) {
        ++_storage.cntr2.stop_count;
        if (!_temp_sensors.get_output_temp()) {
            ++_storage.cntr2.temp_read_failure_count;
        } else if (is_overheat()) {
            ++_storage.cntr1.overheat_count;
        }
        if (_sensors.feeder_overheat) {
            ++_storage.cntr1.feeder_overheat_count;
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

constexpr std::pair<const char *, uint8_t Controller::ManualControlStruct::*> manual_control_table[] ={
        {"feed.tr",&Controller::ManualControlStruct::_feeder_time},
        {"fan.timer",&Controller::ManualControlStruct::_fan_time},
        {"fan.speed",&Controller::ManualControlStruct::_fan_speed},
        {"pump.force",&Controller::ManualControlStruct::_force_pump},
};


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
    }
    return true;
}

bool Controller::manual_control(std::string_view body, std::string_view &&error_field) {
    ManualControlStruct cntr = {};
    do {
        auto ln = split(body,"&");
        if (!update_settings_kv(manual_control_table, cntr, ln)) {
            error_field = ln;
            return false;
        }
    } while (!body.empty());
    if (!manual_control(cntr)) {
        error_field = "invalid mode";
        return false;
    }
    return true;

}

TimeStampMs Controller::update_motorhours(TimeStampMs now) {
    bool fd = is_feeder_on();
    bool pm = is_pump_on();
    bool fa = is_fan_on();
    Storage &stor = get_storage();
    if (fd) ++stor.tray.feeder_time;
    if (pm) ++stor.runtm.pump_time;
    if (fa) ++stor.runtm.fan_time;
    if (_cur_mode == DriveMode::automatic) {
        switch (_auto_mode) {
            case AutoMode::fullpower: ++stor.runtm.full_power_time;break;
            case AutoMode::lowpower: ++stor.runtm.low_power_time;break;
            case AutoMode::off:     ++stor.runtm.cooling_time;break;
            default:break;
        }
    } else if (_cur_mode == DriveMode::stop) {
        ++stor.runtm2.stop_time;
    }
    if (is_overheat()) ++stor.runtm2.overheat_time;
    ++stor.runtm2.active_time;

    if (now >= _flush_time) {
        stor.save();
        _flush_time = now+60000;
    }
    return 1000;
}


bool Controller::set_fuel(const SetFuelParams &sfp) {

    auto filltime  = sfp.absnow?_storage.tray.feeder_time:_storage.tray.tray_fill_time;
    if (sfp.kalib) {
        if (_storage.tray.tray_fill_kg == 0) {
            return false;
        }
        auto total_time = filltime - _storage.tray.tray_fill_time;
        auto consump = total_time / _storage.tray.tray_fill_kg;
        if (consump > 0xFFFF || consump == 0) {
            return false;
        }
        _storage.tray.feeder_1kg_time = consump;
        _storage.tray.tray_fill_kg = sfp.kgchg;
    } else {
        if (sfp.full) {
            _storage.tray.set_max_fill(filltime, _storage.config.tray_kg);
        } else {
            _storage.tray.update_tray_fill(filltime, sfp.kgchg);
        }

    }

    _storage.tray.tray_fill_time = filltime;
    _storage.save();
    return true;


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
    auto t_output = _temp_sensors.get_output_ampl();
    if (_cur_mode != DriveMode::automatic) {
        return from_minutes(60);
    }
    if (_sensors.tray_open) {
        return 1000;
    }

    AutoMode prev_mode = _auto_mode;

    if (t_input < _storage.config.input_min_temp) {
        _auto_mode = AutoMode::fullpower;
    } else if (t_input >= _storage.config.input_min_temp + 5) {
        _auto_mode = AutoMode::off;  
    } else {
        if (t_output< _storage.config.input_min_temp) {
            _auto_mode = AutoMode::fullpower;
        } else if (t_output< _storage.config.output_max_temp ) {
            _auto_mode = AutoMode::lowpower;
        } else {
            _auto_mode = AutoMode::off;
        }
    }

    if (_auto_mode != prev_mode) {
        switch (_auto_mode) {
            case AutoMode::off: _storage.cntr2.cool_count++;break;
            case AutoMode::fullpower: _storage.cntr2.full_power_count++;break;
            case AutoMode::lowpower: _storage.cntr2.low_power_count++;break;
            default: break;
        }
        _storage.save();
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
    case WsReqCmd::control_status: if (msg.size() == sizeof(ManualControlStruct)) {
        ManualControlStruct s;
        std::copy(msg.begin(), msg.end(), reinterpret_cast<char *>(&s));
        manual_control(s);
        status_out_ws(static_buff);
        _server.send_ws_message(req, ws::Message{static_buff.get_text(), ws::Type::binary});
    }
    break;
    case WsReqCmd::set_fuel: if (msg.size() == sizeof(SetFuelParams)) {
        SetFuelParams s;
        std::copy(msg.begin(), msg.end(), reinterpret_cast<char *>(&s));
        if (!set_fuel(s)) {
            static_buff.print('\x1');
        }
        _server.send_ws_message(req, ws::Message{static_buff.get_text(), ws::Type::binary});
    }
    break;
    case WsReqCmd::get_config: {
        config_out(static_buff);
        _server.send_ws_message(req, ws::Message{static_buff.get_text(), ws::Type::text});
    }
    break;
    case WsReqCmd::get_stats: {
        uint32_t cntr = _storage.get_eeprom().get_crc_error_counter();
        uint32_t timestamp = get_current_timestamp()/1000;
        uint32_t consumed_kg_total = _storage.tray.calc_total_consumed_fuel();
        static_buff.write(reinterpret_cast<const char *>(&_storage.runtm), sizeof(_storage.runtm));
        static_buff.write(reinterpret_cast<const char *>(&_storage.runtm2), sizeof(_storage.runtm2));
        static_buff.write(reinterpret_cast<const char *>(&_storage.cntr1), sizeof(_storage.cntr1));
        static_buff.write(reinterpret_cast<const char *>(&_storage.cntr2), sizeof(_storage.cntr2));
        static_buff.write(reinterpret_cast<const char *>(&_storage.tray), sizeof(_storage.tray));
        static_buff.write(reinterpret_cast<const char *>(&consumed_kg_total), sizeof(consumed_kg_total));
        static_buff.write(reinterpret_cast<const char *>(&cntr), sizeof(cntr));
        static_buff.write(reinterpret_cast<const char *>(&timestamp), sizeof(timestamp));
        _server.send_ws_message(req, ws::Message{static_buff.get_text(), ws::Type::binary});
    }
    break;
    case WsReqCmd::set_config: {
        std::string_view failed_field;
        if (!config_update(msg, std::move(failed_field))) {
            print(static_buff, failed_field);
        }
        _server.send_ws_message(req, ws::Message{static_buff.get_text(), ws::Type::text});
    }
    break;
    case WsReqCmd::file_config:
        static_buff.write(reinterpret_cast<const char *>(&_storage.config), sizeof(_storage.config));
        _server.send_ws_message(req, ws::Message{static_buff.get_text(), ws::Type::binary});
    break;
    case WsReqCmd::file_tray:
        static_buff.write(reinterpret_cast<const char *>(&_storage.tray), sizeof(_storage.tray));
        _server.send_ws_message(req, ws::Message{static_buff.get_text(), ws::Type::binary});
    break;
    case WsReqCmd::file_util1:
        static_buff.write(reinterpret_cast<const char *>(&_storage.runtm), sizeof(_storage.runtm));
        _server.send_ws_message(req, ws::Message{static_buff.get_text(), ws::Type::binary});
    break;
    case WsReqCmd::file_cntrs1:
        static_buff.write(reinterpret_cast<const char *>(&_storage.cntr1), sizeof(_storage.cntr1));
        _server.send_ws_message(req, ws::Message{static_buff.get_text(), ws::Type::binary});
    break;
    case WsReqCmd::file_util2:
        static_buff.write(reinterpret_cast<const char *>(&_storage.runtm2), sizeof(_storage.runtm2));
        _server.send_ws_message(req, ws::Message{static_buff.get_text(), ws::Type::binary});
    break;
    case WsReqCmd::file_cntrs2:
        static_buff.write(reinterpret_cast<const char *>(&_storage.cntr2), sizeof(_storage.cntr2));
        _server.send_ws_message(req, ws::Message{static_buff.get_text(), ws::Type::binary});
    break;
    case WsReqCmd::file_tempsensor:
        static_buff.write(reinterpret_cast<const char *>(&_storage.temp), sizeof(_storage.temp));
        _server.send_ws_message(req, ws::Message{static_buff.get_text(), ws::Type::binary});
    break;
    case WsReqCmd::file_wifi_ssid:
        static_buff.write(reinterpret_cast<const char *>(&_storage.wifi_ssid), sizeof(_storage.wifi_ssid));
        _server.send_ws_message(req, ws::Message{static_buff.get_text(), ws::Type::binary});
    break;
    case WsReqCmd::file_wifi_net:
        static_buff.write(reinterpret_cast<const char *>(&_storage.wifi_config), sizeof(_storage.wifi_config));
        _server.send_ws_message(req, ws::Message{static_buff.get_text(), ws::Type::binary});
    break;
    case WsReqCmd::file_wifi_pwd:
        static_buff.print(_storage.wifi_password.password.get().empty()?"":"****");
        _server.send_ws_message(req, ws::Message{static_buff.get_text(), ws::Type::binary});
    break;
    case WsReqCmd::enum_tasks:
        _scheduler.enum_tasks([&](const AbstractTask *t){
            print(static_buff,get_task_name(t)," ",t->_run_time," ",t->get_scheduled_time(),"\r\n");
        });
        _server.send_ws_message(req, ws::Message{static_buff.get_text(), ws::Type::text});
        break;
    case WsReqCmd::generate_code:
        generate_otp_code();
        static_buff.write(_last_code.data(), _last_code.size());
        _server.send_ws_message(req, ws::Message{static_buff.get_text(), ws::Type::text});
        break;
    case WsReqCmd::unpair_all:
        generate_pair_secret();
        gen_and_print_token();
        _server.send_ws_message(req, ws::Message{static_buff.get_text(), ws::Type::text});
        break;
    case WsReqCmd::reset:
        _server.send_ws_message(req, ws::Message{static_buff.get_text(), ws::Type::text});
        delay(10000);   //delay more than 5 sec invokes WDT
        break;
    case WsReqCmd::clear_stats:
        _storage.tray.commit_consumed(_storage.tray.feeder_time);
        _storage.tray.feeder_time = _storage.tray.feeder_time - _storage.tray.tray_fill_time ;
        _storage.tray.tray_open_time = 0;
        _storage.tray.tray_fill_time = 0;
        _storage.tray.consumed_fuel_kg = 0;
        _storage.cntr1 = {};
        _storage.cntr2 = {};
        _storage.runtm = {};
        _storage.runtm2 = {};
        _storage.save();
        _server.send_ws_message(req, ws::Message{static_buff.get_text(), ws::Type::text});
        break;
    default:
        break;
    }
}

struct StatusOutWs {
    uint32_t cur_time;
    uint32_t feeder_time;
    uint32_t tray_open_time;
    uint32_t tray_fill_time;
    int16_t tray_fill_kg;
    int16_t bag_consumption;
    int16_t temp_output_value;
    int16_t temp_output_amp_value;
    int16_t temp_input_value;
    int16_t temp_input_amp_value;
    int16_t rssi;
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

static int16_t encode_temp(std::optional<float> v) {
    if (v.has_value()) {
        return static_cast<int16_t>(*v*10.0);
    } else {
        return static_cast<int16_t>(0x8000);
    }
}

void Controller::status_out_ws(Stream &s) {
    StatusOutWs st{
        get_current_time(),
        _storage.tray.feeder_time,
        _storage.tray.tray_open_time,
        _storage.tray.tray_fill_time,
        static_cast<int16_t>(_storage.tray.tray_fill_kg),
        static_cast<int16_t>(_storage.tray.feeder_1kg_time),
        encode_temp(_temp_sensors.get_output_temp()),
        encode_temp(_temp_sensors.get_output_ampl()),
        encode_temp(_temp_sensors.get_input_temp()),
        encode_temp(_temp_sensors.get_input_ampl()),
        static_cast<int16_t>(_network.get_rssi()),
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
    s.write(reinterpret_cast<const char *>(&st), sizeof(st));
}


TimeStampMs Controller::read_serial(TimeStampMs) {
    while (handle_serial(*this));
    return 250;
}


std::string_view Controller::get_task_name(const AbstractTask *task) {
    if (task == &_feeder) return "feeder";
    if (task == &_fan) return "fan";
    if (task == &_temp_sensors) return "temp_sensors";
    if (task == &_display) return "display";
    if (task == &_motoruntime) return "motoruntime";
    if (task == &_auto_drive_cycle) return "auto_drive_cycle";
    if (task == &_network) return "network";
    if (task == &_read_serial) return "read_serial";
    if (task == &_refresh_wdt) return "wdt";
    return "unknown";
}

bool Controller::enable_temperature_simulation(std::string_view b) {
    SimulInfo sinfo;
    do {
       if (!update_settings_kv(simulate_temp_table, sinfo, split(b, "&"))) {
           return false;
       }
    }while (!b.empty());
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
    print(static_buff, "token=", std::string_view(tkn.data(), tkn.size()),"\r\n");
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
            }
        } else {
            if (stop_btn.stabilize(default_btn_release_interval_ms)) {
                if (!stop_btn.test_and_reset_user_state()) {
                    _storage.config.operation_mode = 0;
                    _feeder.stop();
                    _fan.stop();
                    _storage.save();
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
                    }
                }
                if (feeder_btn.stabilize(feeder_min_press_interval_ms)) {
                    _feeder.keep_running(cur_time+1000);
                    feeder_btn.set_user_state();
                }
            }
            auto &fan_btn = _kbdstate.get_state(key_code_fan);
            if (fan_btn.stabilize(default_btn_release_interval_ms) && fan_btn.pressed()) {
                if (_fan.is_active()) {
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
            }

        }

    }
    return 2;
}

}

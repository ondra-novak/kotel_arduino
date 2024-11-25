#include "controller.h"

#include "http_utils.h"
#include "icons.h"
#include "web_page.h"
#include "hash_cnt_stream.h"

#include "stringstream.h"

#ifdef WEB_DEVEL
#include <fstream>
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
        ,_wifi_mon(this)
        ,_scheduler({&_feeder, &_fan, &_temp_sensors,  &_display,
            &_motoruntime, &_auto_drive_cycle, &_wifi_mon})
        ,_server(80)
{

}

void Controller::begin() {
    _fan.begin();
    _feeder.begin();
    _pump.begin();
    _storage.begin();
    _temp_sensors.begin();
    _display.begin();
    _scheduler.reschedule();
    init_wifi();
    _server.begin();
}

static inline bool defined_and_above(const std::optional<float> &val, float cmp) {
    return val.has_value() && *val > cmp;
}

void Controller::run() {
    _sensors.read_sensors();
    _temp_sensors.async_cycle(_scheduler);

    auto prev_mode = _cur_mode;

    auto req = _server.get_request();
    if (req.client) {
        handle_server(req);
    }
    control_pump();
    if (_sensors.tray_open) {
        _was_tray_open = true;
        _feeder.stop();
    } else {
        if (_was_tray_open) {
            _was_tray_open = false;
            _storage.tray.tray_open_time = _storage.tray.feeder_time;
            _storage.save();
        }
        if (!_sensors.motor_temp_monitor //motor temperature check
             || (defined_and_above(_temp_sensors.get_output_temp(),_storage.config.output_max_temp))
             || (defined_and_above(_temp_sensors.get_input_temp(),_storage.config.output_max_temp))) {
            run_stop_mode();
        } else if (_storage.config.operation_mode == 0) {
            run_manual_mode();
        } else if (_storage.config.operation_mode == 1) {
            if ( (!_temp_sensors.get_output_temp().has_value()) //we don't have output temperature
                || (_temp_sensors.get_output_temp().has_value() //output temperature is less than input temperature
                        && _temp_sensors.get_input_temp().has_value()
                        && *_temp_sensors.get_output_temp() < *_temp_sensors.get_input_temp())
                || (_temp_sensors.get_output_temp().has_value() //output temp is too low
                        && *_temp_sensors.get_output_temp() < _storage.config.pump_start_temp
                        && !_auto_stop_disabled)){
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
    if (_list_temp_async && !_temp_sensors.is_reading()) {
        list_onewire_sensors(*_list_temp_async);
        _list_temp_async->stop();
        _list_temp_async.reset();
    }
    _scheduler.run();
    _storage.flush();

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
        {"tray.bgfc", &Tray::bag_fill_count},
        {"tray.bct", &Tray::bag_consump_time}
};

static constexpr std::pair<const char *, uint32_t Utilization::*> utilization_table[] ={
        {"utlz.fan", &Utilization::fan_time},
        {"utlz.pump", &Utilization::pump_time},
        {"atime", &Utilization::active_time},
};

static constexpr std::pair<const char *, uint32_t Counters1::*> counters1_table[] ={
        {"sc.feeder", &Counters1::feeder_start_count},
        {"sc.fan", &Counters1::fan_start_count},
        {"sc.pump", &Counters1::pump_start_count},
};

static constexpr std::pair<const char *, TextSector WiFi_SSID::*> wifi_ssid_table[] ={
        {"wifi.ssid", &WiFi_SSID::ssid},
};
static constexpr std::pair<const char *, PasswordSector WiFi_Password::*> wifi_password_table[] ={
        {"wifi.password", &WiFi_Password::password},
};

static constexpr std::pair<const char *, IPAddr WiFi_NetSettings::*> wifi_netcfg_table[] ={
        {"net.ip", &WiFi_NetSettings::ipaddr},
        {"net.dns", &WiFi_NetSettings::dns},
        {"net.gateway", &WiFi_NetSettings::gateway},
        {"net.netmask", &WiFi_NetSettings::netmask},
};

static constexpr std::pair<const char *, Tray Storage::*> tray_control_table[] ={
        {"tray.bgfc", &Storage::tray},
};
static constexpr std::pair<const char *, uint16_t Tray::*> tray_control_table_2[] ={
        {"tray.bct", &Tray::bag_consump_time}
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

void Controller::stats_out(Stream &s) {

    print_table(s, tray_table, _storage.tray);
    print_table(s, utilization_table, _storage.utlz);
    print_table(s, counters1_table, _storage.cntr1);
    s.print("eee=");
    print_data(s, _storage.get_eeprom().get_crc_error_counter());
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
    if (!parse_to_field(fld.bag_fill_count, value)) return false;
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
                || update_settings(tray_control_table, _storage, key, value)
                || update_settings(tray_control_table_2, _storage.tray, key, value);
        if (!ok) {
            failed_field = key;
            return false;
        }

    } while (!body.empty());
    _storage.save();
    _scheduler.reschedule();
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

void Controller::clear_error()
{
    _storage.status.error = ErrorCode::no_error;
    _storage.save();
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
    print_data_line(s,"motor_temp_ok", _sensors.motor_temp_monitor);
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
    _auto_stop_disabled = true;
    _cur_mode = DriveMode::manual;

}

void Controller::run_auto_mode() {

    if (_cur_mode != DriveMode::automatic)  {
        _cur_mode = DriveMode::automatic;
        _auto_drive_cycle.wake_up(_scheduler);
    }
}

void Controller::run_other_mode() {
    _cur_mode = DriveMode::other;

}

void Controller::run_stop_mode() {
    _cur_mode = DriveMode::stop;
}

int Controller::calc_tray_remain() const {
    if (_storage.tray.tray_fill_time > _storage.tray.feeder_time) return -1;
    if (_storage.tray.bag_consump_time == 0) return -1;
    auto total = static_cast<uint32_t>(_storage.tray.bag_fill_count)
                      * static_cast<uint32_t>(_storage.tray.bag_consump_time);
    auto end = _storage.tray.tray_fill_time + total;
    if (_storage.tray.feeder_time > end) return 0;
    auto rmn = end - _storage.tray.feeder_time;
    return rmn / _storage.tray.bag_consump_time;
}

static IPAddress conv_ip(const IPAddr &x) {
    return IPAddress(x.ip[0], x.ip[1], x.ip[2], x.ip[3]);
}


void Controller::init_wifi() {
    WiFi.setTimeout(0);
    if (_storage.wifi_config.ipaddr != IPAddr{}) {
        WiFi.config(conv_ip(_storage.wifi_config.ipaddr),
                conv_ip(_storage.wifi_config.dns),
                conv_ip(_storage.wifi_config.gateway),
                conv_ip(_storage.wifi_config.netmask)
        );
    }


    char ssid[sizeof(WiFi_SSID)+1];
    char pwd[sizeof(WiFi_Password)+1];
    auto x = _storage.wifi_ssid.ssid.get();
    *std::copy(x.begin(), x.end(), ssid) = 0;
    x = _storage.wifi_password.password.get();
    *std::copy(x.begin(), x.end(), pwd) = 0;
    if (pwd[0] && ssid[0]) {
        WiFi.begin(ssid, pwd);
    } else if (ssid[0]) {
        WiFi.begin(ssid);
    }
}

bool Controller::is_wifi() const {
    return _wifi_connected;
}


static constexpr std::pair<const char *, uint8_t Controller::ManualControlStruct::*> manual_control_table[] ={
        {"feed.tr",&Controller::ManualControlStruct::_feeder_time},
        {"fan.timer",&Controller::ManualControlStruct::_fan_time},
        {"fan.speed",&Controller::ManualControlStruct::_fan_speed},
        {"pump.force",&Controller::ManualControlStruct::_force_pump},
};


#ifdef WEB_DEVEL

void Controller::send_file(MyHttpServer::Request &req, std::string_view content_type, std::string_view file_name) {
    std::string path = "./src/www";
    path.append(file_name);
    std::ifstream f(path);
    if (!f) {
        _server.error_response(req, 404, {}, {}, path);
    } else {
        _server.send_simple_header(req, content_type, -1);
        int i = f.get();
        while (i != EOF) {
            req.client.print(static_cast<char>(i));
            i = f.get();
        }
    }
    req.client.stop();
}
#endif

struct SimulInfo {
    float input = 0;
    float output = 0;
};

static constexpr std::pair<const char *, float SimulInfo::*> simulate_temp_table[] = {
        {"input", &SimulInfo::input},
        {"output", &SimulInfo::output},
};



void Controller::handle_server(MyHttpServer::Request &req) {

    using Ctx = MyHttpServer::ContentType;

    set_wifi_used();
    if (req.request_line.method == HttpMethod::WS) {
        handle_ws_request(req);
        return;
    } else if (req.request_line.path == "/api/scan_temp" && req.request_line.method == HttpMethod::POST) {
        if (_list_temp_async.has_value()) {
            _server.error_response(req, 503, "Service unavailable" , {}, {});
        } else {
            _server.send_simple_header(req, Ctx::text);
            _list_temp_async.emplace(std::move(req.client));
            return;
        }
    } else if (req.request_line.path == "/api/ws" && req.request_line.method == HttpMethod::GET) {
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
            return;
        } else {
            _server.error_response(req,400,{});
        }

    } else if (req.request_line.path == "/") {
#ifdef WEB_DEVEL
        send_file(req, Ctx::html, "/index.html");
#else
        _server.send_file(req, HttpServerBase::ContentType::html, embedded_index_html, true);
#endif
        return; //do not stop
    } else if (req.request_line.path == "/code.js") {
#ifdef WEB_DEVEL
        send_file(req, Ctx::javascript, req.request_line.path);
#else
        _server.send_file(req, Ctx::javascript, embedded_code_js, true);
#endif
        return; //do not stop
    } else if (req.request_line.path == "/style.css") {
#ifdef WEB_DEVEL
        send_file(req, Ctx::css, req.request_line.path);
#else
        _server.send_file(req, Ctx::css, embedded_style_css, true);
#endif
        return; //do not stop
    } else if (req.request_line.path == "/api/simulate_temperature") {
        if (req.request_line.method == HttpMethod::PUT) {
            std::string_view b = req.body;
            SimulInfo sinfo;
            do {
               if (!update_settings_kv(simulate_temp_table, sinfo, split(b, "&"))) {
                   _server.error_response(req, 400, {}, {}, {});
               }
            }while (!b.empty());
            if (sinfo.input>0 && sinfo.output > 0) {
                _temp_sensors.simulate_temperature(sinfo.input,sinfo.output);
                _server.error_response(req, 202, {});
            } else {
                _server.error_response(req, 400, {}, {}, {});
            }

        } else if (req.request_line.method == HttpMethod::DELETE) {
            _temp_sensors.disable_simulated_temperature();
            _server.error_response(req, 202, {});
        } else {
            _server.error_response(req, 405, {}, {{"Allow","PUT, DELETE"}}, {});
        }
    } else {
        _server.error_response(req, 404, "Not found");
    }
    req.client.stop();
}


TimeStampMs Controller::run_http(TimeStampMs) {
    return 200;

}


bool Controller::manual_control(const ManualControlStruct &cntr) {
    if (_cur_mode != DriveMode::manual || _sensors.tray_open) return false;
    if (cntr._fan_speed != 0xFF) {
        _fan.set_speed(cntr._fan_speed);
    }
    if (cntr._fan_time != 0xFF) {
        if (cntr._fan_time != 0) {
           _fan.keep_running(_scheduler, get_current_timestamp()+from_seconds(cntr._fan_time));
        } else {
            _fan.stop();
        }
    }

    if (cntr._feeder_time != 0xFF) {
        if (cntr._feeder_time != 0) {
            _feeder.keep_running(_scheduler, get_current_timestamp()+from_seconds(cntr._feeder_time));
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
    bool at = is_attenuation();
    Storage &stor = get_storage();
    ++stor.utlz.active_time;
    if (fd) ++stor.tray.feeder_time;
    if (pm) ++stor.utlz.pump_time;
    if (fa) ++stor.utlz.fan_time;
    if (at) ++stor.utlz.attent_time;



    bool anything_running = fd || pm || fa ;

    if (anything_running && _flush_time == max_timestamp) {
        _flush_time = now+60000;
    } else if (now >= _flush_time) {
        stor.save();
        if (!anything_running) _flush_time = max_timestamp;
        else _flush_time = now+60000;
    }
    return 1000;
}



static constexpr std::pair<const char *, int8_t Controller::SetFuelParams::*> set_fuel_params_table[] = {
        {"full", &Controller::SetFuelParams::full},
        {"bagcount", &Controller::SetFuelParams::bagcount},
        {"kalib", &Controller::SetFuelParams::kalib},
        {"absnow", &Controller::SetFuelParams::absnow},
};

bool Controller::set_fuel(std::string_view body, std::string_view &&error) {
    SetFuelParams sfp;
    do {
        auto ln = split(body,"&");
        if (!update_settings_kv(set_fuel_params_table, sfp, ln)) {
            error = ln;
            return false;
        }
    } while (!body.empty());
    bool b = set_fuel(sfp);
    if (!b) error = "kalib";
    return b;
}

bool Controller::set_fuel(const SetFuelParams &sfp) {

    auto filltime  = sfp.absnow?_storage.tray.feeder_time:_storage.tray.tray_fill_time;
    if (sfp.kalib) {
        if (_storage.tray.bag_fill_count == 0) {
            return false;
        }
        auto total_time = filltime - _storage.tray.tray_fill_time;
        auto consump = total_time / _storage.tray.bag_fill_count;
        if (consump > 0xFFFF || consump == 0) {
            return false;
        }
        _storage.tray.bag_consump_time = consump;
        _storage.tray.bag_fill_count = sfp.bagcount;
    } else {
        if (sfp.full) {
            _storage.tray.bag_fill_count = 15;
            _storage.tray.tray_fill_time = filltime;
        } else {
            _storage.tray.alter_bag_count(filltime, sfp.bagcount);
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


    if (t_input < _storage.config.input_min_temp) {
        if (t_output < _storage.config.output_max_temp) {
            _auto_mode = AutoMode::fullpower;
        } else {
            _auto_mode = AutoMode::off;
        }
    } else {
        if (t_output< _storage.config.output_max_temp) {
            _auto_mode = AutoMode::lowpower;
        } else {
            _auto_mode = AutoMode::off;
        }
    }

    const Profile *p;

    switch (_auto_mode) {
        default:
        case AutoMode::off:
            _auto_stop_disabled = false;
            _fan.stop();
            _feeder.stop();
            return TempSensors::measure_interval;
        case AutoMode::fullpower:
            p = &_storage.config.full_power;
            break;
        case AutoMode::lowpower:
            _auto_stop_disabled = false;
            p = &_storage.config.low_power;
            break;
    }

    auto cycle_interval = from_seconds(p->burnout_sec)+from_seconds(p->fueling_sec);
    bool fan_active = _fan.is_active();
    _fan.set_speed(p->fanpw);
    _fan.keep_running(_scheduler,cur_time + cycle_interval+1000);
     auto t = from_seconds(p->fueling_sec);
    if (fan_active) {
        _feeder.keep_running(_scheduler, cur_time+t);
        return cycle_interval;
    } else {
        return cycle_interval/2;
    }
}

void Controller::handle_ws_request(MyHttpServer::Request &req)
{
    static_buff.clear();

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
        stats_out(static_buff);
        _server.send_ws_message(req, ws::Message{static_buff.get_text(), ws::Type::text});
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
    case WsReqCmd::file_util:
        static_buff.write(reinterpret_cast<const char *>(&_storage.utlz), sizeof(_storage.utlz));
        _server.send_ws_message(req, ws::Message{static_buff.get_text(), ws::Type::binary});
    break;
    case WsReqCmd::file_cntrs1:
        static_buff.write(reinterpret_cast<const char *>(&_storage.cntr1), sizeof(_storage.cntr1));
        _server.send_ws_message(req, ws::Message{static_buff.get_text(), ws::Type::binary});
    break;
    case WsReqCmd::file_status:
        static_buff.write(reinterpret_cast<const char *>(&_storage.status), sizeof(_storage.status));
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
    default:
        break;
    }
}

struct StatusOutWs {
    uint32_t feeder_time;
    uint32_t tray_open_time;
    uint32_t tray_fill_time;
    int16_t bag_fill_count;
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
        _storage.tray.feeder_time,
        _storage.tray.tray_open_time,
        _storage.tray.tray_fill_time,
        static_cast<int16_t>(_storage.tray.bag_fill_count),
        static_cast<int16_t>(_storage.tray.bag_consump_time),
        encode_temp(_temp_sensors.get_output_temp()),
        encode_temp(_temp_sensors.get_output_ampl()),
        encode_temp(_temp_sensors.get_input_temp()),
        encode_temp(_temp_sensors.get_input_ampl()),
        static_cast<int16_t>(_wifi_rssi),
        static_cast<uint8_t>(_temp_sensors.is_simulated()?1:0),
        static_cast<uint8_t>(_temp_sensors.get_output_status()),
        static_cast<uint8_t>(_temp_sensors.get_input_status()),
        static_cast<uint8_t>(_cur_mode),
        static_cast<uint8_t>(_auto_mode),
        static_cast<uint8_t>(_sensors.tray_open?1:0),
        static_cast<uint8_t>(_sensors.motor_temp_monitor?1:0),
        static_cast<uint8_t>(_pump.is_active()?1:0),
        static_cast<uint8_t>(_feeder.is_active()?1:0),
        static_cast<uint8_t>(_fan.get_current_speed()),
    };
    s.write(reinterpret_cast<const char *>(&st), sizeof(st));
}

TimeStampMs Controller::wifi_mon(TimeStampMs) {
    _wifi_connected = WiFi.status() == WL_CONNECTED;
    if (_wifi_connected) _wifi_rssi = WiFi.RSSI();
    return 1023;
}

}

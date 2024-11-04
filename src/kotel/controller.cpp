#include "controller.h"

#include "http_utils.h"
#include "icons.h"
namespace kotel {



Controller::Controller()
        :_feeder(_storage)
        ,_fan(_storage)
        ,_pump(_storage)
        ,_temp_sensors(_storage)
        ,_wifi_mon(_storage)
        ,_display(*this)
        ,_scheduler({&_feeder, &_fan, &_temp_sensors, &_wifi_mon, &_display, &_pump})
{

}

void Controller::begin() {
    _storage.begin();
    _temp_sensors.begin();
    _display.begin();
    _scheduler.reschedule();
}

void Controller::run() {
    _sensors.read_sensors();
    _scheduler.run();
    control_pump();
    if (_is_stop || !_sensors.motor_temp_monitor
            || _sensors.tray_open
            || !_temp_sensors.get_output_temp().has_value()
            || *(_temp_sensors.get_output_temp()) >= static_cast<float>(_storage.config.output_max_temp)) {
        run_stop_mode();
    } else if (_storage.config.operation_mode == 0) {
        run_manual_mode();
    } else if (_storage.config.operation_mode == 1) {
        run_auto_mode();
    } else {
        run_other_mode();
    }
    _storage.flush();


}


static constexpr std::pair<const char *, uint8_t Config::*> config_table[] ={
        {"fan_power_pct", &Config::fan_power_pct},
        {"default_bag_count", &Config::default_bag_count},
        {"fan_rundown_sec", &Config::fan_rundown_sec},
        {"feeder_first_on_sec", &Config::feeder_first_on_sec},
        {"feeder_on_sec", &Config::feeder_on_sec},
        {"feeder_off_sec", &Config::feeder_off_sec},
        {"input_min_temp", &Config::input_min_temp},
        {"max_atten_min", &Config::max_atten_min},
        {"operation_mode", &Config::operation_mode},
        {"output_max_temp", &Config::output_max_temp},
        {"pump_temp", &Config::pump_temp},
};

template<typename X>
void print_data(Stream &s, const X &data) {
    s.print(data);
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
void print_table(Stream &s, const Table &table, const Object &object) {
    for (const auto &[k,ptr]: table) {
        print_data_line(s, k, object.*ptr);
    }

}
static constexpr std::pair<const char *, SimpleDallasTemp::Address TempSensor::*> tempsensor_table_1[] ={
        {"temp_sensor.input.addr", &TempSensor::input_temp},
        {"temp_sensor.output.addr", &TempSensor::output_temp},
};

static constexpr std::pair<const char *, uint8_t TempSensor::*> tempsensor_table_2[] ={
        {"temp_sensor.interval", &TempSensor::interval},
        {"temp_sensor.trend_smooth", &TempSensor::trend_smooth},
};


static constexpr std::pair<const char *, uint32_t Tray::*> tray_table[] ={
        {"feeder.time", &Tray::feeder_time},
        {"tray.open_time", &Tray::tray_open_time},
        {"tray.fill_time", &Tray::tray_fill_time}
};

static constexpr std::pair<const char *, uint16_t Tray::*> tray_table_2[] ={
        {"tray.bag_fill_count", &Tray::bag_fill_count},
        {"tray.bag_consump_time", &Tray::bag_consump_time}
};

static constexpr std::pair<const char *, uint32_t Utilization::*> utilization_table[] ={
        {"utilization.fan", &Utilization::fan_time},
        {"utilization.pump", &Utilization::pump_time},
        {"attent_time", &Utilization::attent_time},
        {"active_time", &Utilization::active_time},
};

static constexpr std::pair<const char *, uint32_t Counters1::*> counters1_table[] ={
        {"feeder.start_count", &Counters1::feeder_start_count},
        {"fan.start_count", &Counters1::fan_start_count},
        {"pump.start_coun", &Counters1::pump_start_count},
        {"attenuation.count", &Counters1::attent_count},
        {"attenuation.long_count", &Counters1::long_attents_count},
};

static constexpr std::pair<const char *, TextSector WiFi_SSID::*> wifi_ssid_table[] ={
        {"wifi.ssid", &WiFi_SSID::ssid},
};
static constexpr std::pair<const char *, TextSector WiFi_Password::*> wifi_password_table[] ={
        {"wifi.password", &WiFi_Password::password},
};

static constexpr std::pair<const char *, IPAddr WiFi_NetSettings::*> wifi_netcfg_table[] ={
        {"net.ip", &WiFi_NetSettings::ipaddr},
        {"net.dns", &WiFi_NetSettings::dns},
        {"net.gateway", &WiFi_NetSettings::gateway},
        {"net.netmask", &WiFi_NetSettings::netmask},
};

static constexpr std::pair<const char *, Tray Storage::*> tray_control_table[] ={
        {"tray.bag_fill_count", &Storage::tray},
};
static constexpr std::pair<const char *, uint16_t Tray::*> tray_control_table_2[] ={
        {"tray.bag_consumption_time", &Tray::bag_consump_time}
};



void Controller::config_out(Stream &s) {

    print_table(s, config_table, _storage.config);
    print_table(s, tempsensor_table_1, _storage.temp);
    print_table(s, tempsensor_table_2, _storage.temp);
    print_table(s, wifi_ssid_table, _storage.wifi_ssid);
    print_table(s, wifi_netcfg_table, _storage.wifi_config);
}

void Controller::stats_out(Stream &s) {

    print_table(s, tray_table, _storage.tray);
    print_table(s, tray_table_2, _storage.tray);
    print_table(s, utilization_table, _storage.utlz);
    print_table(s, counters1_table, _storage.cntr1);
    s.print("eeprom_checksum_error=");
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
    fld  = static_cast<uint8_t>(v);
    return true;
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
    fld.set(value);
    return true;
}

bool parse_to_field(Tray &fld, std::string_view value) {
    if (!parse_to_field(fld.bag_fill_count, value)) return false;
    fld.tray_fill_time = fld.tray_open_time;
    return true;
}



template<typename Table, typename Config>
bool update_settings(const Table &table, Config &config, std::string_view key, std::string_view value) {
    for (const auto &[k,ptr]: table) {
        if (k == key) {
            return parse_to_field(config.*ptr, value);
        }
    }
    return false;
}

bool Controller::config_update(std::string_view body, std::string_view &&failed_field) {

    do {
        auto value = split(body, "&");
        auto key = split(value, "=");
        bool ok = update_settings(config_table, _storage.config, key, value)
                || update_settings(tempsensor_table_1, _storage.temp, key, value)
                || update_settings(tempsensor_table_2, _storage.temp, key, value)
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
    while (_temp_sensors.is_reading()) {
        run();
    }
    cntr.request_temp();
    auto n = millis();
    while (millis() - n < 750) {
        run();
    }
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
    print_data_line(s,"error_code", static_cast<int>(_storage.status.error));
    print_data_line(s,"temp.output.value", _temp_sensors.get_output_temp());
    print_data_line(s,"temp.output.status", static_cast<int>(_temp_sensors.get_output_status()));
    print_data_line(s,"temp.input.value", _temp_sensors.get_input_temp());
    print_data_line(s,"temp.input.status", static_cast<int>(_temp_sensors.get_input_status()));
    print_data_line(s,"tray_open", _sensors.tray_open);
    print_data_line(s,"motor_temp_ok", _sensors.motor_temp_monitor);
    print_data_line(s,"pump", _pump.is_active());
    print_data_line(s,"feeder", _feeder.is_active());
    print_data_line(s,"fan", _fan.get_speed());
    print_data_line(s,"network.ip", WiFi.localIP());
    print_data_line(s,"network.netmask", WiFi.subnetMask());
    print_data_line(s,"network.gateway", WiFi.gatewayIP());
    print_data_line(s,"network.ssid", WiFi.SSID());

}

void Controller::control_pump() {
    auto t = _temp_sensors.get_output_temp();
    if (t.has_value()) {
        if (!_pump.is_active() && *t >= static_cast<float>(_storage.config.pump_temp)) {
            _pump.set_active(true);
        } else if (_pump.is_active() && *t <= static_cast<float>(_storage.config.pump_temp-2)) {
            _pump.set_active(false);
        }
    }
}

void Controller::run_manual_mode() {
    _auto_stop_disabled = true;
    _is_stop = false;
}

void Controller::run_auto_mode() {
    float output_temp = *_temp_sensors.get_output_temp();
    float input_temp = _temp_sensors.get_input_temp().has_value()?
                    *_temp_sensors.get_input_temp():output_temp;   //fallback if one temp is failed

    float min_temp = static_cast<float>(_storage.config.stop_temp);
    float attent_temp = static_cast<float>(_storage.config.input_min_temp+2);

    if (!_auto_stop_disabled && (output_temp<min_temp || input_temp < min_temp)) {
        _is_stop = true;
        return;
    }

    if (_attenuation) {
        _auto_stop_disabled = false;
        if (input_temp > _storage.config.input_min_temp) {
            //stay here
            return;
        }
        _attenuation = false;
        _feeder.set_mode(_scheduler, Feeder::cycle_after_atten);
        _fan.set_mode(_scheduler, Fan::running);
    } else {
        if (input_temp >=attent_temp) {
            _attenuation = true;
            _feeder.set_mode(_scheduler, Feeder::stop);
            _fan.set_mode(_scheduler, Fan::run_away);
        } else {
            _feeder.set_mode(_scheduler, Feeder::cycle);
            _fan.set_mode(_scheduler, Fan::running);
        }
    }
}

void Controller::run_other_mode() {
    _feeder.set_mode(_scheduler, Feeder::stop); //unknown mode
    _fan.set_mode(_scheduler, Fan::stop);
}

void Controller::run_stop_mode() {
    _feeder.set_mode(_scheduler, Feeder::stop); //force stop all the time
    _fan.set_mode(_scheduler, Fan::stop);
}

}

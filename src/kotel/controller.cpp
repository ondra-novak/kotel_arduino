#include "controller.h"

#include "http_utils.h"
namespace kotel {

Controller::Controller()
        :_feeder(_storage)
        ,_temp_sensors(_storage)
        ,_scheduler({&_feeder, &_fan, &_pump, &_temp_sensors})
{

}

void Controller::begin() {
    _feeder.start();
    _scheduler.reschedule();
}

void Controller::run() {
    _scheduler.run();
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

template<typename Table, typename Object>
void print_table(Stream &s, const Table &table, const Object &object) {
    for (const auto &[k,ptr]: table) {
        s.print(k);
        s.print('=');
        print_data(s,(object.*ptr));
        s.println();
    }

}
static constexpr std::pair<const char *, SimpleDallasTemp::Address TempSensor::*> tempsensor_table_1[] ={
        {"input_temp_sensor_addr", &TempSensor::input_temp},
        {"output_temp_sensor_addr", &TempSensor::output_temp},
};

static constexpr std::pair<const char *, uint8_t TempSensor::*> tempsensor_table_2[] ={
        {"temp_measure_interval", &TempSensor::interval},
        {"temp_trend_smooth", &TempSensor::trend_smooth},
};


static constexpr std::pair<const char *, uint32_t Tray::*> tray_table[] ={
        {"feeder_time", &Tray::feeder_time},
        {"tray_open_time", &Tray::tray_open_time},
        {"tray_empty_time", &Tray::tray_empty_time},
        {"bag_consump_time", &Tray::bag_consump_time},
};

static constexpr std::pair<const char *, uint32_t Utilization::*> utilization_table[] ={
        {"fan_time", &Utilization::fan_time},
        {"pump_time", &Utilization::pump_time},
        {"attent_time", &Utilization::attent_time},
        {"active_time", &Utilization::active_time},
};

static constexpr std::pair<const char *, uint32_t Counters1::*> counters1_table[] ={
        {"feeder_start_count", &Counters1::feeder_start_count},
        {"fan_start_count", &Counters1::fan_start_count},
        {"pump_start_coun", &Counters1::pump_start_coun},
        {"attent_count", &Counters1::attent_count},
};

static constexpr std::pair<const char *, uint32_t Counters2::*> counters2_table[] ={
        {"long_attents_count", &Counters2::long_attents_count},
};



void Controller::config_out(Stream &s) {

    print_table(s, config_table, _storage.config);
    print_table(s, tempsensor_table_1, _storage.temp);
    print_table(s, tempsensor_table_2, _storage.temp);
}

void Controller::stats_out(Stream &s) {

    print_table(s, tray_table, _storage.tray);
    print_table(s, utilization_table, _storage.utlz);
    print_table(s, counters1_table, _storage.cntr1);
    print_table(s, counters2_table, _storage.cntr2);
    s.print("eeprom_checksum_error=");
    print_data(s, _storage.get_eeprom().get_crc_error_counter());
}

bool parse_to_field(uint8_t &fld, std::string_view value) {
    int v = 0;
    for (char c: value) {
        if (c < '0' || c > '9') return false;
        v = v * 10 + (c - '0');
        if (v > 255) return false;
    }
    fld  = static_cast<uint8_t>(v);
    return true;
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
                || update_settings(tempsensor_table_2, _storage.temp, key, value);
        if (!ok) {
            failed_field = key;
            return false;
        }

    } while (!body.empty());
    _storage.update(_storage.config);
    _storage.update(_storage.temp);
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
        else s.print("null");
        s.println();
        return true;
    });
}

}

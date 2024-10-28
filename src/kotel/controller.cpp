#include "controller.h"

namespace kotel {

Controller::Controller()
        :_feeder(_storage)
        ,_scheduler({&_feeder, &_fan, &_pump})
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
    s.println(data);
}

template<>
void print_data(Stream &s, const SimpleDallasTemp::Address &data) {
    s.print(data[0], HEX);
    for (unsigned int i = 1; i < data.size();++i) {
        s.print('-');
        s.print(data[i],HEX);
    }
    s.println();
}

template<typename Table, typename Object>
void print_table(Stream &s, const Table &table, const Object &object) {
    for (const auto &[k,ptr]: table) {
        s.print(k);
        s.print(':');
        print_data(s,(object.*ptr));
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
}

void Controller::config_update(std::string_view body) {

}


}

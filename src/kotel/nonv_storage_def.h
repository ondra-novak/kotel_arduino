#pragma once
#include <stdint.h>
#include <array>

constexpr unsigned int file_config = 0;
constexpr unsigned int file_snapshot = 1;
constexpr unsigned int file_runtime = 2;
constexpr unsigned int file_cntrs = 3;
constexpr unsigned int file_tempsensor = 4;
constexpr unsigned int file_wifi_ssid = 5;
constexpr unsigned int file_wifi_pwd = 6;
constexpr unsigned int file_wifi_net = 7;
constexpr unsigned int file_cntrs2 = 8;
constexpr unsigned int file_pair_secret = 9;
constexpr unsigned int file_tray = 10;
constexpr unsigned int graph_files=12;
constexpr unsigned int files_feeder_graph = 11;
constexpr unsigned int files_feeder_low_graph = files_feeder_graph+graph_files;
constexpr unsigned int files_fuel_fill_graph = files_feeder_low_graph+graph_files;
constexpr unsigned int files_errors_log = files_fuel_fill_graph+graph_files;
constexpr unsigned int files_control_log = files_errors_log+graph_files;
constexpr unsigned int file_directory_len = files_control_log+graph_files;

constexpr unsigned int eeprom_sector_size = 30;


namespace kotel {

enum class ErrorCode: uint8_t {
    no_error = 0,
    stop_low_temp = 1,
    motor_high_temp = 2,
};


struct Profile {
    uint8_t fueling_sec;    //doba prikladani
    uint8_t burnout_sec;    //doba dohorivani
    uint8_t fanpw;      //vykon ventilatoru
};

enum class OperationMode: uint8_t {
    manual = 0,
    automatic = 1,
};

struct OneDecimalValue {
    uint8_t value; //separated because config output
};

struct Config {
    uint8_t operation_mode = static_cast<uint8_t>(OperationMode::manual);
    Profile full_power = {8, 20, 60};
    Profile low_power = {5, 30, 40};
    OneDecimalValue  heat_value  = {170};
    uint8_t input_temp_full_power = 60; //vstupni teplota < pri ktere se zapina plny vykon
    uint8_t input_temp_low_power = 65; //vstupni teplota < pri ktere se zapina nizky vykon
    uint8_t output_max_temp = 75;      //maximalni teplota, pri ktere se prejde do overheat
    uint8_t lowerst_safe_temp = 55;    //nejnizsi bezpecna teplota (oba teplomery)
    uint8_t input_temp_samples = 10;    //pocet vzorcu pro vypocet trendu
    uint8_t output_temp_samples = 10;   //pocet vzorcu pro vypocet trendu
    uint8_t fan_pulse_interval = 100;   //interval pulzovani
    OneDecimalValue fan_nonlinear_correction = {20}; //parametr T * 0.1 ve vzorci pro nelinearitu

    uint8_t serial_log_out = 0;     //if there is 1, serial output is used for logging
    uint8_t bag_kg = 15;            //velikost pytle
    uint8_t tray_kg = 15*15;        //velikost nasypky v kg
    uint8_t display_intensity = 1;
};

static_assert(sizeof(Config)<=eeprom_sector_size);


struct Runtime {
    uint32_t feeder = 0;       //cisty cas podavace
    uint32_t fan = 0;          //cisty cas ventilatoru
    uint32_t feeder_low = 0;   //cisty cas podavace jen v nizkem vykonu
    uint32_t full_power = 0;   //doba plnyho vykonu
    uint32_t low_power = 0;    //doba nizkeho vykonu
    uint32_t cooling = 0;      //doba chlazeni
    uint32_t stop_time=0;      //doba ve stop/havarijnim rezimu
};

static_assert(sizeof(Runtime)<=eeprom_sector_size);

struct Counters {
    uint16_t feeder_overheat_count = 0;  //kolikrat se motor prehral
    uint16_t tray_open_count = 0;        //kolikrat se otevrela nasypka
    uint16_t restart_count = 0;          //kolikrat byl system restartovan
    uint16_t overheat_count = 0;         //kolikrat se hlasilo prehrati
    uint16_t therm_failure_count=0;  //kolikrat se objevila chyba teplomeru
    uint16_t fan_start_count = 0;        //spusteni teplomer
    uint16_t feeder_start_count = 0;     //spusteni feedera
    uint16_t stop_count = 0;             //pocet zastaveni
    uint16_t manual_control_count = 0;   //pocet prepnuti do rucniho rezimu


};

static_assert(sizeof(Counters)<=eeprom_sector_size);


struct Tray {
    uint32_t feeder_time_accum = 0;     //celkovy cas prikladani a spalovani
    uint32_t fuel_kg_accum = 0;         //celkove prilozene palivo
    uint32_t feeder_time_enter_full_power = 0; //cas kdy naposledy doslo k aktivaci plneho vykonu
    uint32_t feeder_time_open_tray = 0; //cas posledniho otevreni zasobniku
    uint32_t feeder_time_last_fill = 0;      //cas podavace, kdy doslo k poslednimu plneni
    uint16_t feeder_speed = 251;        //rychlost podavace s/kg - vypocteno
    int16_t initial_fill_adj = 0;       //pocatecni naplneni - vypocteno
};

static_assert(sizeof(Tray)<=eeprom_sector_size);

struct   Snapshot {
    uint32_t feeder = 0;           //4 feeder snapshot
    uint32_t feeder_low = 0;       //8 feeder_low snapshot
    uint32_t fuel_kg = 0;          //12 fuel_kg_accum snapshot
    uint16_t manual_count = 0;      //14 snapshot manual_control_count
    uint16_t overheat_count = 0;    //16 snapshot overheat_count
    uint16_t therm_fail_count = 0;  //18 snapshot therm_fail_count
    uint16_t restart_count = 0;     //20 snapshot restart_count
    uint16_t day_number = 0;       //22
};

struct DailyData {
    uint8_t _days[eeprom_sector_size] = {};  //denní data, 0-255, jeden bajt, jeden den
};

static_assert(sizeof(Snapshot)<=eeprom_sector_size);
static_assert(sizeof(DailyData)==eeprom_sector_size);


struct TempSensor {
    std::array<uint8_t,8> input_temp;         //address of input temperature sensor
    std::array<uint8_t,8> output_temp;        //address of output temperature sensor
    int8_t input_calib_60 = 0;                    //input temp calibration on 60C
    int8_t output_calib_60 = 0;                   //output temp calibration on 60C
};

static_assert(sizeof(TempSensor)<=eeprom_sector_size);

struct IPAddr {
    uint8_t ip[4] = {};
    bool operator==(const IPAddr &other) const {
        return std::equal(std::begin(ip), std::end(ip), std::begin(other.ip));
    }
    bool operator!=(const IPAddr &other) const {
        return !operator==(other);
    }
};

struct WiFi_NetSettings {
    IPAddr ipaddr = {};
    IPAddr dns = {};
    IPAddr gateway = {};
    IPAddr netmask = {255,255,255,0};
};

static_assert(sizeof(WiFi_NetSettings)<=eeprom_sector_size);




/* rezim kalibrace:
 *
 * V rezimu kalibrace je bag_consump_time == 0,
 * Zahajeni kalibrace je tray_open_time == tray_empty_time
 * Ukonceni kalibrace je tray_open_time > tray_empty_time
 * Provede se vypocet bag_consumpt_time se zadaneho mnozstvi pytlu (z PC)
 *
 *
 * normalni rezim:
 *
 * pokud je feeder_time > tray_open_time
 *     a tray_open_time + default_bag_count * bag_consump_time > tray_empty_time
 *       -> tray_empty_time =  tray_open_time + default_bag_count * bag_consump_time
 *
 * kdykoliv muze uzivatel zmenit zadani poctu pytlu, pak se pocita
 *  z tray_open_time + bag_consump_time * pocet_pytlu
 */
}

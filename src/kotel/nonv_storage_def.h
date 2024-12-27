#pragma once
#include <stdint.h>
#include <array>

constexpr unsigned int file_config = 0;
constexpr unsigned int file_tray = 1;
constexpr unsigned int file_runtime1 = 2;
constexpr unsigned int file_cntrs1 = 3;
constexpr unsigned int file_runtime2 = 4;
constexpr unsigned int file_tempsensor = 5;
constexpr unsigned int file_wifi_ssid = 6;
constexpr unsigned int file_wifi_pwd = 7;
constexpr unsigned int file_wifi_net = 8;
constexpr unsigned int file_cntrs2 = 9;
constexpr unsigned int file_pair_secret = 10;
constexpr unsigned int file_directory_len = 11;

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

struct HeatValue {
    uint8_t heatvalue = 170;
};

struct Config {
    Profile full_power = {8, 20, 60};
    Profile low_power = {5, 30, 40};
    HeatValue heat_value = {170};
    uint8_t input_min_temp = 60;
    uint8_t input_min_temp_samples = 10;
    uint8_t output_max_temp = 85;
    uint8_t output_max_temp_samples = 10;
    uint8_t pump_start_temp = 40;
    uint8_t operation_mode = static_cast<uint8_t>(OperationMode::manual);
    uint8_t fan_pulse_count = 100;
    uint8_t serial_log_out = 0;     //if there is 1, serial output is used for logging
    uint8_t bag_kg = 15;            //velikost pytle
    uint8_t tray_kg = 15*15;        //velikost nasypky v kg
    uint8_t display_intensity = 0;
};


struct Tray {
    uint32_t feeder_time = 0;       //cisty cas podavace (v sec)
    uint32_t tray_open_time = 0;    //cisty cas podavace, kdy doslo k otevreni zasobniku
    uint32_t tray_fill_time = 0;    //cisty cas podavace, kdy doslo k naplneni
    uint16_t feeder_1kg_time = 240; //pocet sekund podavace pro presun 1kg paliva
    uint16_t tray_fill_kg = 0;      //celkove nalozeni v kg
    uint32_t consumed_fuel_kg = 0;  //suma spotrebovaneho paliva (z predchoziho cyklu)

    constexpr uint32_t calc_tray_empty_time() const {
        return tray_fill_time +tray_fill_kg * feeder_1kg_time;
    }

    constexpr uint32_t calc_this_cycle_consumed_fuel(uint32_t reftime) const {
        if (feeder_1kg_time == 0) return 0;
        return (reftime - tray_fill_time) / feeder_1kg_time;;
    }

    constexpr uint32_t calc_this_cycle_consumed_fuel() const {
        return calc_this_cycle_consumed_fuel(feeder_time);
    }

    constexpr uint32_t calc_tray_fill() const {
        if (feeder_1kg_time == 0) return tray_fill_kg;
        uint32_t c = calc_this_cycle_consumed_fuel();
        if (c > tray_fill_kg) return 0;
        else return tray_fill_kg - c;
    }

    ///aktualne zpotrebovane palivo zapise do statistik a posune fill_time podle toho
    constexpr void commit_consumed(uint32_t filltime) {
        //pokud je fill time v minulosti tak nedelej nic
        if (filltime <= tray_fill_time) return;
        //spocitej kolik se spotrebovalo k filltime
        auto c = calc_this_cycle_consumed_fuel(filltime);
        //zapis do statistik
        consumed_fuel_kg += c;
        //uprav tray_fill_kg aby vysledek byl stejny
        //za predpokladu, ze se nespotrebovalo vic nez kolik bylo v zasobniku
        if (c > tray_fill_kg) tray_fill_kg = 0; else tray_fill_kg -= c;
        //posun fill time
        tray_fill_time += c * feeder_1kg_time;
    }

    ///aktualizuje nalozeni,
    /**
     * @param filltime cas podavace vuci kteremu se provadi aktualizace
     * @param increment zmena
     */
    constexpr void update_tray_fill(uint32_t filltime, int increment)  {
        if (feeder_1kg_time == 0) {
            tray_fill_kg= std::max<uint16_t> (tray_fill_kg+increment, 0);
        } else {
            //pokud se pridalo vyznamne vic paliva
            auto c = calc_this_cycle_consumed_fuel(filltime);
            if (static_cast<uint32_t>(increment) > (c >> 1)) {
                //aktualni spotreba se zapocte do statistik
                commit_consumed(filltime);
            }
            //aktualizuj tray_fill_kg
            int32_t r = static_cast<int32_t>(tray_fill_kg)+increment;
            //ale aby nevysel zaporny
            tray_fill_kg = static_cast<uint16_t>(std::max(r, r-r));
        }
    }

    constexpr void set_max_fill(uint32_t filltime, uint32_t max_fill)  {
        auto remain = calc_tray_fill();
        if (remain > max_fill) return;
        auto diff = max_fill - remain;
        update_tray_fill(filltime, diff);
    }

    constexpr uint32_t calc_total_consumed_fuel() const {
        return consumed_fuel_kg + calc_this_cycle_consumed_fuel();
    }

};


struct Runtime {
    uint32_t fan_time = 0;          //jak dlouho bezi ventilator
    uint32_t pump_time = 0;         //jak dlouho bezi cerpadlo
    uint32_t full_power_time = 0;
    uint32_t low_power_time = 0;
    uint32_t cooling_time = 0;
};

struct Runtime2 {
    uint32_t active_time=0;         //jak dlouho to bezi
    uint32_t overheat_time=0;       //jak dlouho byl prehraty
    uint32_t stop_time=0;           //jak dlouho byl v stop stavu
    uint32_t reserved1=0;
    uint32_t reserved2=0;
};


struct Counters1 {
    uint32_t feeder_start_count = 0;  //pocet spusteni podavace
    uint32_t fan_start_count = 0;     //pocet spusteni ventilatoru
    uint32_t pump_start_count = 0;     //pocet spusteni cerpadla
    uint16_t feeder_overheat_count = 0;  //kolikrat se motor prehral
    uint16_t tray_open_count = 0;       //kolikrat se otevrela nasypka
    uint16_t restart_count = 0;           //kolikrat byl system restartovan
    uint16_t overheat_count = 0;

};

struct Counters2 {
    uint32_t full_power_count = 0;
    uint32_t low_power_count = 0;
    uint32_t cool_count = 0;
    uint16_t stop_count = 0;
    uint16_t temp_read_failure_count=0; //kolikrat se objevila chyba teplomeru
    uint16_t reserved = 0;
    uint16_t reserved2 = 0;
};


/*
struct Counters2 {
};
*/
struct TempSensor {
    std::array<uint8_t,8> input_temp;         //address of input temperature sensor
    std::array<uint8_t,8> output_temp;        //address of output temperature sensor

};

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



union StorageSector {
    Config cfg;
    Tray tray;
    Runtime util;
    Counters1 cntr1;
//  Counters2 cntr2;
    TempSensor tempsensor;
    WiFi_NetSettings wifi_cfg;


    StorageSector() {}
    ~StorageSector() {}
};

static_assert(sizeof(StorageSector) == 20);

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

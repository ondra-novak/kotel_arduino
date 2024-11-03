#pragma once
#include <stdint.h>
#include <array>

constexpr unsigned int file_config = 0;
constexpr unsigned int file_tray = 1;
constexpr unsigned int file_util = 2;
constexpr unsigned int file_cntrs1 = 3;
constexpr unsigned int file_tempsensor = 5;
constexpr unsigned int file_wifi_ssid = 6;
constexpr unsigned int file_wifi_pwd = 7;
constexpr unsigned int file_wifi_net = 8;
constexpr unsigned int file_directory_len = 9;

namespace kotel {

struct Config {
    uint8_t feeder_on_sec = 5; //doba zapnuti podavace v sec
    uint8_t feeder_first_on_sec = 15; //prvni doba zapnuti podavace po utlumu
    uint8_t feeder_off_sec = 25; //doba vypnuti podavace v sec
    uint8_t fan_power_pct = 60; //sila ventilatoru 0-100
    uint8_t fan_rundown_sec  = 60; //dobeh ventilatoru
    uint8_t max_atten_min  = 30; //maximalni doba utlumu v minutach
    uint8_t input_min_temp  = 60;  //minimalni teplota na vstupu
    uint8_t output_max_temp = 85;  //maximalni teplota na vystupu
    uint8_t stop_temp = 35;   //teplota pod kterou se vypne automatika a musi se rucne zapnout
    uint8_t pump_temp = 40;   //zapinaci teplota cerpadla
    uint8_t default_bag_count = 15; //vychozi plneni
    uint8_t operation_mode = 1;     //0 - manual, 1 - automatic
    uint8_t fan_pulse = 5;          //delka pulzu ventilátoru ve vlnach 50Hz

};


struct Tray {
    uint32_t feeder_time = 0;       //cisty cas podavace
    uint32_t tray_open_time = 0;    //cisty cas podavace, kdy doslo k otevreni zasobniku
    uint32_t tray_empty_time = 0;   //cisty cas podavace, kdy zasobnik bude prazdny
    uint32_t bag_consump_time = 0;  //cisty cas na spotrebu jednoho pytle
};

struct Utilization {
    uint32_t fan_time = 0;          //jak dlouho bezi ventilator
    uint32_t pump_time = 0;         //jak dlouho bezi cerpadlo
    uint32_t attent_time = 0;       //jak dlouho trval utlum
    uint32_t active_time = 0;       //celkovy cas po kterou je zarizeni v provozu
};

struct Counters1 {
    uint32_t feeder_start_count = 0;  //pocet spusteni podavace
    uint32_t fan_start_count = 0;     //pocet spusteni ventilatoru
    uint32_t pump_start_count = 0;     //pocet spusteni cerpadla
    uint32_t attent_count = 0;        //pocet utlumu
    uint32_t long_attents_count;     //pocet dlouhych utlumu (spusteni na chvili)
};
/*
struct Counters2 {
};
*/
struct TempSensor {
    std::array<uint8_t,8> input_temp;         //address of input temperature sensor
    std::array<uint8_t,8> output_temp;        //address of output temperature sensor
    uint8_t interval = 5;              //reading interval in seconds
    uint8_t trend_smooth = 4;          //kolik vzorku zmen pro vypocet trendu
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
    Utilization util;
    Counters1 cntr1;
//  Counters2 cntr2;
    TempSensor tempsensor;
    WiFi_NetSettings wifi_cfg;

    StorageSector() {}
    ~StorageSector() {}
};


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

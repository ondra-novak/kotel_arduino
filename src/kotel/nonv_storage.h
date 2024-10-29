#pragma once
#include "nonv_storage_def.h"
#include <r4eeprom.h>

#include <algorithm>
namespace kotel {

struct TextSector {
    char text[sizeof(StorageSector)] = {};
    void set(std::string_view txt) {
        auto b = txt.begin();
        auto e = txt.end();
        for (char &c: text) {
            if (b == e) {
                c = '\0';
            } else {
                c = *b;
                ++b;
            }
        }
    }
    std::string_view get() const {
        auto iter = std::find(std::begin(text), std::end(text), '\0');
        return std::string_view(text, std::distance(std::begin(text), iter));
    }
};

struct WiFi_SSID {
    TextSector ssid = {};
};
struct WiFi_Password{
    TextSector password = {};
};

class Storage {
public:


    Config config;
    Tray tray;
    Utilization utlz;
    Counters1 cntr1;
    TempSensor temp;
    WiFi_SSID wifi_ssid;
    WiFi_Password wifi_password;
    WiFi_NetSettings wifi_config;


    void update(const Config &x) {
        _eeprom.update_file(file_config, x);
    }
    void update(const Tray &x) {
        _eeprom.update_file(file_tray, x);
    }
    void update(const Utilization &x) {
        _eeprom.update_file(file_util, x);
    }
    void update(const Counters1 &x) {
        _eeprom.update_file(file_cntrs1, x);
    }
    void update(const TempSensor &x) {
        _eeprom.update_file(file_tempsensor, x);
    }
    void update(const WiFi_SSID &x) {
        _eeprom.update_file(file_wifi_ssid, x);
    }
    void update(const WiFi_Password &x) {
        _eeprom.update_file(file_wifi_pwd, x);
    }
    void update(const WiFi_NetSettings &x) {
        _eeprom.update_file(file_wifi_net, x);
    }

    Storage() {
        _eeprom.read_file(file_config, config);
        _eeprom.read_file(file_tray, tray);
        _eeprom.read_file(file_util, utlz);
        _eeprom.read_file(file_cntrs1, cntr1);
        _eeprom.read_file(file_tempsensor, temp);
        _eeprom.read_file(file_wifi_ssid, wifi_ssid);
        _eeprom.read_file(file_wifi_pwd, wifi_password);
        _eeprom.read_file(file_wifi_net, wifi_config);
    }
    auto get_eeprom() const {
        return _eeprom;
    }


protected:
    EEPROM<sizeof(StorageSector),file_directory_len> _eeprom;


};


}

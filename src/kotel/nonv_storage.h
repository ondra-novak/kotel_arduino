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
        update_config = true;
        config = x;

    }
    void update(const Tray &x) {
        update_tray = true;
        tray = x;
    }
    void update(const Utilization &x) {
        update_utlz = true;
        utlz = x;
    }
    void update(const Counters1 &x) {
        update_counter1 = true;
        cntr1 = x;
    }
    void update(const TempSensor &x) {
        update_temp_sensor = true;
        temp = x;
    }
    void update(const WiFi_SSID &x, const WiFi_Password &y) {
        update_wifi = true;
        wifi_ssid = x;
        wifi_password = y;
    }
    void update(const WiFi_NetSettings &x) {
        update_net_settings = true;
        wifi_config = x;
    }

    void begin() {
        _eeprom.begin();
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

    void flush() {
        if (update_config) _eeprom.update_file(file_config,config);
        if (update_tray) _eeprom.update_file(file_tray,tray);
        if (update_utlz) _eeprom.update_file(file_util,utlz);
        if (update_counter1) _eeprom.update_file(file_cntrs1,cntr1);
        if (update_temp_sensor) _eeprom.update_file(file_tempsensor,temp);
        if (update_wifi) {
            _eeprom.update_file(file_wifi_ssid,wifi_ssid);
            _eeprom.update_file(file_wifi_pwd,wifi_password);
        }
        if (update_net_settings) _eeprom.update_file(file_wifi_net, wifi_config);
        update_config = update_tray = update_utlz = update_counter1
                = update_temp_sensor = update_wifi = update_net_settings = false;
    }

    bool is_wifi_changed() const {
        return update_wifi || update_net_settings;
    }

    void update_all() {
        update_config = update_tray = update_utlz = update_counter1
                = update_temp_sensor = update_wifi = update_net_settings = true;
    }

protected:
    EEPROM<sizeof(StorageSector),file_directory_len> _eeprom;
    bool update_config = false;
    bool update_tray = false;
    bool update_utlz = false;
    bool update_counter1 = false;
    bool update_temp_sensor = false;
    bool update_wifi = false;
    bool update_net_settings = false;



};


}


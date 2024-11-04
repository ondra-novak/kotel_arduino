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
    Status status;
    TempSensor temp;
    WiFi_SSID wifi_ssid;
    WiFi_Password wifi_password;
    WiFi_NetSettings wifi_config;


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

    ///save change sesttings now
    /** This operation is still asynchronous. The controller must call flush() 
     * to perform actual store
     */ 
    void save() {
        _update_flag = true;
    }

    void flush() {
        if (_update_flag) {
            _eeprom.update_file(file_config,config);
            _eeprom.update_file(file_tray,tray);
            _eeprom.update_file(file_util,utlz);
            _eeprom.update_file(file_cntrs1,cntr1);
            _eeprom.update_file(file_tempsensor,temp);
            _eeprom.update_file(file_wifi_ssid,wifi_ssid);
            _eeprom.update_file(file_wifi_pwd,wifi_password);
            _eeprom.update_file(file_wifi_net, wifi_config);
            _update_flag = false;
        }
    }


protected:
    EEPROM<sizeof(StorageSector),file_directory_len> _eeprom;
    bool _update_flag = false;


};


}


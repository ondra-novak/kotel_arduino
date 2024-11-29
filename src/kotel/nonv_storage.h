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

    static char from_hex_digit(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A';
        if (c >= 'a' && c <= 'f') return c - 'a';
        return 2;
    }

    void set_url_dec(std::string_view txt) {
        auto b = txt.begin();
        auto e = txt.end();
        for (char &c: text) {
            if (b == e) {
                c = '\0';
            } else {
                c = *b;
                ++b;
                if (c == '%' && b != e) {
                    c = from_hex_digit(*b);
                    ++b;
                    if (b != e) {
                        c = (c << 4) | from_hex_digit(*b);
                    }
                    ++b;
                }
            }
        }
    }
    std::string_view get() const {
        auto iter = std::find(std::begin(text), std::end(text), '\0');
        return std::string_view(text, std::distance(std::begin(text), iter));
    }
};

struct PasswordSector: TextSector {};

struct WiFi_SSID {
    TextSector ssid = {};
};
struct WiFi_Password{
    PasswordSector password = {};
};



class Storage {
public:


    Config config;
    Tray tray;
    Runtime runtm;
    Counters1 cntr1;
    Runtime2 runtm2;
    Counters2 cntr2;
    TempSensor temp;
    WiFi_SSID wifi_ssid;
    WiFi_Password wifi_password;
    WiFi_Password pair_secret;
    WiFi_NetSettings wifi_config;
    bool pair_secret_need_init = false;


    void begin() {
        _eeprom.begin();
        _eeprom.read_file(file_config, config);
        _eeprom.read_file(file_tray, tray);
        _eeprom.read_file(file_runtime1, runtm);
        _eeprom.read_file(file_runtime2, runtm2);
        _eeprom.read_file(file_cntrs1, cntr1);
        _eeprom.read_file(file_cntrs2, cntr2);
        _eeprom.read_file(file_tempsensor, temp);
        _eeprom.read_file(file_wifi_ssid, wifi_ssid);
        _eeprom.read_file(file_wifi_pwd, wifi_password);
        _eeprom.read_file(file_wifi_net, wifi_config);
        pair_secret_need_init = !_eeprom.read_file(file_pair_secret,pair_secret);
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

    void commit() {
        if (_update_flag) {
            _eeprom.update_file(file_config,config);
            _eeprom.update_file(file_tray,tray);
            _eeprom.update_file(file_runtime1,runtm);
            _eeprom.update_file(file_cntrs1,cntr1);
            _eeprom.update_file(file_runtime2,runtm2);
            _eeprom.update_file(file_cntrs2,cntr2);
            _eeprom.update_file(file_tempsensor,temp);
            _eeprom.update_file(file_wifi_ssid,wifi_ssid);
            _eeprom.update_file(file_wifi_pwd,wifi_password);
            _eeprom.update_file(file_wifi_net, wifi_config);
            _eeprom.update_file(file_pair_secret, pair_secret);
            _update_flag = false;
        }
    }


protected:
    EEPROM<sizeof(StorageSector),file_directory_len> _eeprom;
    bool _update_flag = false;


};


}


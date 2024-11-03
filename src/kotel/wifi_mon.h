#pragma once
#include "timed_task.h"
#include "nonv_storage.h"
#include <WiFiS3.h>
namespace kotel {

class WiFiMonitor: public AbstractTimedTask {
public:

    WiFiMonitor(const Storage &stor):_stor(stor) {
        WiFi.setTimeout(0);
    }

    virtual TimeStampMs get_scheduled_time() const override {
        return _next_run;
    }
    virtual void run(TimeStampMs cur_time) override {
        _next_run =  cur_time + from_seconds(5);

        if (test_changed()) {
            WiFi.disconnect();
            update_changed();
            _connected = false;
            _set_settings  = true;
        } else if (_set_settings) {
            if (_cur_settings.ipaddr != IPAddr{}) {
                WiFi.config(conv_ip(_cur_settings.ipaddr),
                        conv_ip(_cur_settings.dns),
                        conv_ip(_cur_settings.gateway),
                        conv_ip(_cur_settings.netmask)
                );
            }
            char ssid_txt[sizeof(TextSector)+1] = {};
            char password_txt[sizeof(TextSector)+1] = {};

            auto ssid = _cur_ssid.ssid.get();
            auto password = _cur_password.password.get();
            if (!ssid.empty()) {
                *(std::copy(ssid.begin(), ssid.end(), ssid_txt)) = '\0';
                if (password.empty()) {
                    WiFi.begin(ssid_txt);
                } else {
                    *(std::copy(password.begin(), password.end(), password_txt)) = '\0';
                    WiFi.begin(ssid_txt,password_txt);
                }
            }
            _set_settings  = false;
        } else {
            _connected = WiFi.status() == WL_CONNECTED;
        }
    }

    bool is_connected() const {return _connected;}

protected:
    TimeStampMs _next_run = 0;
    bool _connected = true;
    bool _set_settings = true;

    const Storage &_stor;
    WiFi_NetSettings _cur_settings = {};
    WiFi_Password _cur_password = {};
    WiFi_SSID _cur_ssid = {};


    bool test_changed() const {
        return _stor.wifi_ssid.ssid.get() != _cur_ssid.ssid.get()
            || _stor.wifi_password.password.get() != _cur_password.password.get()
            || _stor.wifi_config.ipaddr != _cur_settings.ipaddr
            || _stor.wifi_config.dns != _cur_settings.dns
            || _stor.wifi_config.gateway != _cur_settings.gateway
            || _stor.wifi_config.netmask != _cur_settings.netmask;
    }

    void update_changed() {
        _cur_ssid.ssid=_stor.wifi_ssid.ssid;
        _cur_password.password=_stor.wifi_password.password;
        _cur_settings.ipaddr = _stor.wifi_config.ipaddr;
        _cur_settings.dns = _stor.wifi_config.dns;
        _cur_settings.gateway = _stor.wifi_config.gateway;
        _cur_settings.netmask = _stor.wifi_config.netmask;
    }
    static IPAddress conv_ip(const IPAddr &x) {
        return IPAddress(x.ip[0], x.ip[1], x.ip[2], x.ip[3]);

    }
};

}

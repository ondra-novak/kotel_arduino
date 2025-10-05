#pragma once
#include "wifi/TCPClient.h"
#include "wifi/TCPServer.h"

#include "WiFiS3.h"

#include <vector>
#include <array>



struct WiFiUtils {


    struct AccessPoint {
      std::string ssid;
      std::string bssid;
      std::string rssi;
      std::string channel;
      std::string encryption_mode;
    };



    static bool localIP(IPAddress &local_IP) {
        local_IP = WiFi.localIP();
        return local_IP != IPAddress(0,0,0,0);
    }
    static uint8_t status() {
        return WiFi.status();
    }
    static void reset() {
    }

    struct Scanner {
        TimeStampMs start_tm = 0;
        void begin() {start_tm = get_current_timestamp();}
        bool is_ready() {return get_current_timestamp() - start_tm > 8000;}
        std::vector<AccessPoint> get_result() {
            constexpr auto variants = std::array<std::string_view,8>({
                "WEP",
                "WPA",
                "TKIP"
                "WPA2",
                "WPA2_ENTERPRISE",
                "WPA3",
                "NONE"
            });
            std::vector<AccessPoint> ret;
            int i = 0;
            for (const auto &v : variants) {
                ret.push_back(AccessPoint{"SimAP"+std::to_string(i), {}, std::to_string(-i*10-20),"1",std::string(v)});
                ++i;
            }
            return ret;
        }


    };
};

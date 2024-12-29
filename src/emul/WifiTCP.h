#pragma once
#include "wifi/TCPClient.h"
#include "wifi/TCPServer.h"
#include "WiFiS3.h"

#include <vector>


struct WiFiUtils {


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
        void begin() {}
        bool is_ready() {return true;}
        std::vector<CAccessPoint> get_result() {
            std::vector<CAccessPoint> ret;
            int i = WiFi.scanNetworks();
            for (int j = 0; j < i; j++) {
                ret.push_back({WiFi.SSID(j), {}, {}, {}, {}, {}});
            }
            return ret;
        }


    };
};

#pragma once
#include "wifi/TCPClient.h"
#include "wifi/TCPServer.h"
#include "WiFiS3.h"


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
};

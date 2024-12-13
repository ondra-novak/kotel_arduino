#pragma once
#include "../r4ext/TCPClient.h"
#include "../r4ext/TCPServer.h"
#include "../r4ext/UDPClient.h"

struct WiFiUtils {

    static std::string &modem_cmd(const char *prompt);
    static std::string &modem_res();

    static bool localIP(IPAddress &local_IP);
    static uint8_t status();
    static void reset();


};

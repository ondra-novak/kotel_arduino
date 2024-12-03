#pragma once
#include "TCPClient.h"
#include "TCPServer.h"
#include "UDPClient.h"

struct WiFiUtils {

    static std::string &modem_cmd(const char *prompt);
    static std::string &modem_res();

    static bool localIP(IPAddress &local_IP);
    static uint8_t status();
    static void reset();


};

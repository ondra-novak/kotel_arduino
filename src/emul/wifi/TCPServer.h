#pragma once
#include "WiFiServer.h"

#include "TCPClient.h"
class TCPServer: public WiFiServer {
public:

    using WiFiServer::WiFiServer;



    bool available(TCPClient &cln) {
        WiFiClient x = WiFiServer::available();
        if (x) {
            cln = std::move(x);
            return true;
        }
        return false;
    }
};

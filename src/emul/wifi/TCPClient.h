#pragma once
#include "WiFiClient.h"

class TCPClient: public WiFiClient {
public:
    using WiFiClient::WiFiClient;

    TCPClient(WiFiClient &&x):WiFiClient(std::move(x)) {}
    TCPClient &operator=(WiFiClient &&x) {
        if (this != &x) {
            WiFiClient::operator=(std::move(x));
        }
        return *this;
    }

    void detach() {
        WiFiClient::operator=({});
    }

    int read_nb() {
        if (this->_ctx && !this->_ctx->_data.empty()) {
            char c = _ctx->_data[0];
            _ctx->_data = _ctx->_data.substr(1);
            return static_cast<uint8_t>(c);
        }
        return -1;
    }
};

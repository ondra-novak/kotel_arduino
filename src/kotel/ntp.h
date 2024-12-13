#include <WiFiS3.h>

#pragma once

class NTPClient {
public:


    int request(IPAddress addr, unsigned int port) {
        int r = _client.open(62123);
        if (r == -1) return r;
        _opened =true;
        std::fill(_client.begin(), _client.end(), 0);
        constexpr unsigned char hdr[4] = {0x23,0x00,0x06,0xEC};
        std::copy(std::begin(hdr), std::end(hdr), _client.data());
        r = _client.send(addr, port, _client.size());
        if (r == -1) {
            _client.close();
            return r;
        }
        _opened = true;
        return 0;
    }
    bool is_ready() {
        if (!_opened) return false;
        auto data = _client.receive();
        if (data.empty()) return false;
        constexpr int TRANSMIT_TIMESTAMP_OFFSET = 40;
        constexpr uint32_t NTP_UNIX_EPOCH_DIFF = 2208988800;
        auto bseconds = reinterpret_cast<const unsigned char *>(data.data()+TRANSMIT_TIMESTAMP_OFFSET);

        uint32_t seconds = (uint32_t(1) << 24) * bseconds[0]
                          +(uint32_t(1) << 16) * bseconds[1]
                          +(uint32_t(1) << 8) * bseconds[2]
                          +bseconds[3];
        if (seconds < NTP_UNIX_EPOCH_DIFF) {
            _result = (uint64_t(1) << 32) + seconds;
        } else {
            _result = seconds;
        }
        _result -= NTP_UNIX_EPOCH_DIFF;
        cancel();
        return true;
    }
    uint64_t get_result() const {
        return _result;
    }

    void cancel() {
        if (_opened) {
            _client.close();
            _opened = false;
        }
    }

protected:
    UDPClient<48> _client;
    bool _opened = false;
    uint64_t _result = 0;
};

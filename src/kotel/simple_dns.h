/*
 * simple_dns.h
 *
 *  Created on: 3. 12. 2024
 *      Author: ondra
 */

#ifndef SRC_KOTEL_SIMPLE_DNS_H_
#define SRC_KOTEL_SIMPLE_DNS_H_

#include <api/IPAddress.h>


class SimpleDNS {
public:


    int request(const char *host) {
        int r = _client.open(62124);
        if (r == -1) return r;
        _opened =true;
        auto sz = createDNSQuery(host);
        auto dnsip = WiFi.dnsIP();
        r = _client.send(dnsip, 53, sz);
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
        _result = parseDNSResponse(data);
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
    IPAddress _result = {};
    uint16_t _id = 0;


    struct DNSHeader {
        uint16_t id = 0;       // Identification
        uint16_t flags = 0;    // Flags and codes
        uint16_t qdCount = 0;  // Number of questions
        uint16_t anCount = 0;  // Number of answers
        uint16_t nsCount = 0;  // Number of authority records
        uint16_t arCount = 0;  // Number of additional records
    };

    struct DNSFooter {
        uint16_t qType = 0;
        uint16_t qClass = 0;
    };

    static constexpr uint16_t htons(uint16_t x) {
        return ((x & 0xFF) << 8) | ((x >> 8) & 0xFF);
    }

    template<typename T>
    static char * append(const T &x, char *c) {
        return std::copy(reinterpret_cast<const char *>(&x),
                reinterpret_cast<const char *>(&x)+sizeof(T), c);

    }

    // Function to create a DNS query packet
    unsigned int createDNSQuery(const std::string& domain) {

        // DNS Header
        DNSHeader header = {
                htons(++_id),
                htons(0x0100),
                htons(1)
        };
        DNSFooter footer = {htons(0x01), htons(0x01)};
        char *c = append(header, _client.data());
        // Encode domain name
        std::size_t pos = 0;
        std::size_t lastPos = 0;
        while (pos < domain.size()) {
            pos = std::min(domain.find('.', lastPos),domain.size());
            uint8_t length = static_cast<uint8_t>(pos - lastPos);
            *c++ = static_cast<uint8_t>(length);
            c =std::copy(domain.begin() + lastPos, domain.begin() + pos, c);
            lastPos = pos + 1;
        }
        *c++ = 0;
        c = append(footer, c);
        return c - _client.data();
    }


    IPAddress parseDNSResponse(std::string_view response) {
        IPAddress ret;

        if (response.size() < sizeof(DNSHeader)) return ret;


        // Skip DNS Header
        size_t offset = sizeof(DNSHeader);

        // Skip question section
        while (offset < response.size() && response[offset] != 0x00) {
            offset += response[offset] + 1; // Skip label
        }
        offset += 5; // Null byte + QType and QClass

        // Parse answer section
        while (offset < response.size()) {
            offset += 2; // Name (pointer, usually 0xC0xx)
            uint16_t type = static_cast<uint8_t>(response[offset])*256+
                            static_cast<uint8_t>(response[offset+1]);
            offset += 2; // Type
            offset += 2; // Class
            offset += 4; // TTL
            uint16_t dataLen = static_cast<uint8_t>(response[offset])*256+
                               static_cast<uint8_t>(response[offset+1]);
            offset += 2; // Data length

            if (type == 0x0001 && dataLen == 4) { // Type A, IPv4 address
                uint8_t a1 = static_cast<uint8_t>(response[offset]);
                uint8_t a2 = static_cast<uint8_t>(response[offset+1]);
                uint8_t a3 = static_cast<uint8_t>(response[offset+2]);
                uint8_t a4 = static_cast<uint8_t>(response[offset+3]);
                ret = IPAddress(a1,a2,a3,a4);
                return ret;
            }

            offset += dataLen; // Skip data
        }

        return ret;
    }
};




#endif /* SRC_KOTEL_SIMPLE_DNS_H_ */

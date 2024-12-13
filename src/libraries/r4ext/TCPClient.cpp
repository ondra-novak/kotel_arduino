#include "../r4ext/TCPClient.h"

#include "../r4ext/TCPServer.h"
#include "../r4ext/WifiTCP.h"

#include "WiFiCommands.h"
#include "WiFiTypes.h"
#include "Modem.h"

int TCPClient::connect_timeout = 10000;

using namespace std;


TCPClient::TCPClient(TCPClient &&c) :
        _sock(c._sock), _size(c._size - c._rdpos) {
    std::copy(c._buffer + c._rdpos, c._buffer + c._size, _buffer);
    c._sock = -1;
    c.clear_buffer();
}

TCPClient& TCPClient::operator=(TCPClient &&c) {
    if (this != &c) {
        _sock = c._sock;
        _rdpos = 0;
        _size = c._size - c._rdpos;
        std::copy(c._buffer + c._rdpos, c._buffer + c._size, _buffer);
        c._size = c._rdpos = 0;
        c._sock = -1;
    }
    return *this;
}

void TCPClient::getSocket() {
    if (_sock >= 0 && !connected()) {
        // if sock >= 0 -> it means we were connected, but something happened and we need
        // to reset this socket in order to be able to connect again
        stop();
    }

    if (_sock == -1) {
        string &res = WiFiUtils::modem_res();
        modem.begin();
        if (modem.write(WiFiUtils::modem_cmd(PROMPT(_BEGINCLIENT)), res, "%s",
                CMD(_BEGINCLIENT))) {
            _sock = atoi(res.c_str());
        }
    }
}

int TCPClient::read_nb() {
    if (_rdpos < _size) return static_cast<uint8_t>(_buffer[_rdpos++]);
    return -1;
}

int TCPClient::connect(IPAddress ip, uint16_t port) {

    return connect(ip.toString().c_str(), port);
}

int TCPClient::connect(const char *host, uint16_t port) {
    getSocket();
    if (_sock >= 0) {
        string &res = WiFiUtils::modem_res();
        modem.begin();
        if (connect_timeout) {
            if (modem.write(WiFiUtils::modem_cmd(PROMPT(_CLIENTCONNECT)), res,
                    "%s%d,%s,%d,%d\r\n", CMD_WRITE(_CLIENTCONNECT), _sock, host,
                    port, connect_timeout)) {
                return 1;
            }
        } else {
            if (modem.write(WiFiUtils::WiFiUtils::modem_cmd(PROMPT(_CLIENTCONNECTNAME)), res,
                    "%s%d,%s,%d\r\n", CMD_WRITE(_CLIENTCONNECTNAME), _sock,
                    host, port)) {
                return 1;
            }
        }
    }
    return 0;
}

size_t TCPClient::write(uint8_t b) {

    return write(&b, 1);
}

size_t TCPClient::write(const uint8_t *buf, size_t size) {

    if (_sock >= 0) {
        string &res = WiFiUtils::modem_res();
        modem.begin();
        modem.write_nowait(WiFiUtils::modem_cmd(PROMPT(_CLIENTSEND)), res, "%s%d,%d\r\n",
                CMD_WRITE(_CLIENTSEND), _sock, size);
        if (modem.passthrough(buf, size)) {
            return size;
        } else {
            return 0;
        }
    }
    return 0;

}

int TCPClient::available() {

    if (_size > _rdpos)
        return _size - _rdpos;
    if (_sock < 0)
        return 0;
    clear_buffer();

    string &res = WiFiUtils::modem_res();
    modem.begin();

    if(modem.write(string(PROMPT(_AVAILABLE)),res, "%s%d\r\n" , CMD_WRITE(_AVAILABLE),_sock)) {
       int rv = atoi(res.c_str());
       if (rv <=0) {
          return 0;
       }
    }


    /* important - it works one shot */
    modem.avoid_trim_results();
    modem.read_using_size();
    if (modem.write(WiFiUtils::modem_cmd(PROMPT(_CLIENTRECEIVE)), res, "%s%d,%d\r\n",
            CMD_WRITE(_CLIENTRECEIVE), _sock, buffer_size)) {
        std::copy(res.begin(), res.end(), _buffer);
        _size = res.size();
    }

    return _size - _rdpos;
}

int TCPClient::read() {

    uint8_t b;
    if (TCPClient::read(&b, 1) == 1) {
        return b;
    }
    return -1;
}

int TCPClient::read(uint8_t *buf, size_t size) {
    size = std::min<std::size_t>(TCPClient::available(), size);
    if (size)
        std::copy(_buffer + _rdpos, _buffer + _rdpos + size, buf);
    _rdpos += size;
    return size;
}

int TCPClient::peek() {
    if (_rdpos < _size)
        return _buffer[_rdpos];
    int rv = -1;
    if (_sock >= 0) {
        string &res = WiFiUtils::modem_res();
        modem.begin();
        if (modem.write(WiFiUtils::modem_cmd(PROMPT(_PEEK)), res, "%s%d\r\n",
                CMD_WRITE(_PEEK), _sock)) {
            rv = atoi(res.c_str());
        }
    }
    return rv;
}

void TCPClient::flush() {

    if (_sock >= 0) {
        string &res = WiFiUtils::modem_res();
        modem.begin();
        modem.write(WiFiUtils::modem_cmd(PROMPT(_CLIENTFLUSH)), res, "%s%d\r\n",
                CMD_WRITE(_CLIENTFLUSH), _sock);
    }
}

void TCPClient::stop() {

    if (_sock >= 0) {
        string &res = WiFiUtils::modem_res();
        modem.begin();
        modem.write(WiFiUtils::modem_cmd(PROMPT(_CLIENTCLOSE)), res, "%s%d\r\n",
                CMD_WRITE(_CLIENTCLOSE), _sock);
        _sock = -1;
    }
    clear_buffer();
}

uint8_t TCPClient::connected() {
    uint8_t rv = 0;
    if (available() > 0)
        return 1;
    if (_sock >= 0) {
        string &res = WiFiUtils::modem_res();
        modem.begin();
        if (modem.write(WiFiUtils::modem_cmd(PROMPT(_CLIENTCONNECTED)), res, "%s%d\r\n",
                CMD_WRITE(_CLIENTCONNECTED), _sock)) {
            rv = atoi(res.c_str());
        }
    }

    return rv;
}

bool TCPClient::operator==(const TCPClient &whs) {
    return _sock == whs._sock;
}
bool TCPClient::operator!=(const TCPClient &whs) {
    return _sock != whs._sock;
}

IPAddress TCPClient::remoteIP() {
    IPAddress ip;
    if (_sock >= 0) {
        string &res = WiFiUtils::modem_res();
        modem.begin();
        if (modem.write(WiFiUtils::modem_cmd(PROMPT(_REMOTEIP)), res, "%s%d\r\n",
                CMD_WRITE(_REMOTEIP), _sock)) {
            ip.fromString(res.c_str());
            return ip;
        }
    }
    return IPAddress(0, 0, 0, 0);
}

uint16_t TCPClient::remotePort() {

    uint16_t rv = 0;
    if (_sock >= 0) {
        string &res = WiFiUtils::modem_res();
        modem.begin();
        if (modem.write(WiFiUtils::modem_cmd(PROMPT(_REMOTEPORT)), res, "%s%d\r\n",
                CMD_WRITE(_REMOTEPORT), _sock)) {
            rv = atoi(res.c_str());
            return rv;
        }
    }
    return rv;
}

#include "WiFiClient.h"

#include <fcntl.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <unistd.h>
WiFiClient::WiFiClient() {}

WiFiClient::WiFiClient(std::shared_ptr<Context> ctx):_ctx(ctx) {}

int WiFiClient::connect(IPAddress , uint16_t ) {
    throw std::runtime_error("unsupported");
}

int WiFiClient::connect(const char *, uint16_t ) {
    throw std::runtime_error("unsupported");
}

size_t WiFiClient::write(uint8_t unsignedChar) {
    return write(&unsignedChar, 1);
}

size_t WiFiClient::write(const uint8_t *buf, size_t size) {
    auto ret = size;
    while (size) {
        int r = ::send(_ctx->_sock, buf, size, 0);
        if (r < 1){
            _ctx->_connected = false;
            return 0;
        } else {
            size -= r;
            buf += size;
        }
    }
    return ret;
}

int WiFiClient::available() {
    if (_ctx == nullptr) return 0;
    if (_ctx->_data.empty()) {
        pollfd fd;
        fd.events = POLLIN;
        fd.fd = _ctx->_sock;
        fd.revents = 0;
        if (poll(&fd, 1, 0) <= 0) {
            return 0;
        }
        do {
            int r =::recv(_ctx->_sock,_ctx->_buff,sizeof(_ctx->_buff),0);
            if (r > 0) {
                _ctx->_data = std::string_view(_ctx->_buff, r);
                break;
            } else if (r == 0) {
                _ctx->_connected = false;
                break;
            } else {
                int e = errno;
                if (e == EINTR) continue;
                _ctx->_connected = false;
                break;
            }
        } while (true);
        return _ctx->_data.size();
    } else {
        return _ctx->_data.size();
    }
}

int WiFiClient::read() {
    uint8_t b;
    if (read(&b, 1) == 1) return b;
    return -1;

}

int WiFiClient::read(uint8_t *buf, size_t size) {
    if (!available() || !_ctx->_connected) return 0;
    if (size == 0) return _ctx->_data.size();
    auto sz = std::min(size, _ctx->_data.size());
    auto sub = _ctx->_data.substr(0, sz);
    std::copy(sub.begin(), sub.end(), buf);
    _ctx->_data = _ctx->_data.substr(sz);
    return sub.size();
}


int WiFiClient::peek() {
    int r = read();
    if (r>=0) {
        put_back_byte();
    }
    return r;
}

void WiFiClient::flush() {
    //not implemented
}

void WiFiClient::stop() {
    if (_ctx) {
        shutdown(_ctx->_sock, SHUT_WR);
        _ctx->_connected = false;
        _ctx.reset();
    }
}

uint8_t WiFiClient::connected()  {
    return _ctx->_connected?1:0;
}

bool WiFiClient::operator ==(const WiFiClient& other) {
    return _ctx->_sock == other._ctx->_sock;
}

IPAddress WiFiClient::remoteIP() {
    throw std::runtime_error("not implemented");
}

uint16_t WiFiClient::remotePort() {
    throw std::runtime_error("not implemented");
}

void WiFiClient::put_back_byte() {
    _ctx->_data = std::string_view(_ctx->_data.data()-1, _ctx->_data.size()+1);
}

WiFiClient::Context::~Context() {
    if (_sock !=-1) ::close(_sock);
}

#include "WiFiServer.h"

#include <algorithm>
#include <cerrno>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <system_error>
#include <unistd.h>
WiFiServer::WiFiServer():_port(0) {

}

WiFiServer::WiFiServer(int p):_port(p<1024?p+8000:p) {
}

WiFiClient WiFiServer::available() {
    if (!_ctx) return {};
    cleanup();
    auto iter = std::find_if(_ctx->_active_connections.begin(),
            _ctx->_active_connections.end(),
            [&](WiFiClient c) {return c.available();});

    if (iter == _ctx->_active_connections.end()) {
        return accept();
    }
    return WiFiClient(*iter);
}

WiFiClient WiFiServer::accept() {
    sockaddr_storage stor;
    socklen_t slen = sizeof(stor);
    int s = accept4(_ctx->_sock,reinterpret_cast<sockaddr *>(&stor), &slen,SOCK_CLOEXEC);
    if (s < 0) {
        return WiFiClient();
    }
    auto ctx = std::make_shared<WiFiClient::Context>(s);
    _ctx->_active_connections.push_back(ctx);
    return WiFiClient(ctx);
}

void WiFiServer::begin(int port) {
    _port = port;
    begin();
}

void WiFiServer::begin() {
    sockaddr_in sin={};
    sin.sin_family = AF_INET;
    sin.sin_port = htons(_port);
    sin.sin_addr.s_addr = INADDR_ANY;

    int s = socket(AF_INET,SOCK_STREAM|SOCK_CLOEXEC|SOCK_NONBLOCK,IPPROTO_TCP);
    if (s<0) throw std::system_error(errno, std::system_category());
    try {
        int opt = 1;
        if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
            throw std::system_error(errno, std::system_category());
        }
        if (bind(s,reinterpret_cast<const sockaddr *>(&sin), sizeof(sin)) == -1) {
            throw std::system_error(errno, std::system_category());
        }
        if (listen(s, SOMAXCONN) == -1) {
            throw std::system_error(errno, std::system_category());
        }
        _ctx = std::make_shared<Context>(s);
    } catch(...) {
        ::close(s);
        throw;
    }
}

size_t WiFiServer::write(uint8_t unsignedChar) {
    for (WiFiClient c: _ctx->_active_connections) {
        c.write(unsignedChar);
    }
    return 1;
}

size_t WiFiServer::write(const uint8_t *buf, size_t size) {
    for (WiFiClient c: _ctx->_active_connections) {
        c.write(buf, size);
    }
    return size;
}

void WiFiServer::end() {
    _ctx = nullptr;
}

WiFiServer::operator bool() {
    return !!_ctx;
}

bool WiFiServer::operator ==(const WiFiServer& other) {
    return _ctx == other._ctx;
}

void WiFiServer::cleanup() {
    auto end = std::remove_if(_ctx->_active_connections.begin(),
            _ctx->_active_connections.end(),
            [&](WiFiClient c) {return !c.connected();});
    _ctx->_active_connections.erase(end, _ctx->_active_connections.end());

}

WiFiServer::Context::~Context() {
    close(_sock);
}

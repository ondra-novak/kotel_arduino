#pragma once
#include <string_view>
#include "api/IPAddress.h"

class UDPClientBase {
public:
    int  open(uint16_t port);
    int send(IPAddress ip, uint16_t port, const std::string_view &data);
    int send(const char *host, uint16_t port, const std::string_view &data);
    std::size_t receive(char *buffer, std::size_t buffer_size);
    void close();

protected:
    int _sock;
};


template<unsigned int buffer_size>
class UDPClient: public UDPClientBase {
public:

    using UDPClientBase::receive;
    using UDPClientBase::send;


    std::string_view receive() {
        auto sz = UDPClientBase::receive(_buffer, buffer_size);
        return {_buffer, sz};
    }

    static constexpr std::size_t size()  {return buffer_size;}
    char *data() {return _buffer;}
    const char *data() const {return _buffer;}

    auto begin() {return data();}
    const auto begin() const  {return data();}
    auto end() {return data()+size();}
    const auto end() const  {return data()+size();}

    int send(IPAddress ip, uint16_t port, std::size_t sz) {
        return UDPClientBase::send(ip,port,{_buffer, sz});
    }
    int send(const char *host, uint16_t port, std::size_t sz) {
        return UDPClientBase::send(host,port,{_buffer, sz});
    }




protected:
    char _buffer[buffer_size];

};

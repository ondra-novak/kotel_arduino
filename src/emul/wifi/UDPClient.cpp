#include "UDPClient.h"

#include <cstdio>
#include <netinet/in.h>
#include <netdb.h>
#include <stdexcept>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <csignal>

int  UDPClientBase::open(uint16_t port) {
   _sock = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
   if (_sock < 0) {
       return -1;
   }

   sockaddr_in localAddr{};
   localAddr.sin_family = AF_INET;
   localAddr.sin_addr.s_addr = INADDR_ANY;
   localAddr.sin_port = htons(port);

   if (bind(_sock, reinterpret_cast<sockaddr*>(&localAddr), sizeof(localAddr)) < 0) {
       close();
       return -1;
   }
   return 0;
}

int UDPClientBase::send(IPAddress ip, uint16_t port, const std::string_view &data) {
    sockaddr_in remoteAddr{};
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(port);

    uint32_t ipAsInt = (ip[0] << 24) | (ip[1] << 16) | (ip[2] << 8) | ip[3];
    remoteAddr.sin_addr.s_addr = htonl(ipAsInt);

    int bytesSent = sendto(_sock, data.data(), data.size(), 0,
                           reinterpret_cast<sockaddr*>(&remoteAddr), sizeof(remoteAddr));
    if (bytesSent < 0) {
        perror("Send failed");
        return -1;
    }

    return bytesSent;
}

std::size_t UDPClientBase::receive(char *buffer, std::size_t buffer_size) {
    sockaddr_in senderAddr{};
    socklen_t addrLen = sizeof(senderAddr);

    ssize_t bytesRead = recvfrom(_sock, buffer, buffer_size, 0,
                                 reinterpret_cast<sockaddr*>(&senderAddr), &addrLen);
    if (bytesRead < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return 0;
        }
        perror("Receive failed");
        return -1;
    }
    return bytesRead;
}

int UDPClientBase::send(const char *host, uint16_t port,
        const std::string_view &data) {
       addrinfo hints{};
       addrinfo *res = nullptr;
       hints.ai_family = AF_INET; // Pouze IPv4
       hints.ai_socktype = SOCK_DGRAM;

       int status = getaddrinfo(host, nullptr, &hints, &res);
       if (status != 0) {
           return -1;
       }

       sockaddr_in remoteAddr{};
       remoteAddr.sin_family = AF_INET;
       remoteAddr.sin_port = htons(port);

       if (res->ai_family == AF_INET) {
           sockaddr_in *addr = reinterpret_cast<sockaddr_in*>(res->ai_addr);
           remoteAddr.sin_addr = addr->sin_addr;
       } else {
           freeaddrinfo(res);
           return -1;
       }

       freeaddrinfo(res);

       int bytesSent = sendto(_sock, data.data(), data.size(), 0,
                              reinterpret_cast<sockaddr*>(&remoteAddr), sizeof(remoteAddr));
       if (bytesSent < 0) {
           return -1;
       }

       return bytesSent;
}

void UDPClientBase::close() {
    if (_sock >= 0) {
        ::close(_sock);
        _sock = -1;
    }
}

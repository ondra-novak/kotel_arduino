#pragma once


#include "../r4ext/TCPClient.h"
#include "api/Server.h"

class UDPClientBase;


class TCPServer : public arduino::Server {
private:
  int _sock;
  int _port;

public:
  TCPServer();
  TCPServer(int p);
  bool available(TCPClient &cl);
  bool accept(TCPClient &cl);
  void begin(int port);
  void begin();
  virtual size_t write(uint8_t);
  virtual size_t write(const uint8_t *buf, size_t size);
  void end();
  explicit operator bool();
  virtual bool operator==(const TCPServer&);
  virtual bool operator!=(const TCPServer& whs)
  {
    return !this->operator==(whs);
  };

  using Print::write;

  friend class TCPClient;


};


#pragma once

#include "api/Client.h"
#include "api/IPAddress.h"

#include <memory>

class TCPClient : public arduino::Client {
public:
    static constexpr unsigned int buffer_size = 256;

  TCPClient() = default;
  TCPClient(int s):_sock(s) {}
  TCPClient(const TCPClient& c) = delete;
  TCPClient(TCPClient&& c);
  TCPClient &operator=(TCPClient&& c);
  virtual int connect(IPAddress ip, uint16_t port) override;
  virtual int connect(const char *host, uint16_t port) override;
  virtual size_t write(uint8_t) override;
  virtual size_t write(const uint8_t *buf, size_t size) override;
  virtual int available() override;
  virtual int read() override;
  virtual int read(uint8_t *buf, size_t size) override;
  virtual int peek() override;
  virtual void flush() override;
  virtual void stop() override;
  virtual uint8_t connected() override;
  virtual operator bool() override {
    return _sock != -1;
  }
  bool operator==(const TCPClient&) ;
  bool operator!=(const TCPClient& whs);
  IPAddress remoteIP();
  uint16_t remotePort();

  int read_nb();

  static int connect_timeout;

  friend class TCPServer;

  using Print::write;
  void clear_buffer() {_size = 0;_rdpos = 0;}
  void detach() {_sock = -1; clear_buffer();}
  bool empty_buffer() const {return _size == _rdpos;}

protected:
  int _sock = -1;
  char _buffer[buffer_size] = {};
  unsigned int _size = 0;
  unsigned int _rdpos = 0;

  void getSocket();


};




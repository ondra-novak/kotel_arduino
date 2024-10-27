#pragma once


#include <api/Server.h>
#include "WiFiClient.h"
#include <vector>


class WiFiServer : public arduino::Server {
private:


  struct Context {
      int _sock;
      std::vector<std::shared_ptr<WiFiClient::Context> > _active_connections;
      Context(int s):_sock(s) {}
      ~Context();
  };



public:
  WiFiServer();
  WiFiServer(int p);
  WiFiClient available();
  WiFiClient accept();
  void begin(int port);
  void begin();
  virtual size_t write(uint8_t);
  virtual size_t write(const uint8_t *buf, size_t size);
  void end();
  explicit operator bool();
  virtual bool operator==(const WiFiServer&);
  virtual bool operator!=(const WiFiServer& whs)
  {
    return !this->operator==(whs);
  };

  using Print::write;

  friend class WiFiClient;
private:
  int _port;

  std::shared_ptr<Context> _ctx;

  void cleanup();
};

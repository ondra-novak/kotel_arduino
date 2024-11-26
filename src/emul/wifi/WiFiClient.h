#pragma once

#include <api/Print.h>
#include <api/Client.h>
#include <api/IPAddress.h>

#include <memory>
#include <string_view>





class WiFiClient : public arduino::Client {
public:

   struct Context {
       int _sock = -1;
       bool _connected = true;
       char _buff[2048] = {};
       std::string_view _data = {};
       Context(int sock):_sock(sock) {}
       ~Context();
   };


  WiFiClient();
  WiFiClient(std::shared_ptr<Context> ctx);
  virtual int connect(IPAddress ip, uint16_t port);
  virtual int connect(const char *host, uint16_t port);
  virtual size_t write(uint8_t);
  virtual size_t write(const uint8_t *buf, size_t size);
  virtual int available();
  virtual int read();
  virtual int read(uint8_t *buf, size_t size);
  virtual int peek();
  virtual void flush();
  virtual void stop();
  virtual uint8_t connected() ;
  virtual operator bool() {
    return !!_ctx;
  }
  virtual bool operator==(const WiFiClient&);
  virtual bool operator!=(const WiFiClient& whs)
  {
      return !this->operator==(whs);
  };
  virtual IPAddress remoteIP();
  virtual uint16_t remotePort();


  friend class WiFiServer;

  using Print::write;

protected:
  std::shared_ptr<Context> _ctx;

  void put_back_byte();

};



#include "../r4ext/TCPServer.h"

#include "../r4ext/WifiTCP.h"

#include "WiFiCommands.h"
#include "WiFiTypes.h"
#include "Modem.h"


using namespace std;



/* -------------------------------------------------------------------------- */
TCPServer::TCPServer() : _sock(-1), _port(80) {}
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
TCPServer::TCPServer(int p) : _sock(-1), _port(p) {}
/* -------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------- */
bool TCPServer::available(TCPClient &cl) {
/* -------------------------------------------------------------------------- */
   if (!cl.empty_buffer()) return true;

   if(_sock != -1) {
      string &res = WiFiUtils::modem_res();
      modem.begin();
      /* call the server available on esp so that the accept is performed */
      if(modem.write(WiFiUtils::modem_cmd(PROMPT(_SERVERAVAILABLE)),res, "%s%d\r\n" , CMD_WRITE(_SERVERAVAILABLE), _sock)) {
         int client_sock = atoi(res.c_str());
         cl._sock = client_sock;
         return cl._sock >=0;
      }
   }
   return TCPClient();
}

/* -------------------------------------------------------------------------- */
bool TCPServer::accept(TCPClient &cl) {
/* -------------------------------------------------------------------------- */
   if(_sock != -1) {
       string &res = WiFiUtils::modem_res();
      modem.begin();
      /* call the server accept on esp so that the accept is performed */
      if(modem.write(WiFiUtils::modem_cmd(PROMPT(_SERVERACCEPT)),res, "%s%d\r\n" , CMD_WRITE(_SERVERACCEPT), _sock)) {
          int client_sock = atoi(res.c_str());
          if (client_sock >= 0) {
              cl._sock = client_sock;
              cl.clear_buffer();
              return true;
          } else {
              return false;
          }
      }
   }
   return TCPClient();
}

/* -------------------------------------------------------------------------- */
void TCPServer::begin(int port) {
/* -------------------------------------------------------------------------- */
   if(_sock == -1) {
       string &res = WiFiUtils::modem_res();
      modem.begin();
      if(modem.write(WiFiUtils::modem_cmd(PROMPT(_BEGINSERVER)),res, "%s%d\r\n" , CMD_WRITE(_BEGINSERVER), port)) {
         _sock = atoi(res.c_str());
      }
   }
}

/* -------------------------------------------------------------------------- */
void TCPServer::begin() {
/* -------------------------------------------------------------------------- */
   begin(_port);
}



/* -------------------------------------------------------------------------- */
size_t TCPServer::write(uint8_t b){
/* -------------------------------------------------------------------------- */
   return write(&b, 1);
}

/* -------------------------------------------------------------------------- */
size_t TCPServer::write(const uint8_t *buf, size_t size) {
/* -------------------------------------------------------------------------- */
   if(_sock >= 0) {
       string &res = WiFiUtils::modem_res();
      modem.begin();
      modem.write_nowait(WiFiUtils::modem_cmd(PROMPT(_SERVERWRITE)),res, "%s%d,%d\r\n" , CMD_WRITE(_SERVERWRITE), _sock, size);
      if(modem.passthrough(buf,size)) {
         return size;
      }

   }
   return 0;
}

/* -------------------------------------------------------------------------- */
void TCPServer::end() {
/* -------------------------------------------------------------------------- */
   if(_sock != -1) {
       string &res = WiFiUtils::modem_res();
      modem.begin();
      modem.write(WiFiUtils::modem_cmd(PROMPT(_SERVEREND)),res, "%s%d\r\n" , CMD_WRITE(_SERVEREND), _sock);
      _sock = -1;
   }
}

TCPServer::operator bool()
{
   return (_sock != -1);
}

bool TCPServer::operator==(const TCPServer& whs)
{
       return _sock == whs._sock;
}

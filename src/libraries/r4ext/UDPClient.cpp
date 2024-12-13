#include "../r4ext/UDPClient.h"

#include "../r4ext/WifiTCP.h"

#include "WiFiCommands.h"
#include "WiFiTypes.h"
#include "Modem.h"



using namespace std;


int UDPClientBase::open(uint16_t p) {
   if(_sock == -1) {
      string &res = WiFiUtils::modem_res();
      modem.begin();
      if(modem.write(WiFiUtils::modem_cmd(PROMPT(_UDPBEGIN)),res, "%s%d\r\n" , CMD_WRITE(_UDPBEGIN),p)) {
         _sock = atoi(res.c_str());
         return 0;
      }
   }
   return -1;
}

void UDPClientBase::close() {
    if(_sock >= 0) {
       string &res = WiFiUtils::modem_res();
       modem.begin();
       modem.write(WiFiUtils::modem_cmd(PROMPT(_UDPSTOP)),res, "%s%d\r\n" , CMD_WRITE(_UDPSTOP), _sock);
       _sock = -1;
    }

}

int UDPClientBase::send(IPAddress ip, uint16_t port, const std::string_view &data) {
    if (!beginPacket(ip, port)) return -1;
    if (write(reinterpret_cast<const unsigned char *>(data.data()), data.size()) != data.size()) return -2;
    if (!endPacket()) return -3;
    return data.size();
}
int UDPClientBase::send(const char *host, uint16_t port, const std::string_view &data) {
    if (!beginPacket(host, port)) return -1;
    if (write(reinterpret_cast<const unsigned char *>(data.data()), data.size()) != data.size()) return -2;
    if (!endPacket()) return -3;
    return data.size();
}
std::size_t UDPClientBase::receive(char *buffer, std::size_t buffer_size) {
    if (!parsePacket()) return 0;
    return read(buffer, buffer_size);
}


int UDPClientBase::beginPacket(IPAddress ip, uint16_t p) {
   if(_sock >= 0) {
      string &res = WiFiUtils::modem_res();
      modem.begin();
      if(modem.write(WiFiUtils::modem_cmd(PROMPT(_UDPBEGINPACKETIP)),res, "%s%d,%d,%s\r\n" , CMD_WRITE(_UDPBEGINPACKETIP),_sock,p,ip.toString().c_str())) {
         return 1;
      }
   }
   return 0;
}

int UDPClientBase::beginPacket(const char *host, uint16_t p) {
   if(_sock >= 0) {
      string &res = WiFiUtils::modem_res();
      modem.begin();
      if(modem.write(WiFiUtils::modem_cmd(PROMPT(_UDPBEGINPACKETNAME)),res, "%s%d,%d,%s\r\n" , CMD_WRITE(_UDPBEGINPACKETNAME),_sock,p,host)) {
         return 1;
      }
   }
   return 0;
}


int UDPClientBase::endPacket() {
   if(_sock >= 0) {
      string &res = WiFiUtils::modem_res();
      modem.begin();
      if(modem.write(WiFiUtils::modem_cmd(PROMPT(_UDPENDPACKET)),res, "%s%d\r\n" , CMD_WRITE(_UDPENDPACKET), _sock)) {
         return 1;
      }
   }
   return 0;
}


size_t UDPClientBase::write(const uint8_t *buf, size_t size) {
   if(_sock >= 0) {
      string &res = WiFiUtils::modem_res();
      modem.begin();
      modem.write_nowait(WiFiUtils::modem_cmd(PROMPT(_UDPWRITE)),res, "%s%d,%d\r\n" , CMD_WRITE(_UDPWRITE), _sock, size);
      if(modem.passthrough(buf,size)) {
         return size;
      }

   }
   return 0;
}

int UDPClientBase::parsePacket() {
   if(_sock >= 0) {
      string &res = WiFiUtils::modem_res();
      modem.begin();
      if(modem.write(WiFiUtils::modem_cmd(PROMPT(_UDPPARSE)),res, "%s%d\r\n" , CMD_WRITE(_UDPPARSE), _sock)) {
         return atoi(res.c_str());
      }
   }
   return 0;
}


int UDPClientBase::read(char *buffer, std::size_t size) {
   int rv = -1;
   if(_sock >= 0) {
      string &res = WiFiUtils::modem_res();
      modem.begin();

      /* important - it works one shot */
      modem.avoid_trim_results();
      modem.read_using_size();
      if(modem.write(WiFiUtils::modem_cmd(PROMPT(_UDPREAD)),res, "%s%d,%d\r\n" , CMD_WRITE(_UDPREAD), _sock, size)) {
         if(res.size() > 0) {
            for(std::size_t i = 0; i < size && i < res.size(); i++) {
                buffer[i] = res[i];
            }
            rv = res.size();
         }
         else {
            rv = 0;
         }
      }
   }
   return rv;
}

void UDPClientBase::flush() {
   if(_sock >= 0) {
      string &res = WiFiUtils::modem_res();
      modem.begin();
      modem.write(WiFiUtils::modem_cmd(PROMPT(_UDPFLUSH)),res, "%s%d\r\n" , CMD_WRITE(_UDPFLUSH), _sock);
   }
}


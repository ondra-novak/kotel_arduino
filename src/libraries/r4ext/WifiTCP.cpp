#include "../r4ext/WifiTCP.h"

#include "Modem.h"

std::string& WiFiUtils::modem_cmd(const char *prompt) {
    static std::string str;
    str.clear();
    str.append(prompt);
    return str;
}

std::string& WiFiUtils::modem_res() {
    static std::string str;
    str.clear();
    return str;
}

bool WiFiUtils::localIP(IPAddress &local_IP) {
    modem.begin();
    std::string &res =modem_res();
    IPAddress empty_IP(0, 0, 0, 0);
    local_IP = empty_IP;

    if (modem.write(modem_cmd(PROMPT(_MODE)), res, "%s", CMD_READ(_MODE))) {
        if (atoi(res.c_str()) == 1) {
            if (modem.write(modem_cmd(PROMPT(_IPSTA)), res, "%s%d\r\n",
                    CMD_WRITE(_IPSTA), IP_ADDR)) {

                local_IP.fromString(res.c_str());

            }
        } else if (atoi(res.c_str()) == 2) {
            if (modem.write(modem_cmd(PROMPT(_IPSOFTAP)), res, CMD(_IPSOFTAP))) {

                local_IP.fromString(res.c_str());
            }
        }
    }

    return local_IP != empty_IP;
}

uint8_t WiFiUtils::status() {
   modem.begin();
   modem.timeout(2000);
   while (Serial2.available()) Serial2.read();
   std::string &res = modem_res();
   if(modem.write(modem_cmd(PROMPT(_GETSTATUS)), res, CMD_READ(_GETSTATUS))) {
      return atoi(res.c_str());
   }
   return 0;
}

void WiFiUtils::reset() {
    modem.begin();
    std::string &res =modem_res();
    modem.write(modem_cmd(PROMPT(_SOFTRESETWIFI)),res, "%s" , CMD(_SOFTRESETWIFI));
}




void WiFiUtils::Scanner::begin() {
    modem.begin();
    modem.timeout(0);
    std::string &res =modem_res();
    modem.write(modem_cmd(PROMPT(_WIFISCAN)),res,CMD(_WIFISCAN));
}

bool WiFiUtils::Scanner::is_ready() {
    return Serial2.available();
}

std::vector<CAccessPoint> WiFiUtils::Scanner::get_result() {
    std::vector<CAccessPoint> out;
    modem.timeout(4500);

    modem.avoid_trim_results();
    modem.read_using_size();

    std::vector<std::string> aps;
    std::string &res =modem_res();
    if (modem.write(modem_cmd(PROMPT(_WIFISCAN)),res,"")) {
       split(aps, res, "\r\n");
       std::vector<std::string> tokens;
       for(uint16_t i = 0; i < aps.size(); i++) {
          CAccessPoint ap;
          tokens.clear();
          split(tokens, aps[i], "|");
          if(tokens.size() >= 5) {
             ap.ssid            = tokens[0];
             ap.bssid           = tokens[1];
 //            macStr2macArray(ap.uint_bssid, ap.bssid.c_str());
             ap.rssi            = tokens[2];
             ap.channel         = tokens[3];
             ap.encryption_mode = tokens[4];
             out.push_back(ap);
          }
       }
    }
    return out;
}

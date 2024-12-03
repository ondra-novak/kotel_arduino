#include "Modem.h"
#include "WifiTCP.h"

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
   modem.timeout(1000);
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




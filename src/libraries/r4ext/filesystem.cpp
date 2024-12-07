#include "filesystem.h"
#include "Modem.h"
#include "WifiTCP.h"

bool ESP32Filesystem::begin() {
   modem.begin();
   std::string &res = WiFiUtils::modem_res();
   return !!modem.write(WiFiUtils::modem_cmd(PROMPT(_MOUNTFS)), res, "%s%d\r\n" , CMD_WRITE(_MOUNTFS), 1);
}


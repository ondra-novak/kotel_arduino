#include "../WiFiS3.h"

#include <thread>

CWifi WiFi;

CWifi::CWifi():_timeout(10000) {
}

int CWifi::begin(const char *) {
    std::this_thread::sleep_for(std::chrono::milliseconds(_timeout));
    return WL_IDLE_STATUS;
}

int CWifi::begin(const char *, const char *) {
    std::this_thread::sleep_for(std::chrono::milliseconds(_timeout));
    return WL_IDLE_STATUS;
}

void CWifi::config(IPAddress ) {
}

void CWifi::config(IPAddress , IPAddress , IPAddress , IPAddress ) {
}

int CWifi::disconnect(void) {
    return WL_DISCONNECTED;
}

uint8_t CWifi::status() {
    return WL_CONNECTED;
}

void CWifi::setTimeout(unsigned long timeout) {
    _timeout = timeout;
}

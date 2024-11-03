#include "../WiFiS3.h"

#include <thread>

CWifi WiFi;

CWifi::CWifi():_timeout(10000) {
}

static bool simul_st = true;

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
    return simul_st?WL_CONNECTED:WL_IDLE_STATUS;
}

void CWifi::setTimeout(unsigned long timeout) {
    _timeout = timeout;
}

void simul_wifi_set_state(bool st) {
    simul_st =  st;
}

IPAddress CWifi::localIP() {
    return IPAddress(127,0,0,1);
}
IPAddress CWifi::subnetMask() {
    return IPAddress(255,255,255,0);
}
IPAddress CWifi::gatewayIP() {
    return IPAddress(127,0,0,100);
}
const char *CWifi::SSID() {
    return "SimulatedAP";
}

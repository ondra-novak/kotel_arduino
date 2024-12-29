#include "../WiFiS3.h"

#include <random>
#include <thread>

CWifi WiFi;

CWifi::CWifi():_timeout(10000) {
}

static bool simul_st = true;

uint8_t xstatus = WL_DISCONNECTED;

int CWifi::begin(const char *) {
    std::this_thread::sleep_for(std::chrono::milliseconds(_timeout));
    xstatus = WL_CONNECTED;
    return WL_IDLE_STATUS;
}

int CWifi::begin(const char *, const char *) {
    std::this_thread::sleep_for(std::chrono::milliseconds(_timeout));
    xstatus = WL_CONNECTED;
    return WL_IDLE_STATUS;
}

void CWifi::config(IPAddress ) {
}

void CWifi::config(IPAddress , IPAddress , IPAddress , IPAddress ) {
}

int CWifi::disconnect(void) {
    xstatus = WL_DISCONNECTED;
    return WL_DISCONNECTED;
}

uint8_t CWifi::status() {
    return simul_st?xstatus:static_cast<uint8_t>(WL_IDLE_STATUS);
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
std::int32_t CWifi::RSSI() {
    std::random_device rdev;
    std::uniform_int_distribution<int> rnd(-70,-50);
    return rnd(rdev);
}

IPAddress CWifi::dnsIP(int) {
    return IPAddress(8,8,8,8);
}

void CWifi::end(void) {
    xstatus = WL_DISCONNECTED;
}

uint8_t CWifi::beginAP(const char *) {
    xstatus = WL_AP_LISTENING;
    return WL_AP_LISTENING;
}

int8_t CWifi::scanNetworks() {
    return 1;
}

const char* CWifi::SSID(uint8_t networkItem) {
    if (networkItem == 0) return "SimulatedAP";
    else return nullptr;
}

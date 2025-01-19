#include "network_control.h"
#include "controller.h"
#include "WifiTCP.h"

namespace kotel {

NetworkControl::NetworkControl(Controller &cntr):_cntr(cntr),_server(80) {

}

void NetworkControl::begin() {
    WiFi.status();      //initialize Wifi modem, can take long at the beginning

}

void NetworkControl::run(TimeStampMs cur_time) {
    if (_cntr.get_storage().config.serial_log_out) {
        Serial.println("NetworkControl cycle");
    }
    if (!_cntr.is_safe_for_blocking()) {
        resume_at(cur_time+1);
        return;
    }
    resume_at(cur_time+10);
    switch (_mode) {
        case WifiMode::inactive:
            _connected = false;
            init_wifi();
            return;

        case WifiMode::client:
            if (cur_time > _wifi_check_at) {
                _wifi_check_at = cur_time + 1000;
                _action = Action::get_status;
            } else if (cur_time > _wifi_reset_at ) {
                stop_wifi();
                init_wifi_client();
                _wifi_reset_at = cur_time+from_minutes(2);
                return;
            }
            break;
        case WifiMode::ap:
            if (cur_time > _wifi_reset_at) {
                if (!any_active_client()) {
                    init_wifi();
                    return;
                }
            }
            _rssi = 0;
            _last_status = WL_CONNECTED;
            _connected = true;
            break;
    }
    switch (_action) {
        case Action::inactive:
            break;
        case Action::wait_dns:
            if (_sdns.is_ready()) {
                IPAddress adr(_sdns.get_result());
                _ntp.cancel();
                _ntp.request(adr, 123);
                _action = Action::wait_ntp;
            }
            break;
        case Action::wait_ntp:
            if (_ntp.is_ready()){
                set_current_time(static_cast<uint32_t>(_ntp.get_result()));
                _ntp_resync = cur_time + 24*60*60*1000;
                _action = Action::inactive;
            }
            break;
        case Action::get_status:
            _last_status = WiFiUtils::status();
            _action = Action::determine_connection;
            break;
        case Action::determine_connection:
            _connected = _last_status == WL_CONNECTED;
            if (_connected) {
                _disconnected_streak = 0;
                _action = Action::get_rssi;
            } else {
                ++_disconnected_streak;
                _action = Action::inactive;
            }
            if (_disconnected_streak>30) {
                init_wifi_ap();
                return;
            }
            break;
        case Action::get_rssi:
            _rssi = WiFi.RSSI();
            _action = Action::get_ip;
            break;
        case Action::get_ip:
            WiFiUtils::localIP(_local_ip);
            _action = Action::inactive;
            break;
    }
    if (_connected) {
        auto req = _server.get_request();
        if (req.client) {
            _wifi_last_activity = cur_time;
            _cntr.handle_server(req);
        }
        if (_mode == WifiMode::client && _ntp_resync < cur_time) {
            _ntp_resync = cur_time + 5000;
            _sdns.cancel();
            _sdns.request("pool.ntp.org");
            _action = Action::wait_dns;
        }
    }
}

void NetworkControl::init_wifi() {
    const auto &storage = _cntr.get_storage();
    auto x = storage.wifi_ssid.ssid.get();
    if (x.empty()) init_wifi_ap();
    else init_wifi_client();
}

static IPAddress conv_ip(const IPAddr &x) {
    return IPAddress(x.ip[0], x.ip[1], x.ip[2], x.ip[3]);
}



void NetworkControl::init_wifi_client() {
    if (_mode == WifiMode::inactive) {
        stop_wifi();
    }
    WiFi.setTimeout(0);
    const auto &storage = _cntr.get_storage();
    if (storage.wifi_config.ipaddr != IPAddr{}) {
        WiFi.config(conv_ip(storage.wifi_config.ipaddr),
                conv_ip(storage.wifi_config.dns),
                conv_ip(storage.wifi_config.gateway),
                conv_ip(storage.wifi_config.netmask)
        );
    }
    char ssid[sizeof(WiFi_SSID)+1];
    char pwd[sizeof(WiFi_Password)+1];
    auto x = storage.wifi_ssid.ssid.get();
    *std::copy(x.begin(), x.end(), ssid) = 0;
    x = storage.wifi_password.password.get();
    *std::copy(x.begin(), x.end(), pwd) = 0;
    if (pwd[0] && ssid[0]) {
        WiFi.begin(ssid, pwd);
    } else if (ssid[0]) {
        WiFi.begin(ssid);
    }
    _mode = WifiMode::client;
    auto now = get_current_timestamp();
    _wifi_reset_at = now + from_minutes(5);
    _wifi_check_at = now;
    _server.begin();
    _disconnected_streak = 0;
}

void NetworkControl::init_wifi_ap() {
    if (_mode == WifiMode::inactive) {
        stop_wifi();
    }
    WiFi.setTimeout(0);
    auto status = WiFi.beginAP("kotel");
    auto now = get_current_timestamp();
    _wifi_reset_at = now+from_minutes(1);
    _wifi_check_at = now+1000;
    _connected = status == WL_AP_LISTENING;
    _mode = WifiMode::ap;
    _server.begin();
    _local_ip = WiFi.localIP();

}

void NetworkControl::stop_wifi() {
    _server.end();
    _sdns.cancel();
    _ntp.cancel();
    _action = Action::inactive;
    _mode = WifiMode::inactive;
    _local_ip = IPAddress{};
    WiFi.end();
}


void NetworkControl::continue_init_wifi(const std::vector<CAccessPoint> &aps) {
    auto ssid = _cntr.get_storage().wifi_ssid.ssid.get();
    for (const auto &x: aps) {
        std::string_view thisnetssid = x.ssid;
        if (thisnetssid == ssid) {
            init_wifi_client();
            return;
        }
    }
    init_wifi_ap();

}

}

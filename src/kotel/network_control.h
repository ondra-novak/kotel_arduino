#pragma once
#include "task.h"
#include "http_server.h"

#include "ntp.h"

#include "simple_dns.h"
namespace kotel {


class Controller;

class NetworkControl: public AbstractTask {
public:

    using MyHttpServer = HttpServer<4096,32>;

    NetworkControl(Controller &cntr);

    void begin();
    virtual void run(TimeStampMs cur_time) override;

    bool is_ap_mode() const {return _mode == WifiMode::ap;}
    bool any_active_client() const {
        return get_current_timestamp() < _wifi_last_activity + from_seconds(10);
    }
    bool is_connected() const {return _connected;}
    int8_t get_rssi() const {return _rssi;}
    MyHttpServer &get_server() {return _server;}
    const MyHttpServer &get_server() const {return _server;}
    const IPAddress get_local_ip() const {return _local_ip;}

protected:

    enum class WifiMode : uint8_t{
        inactive,
        client,
        ap,
    };

    enum class Action : uint8_t {
        inactive,
        wait_dns,
        wait_ntp,
        get_rssi,
        get_ip,
        get_status,
        determine_connection
    };

    Controller &_cntr;
    MyHttpServer _server;
    TimeStampMs _wifi_reset_at = disabled_task;
    TimeStampMs _wifi_check_at = disabled_task;
    TimeStampMs _wifi_last_activity = 0;
    TimeStampMs _ntp_resync = 0;
    NTPClient _ntp;
    SimpleDNS _sdns;
    int8_t _rssi = 0;
    uint8_t _last_status = 0;
    WifiMode _mode = WifiMode::inactive;
    Action _action = Action::inactive;
    uint8_t _disconnected_streak = 0;
    bool _connected = false;
    IPAddress _local_ip;


    void init_wifi();
    void init_wifi_client();
    void init_wifi_ap();
    void stop_wifi();

    void continue_init_wifi(const std::vector<CAccessPoint> &aps);


};


}

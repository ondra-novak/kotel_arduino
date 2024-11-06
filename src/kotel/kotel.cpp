#include "kotel.h"
#include "nonv_storage.h"
#include "http_server.h"
#include <r4eeprom.h>

#include "controller.h"

#include "http_utils.h"


namespace kotel {

using MyHttpServer = HttpServer<4096,32>;

Controller controller;

MyHttpServer server(8300);


void setup() {
    Serial.begin(115200);
    pinMode(pin_in_motor_temp,INPUT);
    pinMode(pin_in_tray,INPUT);
    pinMode(pin_out_fan_on,OUTPUT);
    pinMode(pin_out_feeder_on,OUTPUT);
    pinMode(pin_out_pump_on,OUTPUT);
    pinMode(pin_in_pulpup,INPUT_PULLUP);
    controller.begin();
    server.begin();
}



constexpr const char *response_header = "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/pain\r\n"
        "Connection: close\r\n"
        "\r\n";

void handle_server(MyHttpServer::Request &req) {
    controller.set_wifi_used();
    if (req.request_line.path == "/config") {
            if (req.request_line.method == HttpMethod::GET) {
                req.client.print(response_header);
                controller.config_out(req.client);
            } else if (req.request_line.method == HttpMethod::PUT) {
                std::string_view f;
                if (controller.config_update(req.body, std::move(f))) {
                    server.error_response(req, 202, "Accepted");
                } else {
                    server.error_response(req, 409, "Conflict",{},f);
                }
            } else {
                server.error_response(req, 405, "Method not allowed", "Allow: GET,PUT\r\n");
            }
    } else if (req.request_line.path == "/scan_temp" && req.request_line.method == HttpMethod::POST) {
        req.client.print(response_header);
        controller.list_onewire_sensors(req.client);
    } else if (req.request_line.path == "/stats" && req.request_line.method == HttpMethod::GET) {
        req.client.print(response_header);
        controller.stats_out(req.client);
    } else if (req.request_line.path == "/status" && req.request_line.method == HttpMethod::GET) {
        req.client.print(response_header);
        controller.status_out(req.client);
    } else {
        server.error_response(req, 404, "Not found");
    }
    req.client.stop();
}

void handle_serial() {
    static char buffer[512] = {};
    static unsigned int buffer_use = 0;
    int c = Serial.read();
    if (c == '\n') {
        if (buffer_use == sizeof(buffer)) {
            Serial.println("?Serial buffer overflow, command ignored");
            return;
        } else {
            buffer[buffer_use] = 0;
            std::string_view cmd(buffer, buffer_use);
            buffer_use  = 0;
            if (cmd.empty()) {
                Serial.println("Help: /s - status, /c - config, /t - stats, <field>=<value>");
            } else if (cmd.size()>1 && cmd[0] == '/') {
                switch (cmd[1]) {
                    case 's':controller.status_out(Serial);break;
                    case 'c':controller.config_out(Serial);break;
                    case 't':controller.stats_out(Serial);break;
                    default: Serial.println("Unknown command");break;
                }
            } else {
                std::string_view failed_field;
                if (controller.config_update(cmd, std::move(failed_field))) {
                    Serial.println("!OK");
                } else {
                    Serial.print("?Error: ");
                    Serial.write(failed_field.data(), failed_field.size());
                    Serial.println();
                }
            }
        }
    } else if (buffer_use < sizeof(buffer)) {
        buffer[buffer_use] = static_cast<char>(c);
        ++buffer_use;
    }
}

void loop() {
    while (true) {
        auto req = server.get_request();
        if (req.client) {
            handle_server(req);
        } else if (Serial.available()) {
            handle_serial();
        } else {
            controller.run();
            break;
        }
    }

}

}

#include "kotel.h"
#include "nonv_storage.h"
#include "http_server.h"
#include <r4eeprom.h>

#include "controller.h"



namespace kotel {
Controller controller;
Storage storage;

HttpServer<4096,32> server(8300);


void setup() {
    controller.begin();
    server.begin();
}

constexpr const char *response_header = "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/pain\r\n"
        "Connection: close\r\n"
        "\r\n";


void loop() {
    controller.run();
    auto req = server.get_request();
    if (req.client) {
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

}

}

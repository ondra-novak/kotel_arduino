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
        if (req.request_line.path == "/config" && req.request_line.method == "GET") {
            req.client.print(response_header);
            controller.config_out(req.client);
        } else if (req.request_line.path == "/stats" && req.request_line.method == "GET") {
            req.client.print(response_header);
            controller.stats_out(req.client);
        } else {
            server.error_response(req, 404, "Not found");
        }
        req.client.stop();
    }

}

}

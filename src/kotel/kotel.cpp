#include "kotel.h"
#include "feeder.h"
#include "nonv_storage.h"
#include "http_server.h"
#include <r4eeprom.h>




namespace kotel {
Storage storage;


Feeder podavac(storage);
Ventilator ventilator;
Cerpadlo cerpadlo;
HttpServer<> server(8300);

auto planovac = init_scheduler(&podavac,
        &ventilator,
        &cerpadlo);

void setup() {
    podavac.start();
    planovac.reschedule();
    server.begin();
}

void loop() {
    planovac.run();
    auto req = server.get_request();
    if (req.client) {
        constexpr std::string_view response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Connection: close\r\n"
                "\r\n"
                "<H1>It works</H1>";
        req.client.write(response.data(), response.size());
        req.client.stop();
    }

}

}

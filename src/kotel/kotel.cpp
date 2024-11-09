#include "kotel.h"
#include "nonv_storage.h"
#include <r4eeprom.h>

#include "controller.h"



namespace kotel {

Controller controller;


void setup() {
    Serial.begin(115200);
    pinMode(pin_in_motor_temp,INPUT_PULLUP);
    pinMode(pin_in_tray,INPUT_PULLUP);
    pinMode(pin_out_fan_on,OUTPUT);
    pinMode(pin_out_feeder_on,OUTPUT);
    pinMode(pin_out_pump_on,OUTPUT);
    pinMode(pin_in_pulpup,INPUT_PULLUP);
    pinMode(LED_BUILTIN, OUTPUT);
    controller.begin();

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
        if (Serial.available()) {
            handle_serial();
        } else {
            controller.run();
            break;
        }
    }

}

}

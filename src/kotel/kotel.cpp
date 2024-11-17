#include "kotel.h"
#include "nonv_storage.h"
#include <r4eeprom.h>

#include "controller.h"

namespace kotel {

Controller controller;

void setup() {
    Serial.begin(115200);
    pinMode(pin_in_motor_temp, INPUT_PULLUP);
    pinMode(pin_in_tray, INPUT_PULLUP);
    pinMode(pin_out_fan_on, OUTPUT);
    pinMode(pin_out_feeder_on, OUTPUT);
    pinMode(pin_out_pump_on, OUTPUT);
    pinMode(pin_in_pulpup, INPUT_PULLUP);
    pinMode(LED_BUILTIN, OUTPUT);
    controller.begin();

}

void print_hex(unsigned int number, int zeroes) {
  if (zeroes) {
    print_hex(number/16, zeroes-1);
    auto n = number % 16;
    Serial.print(n, HEX);
  }
}

void dump_eeprom() {
  uint8_t buff[256];
  for (int addr = 0; addr<FLASH_TOTAL_SIZE; addr+=256) {
      DataFlashBlockDevice::getInstance().read(buff, addr, sizeof(buff));
      for (int idx = 0; idx < 256; idx +=32) {
        print_hex(addr+idx, 4);
        Serial.print(' ');
        for (int i = 0; i < 32; ++i) {
          auto x = buff[i+idx];
          print_hex(x,2);
          Serial.print(' ');
        }
        for (int i = 0; i < 32; ++i) {
          char c = buff[i+idx];
          if (c >= 32) Serial.print(c); else Serial.print('.');
        }
        Serial.println();
      }
  }
}


void handle_serial() {
    static char buffer[512] = { };
    static unsigned int buffer_use = 0;
    static bool rst = false;
    int c = Serial.read();
    if (c == '\n') {
        if (buffer_use == sizeof(buffer)) {
            Serial.println("?Serial buffer overflow, command ignored");
            return;
        } else {
            buffer[buffer_use] = 0;
            std::string_view cmd(buffer, buffer_use);
            buffer_use = 0;
            if (cmd == "reset") {
                if (rst) {
                    Serial.println("Device halted, please reset");
                    controller.factory_reset();
                    return;
                } else {
                    rst = true;
                    Serial.println("Factor reset. Enter \"reset\" again as confirmation");
                }
            } else {
                rst = false;
                if (cmd.empty()) {
                    Serial.println(
                            "Help: /s - status, /c - config, /t - stats, /d - dump eeprom, reset, eeprom <addr> <field>=<value>");
                } else if (cmd.size() > 1 && cmd[0] == '/') {
                    rst = false;
                    switch (cmd[1]) {
                        case 's':
                            controller.status_out(Serial);
                            break;
                        case 'c':
                            controller.config_out(Serial);
                            break;
                        case 't':
                            controller.stats_out(Serial);
                            break;
                        case 'd':
                            dump_eeprom();
                            break;
                        default:
                            Serial.println("Unknown command");
                            break;
                    }
                } else {
                    std::string_view failed_field;
                    if (controller.config_update(cmd,
                            std::move(failed_field))) {
                        Serial.println("!OK");
                    } else {
                        Serial.print("?Error: ");
                        Serial.write(failed_field.data(), failed_field.size());
                        Serial.println();
                    }
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

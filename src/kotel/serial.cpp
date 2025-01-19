#include "serial.h"
#include <r4eeprom.h>
#include "controller.h"

namespace kotel {

namespace {
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

void print_ok() {
    Serial.println("+OK");
}

void print_dot() {
    Serial.println(".");
}


void print_error() {
    Serial.println("-ERR");
}

void print_error(std::string_view text) {
    Serial.print("-ERR: ");
    Serial.write(text.data(), text.size());
    Serial.println();
}

}

bool handle_serial(Controller &controller) {
    static char buffer[512] = { };
    static unsigned int buffer_use = 0;
    static bool rst = false;
    if (!Serial.available()) return false;
    int c = Serial.read();
    if (c == '\n') {
        if (buffer_use == sizeof(buffer)) {
            buffer_use = 0;
            Serial.println("?Serial buffer overflow, command ignored");
            return true;
        } else {
            buffer[buffer_use] = 0;
            std::string_view cmd(buffer, buffer_use);
            buffer_use = 0;
            if (cmd == "reset") {
                if (rst) {
                    Serial.println("Device halted, please reset");
                    controller.factory_reset();
                    return true;
                } else {
                    rst = true;
                    Serial.println("Factor reset. Enter \"reset\" again as confirmation");
                }
            } else {
                rst = false;
                if (cmd.empty() || cmd == "/h" || cmd == "help") {
                    print_ok();
                    Serial.println(
                            "Help: \r\n"
                            "/s - status, \r\n"
                            "/c - config, \r\n"
                            "/d - dump eeprom, \r\n"
                            "/e <field>=<value> simulate temperature\r\n"
                            "/x - disable simulate temperature\r\n"
                            "/k - kbtest\r\n"
                            "reset - factory reset\r\n"
                            "<field>=<value> - configuration");
                    print_dot();
                } else if (cmd.size() > 1 && cmd[0] == '/') {
                    rst = false;
                    switch (cmd[1]) {
                        case 's':
                            print_ok();
                            controller.status_out(Serial);
                            print_dot();
                            break;
                        case 'c':
                            print_ok();
                            controller.config_out(Serial);
                            print_dot();
                            break;
                        case 'd':
                            print_ok();
                            dump_eeprom();
                            print_dot();
                            break;
                        case 'e':
                            if (controller.enable_temperature_simulation(cmd.substr(2))) {
                                print_ok();
                            } else {
                                print_error();
                            }
                            break;
                        case 'x': controller.disable_temperature_simulation();
                            print_ok();
                            break;
                        case 'k': Serial.println(analogRead(A5));
                            break;
                        default:
                            print_error("unknown command");
                            break;
                    }
                } else {
                    std::string_view failed_field;
                    if (controller.config_update(cmd,
                            std::move(failed_field))) {
                        print_ok();
                    } else {
                        print_error(failed_field);
                    }
                }
            }
        }
    } else if (buffer_use < sizeof(buffer)) {
        buffer[buffer_use] = static_cast<char>(c);
        ++buffer_use;
    }
    return true;
}

}

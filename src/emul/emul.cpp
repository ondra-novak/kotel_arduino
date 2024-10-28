#include "../kotel/kotel.h"
#include "../kotel/controller.h"
#include <thread>
#include <iostream>

namespace kotel {
    extern Controller controller;
}

static std::chrono::system_clock::time_point start_time;

int main() {
    start_time = std::chrono::system_clock::now();
    kotel::setup();
    while (true) {
        kotel::loop();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

unsigned long millis() {
    return static_cast<unsigned long>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now() - start_time).count());
}


void digitalWrite(int pin, int level) {
    std::cout << "PIN " << pin << ": " << level << std::endl;
}

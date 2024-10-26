#include "kotel.h"
#include "feeder.h"
#include "nonv_storage.h"


namespace kotel {
Storage storage;


Feeder podavac(storage);
Ventilator ventilator;
Cerpadlo cerpadlo;

auto planovac = init_scheduler(&podavac,
        &ventilator,
        &cerpadlo);

void setup() {
    podavac.start();
    planovac.reschedule();
}

void loop() {
    planovac.run();

}

}

#include "kotel.h"
#include "config.h"
#include "motohodiny.h"


namespace kotel {

Podavac podavac;
Ventilator ventilator;
Cerpadlo cerpadlo;

auto planovac = init_scheduler(&podavac,
        &ventilator,
        &cerpadlo);

void setup() {

}

void loop() {
    planovac.run();

}

}

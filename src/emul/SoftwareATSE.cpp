#include "SoftwareATSE.h"

#include <random>

SoftwareATSEClass SATSE;

int SoftwareATSEClass::begin(const char *, bool , const char *) {
    return 1;
}

void SoftwareATSEClass::end() {
}

int SoftwareATSEClass::random(byte data[], size_t length) {
    std::random_device dev;
    std::uniform_int_distribution<byte> rnd(0,255);
    for (std::size_t x = 0; x < length; ++x) {
        data[x] = rnd(dev);
    }
    return 1;
}

SoftwareATSEClass::SoftwareATSEClass() {
}

SoftwareATSEClass::~SoftwareATSEClass() {
}

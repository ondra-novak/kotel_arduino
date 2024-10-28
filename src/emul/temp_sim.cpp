#include "../SimpleDallasTemp/SimpleDallasTemp.h"

#include <algorithm>
#include <iterator>
constexpr int count_devices =2;

constexpr SimpleDallasTemp::Address devices[count_devices] = {
        {1,253,125,69,56,252,137,64},
        {2,109,56,217,120,239,0,30}
};

float temps[count_devices] = {85,85};


void set_temp(int device, float value) {
    if (device >= 0 && device < count_devices)
        temps[device] = value;
}

SimpleDallasTemp::SimpleDallasTemp(OneWire &one):_wire(one) {}

bool SimpleDallasTemp::is_valid_address(const Address &a) {
    return std::find(std::begin(devices), std::end(devices), a) != std::end(devices);
}

unsigned long SimpleDallasTemp::request_temp(const Address &) {
    return 750;
}

unsigned long SimpleDallasTemp::request_temp() {
   return 750;
}

std::optional<float> SimpleDallasTemp::read_temp_celsius(const Address &addr) {
    auto iter = std::find(std::begin(devices), std::end(devices), addr);
    if (iter == std::end(devices)) {
        _last_status = Status::fault_not_present;
        return {};
    }
    auto index = std::distance(std::begin(devices), iter);
    return temps[index];
}

static int srch_index = 0;

void SimpleDallasTemp::wire_reset_search() {
    srch_index = 0;
}
bool SimpleDallasTemp::wire_search(Address &addr) {
    if (srch_index >= count_devices) return false;
    addr = devices[srch_index];
    ++srch_index;
    return true;
}



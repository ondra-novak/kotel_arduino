#include "../SimpleDallasTemp/SimpleDallasTemp.h"
#include "OneWire.h"

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

bool SimpleDallasTemp::request_temp(const Address &) {
    return true;
}

bool SimpleDallasTemp::request_temp() {
   return true;
}

void OneWire::begin(uint8_t ) {}

std::optional<float> SimpleDallasTemp::read_temp_celsius(const Address &addr) {
    auto iter = std::find(std::begin(devices), std::end(devices), addr);
    if (iter == std::end(devices)) {
        _last_status = Status::fault_not_present;
        return {};
    }
    auto index = std::distance(std::begin(devices), iter);
    return temps[index];
}


void SimpleDallasTemp::enum_devices_cb(EnumCallback &cb) {
    for (const auto &x: devices) {
        if (!cb(x)) return;
    }
}



std::optional<float> SimpleDallasTemp::async_read_temp_celsius(AsyncState &st) {
    if (st.buffer[8] == 1) {
        return *reinterpret_cast<float *>(st.buffer);
    } else {
        return {};
    }
}

void SimpleDallasTemp::async_request_temp(AsyncState &st, const Address &addr) {
    st.phase = 0;
    st.state = AsyncCommand::request_temp_addr;
    st.st = Status::ok;
    st.addr = addr;
}

void SimpleDallasTemp::async_request_temp(AsyncState &st) {
    st.phase = 0;
    st.state = AsyncCommand::request_temp_global;
    st.st = Status::ok;

}

void SimpleDallasTemp::async_read_temp(AsyncState &st, const Address &addr) {
    st.phase = 0;
    st.addr = addr;
    st.state = AsyncCommand::read_temp;
    st.st = Status::ok;
}

bool SimpleDallasTemp::async_cycle(AsyncState &st) {
    if (st.phase < 3) {
        ++st.phase;
        return false;
    }
    auto x = read_temp_celsius(st.addr);;
    if (x) {
        float *z = reinterpret_cast<float *>(st.buffer);
        *z =  *x;
        st.buffer[8] = 1;
    } else {
        st.buffer[8] = 0;
        st.st = get_last_error();
    }
    return true;

}



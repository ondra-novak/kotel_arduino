#include "../../../tests/check.h"
#include "../OneWire.h"
#include <chrono>
#include <cmath>
#include <deque>
#include <vector>



static std::deque<unsigned long> master_pin_changes = {};
static std::deque<unsigned long> global_pin_changes = {};
static std::deque<unsigned long> slave_pin_changes = {};
static unsigned long sim_time = 0;
static bool cur_master_pin_val = true;
static bool cur_slave_pin_val = true;
static bool cur_global_pin_val = true;


static bool get_pin_value() {
    return cur_master_pin_val && cur_slave_pin_val;
}


static unsigned long get_current_time() {
    return sim_time>>4;
}

static void set_global_pin(bool val) {
    if (cur_global_pin_val != val) {
        cur_global_pin_val = val;
        global_pin_changes.push_back(get_current_time());
    }
}

static void set_master_pin(bool val) {
    if (!val && !cur_slave_pin_val) {
        bool collision_master = false;
        CHECK(collision_master);
    }
    if (cur_master_pin_val != val) {
        cur_master_pin_val = val;
        master_pin_changes.push_back(get_current_time());
    }
    set_global_pin(get_pin_value());

}


static void update_slave_pin() {

    while (!slave_pin_changes.empty() && slave_pin_changes.front() <= get_current_time()) {
        if (!cur_master_pin_val) {
            bool collision_slave = false;
            CHECK(collision_slave);
        }
        cur_slave_pin_val = !cur_slave_pin_val;
        slave_pin_changes.pop_front();
    }
}

static void set_slave_response(std::initializer_list<unsigned long> delays, bool initial_state = true) {
    auto tp = get_current_time();
    slave_pin_changes = {};
    for (auto x: delays) {
        slave_pin_changes.push_back(tp += x);
    }
    cur_slave_pin_val = initial_state;
}

static void clear_state() {
    sim_time = 0;
    master_pin_changes = {};
    global_pin_changes = {};
    cur_master_pin_val = true;
    cur_global_pin_val = true;
}


void OneWire::enable_interrupt() {
}


void OneWire::disable_interrupt() {
}

void OneWire::init_pin() {
    set_master_pin(true);
}

void OneWire::release_pin() {
    set_master_pin(true);
}

void OneWire::hold_low_pin() {
    set_master_pin(false);
}

bool OneWire::read_pin() {
    update_slave_pin();
    bool b =  get_pin_value();
    set_global_pin(b);
    return b;
}


unsigned long OneWire::get_current_time() {
    return (++sim_time)>>4;
}

int check_delays(std::initializer_list<unsigned long> delays) {
    int n = 0;
    unsigned long tp = 0;
    auto z = master_pin_changes.begin();
    for (long x: delays) {
        if (z == master_pin_changes.end()) return n;
        long dff = *z - tp;
        tp += x;
        if (dff  != x) {
            std::cerr << dff <<" != " << x << std::endl;
            return n;
        }
        ++n;
        ++z;
    }
    return -1;
}

void print_global_times() {
    unsigned int a = 1;
    for (auto x: global_pin_changes) {
        std::cout  << x << "," << a << std::endl;
        a = 1- a;
        std::cout  << x << "," << a << std::endl;
    }

}

static OneWire wire;

void test_reset_1() {
    clear_state();
    set_slave_response({});
    CHECK(!wire.reset());
    CHECK_LESS(check_delays({10,480}),0);
}

void test_reset_2() {
    clear_state();
    set_slave_response({580,70});
    CHECK(wire.reset());
    CHECK_LESS(check_delays({10,480}),0);
}
void test_reset_3() {
    clear_state();
    set_slave_response({200,580,70}, false);
    CHECK(wire.reset());
    CHECK_LESS(check_delays({210,480}),0);

}

void test_reset_4() {
    clear_state();
    set_slave_response({520,580,70}, false);
    CHECK(!wire.reset());
    CHECK_LESS(check_delays({}),0);

}

void test_write_1() {
    clear_state();
    set_slave_response({});
    CHECK(wire.write(0xF0));
    CHECK_LESS(check_delays({10,60,20,60,20,60,20,60,20,6,74,6,74,6,74,6}),0);
}

void test_write_2() {
    clear_state();
    set_slave_response({});
    CHECK(wire.write(0xAA));
    CHECK_LESS(check_delays({10,60,20,6,74,60,20,6,74,60,20,6,74,60,20,6}),0);
}

void test_read_1() {
    clear_state();
    set_slave_response({});
    uint8_t b = 0;
    CHECK(wire.read(b));
    CHECK_EQUAL(b, 0xFF);
    CHECK_LESS(check_delays({10,6,74,6,74,6,74,6,74,6,74,6,74,6,74,6}),0);
}

void test_read_2() {
    clear_state();
    set_slave_response({20,20,60,20,60,20,60,20,60,20,60,20,60,20,60,20,60});
    uint8_t b = 0xFF;
    CHECK(wire.read(b));
    CHECK_EQUAL(b, 0);
    CHECK_LESS(check_delays({10,6,74,6,74,6,74,6,74,6,74,6,74,6,74,6}),0);
}

void test_read_3() {
    clear_state();
    set_slave_response({20,20,140,20,60,20,140,20,60,20,140});
    uint8_t b = 0xFF;
    CHECK(wire.read(b));
    CHECK_EQUAL(static_cast<int>(b), 0x92);
    CHECK_LESS(check_delays({10,6,74,6,74,6,74,6,74,6,74,6,74,6,74,6}),0);
    print_global_times();
    std::cout << std::endl;
}


int main() {
    wire.begin(1);
    test_reset_1();
    test_reset_2();
    test_reset_3();
    test_reset_4();
    test_write_1();
    test_write_2();
    test_read_1();
    test_read_2();
    test_read_3();
}


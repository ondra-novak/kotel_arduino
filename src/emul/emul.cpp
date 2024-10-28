#include "../kotel/kotel.h"
#include "../kotel/controller.h"
#include "../kotel/http_server.h"

#include "temp_sim.h"

#include <thread>
#include <iostream>
#include <fstream>

namespace kotel {
    extern Controller controller;
}

static std::chrono::system_clock::time_point start_time;




struct Command {
    enum Type {
        config,
        temp_set,
        temp_smooth,
        tray_open,
        tray_close,
        motor_high_temp_on,
        motor_high_temp_off,
        unknown
    };
    unsigned long timestamp = 0;
    Type type;
    std::string arg;
};

constexpr std::pair<Command::Type, std::string_view> command_str_map[] = {
        {Command::config,"config"},
        {Command::temp_set,"temp_set"},
        {Command::temp_smooth,"temp_smooth"},
        {Command::tray_open,"tray_open"},
        {Command::tray_close,"tray_close"},
        {Command::motor_high_temp_on,"motor_high_temp_on"},
        {Command::motor_high_temp_off,"motor_high_temp_off"},

};

bool parse_line(Command &cmd, std::string_view line) {
    line = kotel::trim(line);
    if (line.empty()) return false;
    bool rel = false;
    if (line[0] == '+') {
        rel = true;
        line = line.substr(1);
    }
    char *out;
    double tp = strtod(line.data(),&out);
    line = line.substr(out - line.data());
    line = kotel::trim(line);
    auto cmdstr = kotel::split(line, " ");
    auto arg = kotel::trim(line);

    cmd.type = Command::unknown;
    for (const auto &[t,s]: command_str_map) {
        if (s == cmdstr) {cmd.type = t;break;}
    }
    cmd.arg = arg;
    unsigned long tpms = static_cast<TimeStampMs>(tp * 1000);
    if (rel) cmd.timestamp += tpms;
    else {
        if (tpms < cmd.timestamp) return false;
        cmd.timestamp = tpms;
    }
    if (cmd.type == Command::unknown) return false;
    return true;
}

bool fetch_command(Command &cmd, std::istream &stream) {
    std::string line;
    do {
        std::getline(stream, line);
        if (line.empty()) {
            continue;
        }
        if (parse_line(cmd, line)) {
            return true;
        }
    } while (!stream.eof());
    return false;
}

static double simspeed = 1.0;
static std::pair<double,double> smooth_start_temp = {0.0,0.0};
static unsigned long smooth_start_time = 0;

std::pair<double, double> parse_temp_pair(const std::string &arg) {
    char *x;
    double t1 = std::strtod(arg.c_str(), &x);
    double t2 = std::strtod(x, nullptr);
    return {t1,t2};
}

void process_command(const Command &cmd) {
    switch (cmd.type) {
        case Command::config: {
            std::string_view f;
            if (!kotel::controller.config_update(cmd.arg, std::move(f))) {
                std::cerr << "ERROR: Failed update config: " << f << std::endl;
            }
        }break;
        case Command::temp_set:
        case Command::temp_smooth: {
            auto tt = parse_temp_pair(cmd.arg);
            smooth_start_temp = tt;
            smooth_start_time = 0;
            set_temp(tt.first, tt.second);
        }break;
        default:break;
    }
}

void smooth_temp(const Command &cmd, unsigned long time) {

    if (cmd.type == Command::temp_smooth) {
        if (!smooth_start_time) smooth_start_time = time;
        unsigned long end = cmd.timestamp;
        double f = static_cast<double>(time - smooth_start_time)/(end - smooth_start_time);
        auto tt = parse_temp_pair(cmd.arg);
        set_temp(0, smooth_start_temp.first+ f * (tt.first - smooth_start_temp.first));
        set_temp(1, smooth_start_temp.second+ f * (tt.second - smooth_start_temp.second));
    }

}

int main(int argc, char **argv) {


    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <script file> <simspeed>" << std::endl;
        return 1;
    }

    std::ifstream f(argv[1]);
    if (!f) {
        std::cerr << "Failed to open: " << argv[1] << std::endl;
        return 2;
    }

    if (argc > 2) {
        simspeed = std::strtod(argv[2],nullptr);
        if (simspeed <= 0) {
            std::cerr << "Invalid speed: " << simspeed;
        }
    }


    start_time = std::chrono::system_clock::now();
    kotel::setup();
    Command cmd;
    bool cont = fetch_command(cmd, f);
    while (cont) {
        auto time = millis();
        smooth_temp(cmd, time);
        while (time >= cmd.timestamp) {
            process_command(cmd);
            cont = fetch_command(cmd, f);
        }
        kotel::loop();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

unsigned long millis() {
    return static_cast<unsigned long>(
            simspeed * std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now() - start_time).count());
}


void digitalWrite(int pin, int level) {
    std::cout << "PIN " << pin << ": " << level << std::endl;
}

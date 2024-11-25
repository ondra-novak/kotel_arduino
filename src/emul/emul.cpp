#include "../kotel/kotel.h"
#include "../kotel/controller.h"
#include "../kotel/http_server.h"
#include "../DotMatrix/DotMatrix.h"
#include "temp_sim.h"
#include "serial_emul.h"

#include <thread>
#include <iostream>
#include <fstream>
#include <iomanip>

void DotMatrix::enable_auto_drive(DotMatrix::TimerFunction, unsigned int) {}
void DotMatrix::DirectDrive::activate_row(int, bool) {}
void DotMatrix::DirectDrive::deactivate_row(int) {}
void DotMatrix::DirectDrive::clear_matrix() {}

template<typename ... Args>
void log_line(Args ... text) {
    float m = millis() *0.001f;
    std::cout << std::setprecision(3) << std::fixed << m << " ";
    (std::cout << ... <<  text) << std::endl;
}



std::size_t dmx_framebuffer_hash = 0;

template<typename T>
bool check_frame_changed(const T &fb) {
    std::string_view pixels(reinterpret_cast<const char *>(fb.pixels), sizeof(fb.pixels));
    std::hash<std::string_view> hasher;
    auto h = hasher(pixels);
    if (h != dmx_framebuffer_hash) {
        dmx_framebuffer_hash = h;
        return true;
    }
    return false;
}


template<typename T>
void output_frame(const T &fb) {
    constexpr std::string_view blocks[] = {
            " ","▀","▄","█"
    };
    std::string ln("DISPLAY: ");
    auto n = ln.size();
    for (uint8_t y = 0; y< 12; y+=2) {
        ln.resize(n);
        for(uint8_t x = 0; x< 8;++x) {
            int c = fb.get_pixel(x,y) + 2*fb.get_pixel(x,y+1);
            ln.append(blocks[c]);
        }
        log_line(ln);
    }
    log_line("DISPLAY: --------");
}


namespace kotel {
    extern Controller controller;
}

static unsigned long current_cycle = 0;



struct Command {
    enum Type {
        config,
        temp_set,
        temp_smooth,
        tray_open,
        tray_close,
        motor_high_temp_on,
        motor_high_temp_off,
        serial,
        wifi,
        reset,
        clear_error,
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
        {Command::serial,"serial"},
        {Command::wifi,"wifi"},
        {Command::reset,"reset"},
        {Command::clear_error, "clear_error"},
        {Command::motor_high_temp_on,"motor_high_temp"},
        {Command::motor_high_temp_off,"motor_norm_temp"},

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
bool state_tray_open = false;
bool state_motor_temp_ok = true;


std::pair<double, double> parse_temp_pair(const std::string &arg) {
    char *x;
    double t1 = std::strtod(arg.c_str(), &x);
    double t2 = std::strtod(x, nullptr);
    return {t1,t2};
}

void simul_wifi_set_state(bool st);

void process_command(const Command &cmd) {
    switch (cmd.type) {
        case Command::config: {
            std::string_view f;
            if (!kotel::controller.config_update(cmd.arg, std::move(f))) {
                std::cerr << "ERROR: Failed update config: " << f << std::endl;
            }
        }break;
        case Command::tray_close:  state_tray_open = false; break;
        case Command::tray_open:  state_tray_open = true; break;
        case Command::motor_high_temp_on:  state_motor_temp_ok = true; break;
        case Command::motor_high_temp_off:  state_motor_temp_ok = false; break;
        case Command::temp_set:
        case Command::temp_smooth: {
            auto tt = parse_temp_pair(cmd.arg);
            smooth_start_temp = tt;
            smooth_start_time = 0;
            set_temp(0,tt.first);
            set_temp(1, tt.second);
        }break;
        case Command::serial:
            uart_input(cmd.arg);
            break;
        case Command::wifi:
            if (cmd.arg == "0") simul_wifi_set_state(false);
            else simul_wifi_set_state(true);
            break;
        case Command::reset:
            std::destroy_at(&kotel::controller);
            new(&kotel::controller) kotel::Controller;
            kotel::controller.begin();
            break;
        case Command::clear_error:
            kotel::controller.clear_error();
            break;
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


    kotel::setup();
    Command cmd;
    bool cont = fetch_command(cmd, f);
    while (cont) {
        auto now = std::chrono::system_clock::now();
        auto time = millis();
        smooth_temp(cmd, time);
        while (cont && time >= cmd.timestamp) {
            process_command(cmd);
            cont = fetch_command(cmd, f);
        }
        kotel::loop();
        const auto &fb = kotel::controller.get_display().frame_buffer;
        if (check_frame_changed(fb)) {
            output_frame(fb);
        }
        ++current_cycle;
        auto str = uart_output();
        if (!str.empty()) {
            log_line("Serial: ", str);
        }
        std::this_thread::sleep_until(now+std::chrono::milliseconds(1));
    }
}

unsigned long millis() {
    return static_cast<unsigned long>(simspeed * current_cycle);
}

static int pins[20] = {};

void digitalWrite(int pin, int level) {
    pins[pin] = level;
    char buff[30];
    for (int i = 0; i<20; ++i) {
        buff[i] = '0'+pins[i];
    }
    buff[20] = 0;
    log_line("PINS: ", buff);
}

void pinMode(int pin, int mode) {
    log_line("Set pin mode: ", pin, " = ", mode);
}

int digitalRead(int pin) {
    switch (pin) {
        case kotel::pin_in_motor_temp: return state_motor_temp_ok?HIGH:LOW;
        case kotel::pin_in_tray: return state_tray_open?LOW:HIGH;
        default:return HIGH;
    }
}

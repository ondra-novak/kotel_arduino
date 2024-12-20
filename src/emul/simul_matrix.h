#pragma once
#include <cstdint>
#include <string>


template<unsigned int modules>
class SimulMatrixMAX7219 {
public:


    void transfer(uint8_t data) {
        unsigned int j = modules-1;
        while (j--) {
            commands[j+1].cmd = commands[j+1].data;
            commands[j+1].data = commands[j].cmd;
        }
        commands[0].cmd = commands[0].data;
        commands[0].data = data;
    }

    void activate() {
        int l = 0;
        for (const auto &cmd: commands) {
            int x = cmd.cmd & 0xF;
            if (x >= 1 && x < 9) {
                _dirty = _dirty || matrix[l][x] != cmd.data;
                matrix[l][x] = cmd.data;
            } else if (x == 10) {
                _dirty = _dirty || intensity[l] != cmd.data;
                intensity[l] = cmd.data;
            } else if (x == 12) {
                bool b = (cmd.data & 0x1) != 0;
                _dirty = _dirty || active[l] != b;
                active[l] = b;
            } else if (x == 15) {
                bool b = (cmd.data & 0x1) != 0;
                _dirty = _dirty || testmode[l] != b;
                testmode[l] = b;
            }
            ++l;
        }
    }


    void draw_part(int part, std::string &out) const {
        for (unsigned int i = 0; i < 1; ++i) {

            unsigned int y = (part << 1) + i;

            for (unsigned int j = 0; j < modules; ++j) {

                for (uint8_t k = 0x80; k ; k = k >> 1) {

                    constexpr std::string_view blocks[] = {
                            " ","▀","▄","█"
                    };

                    int c = 0;
                    c |=  active[j]?testmode[j]?1:matrix[y][j] & k:1;
                    c |= active[j]?testmode[j]?2:matrix[y+1][j] & k:2;

                    out.append(blocks[c]);
                }
            }
        }
    }

    bool is_dirty() const {
        return _dirty;
    }

    void clear_dirty() {
        _dirty = false;
    }

protected:

    bool _dirty = true;

    struct Command {
        uint8_t cmd;
        uint8_t data;
    };

    uint8_t matrix[8][modules] = {};
    uint8_t intensity[modules] = {};
    bool testmode[modules] = {};
    bool active[modules] = {};
    Command commands[modules];

};

extern SimulMatrixMAX7219<4> _instance4;

#pragma once
#include <cstdint>
#include <string>

class SimulMatrixMAX7219_Abstract {
public:
    virtual ~SimulMatrixMAX7219_Abstract() = default;
    virtual void transfer(uint8_t data) = 0;
    virtual void activate() = 0;
    virtual int parts() const = 0;
    virtual void draw_part(int part, std::string &out) const  = 0;
    virtual bool is_dirty() const = 0;
    virtual void clear_dirty()  = 0;

    static SimulMatrixMAX7219_Abstract *current_instance;
};


inline SimulMatrixMAX7219_Abstract *SimulMatrixMAX7219_Abstract::current_instance = nullptr;

template<unsigned int modules>
class SimulMatrixMAX7219: public SimulMatrixMAX7219_Abstract {
public:


    virtual void transfer(uint8_t data) override {
        unsigned int j = modules-1;
        while (j--) {
            commands[j+1].cmd = commands[j+1].data;
            commands[j+1].data = commands[j].cmd;
        }
        commands[0].cmd = commands[0].data;
        commands[0].data = data;
    }

    virtual void activate() override {
        int l = 0;
        for (const auto &cmd: commands) {
            int x = cmd.cmd & 0xF;
            if (x >= 1 && x < 9) {
                --x;
                _dirty = _dirty || matrix[x][l] != cmd.data;
                matrix[x][l] = cmd.data;
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

    virtual int parts() const override {return 4;}

    virtual void draw_part(int part, std::string &out) const override {
        out.clear();
        for (unsigned int i = 0; i < 1; ++i) {

            unsigned int y = (part << 1) + i;

            for (unsigned int j = modules; j--;) {

                for (uint8_t k = 0x80; k ; k = k >> 1) {

                    constexpr std::string_view blocks[] = {
                            " ","▀","▄","█"
                    };

                    int c = 0;
                    c |=  active[j]?testmode[j]?1:matrix[y][j] & k?1:0:0;
                    c |= active[j]?testmode[j]?2:matrix[y+1][j] & k?2:0:0;

                    out.append(blocks[c]);
                }
//                out.push_back('|');
            }
        }
    }

    virtual bool is_dirty() const override {
        return _dirty;
    }

    virtual void clear_dirty() override {
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



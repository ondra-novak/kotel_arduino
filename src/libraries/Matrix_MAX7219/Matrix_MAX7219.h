#pragma once

#include <stdint.h>

namespace Matrix_MAX7219 {

///Base class for Matrix control
/** To control matrix use Control class, this is just utility base class */
class Driver {
public:

    using byte = unsigned char;
    static constexpr unsigned int rows = 8;

    constexpr Driver() = default;
    constexpr Driver(byte data_pin, byte cs_pin, byte clk_pin)
                    :_data_pin(data_pin)
                     ,_cs_pin(cs_pin)
                     ,_clk_pin(clk_pin) {}

    enum Operation: byte {
        OP_NOOP=0,
        OP_DIGIT0=1,
        OP_DIGIT1=2,
        OP_DIGIT2=3,
        OP_DIGIT3=4,
        OP_DIGIT4=5,
        OP_DIGIT5=6,
        OP_DIGIT6=7,
        OP_DIGIT7=8,
        OP_DECODEMODE=9,
        OP_INTENSITY=10,
        OP_SCANLIMIT=11,
        OP_SHUTDOWN=12,
        OP_DISPLAYTEST=15
    };

    ///initialize bus
    void begin(byte data_pin, byte cs_pin, byte clk_pin);

    void begin() const;

    ///start command transfer
    /**
     * @retval true started
     * @retval false bus is shorted
     */
    bool start_packet() const;

    ///send command
    /**
     * @tparam reverse_data - if true, sends data in reversed order
     * @param op opcode
     * @param data argument
     *
     * @note you need to initiate packet by caling start_packet() before sending
     * command. To activate command, use commit_packet(). You need
     * to send many commands as currently connected modules
     */
    void send_command(Operation op, byte data) const;

    ///Commit all changes into modules
    void commit_packet() const;


    static constexpr Operation row2op(byte row) {
        return static_cast<Operation>(static_cast<byte>(OP_DIGIT0)+row);
    }



private:
    ///pin number for data
    byte _data_pin = 2;
    ///pin number for load/cs
    byte _cs_pin = 3;
    ///pin number for clock
    byte _clk_pin = 4;

    ///enable pin - set it to active mode
    void enable_pin(byte pin) const;
    ///disable pin - set it to input pullup
    void disable_pin(byte pin) const;
    ///set pin to high level
    void set_pin_high(byte pin) const;
    ///set pin to low level
    void set_pin_low(byte pin) const;
    ///set pin to specific level
    void set_pin_level(byte pin, bool level) const;
    ///check pin correctly wired
    /**
     * @param pin pin to check
     * @retval true probably ok
     * @retval false bus is shorted
     */
    bool check_pin(byte pin) const;
    ///transfer byte
    void transfer_byte(byte data) const;
};

enum class BitOrder {
    ///the left pixel is most significant bit, the right pixel is least significant bit
    msb_to_lsb,
    ///the left pixel is least significant bit, the left pixel is most significant bit
    lsb_to_msb
};

}

#include "includes/bitmap.h"
#include "includes/device.h"
#include "includes/font.h"

#if 0
enum class Orientation {
    ///connector or the first module is at left side
    left_to_right,
    ///connector or the first module is at right side
    right_to_left
};

enum class BitOrder {
    ///the left pixel is most significant bit, the right pixel is least significant bit
    msb_to_lsb,
    ///the left pixel is least significant bit, the left pixel is most significant bit
    lsb_to_msb
};

template<unsigned int _modules, Orientation _orient, BitOrder _bit_order>
class MatrixControl;



///Declaration of frame buffer
/***
 * @tparam _width with of the frame buffer - must be multiply of 8
 * @tparam _height height of the frame buffer
 *
 * @note zero pixel (0,0) is left top. X extends right, Y extends bottom
 */
template<unsigned int _width, unsigned int _height, BitOrder _bit_order = BitOrder::msb_to_lsb >
class FrameBuffer {
public:
    static_assert((_width & 0x7) == 0, "Must be multiply of eight");
    ///width
    static constexpr unsigned int width = _width;
    ///height
    static constexpr unsigned int height = _height;
    ///total count of pixels of frame buffer
    static constexpr unsigned int count_pixels = width*height;
    ///total active pixels in frame (because pixels at right still need to be counted)
    static constexpr unsigned int whole_frame = 8*_width;
    ///count of bytes reserved for data
    static constexpr unsigned int count_bytes = (count_pixels+7)/8;


    static_assert(width > 0, "Width can't be zero");
    static_assert(height > 0, "Height can't be zero");


    ///actual buffer - it is public, you can directly access
    uint8_t pixels[count_bytes];


    static constexpr uint8_t calc_bitshift(unsigned int bit) {
        if constexpr (_bit_order == BitOrder::lsb_to_msb) {
            return bit & 0x7;
        } else {
            return 7-(bit & 0x7);
        }
    }

    ///set value of pixel
    /**
     * @param x x coord
     * @param y y coord
     * @param value value, it is always masked by the mask
     */
    constexpr void set_pixel(unsigned int x, unsigned int y, uint8_t value) {
        if (x < _width && y < _height) {
            unsigned int bit = (x + y * width);
            unsigned int byte = bit >> 3;
            auto shift = calc_bitshift(bit);
            auto new_val = (pixels[byte] ^ (value << shift)) & (1 << shift);
            pixels[byte] ^= new_val;
        }
    }

    ///retrieve pixel value
    /**
     * @param x x coord
     * @param y y coord
     * @return value of pixel
     */
    constexpr uint8_t get_pixel(unsigned int x, unsigned int y) const {
        if (x < _width && y < _height) {
            unsigned int bit = (x + y * width) ;
            unsigned int byte = bit >> 3;
            auto shift = calc_bitshift(bit);
            return (pixels[byte] >> shift) & 1;
        } else {
            return {};
        }
    }

    ///clear buffer
    /**
     * @param value specifies color.
     */
    constexpr void clear(uint8_t value = 0) {
        uint8_t byte = 0;
        for (int i = 0; i < 8; ++i) {
            byte |= (value & 1) << i;
        }
        for (auto &x: pixels) x = byte;
    }

    ///draw line
    /**
     * @param x0 x-coord of starting point
     * @param y0 y-coord of starting point
     * @param x1 x-coord of ending point
     * @param y1 y-coord of ending point
     * @param color color value
     */
    constexpr void draw_line(int x0, int y0, int x1, int y1, uint8_t color) {
        auto abs = [](auto x){return x<0?-x:x;};
        int dx = abs(x1 - x0);
        int dy = abs(y1 - y0);
        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;
        int err = dx - dy;

        while (true) {
            set_pixel(static_cast<unsigned int>(x0), static_cast<unsigned int>(y0), color);
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x0 += sx;
            }
            if (e2 < dx) {
                err += dx;
                y0 += sy;
            }
        }
    }


    ///draw box
    /**
     * @param x0 x-coord of starting point
     * @param y0 y-coord of starting point
     * @param x1 x-coord of ending point
     * @param y1 y-coord of ending point
     * @param color color value
     *
     * @note all coordinates are included
     */
    constexpr void draw_box(int x0, int y0, int x1, int y1, uint8_t color) {

        auto swap = [](int &a, int &b) {
            a ^= b;     // ab b
            b ^= a;     // ab a
            a ^= b;     // b a
        };


        if (y0 > y1) swap(y0,y1);
        if (x0 > x1) swap(x0,x1);
        for (int y = y0; y <= y1; ++y) {
            for (int x = x0; x <= x1; ++x) {
                set_pixel(static_cast<unsigned int>(x),static_cast<unsigned int>(y), color);
            }
        }
    }


    ///Present the framebuffer at given matrix
    /**
     * @param matrix instance of matrix driver
     * @param x_win x-offset of the visible window (in case that frame buffer is larger)
     * @param y_win y-offset of the visible window (in case that frame buffer is larger)
     * @retval true successfully done
     * @retval false failed to apply update, electrical issue (bus is shorted)
     */
    template<unsigned int _modules, Orientation _orient>
    bool present(MatrixControl<_modules, _orient,  _bit_order> &matrix,
             unsigned int x_win = 0, unsigned int y_win = 0) {
        return matrix.update(pixels+(x_win>>3)+y_win*(_width>>3), false, _width >> 3, x_win & 0x7);
    }



};


///Define control class
/**
 * @tparam _modules count of controlled modules
 * @tparam _orient orientation of display
 * @tparam _bit_order bit order
 */
template<unsigned int _modules, Orientation _orient, BitOrder _bit_order>
class MatrixControl: public MatrixControlBase {
public:

    static constexpr unsigned int modules = _modules;

    static constexpr bool transfer_reversed = (_orient == Orientation::right_to_left) != (_bit_order == BitOrder::msb_to_lsb);

    ///initialize object
    /**
     * @param data_pin number of pin used to transfer data
     * @param clk_pin number of pin used as clock signal
     * @param cs_pin number of pin used as ~CS/LOAD
     * @retval true initialized
     * @retval false bus is shorted
     */
    bool begin(byte data_pin, byte clk_pin, byte cs_pin) {
        MatrixControlBase::begin(data_pin, clk_pin, cs_pin);

        return reset();
    }


    ///update display to show content of frame buffer
    /**
     * @param frame_buffer pointer to a frame buffer. The frame buffer must be ordered by rows,
     * each byte is eight pixels where MSB is left and LSB is right. Total bytes per row is
     * equal to _modules. After the first row follows the second row. There are eight row in total, so
     * the frame buffer must contain _modules * 8 bytes (not checked)
     *
     * @param per_module_intensity if set true, then "9th row" follows the in the frame buffer which
     * each specifies intensity for the whole module. The value is according MAX7219 specification. In this
     * case, the frame buffer must contain _modules * 9 bytes.
     *
     * @retval true updated
     * @retval false bus is shorted
     */
    bool update(const byte *frame_buffer, bool per_module_intensity = false, unsigned int line_length = _modules, unsigned int bit_offset = 0) {

        byte tmp[_modules];

        for (unsigned int i = 0; i < rows; ++i) {

            byte *cur_row = _frame_buffer[i];
            const byte *new_row = frame_buffer;
            frame_buffer += line_length;

            if (bit_offset) {
                for (auto &x: tmp) {
                    if constexpr(_bit_order == BitOrder::msb_to_lsb) {
                        x = (*new_row << bit_offset) & 0xFF;
                        ++new_row;
                        x |= (*new_row >> (8-bit_offset)) & 0xFF;

                    } else {
                        x = (*new_row >> bit_offset) & 0xFF;
                        ++new_row;
                        x |= (*new_row << (8-bit_offset)) & 0xFF;
                    }
                }
                new_row = tmp;
            }

            unsigned int j = _modules;

            while (j--) if (cur_row[j] != new_row[j]) break;
            if (j < _modules) {
                j = _modules;
                if (!start_packet()) return false;
                byte op;
                if constexpr(_orient == Orientation::right_to_left) {
                    op = row_to_op[i];
                } else {
                    op = row_to_op[rows - i - 1];
                }
                while (j--) {

                    int idx;
                    if constexpr(_orient == Orientation::right_to_left) {
                        idx = _modules - j - 1;
                    } else {
                        idx = j;
                    }

                    send_command<transfer_reversed>(op, new_row[idx]);
                    cur_row[idx] = new_row[idx];
                }
                commit_packet();
            }
        }
        if (per_module_intensity) {
            return update_intensity(frame_buffer);
        } else {
            return true;
        }
    }

    ///update intensity per module
    /**
     * @param intensity_per_module specifies intensity per module, the pointer points to
     * array of bytes per module
     * @retval true update
     * @retval false bus is shorted
     */
    bool update_intensity(const byte *intensity_per_module) {
        unsigned int j = _modules;
        while (j--) if (intensity_per_module[j] != _intensity[j]) break;
        if (j < _modules) {
            j = _modules;
            if (!start_packet()) return false;
            while (j--) {
                _intensity[j] = intensity_per_module[j];
                send_command<false>(OP_INTENSITY, calc_module_intensity(j));
            }
            commit_packet();
        }
        return true;
    }


    ///sets master's intensity
    /**
     * The master intensity modulates per module intensity. This value can be set between 0 to 255 and
     * the value is multiplied with every intensity value.
     *
     * For example if you set module's intensity to 8 and master intensity to 64, the final
     * intensity will be 2.
     *
     * @param intensity master intensity 0-255
     * @retval true changed
     * @retval false bus is shorted
     */

    bool set_master_intensity(byte intensity) {
        if (intensity != _global_intensity) {
            unsigned int j = _modules;
            if (!start_packet()) return false;
            while (j--) {
                send_command<false>(OP_INTENSITY, calc_module_intensity(j));
            }
            _global_intensity = intensity;
        }
        return true;
    }


    ///sets all modules intensity
    /**
     * Sets intensity for all modules. It is equal to set same intensity for every module.
     * @param intensity intensity value by chip specification
     */
    void set_all_modules_intensity(byte intensity) {
        char m[_modules];
        for (auto &x:m) x = intensity;
        update_intensity(m);
    }

    ///reset state.
    /**
     * It can be useful when display is reconnected, it needs to be reinitialized
     *
     * The function setups modules and disables shutdown. It doesn't clear
     * content.
     *
     * @retval true success
     * @retval false failure, bus is shorted
     */
    bool reset() {
        for (unsigned int i = 0; i< rows; ++i) {
            if (!global_command(row_to_op[i], 0)) return false;
        }
        for (auto &a: _frame_buffer) for (auto &b: a) b = 0;
        for (auto &a: _intensity) a = 0;

        return global_command(OP_INTENSITY, 0)
                && global_command(OP_DECODEMODE, 0)
                && global_command(OP_SCANLIMIT, 7)
                && global_command(OP_DISPLAYTEST, 0)
                && global_command(OP_SHUTDOWN, 1);
    }

protected:

    bool global_command(byte op, byte data) {
        if (!start_packet()) return false;
        unsigned int j = _modules;
        while (j--) {
            send_command<false>(op, data);
        }
        commit_packet();
        return true;
    }

    byte _frame_buffer[rows][_modules] = {};
    byte _intensity[_modules] = {};

    byte calc_module_intensity(unsigned int module) {
        return (_intensity[module] * _global_intensity + 128) >> 8;
    }

};




}
/*#include "bitmap.h"
#include "font_6p.h"
#include "font_5x3.h"*/

#endif

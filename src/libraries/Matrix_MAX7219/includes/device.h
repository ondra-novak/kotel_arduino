#pragma once

namespace Matrix_MAX7219 {


template<unsigned int _columns, unsigned int _rows, ModuleOrder _mod_order, Transform _transform>
class MatrixDriver {
public:


    static constexpr unsigned int rows = _rows;
    static constexpr unsigned int columns = _columns;
    static constexpr unsigned int modules = rows * columns;
    static constexpr unsigned int xsize = _columns * 8;
    static constexpr unsigned int ysize = _rows * 8;

    constexpr MatrixDriver() = default;
    constexpr MatrixDriver(uint8_t data_pin, uint8_t cs_pin, uint8_t clk_pin)
    :_driver(data_pin, cs_pin, clk_pin) {}

    bool begin(uint8_t data_pin, uint8_t cs_pin, uint8_t clk_pin) {
        _driver.begin(data_pin, cs_pin, clk_pin);
        return init();
    }

    bool begin() const {
        _driver.begin();
        return init();
    }

    ///initialize matrix driver. The function is called from begin();
    bool init() const {
        return global_command(Driver::OP_INTENSITY, 0)
                && global_command(Driver::OP_DECODEMODE, 0)
                && global_command(Driver::OP_SCANLIMIT, 7)
                && global_command(Driver::OP_DISPLAYTEST, 0)
                && global_command(Driver::OP_SHUTDOWN, 1);
    }

    struct ModuleLocation {
        int8_t xs = 0;
        int x = 0;
        int y = 0;
    };

    static constexpr ModuleLocation get_location_left_to_right(unsigned int module_id, unsigned int row) {
        // =0123     |
        //  4567  ---+
        return {-1,static_cast<int>((module_id % _columns) * 8 + 7),
                   static_cast<int>((module_id / _columns) * 8 + 7 - row)};
    }
    static constexpr ModuleLocation get_location_right_to_left(unsigned int module_id, unsigned int row) {
        //  3210= +---
        //  7654  |
        return {1,static_cast<int>(_columns - module_id % _columns -1) * 8,
                  static_cast<int>((module_id / _columns) * 8 + row)};
    }

    static constexpr ModuleLocation get_location(unsigned int module_id, unsigned int row) {
        if constexpr(_mod_order == ModuleOrder::left_to_right)  {
                return get_location_left_to_right(module_id, row);
        } else if constexpr(_mod_order == ModuleOrder::right_to_left) {
                return get_location_right_to_left(module_id, row);
        } else if constexpr(_mod_order == ModuleOrder::left_to_right_zigzag) {
                if ((module_id / _columns) & 1) {
                    return get_location_right_to_left(module_id, row);
                } else {
                    return get_location_left_to_right(module_id, row);
                }
        } else if constexpr(_mod_order == ModuleOrder::right_to_left_zigzag) {
                if ((module_id / _columns) & 1) {
                    return get_location_left_to_right(module_id, row);
                } else {
                    return get_location_right_to_left(module_id, row);
                }
        }

    }



    template<unsigned int _width, unsigned int _height, BitOrder _bit_order, bool _progmem>
    bool display(const Bitmap<_width, _height, _bit_order, _progmem> &bmp_P,
                        int x = 0, int y = 0, bool background = false)  const {
        const auto &bmp = bmp_P.load();
        Matrix mx = ~get_transform(_transform,x,y);
        if (mx.a < 0) mx.u = xsize - 1 - mx.u;
        if (mx.b < 0) mx.u = ysize - 1 - mx.u;
        if (mx.c < 0) mx.v = xsize - 1 - mx.v;
        if (mx.d < 0) mx.v = ysize - 1 - mx.v;


        for (unsigned int row = 0; row < 8; ++row) {
            if (!_driver.start_packet()) return false;
            for (unsigned int mod = modules; mod--; ) {
                auto rw = get_location(mod, row);
                Direction dirx = mx * Direction{rw.xs,0};
                Point pt  = mx * Point{rw.x, rw.y};
                uint8_t data = 0;
                for (uint8_t m = 0x80; m; m = m >> 1, pt = pt + dirx) {
                    auto xp = static_cast<unsigned int>(pt.x);
                    auto yp = static_cast<unsigned int>(pt.y);
                    if (xp < _width && yp < _height) {
                        if (bmp.get_pixel(xp, yp)) data |=m;
                    } else if (background) {
                        data |=m;
                    }
                }
                _driver.send_command(_driver.row2op(row), data);
            }
            _driver.commit_packet();
        }
        return true;
    }

    ///sets global intensity
    bool set_intensity(uint8_t intensity)  const {
        return global_command(Driver::OP_INTENSITY, intensity);
    }

    ///Sets intensity per module
    /**
     * @param fn function which receives x and y coordinate associated with given module. You
     * can use these coordinates to calculate intensity for pixels around this pixel. Note that
     * coordinates have step 8 pixels
     *
     * @param x position of underlying bitmap
     * @param y position of underlying bitmap
     * @retval true success
     * @retval false failure
     */
    template<typename Fn>
    bool set_intensity(Fn fn,  int x = 0, int y = 0)  const {
        if (!_driver.start_packet()) return false;
        Matrix mx = ~get_transform(_transform, x, y);
        for (unsigned int mod = modules; --mod; ) {
            auto rw = get_location(mod, 0);
            Point pt  = mx * Point{rw.x, rw.y};
            uint8_t data = calc_module_intensity(fn(pt.x, pt.y));
            _driver.send_command(Driver::OP_INTENSITY, data);
        }
        _driver.commit_packet();
        return true;

    }

    void clear()  const{
        for (int i = 0; i < 8;++i) {
            global_command(_driver.row2op(i), 0);
        }
    }


protected:

    Driver _driver;

    bool global_command(Driver::Operation op, uint8_t data) const{
        if (!_driver.start_packet()) return false;
        unsigned int j = modules;
        while (j--) {
            _driver.send_command(op, data);
        }
        _driver.commit_packet();
        return true;
    }



};



}

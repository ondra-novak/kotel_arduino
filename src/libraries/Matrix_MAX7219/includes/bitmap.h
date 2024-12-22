#pragma once

#include <stdint.h>
#include "common.h"

namespace Matrix_MAX7219 {



template<Transform trn, BitOp bitop>
struct ImageTransform {
    static constexpr Transform transform = trn;
    static constexpr BitOp bit_operation = bitop;
};

///specifies crop rectangle for put_image
struct CropRect {
    unsigned int left;
    unsigned int top;
    unsigned int right;
    unsigned int bottom;
};

struct DirectionInfo {
    Direction x;
    Direction y;

    constexpr Direction get_width(int width) const {return width * x;}
    constexpr Direction get_height(int height) const {return height * x;}
};

///Bitmap declaration
/**
 * @tparam _width width in pixels
 * @tparam _height height in pixels
 * @tparam _bit_order order of bits (msb->lsb or lsb->msb)
 * @tparam _progmem for AVR only specifies, whether bitmap is initialized in PROGMEM. This must be set
 * along with attribute PROGMEM. Otherwise the argument must be false. For non-AVR platform, this
 * argument is ignored. However for compatibility reasons, it is recommended to specify PROGMEM and
 * set this argument to true for every constexpr instance
 *
 * @code
 * constexpr PROGMEM Bitmap<8,8,BitOrder::msb_to_lsb, true> my_bitmap("....");
 * @endcode
 *
 * To retrieve such bitmap, you need to call load() function
 *
 * @code
 * use_bitmap(my_bitmap.load());
 * @endcode
 *
 * The function load() is converted to const reference on non-AVR platform
 */
template<unsigned int _width, unsigned int _height, BitOrder _bit_order = BitOrder::msb_to_lsb, bool _progmem = false>
class Bitmap {
public:

    static constexpr unsigned int width = _width;
    static constexpr unsigned int height = _height;
    static constexpr unsigned int width_bytes = (width + 7)/8;
    static constexpr unsigned int total_pixels = width*height;
    static constexpr unsigned int total_bytes = width_bytes*height;


    constexpr Bitmap() = default;

    constexpr Bitmap(const char (&asciiart)[total_pixels+1]) {
        const char *ptr = asciiart;
        for (unsigned int h = 0; h < height; ++h) {
            for (unsigned int w = 0; w < width; ++w) {
                if (static_cast<uint8_t>(*ptr) > 32) set_pixel(w,h);
                ++ptr;
            }
        }

    }

    constexpr Bitmap(const uint8_t (&bindata)[total_bytes]) {
        const uint8_t *ptr = bindata;
        for (unsigned int h = 0; h < height; ++h) {
            for (unsigned int w = 0; w < width_bytes; ++w) {
                data[h][w] = *ptr;
                ++ptr;
            }
        }
    }

    static constexpr uint8_t pixel_mask(unsigned int y) {
        if constexpr(_bit_order == BitOrder::msb_to_lsb) {
            return static_cast<uint8_t>(0x80) >> (y & 0x7);
        } else {
            return static_cast<uint8_t>(0x01) << (y & 0x7);
        }
    }

    constexpr void set_pixel(unsigned int x, unsigned int y) {
        data[y][x >> 3] |= pixel_mask(x);
    }

    constexpr void clear_pixel(unsigned int x, unsigned int y) {
        data[y][x >> 3] &= ~pixel_mask(x);
    }

    constexpr void set_pixel(unsigned int x, unsigned int y, bool state) {
        if (state) set_pixel(x, y); else clear_pixel(x, y);
    }

    constexpr bool get_pixel(unsigned int x, unsigned int y) const {
        return (data[y][x >> 3] & pixel_mask(x)) != 0;
    }

    constexpr void clear() {
        for (auto &x: data) for (auto &y: x) y = 0;
    }

    constexpr void fill() {
        for (auto &x: data) for (auto &y: x) y = 0xFF;
    }

    ///draw line
    /**
     * @param x0 x-coord of starting point
     * @param y0 y-coord of starting point
     * @param x1 x-coord of ending point
     * @param y1 y-coord of ending point
     * @param set_pixel set to true to set pixel active, or false to clear pixel
     */
    constexpr void draw_line(int x0, int y0, int x1, int y1, bool set_pixel) {
        if (set_pixel) draw_line<true>(x0, y0, x1, y1);
        else draw_line<false>(x0, y0, x1, y1);
    }

    ///draw line (template version)
    /**
     * @tparam _set_pixel set to true to set pixel active, or false to clear pixel
     * @param x0 x-coord of starting point
     * @param y0 y-coord of starting point
     * @param x1 x-coord of ending point
     * @param y1 y-coord of ending point
     */
    template<bool _set_pixel>
    constexpr void draw_line(int x0, int y0, int x1, int y1) {
#undef abs
        auto abs = [](auto x){return x<0?-x:x;};
        int dx = abs(x1 - x0);
        int dy = abs(y1 - y0);
        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;
        int err = dx - dy;

        while (true) {
            if constexpr(_set_pixel) {
                set_pixel(static_cast<unsigned int>(x0), static_cast<unsigned int>(y0));
            } else {
                clear_pixel(static_cast<unsigned int>(x0), static_cast<unsigned int>(y0));
            }
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

    constexpr void draw_box(int x0, int y0, int x1, int y1, bool set_pixel) {
        if (set_pixel) draw_box<true>(x0, y0, x1, y1);
        else draw_box<false>(x0, y0, x1, y1);
    }


    ///draw box (template version)
    /**
     * @tparam _set_pixel set to true to set pixel active, or false to clear pixel
     * @param x0 x-coord of starting point
     * @param y0 y-coord of starting point
     * @param x1 x-coord of ending point
     * @param y1 y-coord of ending point

     *
     * @note all coordinates are included
     */
    template<bool _set_pixel>
    constexpr void draw_box(int x0, int y0, int x1, int y1) {

        auto swap = [](int &a, int &b) {
            a ^= b;     // ab b
            b ^= a;     // ab a
            a ^= b;     // b a
        };


        if (y0 > y1) swap(y0,y1);
        if (x0 > x1) swap(x0,x1);
        for (int y = y0; y <= y1; ++y) {
            for (int x = x0; x <= x1; ++x) {
                if constexpr(_set_pixel) {
                    set_pixel(static_cast<unsigned int>(x),static_cast<unsigned int>(y));
                } else {
                    clear_pixel(static_cast<unsigned int>(x),static_cast<unsigned int>(y));
                }
            }
        }
    }

    ///Load bitmap from PROGMEM to SRAM
    /**
     * This function is intended for AVR CPUs. If the Bitmap is stored in PROGMEM, the
     * function returns copy of the Bitmap constructed in SRAM (at stack). If the Bitmap
     * is already in SRAM, it returns const reference to it. Note that to correctly handle
     * PROGMEM, the Bitmap must be declared with PROGMEM attribute and the parameter _progmem
     * must be set to true (as there is no other way to detect it)
     *
     * On other platforms (ARM for example), the _progmem attribute is ignored
     *
     * @return
     */
    decltype(auto) load() const {
#ifdef __AVR__
        if constexpr(_progmem) {
            Bitmap<width, height, _bit_order, false> out;
            memcpy_P(&out, this, sizeof(out));
            return out;
        } else {
            return reinterpret_cast<const Bitmap<_width, _height, _bit_order, false> &>(*this);
        }
#else
        return reinterpret_cast<const Bitmap<width, height, _bit_order, false> &>(*this);
#endif
    }

    static constexpr unsigned int get_width()  {return _width;}
    static constexpr unsigned int get_height()  {return _height;}

    ///put other image to the bitmap
    /**
     *
     * @param left x-coord where put left-top corner. This value can be negative (the crop will be applied)
     * @param top y-coord where put left-top corner. This value can be negative (the crop will be applied)
     * @param bmp source bitmap
     * @param trn an object specifies image transformation (use ImageTransform template, or own version)
     * @param crop optional pointer to crop rectangle (if nullptr, not used), this rectangle is applied
     * before the image is transformed
     */
    template<typename _Bitmap, typename _ImageTransform = ImageTransform<Transform::none, BitOp::copy> >
    constexpr DirectionInfo put_image(Point pt, const _Bitmap &bmp_P, _ImageTransform imgtrn = {}, const CropRect *crop = nullptr) {
        const auto &bmp = bmp_P.load();
        unsigned int src_left = 0;
        unsigned int src_top = 0;
        unsigned int src_width = bmp.get_width();
        unsigned int src_height = bmp.get_height();
        if (crop) {
            src_left = crop->left;
            src_top = crop->top;
            src_width = crop->right - crop->left;
            src_height = crop->bottom - crop->top;
        }
        BitOp bitop = imgtrn.bit_operation;
        auto matrix = get_transform(imgtrn.transform);

        Point pty = pt;
        DirectionInfo dir{matrix * Direction{1,0},matrix * Direction{0,1}};

        for (unsigned int y = 0; y < src_height; ++y, pty = pty + dir.y) {
            Point ptx = pty;
            for (unsigned int x = 0; x < src_width; ++x, ptx = ptx + dir.x) {
               auto tx = static_cast<unsigned int>(ptx.x);
               auto ty = static_cast<unsigned int>(ptx.y);
               if (tx < width && ty < height) {
                   bool st = bmp.get_pixel(x+src_left,y+src_top);
                   switch(bitop) {
                       default:
                       case BitOp::copy: set_pixel(tx,ty, st);break;
                       case BitOp::invert_copy: set_pixel(tx,ty, !st);break;
                       case BitOp::mask: set_pixel(tx,ty, st && get_pixel(tx,ty));break;
                       case BitOp::invert_mask: set_pixel(tx,ty, !st && get_pixel(tx,ty));break;
                       case BitOp::merge: set_pixel(tx,ty, st || get_pixel(tx,ty));break;
                       case BitOp::invert_merge: set_pixel(tx,ty, !st || get_pixel(tx,ty));break;
                       case BitOp::flip: set_pixel(tx,ty, st != get_pixel(tx,ty));break;
                       case BitOp::invert_flip: set_pixel(tx,ty, st == get_pixel(tx,ty));break;
                   }
               }

            }
        }
        return dir;
    }

protected:
    uint8_t data[height][width_bytes] = {};
};







}



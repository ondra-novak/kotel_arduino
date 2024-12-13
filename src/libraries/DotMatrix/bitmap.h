#pragma once
#include <array>
#include <cstdint>
#include <type_traits>
#include <string_view>
namespace DotMatrix {

///define bitmap blt operation
enum class BltOp {
    ///copy pixels 1:1
    copy,
    ///clear pixels that are zeroed in bitmap
    and_op,
    ///set pixels that are one in bitmap
    or_op,
    ///xor pixels
    xor_op,
    ///set pixels that are zeroed in bitmap
    nand_op,
    ///clear pixels that are set in bitmap
    nor_op,
    ///copy inverted
    copy_neg,

};

enum class Rotation {
    rot0,
    rot90,
    rot180,
    rot270
};

///specifies colors when blitting the bitmap
/**
 * This is more useful for 2bit pixel format, however you can use
 * this to put bitmap inverted by swapping colors
 */
struct ColorMap {
    ///foreground color
    uint8_t foreground = 0xFF;
    ///background color
    uint8_t background = 0x00;
};

///Declare bitmap
/**
 * Bitmap is monochrome image (icon). Each pixel has one bit
 *
 * @tparam width with of bitmap.
 * @tparam height height of bitmap
 *
 * Similar as framebuffer, bitmap has zero point at left-top. X extends right,
 * Y extends bottom
 */
template<unsigned int width, unsigned int height>
class Bitmap {
public:
    static constexpr auto w = width;
    static constexpr auto h = height;
    static constexpr auto line_width = (w + 7)/8;

    static_assert(width > 0);
    static_assert(height > 0);

    ///retrieve with
    static constexpr int get_width() {
        return width;
    }
    ///retrieve height
    ///
    static constexpr int get_height() {
        return height;
    }

    ///set pixel of bitmap
    /**
     * This sets pixel to 1
     *
     * @param x x coord
     * @param y y coord
     */

    constexpr void set_pixel(unsigned int x, unsigned int y) {
        bitmap[y][x >> 3] |= static_cast<uint8_t>(1) << (x & 0x7);
    }
    ///get value of pixel
    /**
     * @param x x coor
     * @param y y coor
     * @return value
     */
    constexpr bool get_pixel(unsigned int x, unsigned int y) const {
        return (bitmap[y][x >> 3] & (1 << (x & 0x7))) != 0;
    }

    ///initialize bitmap from raw data
    /**
     * @param data raw data, 1 byte = 8 pixels
     */
    constexpr Bitmap(const uint8_t *data) {
        for (auto &a: bitmap) {
            for (auto &b: a) {
                b = *data;
                ++data;
            }
        }
    }

    ///initialize bitmap from ascii art
    /**
     * @param asciiart a string, where space is used for 0, and non-space character as 1
     * The string is split to parts where each part is width characters. The string
     * must have exact width*height characters,
     *
     * The main use if this constructor is in constexpr declaration where the compiler
     * decodes asciiart string and stores bits directly to final compilation
     */
    constexpr Bitmap(const char *asciiart) {
        for (unsigned int y = 0; y < height; ++y) {
            for (unsigned int x = 0; x < width; ++x) {
                if (*asciiart > 32)
                    set_pixel(x, y);
                ++asciiart;
            }
        }
        if (*asciiart != 0) {
            bitmap[height][2] = 1;
        }
    }

    template<typename FrameBuffer>
    constexpr void draw(FrameBuffer &fb, unsigned int x, unsigned int y, ColorMap colors = {},
            BltOp blt_op = BltOp::copy,
            Rotation r = Rotation::rot0) const;

protected:
    uint8_t bitmap[height][line_width] = { };
};

///Defines blt function parameters
/**
 * @tparam op operation
 * @tparam rot rotation of bitmap
 *
 * @note orientation landscape is always
 */
template<BltOp op = BltOp::copy, Rotation rot = Rotation::rot0>
struct BitBlt {

    ///Copy bitmap
    /**
     * @param bm bitmap to copy
     * @param fb target frame buffer
     * @param col column (x coord) where left upper corner of bitmap is mapped
     * @param row row (x coord) where left upper corner of bitmap is mapped
     * @param colors specifies colors of each pixel state
     */
    template <typename Bitmap, typename FrameBuffer>
    static constexpr void bitblt(const Bitmap &bm, FrameBuffer &fb, int col, int row,
        const ColorMap &colors = { }) {
        auto h = bm.get_height();
        auto w = bm.get_width();
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                unsigned int r = 0;
                unsigned int c = 0;
                if constexpr(rot == Rotation::rot0) {
                    r = y + row;
                    c = x + col;
                } else if constexpr(rot == Rotation::rot90) {
                    r = x + row;
                    c = col - y;
                } else if constexpr(rot == Rotation::rot180) {
                    r = row - y;
                    c = col - x;
                } else if constexpr(rot == Rotation::rot270) {
                    r = row - x;
                    c = y + col;
                }
                auto v = bm.get_pixel(x, y);
                if constexpr (op == BltOp::xor_op) {
                    auto cc = fb.get_pixel(c, r);
                    fb.set_pixel(c, r,
                            cc ^ (v ? colors.foreground : colors.background));
                } else if constexpr (op == BltOp::and_op) {
                    if (!v)
                        fb.set_pixel(c, r, colors.background);
                } else if constexpr (op == BltOp::or_op) {
                    if (v)
                        fb.set_pixel(c, r, colors.foreground);
                } else if constexpr (op == BltOp::nand_op) {
                    if (!v)
                        fb.set_pixel(c, r, colors.foreground);
                } else if constexpr (op == BltOp::nor_op) {
                    if (v)
                        fb.set_pixel(c, r, colors.background);
                } else if constexpr (op == BltOp::copy_neg) {
                    fb.set_pixel(c, r, v ? colors.background : colors.foreground);
                } else {
                    fb.set_pixel(c, r, v ? colors.foreground : colors.background);
                }
            }
        }
    }
};

///Bitmap viewer
/**
 * Can be initialized by any bitmap without knowing exact type (because Bitmap is template);
 * It is initialized as view - as a reference. You still need original bitmap
 * to access pixels
 *
 */
class BitmapView {
public:
    ///retrieve width
    int get_width() const {
        return width;
    }
    ///retrieve height
    int get_height() const {
        return height;
    }
    ///retrieve pixel
    uint8_t get_pixel(int w, int h) const {
        return get_pixel_fn(*this, w, h);
    }

    ///construct view from the bitmap
    template<int _w, int _h>
    BitmapView(const Bitmap<_w, _h> &bp) :
            width(_w), height(_h), ref(&bp), get_pixel_fn(
                    [](const BitmapView &me, int w, int h) {
                        auto b = reinterpret_cast<const Bitmap<_h, _w>*>(me.ref);
                        if (w < _w && h < _h) {
                            return b->set_pixel(w, h);
                        }
                        return false;
                    }) {
    }
protected:
    int width;
    int height;
    const void *ref;
    uint8_t (*get_pixel_fn)(const BitmapView&, int, int) = nullptr;
};

///helper class for fonts
/**
 * Just alias of Bitmap, note that height and with are swapped, because
 * sometimes width can be various
 */
template<unsigned int height, unsigned int width>
using FontFace = Bitmap<width,height>;

///helper class for proportional font face
/**
 * @param height height of font face
 * @param max_width maximum width of font face
 */
template<unsigned int height, unsigned int max_width = 8>
struct FontFaceP {
    ///actual width (can be less than max_width)
    uint8_t width;
    ///character face itself
    Bitmap<max_width,height> face;
};
///Declaration of normal font
template<unsigned int height, unsigned int width>
using Font = std::array<FontFace<height, width>, 96>;
///Declaration of proportional font
///
template<unsigned int height, unsigned int max_width>
using FontP = std::array<FontFaceP<height, max_width>, 96>;

///helps to detect font type (fixed/proportional)
template<typename T>
struct FontFaceSpec;

///specialization for proportional font
template<unsigned int height, unsigned int max_width>
struct FontFaceSpec<FontFaceP<height, max_width> > {
    const FontFaceP<height, max_width> &ch;
     constexpr uint8_t get_width() const {
        return ch.width;
    }
     constexpr const Bitmap<max_width, height> &get_face() const {
        return ch.face;
    }
};
///specialization for fixed font
template<unsigned int height, unsigned int width>
struct FontFaceSpec<FontFace<height,width> > {
    const FontFace<height,width> &ch;
     constexpr uint8_t get_width() const{
        return width;
    }
     constexpr const Bitmap<width, height> &get_face() const {
        return ch;
    }
};

///Run operation for specified character
/**
 * @param font selected font
 * @param ascii_char character
 * @param fn function called with FontFaceSpec containing the actual character face
 * @return
 */
template<typename Font, typename Fn>
auto do_for_character(const Font &font, int ascii_char, Fn &&fn) {
    if (ascii_char < 33) ascii_char = 32;
    else if (ascii_char > 127) ascii_char = '?';
    ascii_char -= 32;
    const auto &chdef = font[ascii_char];
    using Spec = FontFaceSpec<std::decay_t<decltype(chdef)> >;
    return fn(Spec{chdef});
}

///Text renderer
/**
 * @tparam op blit operation
 * @tparam rot rotation
 */
template<BltOp op = BltOp::copy, Rotation rot = Rotation::rot0>
struct TextRender {

    ///render single character
    /**
     * @param fb frame buffer
     * @param font font
     * @param x x coord
     * @param y y coord
     * @param ascii_char ascii character
     * @param cols colors
     * @return returns with of the character
     */
    template<typename FrameBuffer, typename Font>
    static uint8_t render_character(FrameBuffer &fb, const Font &font,
            unsigned int x, unsigned int y, int ascii_char, const ColorMap &cols = {}) {
        return do_for_character(font, ascii_char, [&](auto spec){
            BitBlt<op, rot>::template bitblt(spec.get_face() ,fb, x, y, cols);
            return spec.get_width();
        });
    }
    ///retrieve with of the character
    /**
     * @param font font
     * @param ascii_char character
     * @return width
     */
    template<typename Font>
    static uint8_t get_character_width(const Font &font, int ascii_char) {
        return do_for_character(font, ascii_char, [&](auto spec){
            spec.get_width();
        });
    }

    ///render text
    /**
     * @param fb frame buffer
     * @param font font
     * @param x starting x coordinate (left top of first letter)
     * @param y starting y coordinate (left top of first letter)
     * @param text string to render
     * @param cols colors
     * @return new x and new y coordinate (to continue in rendering)
     */
    template<typename FrameBuffer, typename Font>
    static std::pair<unsigned int, unsigned int> render_text(FrameBuffer &fb, const Font &font,
            unsigned int x, unsigned int y, std::string_view text, const ColorMap &cols = {}) {
        for (char c: text) {
            auto s = render_character(fb, font, x, y, c, cols);
            if constexpr(rot == Rotation::rot0) {
                x+=s;
            } else if constexpr(rot == Rotation::rot90) {
                y+=s;
            } else if constexpr(rot == Rotation::rot180) {
                x-=s;
            } else if constexpr(rot == Rotation::rot270) {
                y-=s;
            }
        }
        return {x,y};
    }
};

template<typename T, T from, T to>
struct ToIntergalValue {
    template<typename Fn>
    static constexpr  auto call(T val, Fn &&fn) {
        if (val == from) return fn(std::integral_constant<T, from>());
        else return ToIntergalValue<T, static_cast<T>(static_cast<int>(from)+1), to>::call(val, std::forward<Fn>(fn));
    }
};

template<typename T, T x>
struct ToIntergalValue<T, x, x> {
    template<typename Fn>
    static constexpr  auto call(T , Fn &&fn) {
        return fn(std::integral_constant<T, x>());
    }
};

template<unsigned int w, unsigned int h>
template<typename FrameBuffer>
constexpr void Bitmap<w,h>::draw(FrameBuffer &fb, unsigned int x, unsigned int y, ColorMap colors,
        BltOp blt_op, Rotation r) const {
    ToIntergalValue<BltOp, BltOp::copy, BltOp::copy_neg>
        ::call(blt_op,[&](auto c_op){
            ToIntergalValue<Rotation, Rotation::rot0, Rotation::rot270>
                ::call(r, [&](auto r_op) {
                    BitBlt<c_op.value, r_op.value>::bitblt(*this, fb, x, y, colors);
                });
        });

}


}

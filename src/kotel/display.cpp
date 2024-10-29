#include <type_traits>
#include "display.h"
#include "DotMatrix.h"

#include <iterator>
namespace Matrix {

template<int width, int height>
class FontMatrix {
public:

    static constexpr auto w = width;
    static constexpr auto h = height;

    using RowType = std::conditional_t<width>=16,uint32_t,
                        std::conditional_t<width>=8,uint16_t,uint8_t> >;


    constexpr void set_pixel(int x, int y) {
        bitmap[y] |= static_cast<RowType>(1) << x;
    }

    constexpr bool get_pixel(int x, int y) const {
        return (bitmap[y] & (1 << x)) != 0;
    }

    constexpr FontMatrix(const char *asciiart) {
        for(int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if (*asciiart > 32) set_pixel(x,y);
                ++asciiart;
            }
        }
        if (*asciiart != 0) {
            bitmap[height] = 1;
        }
    }


protected:
    RowType bitmap[height] = {};


};

using Font4x6 = FontMatrix<4,6>;

constexpr Font4x6 font_4x6_table[] = {
        "    "
        "    "
        "    "
        "    "
        "    "
        "    ",
        " X  "
        " X  "
        " X  "
        "    "
        " X  "
        "    ",
        "X X "
        "X X "
        "    "
        "    "
        "    "
        "    ",
        "X X "
        "XXX "
        "X X "
        "XXX "
        "X X "
        "    ",
        "XXX "
        "XX  "
        "XXX "
        " XX "
        "XXX "
        "    ",
        "X X "
        "  X "
        " X  "
        "X   "
        "X X "
        "    ",
        " O  "
        "O O "
        " O  "
        "O  O"
        "OOO "
        "    ",
        " X  "
        " X  "
        "    "
        "    "
        "    "
        "    ",
        "  X "
        " X  "
        " X  "
        " X  "
        "  X "
        "    ",
        "X   "
        " X  "
        " X  "
        " X  "
        "X   "
        "    ",
        "    "
        "X X "
        " X  "
        "X X "
        "    "
        "    ",
        "    "
        " X  "
        "XXX "
        " X  "
        "    "
        "    ",
        "    "
        "    "
        "    "
        " X  "
        " X  "
        "X   ",
        "    "
        "    "
        "XXX "
        "    "
        "    "
        "    ",
        "    "
        "    "
        "    "
        "XX  "
        "XX  "
        "    ",
        "    "
        "  X "
        " X  "
        "X   "
        "    "
        "    ",
        "XXX "
        "X X "
        "X X "
        "X X "
        "XXX "
        "    ",
        "  X "
        "  X "
        "  X "
        "  X "
        "  X "
        "    ",
        "XXX "
        "  X "
        "XXX "
        "X   "
        "XXX "
        "    ",
        "XXX "
        "  X "
        "XXX "
        "  X "
        "XXX "
        "    ",
        "X X "
        "X X "
        "XXX "
        "  X "
        "  X "
        "    ",
        "XXX "
        "X   "
        "XXX "
        "  X "
        "XXX "
        "    ",
        "XXX "
        "X   "
        "XXX "
        "X X "
        "XXX "
        "    ",
        "XXX "
        "  X "
        "  X "
        "  X "
        "  X "
        "    ",
        "XXX "
        "X X "
        "XXX "
        "X X "
        "XXX "
        "    ",
        "XXX "
        "X X "
        "XXX "
        "  X "
        "XXX "
        "    ",
        "    "
        " X  "
        "    "
        " X  "
        "    "
        "    ",
        "    "
        " X  "
        "    "
        " X  "
        " X  "
        "X   ",
        "  X "
        " X  "
        "X   "
        " X  "
        "  X "
        "    ",
        "    "
        "XXX "
        "    "
        "XXX "
        "    "
        "    ",
        "X   "
        " X  "
        "  X "
        " X  "
        "X   "
        "    ",
        "XX  "
        "  X "
        " X  "
        "    "
        " X  "
        "    ",
        " XX "
        "X  X"
        "XXXX"
        "XX X"
        " XX "
        "    ",
        " X  "
        "X X "
        "XXX "
        "X X "
        "X X "
        "    ",
        "XX  "
        "X X "
        "XX  "
        "X X "
        "XX  "
        "    ",
        " XX "
        "X   "
        "X   "
        "X   "
        " XX "
        "    ",
        "XX  "
        "X X "
        "X X "
        "X X "
        "XX  "
        "    ",
        "XXX "
        "X   "
        "XX  "
        "X   "
        "XXX "
        "    ",
        "XXX "
        "X   "
        "XX  "
        "X   "
        "X   "
        "    ",
        " XX "
        "X   "
        "X X "
        "X X "
        " XX "
        "    ",
        "X X "
        "X X "
        "XXX "
        "X X "
        "X X "
        "    ",
        "XXX "
        " X  "
        " X  "
        " X  "
        "XXX "
        "    ",
        "XXX "
        "  X "
        "  X "
        "X X "
        " X  "
        "    ",
        "X X "
        "X X "
        "XX  "
        "X X "
        "X X "
        "    ",
        "X   "
        "X   "
        "X   "
        "X   "
        "XXX "
        "    ",
        "X X "
        "XXX "
        "XXX "
        "X X "
        "X X "
        "    ",
        "XX  "
        "X X "
        "X X "
        "X X "
        "X X "
        "    ",
        " X  "
        "X X "
        "X X "
        "X X "
        " X  "
        "    ",
        "XX  "
        "X X "
        "XXX "
        "X   "
        "X   "
        "    ",
        " X  "
        "X X "
        "X X "
        "XXX "
        " XX "
        "    ",
        "XX  "
        "X X "
        "XX  "
        "X X "
        "X X "
        "    ",
        " XX "
        "X   "
        " X  "
        "  X "
        "XX  "
        "    ",
        "XXX "
        " X  "
        " X  "
        " X  "
        " X  "
        "    ",
        "X X "
        "X X "
        "X X "
        "X X "
        "XXX "
        "    ",
        "X X "
        "X X "
        "X X "
        "X X "
        " X  "
        "    ",
        "X X "
        "X X "
        "XXX "
        "XXX "
        " X  "
        "    ",
        "X X "
        "X X "
        " X  "
        "X X "
        "X X "
        "    ",
        "X X "
        "X X "
        " X  "
        " X  "
        " X  "
        "    ",
        "XXX "
        "  X "
        " X  "
        "X   "
        "XXX "
        "    ",
        " XX "
        " X  "
        " X  "
        " X  "
        " XX "
        "    ",
        "    "
        "X   "
        " X  "
        "  X "
        "    "
        "    ",
        "XX  "
        " X  "
        " X  "
        " X  "
        "XX  "
        "    ",
        " X  "
        "X X "
        "    "
        "    "
        "    "
        "    ",
        "    "
        "    "
        "    "
        "    "
        "    "
        "XXXX",
        " X  "
        "  X "
        "    "
        "    "
        "    "
        "    ",
        " X  "
        "X X "
        "XXX "
        "X X "
        "X X "
        "    ",
        "XX  "
        "X X "
        "XX  "
        "X X "
        "XX  "
        "    ",
        " XX "
        "X   "
        "X   "
        "X   "
        " XX "
        "    ",
        "XX  "
        "X X "
        "X X "
        "X X "
        "XX  "
        "    ",
        "XXX "
        "X   "
        "XX  "
        "X   "
        "XXX "
        "    ",
        "XXX "
        "X   "
        "XX  "
        "X   "
        "X   "
        "    ",
        " XX "
        "X   "
        "X X "
        "X X "
        " XX "
        "    ",
        "X X "
        "X X "
        "XXX "
        "X X "
        "X X "
        "    ",
        "XXX "
        " X  "
        " X  "
        " X  "
        "XXX "
        "    ",
        "XXX "
        "  X "
        "  X "
        "X X "
        " X  "
        "    ",
        "X X "
        "X X "
        "XX  "
        "X X "
        "X X "
        "    ",
        "X   "
        "X   "
        "X   "
        "X   "
        "XXX "
        "    ",
        "X X "
        "XXX "
        "XXX "
        "X X "
        "X X "
        "    ",
        "XX  "
        "X X "
        "X X "
        "X X "
        "X X "
        "    ",
        " X  "
        "X X "
        "X X "
        "X X "
        " X  "
        "    ",
        "XX  "
        "X X "
        "XXX "
        "X   "
        "X   "
        "    ",
        " X  "
        "X X "
        "X X "
        "XXX "
        " XX "
        "    ",
        "XX  "
        "X X "
        "XX  "
        "X X "
        "X X "
        "    ",
        " XX "
        "X   "
        " X  "
        "  X "
        "XX  "
        "    ",
        "XXX "
        " X  "
        " X  "
        " X  "
        " X  "
        "    ",
        "X X "
        "X X "
        "X X "
        "X X "
        "XXX "
        "    ",
        "X X "
        "X X "
        "X X "
        "X X "
        " X  "
        "    ",
        "X X "
        "X X "
        "XXX "
        "XXX "
        " X  "
        "    ",
        "X X "
        "X X "
        " X  "
        "X X "
        "X X "
        "    ",
        "X X "
        "X X "
        " X  "
        " X  "
        " X  "
        "    ",
        "XXX "
        "  X "
        " X  "
        "X   "
        "XXX "
        "    ",
        " XX "
        " X  "
        "X   "
        " X  "
        " XX "
        "    ",
        " X  "
        " X  "
        " X  "
        " X  "
        " X  "
        "    ",
        "XX  "
        " X  "
        "  X "
        " X  "
        "XX  "
        "    ",
        " X X"
        "X X "
        "    "
        "    "
        "    "
        "    ",
        "  X "
        " X  "
        "XXXX"
        " X  "
        "  X "
        "    ",
};



static_assert(sizeof(font_4x6_table)/sizeof(font_4x6_table[0]) ==96);

using FontIconDouble = FontMatrix<8,6>;

constexpr FontIconDouble icons[] = {
        "O O O   "
        " OOO    "
        "  O  O O"
        "  O   O "
        "  O  O O"
        "        ",
        "O O O   "
        " OOO    "
        "  O    O"
        "  O O O "
        "  O  O  "
        "        ",

        "   #   #"
        "  #    #"
        " #     #"
        "#       "
        "#####  #"
        "#####   ",

};



template<typename Font>
void render_character(const Font &chdef, uint8_t x, uint8_t y) {
    for (int cy = 0; cy < chdef.h; ++cy) {
        for (int cx = 0; cx < chdef.w; ++cx) {
            set_pixel(x + cx, y + cy, chdef.get_pixel(cx, cy));
        }
    }
}

template<typename Font>
void render_character(const Font &font, uint8_t x, uint8_t y, int ascii_char) {
    if (ascii_char < 33) ascii_char = 32;
    else if (ascii_char > 127) ascii_char = '?';
    ascii_char -= 32;
    const Font4x6 &chdef = font[ascii_char];
    render_character(chdef, x, y);
}

void render_character_6x4(uint8_t x, uint8_t y, int ascii_char) {
    render_character(font_4x6_table, x, y, ascii_char);
}

void render_icon(uint8_t x, uint8_t y, Icon icon) {
    render_character(icons[static_cast<int>(icon)], x, y);
}

}

;

#pragma once
#include <string.h>
#include <alloca.h>

#ifdef __AVR__
#include <Arduino.h>
#else

namespace arduino {
    class __FlashStringHelper;
}
using arduino::__FlashStringHelper;
#endif

namespace Matrix_MAX7219 {

template<unsigned int _width, unsigned int _height, uint8_t _first_ascii, uint8_t _character_count, bool _monospace, bool _progmem, BitOrder _bit_order = BitOrder::msb_to_lsb>
class Font;


template<unsigned int _width, unsigned int _height, uint8_t _first_ascii, uint8_t _character_count, bool _progmem, BitOrder _bit_order>
class Font<_width, _height, _first_ascii, _character_count, true, _progmem, _bit_order> {
public:
    static constexpr int char_count = _character_count;
    static constexpr bool monospace = true;


    constexpr Font() = default;
    constexpr Font(const Bitmap<_width, _height, _bit_order, _progmem> (&bmps)[char_count]) {
        auto iter = _faces;
        for (const auto &b: bmps) {
            *iter = b;
            ++iter;
        }
    }

    static constexpr auto get_width() {return _width;}
    static constexpr auto get_height() {return _height;}
    static constexpr uint8_t get_face_width(char) {return _width;}

    constexpr const Bitmap<_width, _height, _bit_order, _progmem> *get_face(char c) const {
        uint8_t v = c;
        if (v < _first_ascii || v >= _first_ascii+_character_count) return nullptr;
        return _faces+v-_first_ascii;
    }



protected:


    friend class Font<_width, _height, _first_ascii, _character_count, false, _progmem, _bit_order>;

    Bitmap<_width, _height, _bit_order, _progmem> _faces[char_count] = {};
};

template<unsigned int _width, unsigned int _height, uint8_t _first_ascii, uint8_t _character_count, bool _progmem, BitOrder _bit_order>
class Font<_width, _height, _first_ascii, _character_count, false, _progmem, _bit_order>
    :public Font<_width, _height, _first_ascii, _character_count, true, _progmem, _bit_order>
{
public:

    static constexpr bool monospace = false;

    using Super = Font<_width, _height, _first_ascii, _character_count, true, _progmem, _bit_order>;

    constexpr Font() = default;
    constexpr Font(const Bitmap<_width, _height, _bit_order, _progmem> (&bmps)[Super::char_count], unsigned int space_size)
        :Super(bmps)
    {
        auto iter = this->_faces;
        for (auto &b: pwidth) {
            uint8_t v = 0;
            uint8_t u = _width;
            for (unsigned int y = 0; y < _height; ++y) {
                for (unsigned int x = 0; x < _width; ++x) {
                    if (iter->get_pixel(x, y)) {
                        if (x < u) u = x;
                        auto w = x + 2;
                        if (w > v) v = w;
                    }
                }
            }
            if (v == 0) v = space_size;
            else if (u) {
                for (unsigned int y = 0; y < _height; ++y) {
                    for (unsigned int x = 0; x < _width; ++x) {
                        unsigned int w = x + u;
                        if (w > _width) iter->set_pixel(x, y, false);
                        else iter->set_pixel(x, y, iter->get_pixel(w, y));
                    }
                }
                v -= u;
            }
            ++iter;
            b = v;
        }
    }
    constexpr Font(const Super &mono_font, unsigned int space_size)
            :Font(mono_font._faces, space_size) {}

    constexpr uint8_t get_face_width(char c) const {
        uint8_t v = c;
        if (v < _first_ascii || v >= _first_ascii+_character_count) return 0;
        v -= _first_ascii;
        uint8_t r = pwidth[v];
#ifdef __AVR__
        if constexpr(_progmem) {
            memcpy_P(&r, pwidth+v, sizeof(r));
        }
#endif
        return r;
    }

protected:

    uint8_t pwidth[Super::char_count] = {};

};


template<Transform _transform = Transform::none, BitOp _bitop = BitOp::copy>
struct TextOutputDef {

    template<typename _Bitmap, typename _Font, typename _Iter>
    constexpr static Point textout(_Bitmap &target, const _Font &font, Point pt, _Iter begin, _Iter end) {
        for (auto iter = begin; iter != end; ++iter) {
            auto fc = font.get_face(*iter);
            if (fc) {
                auto dir = target.put_image(pt, *fc, ImageTransform<_transform, _bitop>{});
                if (_Font::monospace) pt = pt + dir.get_width(font.get_width());
                else pt = pt + dir.get_width(font.get_face_width(*iter));
            }
        }
        return pt;
    }

    template<typename _Bitmap, typename _Font>
    constexpr static Point textout(_Bitmap &target, const _Font &font, Point pt, const char *text) {
        return textout(target, font, pt, text, text+strlen(text));
    }

    template<typename _Bitmap, typename _Font>
    static Point textout_P(_Bitmap &target, const _Font &font, Point pt, const char *text) {
#ifdef __AVR__
        auto len = strlen_P(text);
        char *c = alloca(len);
        memcpy_P(c, text, len);
        return textout(target, font, pt, c, c+len);
#else
        return textout(target, font, pt, text);
#endif
    }

    template<typename _Bitmap, typename _Font>
    static Point textout(_Bitmap &target, const _Font &font, Point pt, const __FlashStringHelper *text) {
        return textout_P(target, font, pt, reinterpret_cast<const char *>(text));
    }

    template<typename _Font, typename _Iter>
    constexpr static unsigned int get_text_width(const _Font &font, _Iter begin, _Iter end) {
        unsigned int acc = 0;
        for (auto iter = begin; iter != end; ++iter) {
            acc = acc + font.get_face_width(*iter);
        }
        return acc;

    }

    template<typename _Font>
    constexpr static unsigned int get_text_width(const _Font &font, const char *text) {
        return get_text_width(font, text, text+strlen(text));
    }


    template<typename _Font>
    static unsigned int get_text_width_P(const _Font &font, const char *text) {
    #ifdef __AVR__
        if constexpr(_Font::_monospace) {
            return  strlen_P(text) * font.get_width();
        } else {
            auto len = strlen_P(text);
            char *c = alloca(len);
            memcpy_P(c, text, len);
            return get_text_width(font, c, c+len);
        }
    #else
        return get_text_width(font,text);
    #endif
    }

    template<typename _Font>
    static unsigned int get_text_width(const _Font &font, const __FlashStringHelper *text) {
        return get_text_width_P(font, reinterpret_cast<const char *>(text));
    }
};



}

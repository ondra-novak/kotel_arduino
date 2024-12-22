#pragma once

namespace Matrix_MAX7219 {

///image transformation
enum class Transform {
    ///no transformation is done
    none,
    ///rotate right (90 degrees)
    rotate_clockwise,
    ///rotate 180 degrees
    upsidedown,
    ///rotate left (-90 degrees)
    rotate_counterclockwise,
    ///mirror horizontally (left right)
    mirror_h,
    ///mirror vertically (top bottom)
    mirror_v,
};


///image pixel operation
enum class BitOp {
    ///copy pixels (1:1)
    copy,
    ///copy pixels inverted
    invert_copy,
    ///AND operation
    mask,
    ///invert image and apply AND
    invert_mask,
    ///merge pixels OR
    merge,
    ///invert image and apply OR
    invert_merge,
    ///flip pixels XOR
    flip,
    ///invert image and apply  XOR
    invert_flip,
};

enum class ModuleOrder {
    left_to_right,
    right_to_left,
    left_to_right_zigzag,   //left to right on first line, and right to left on other line
    right_to_left_zigzag,   //right to left on first line, and left to right on other line
};

struct Matrix {
    int8_t a = 0;
    int8_t b = 0;
    int8_t c = 0;
    int8_t d = 0;
    int u = 0;
    int v = 0;
};

struct Point {
    int x = 0;
    int y = 0;
};

struct Direction {
    int x = 0;
    int y = 0;
};

constexpr Matrix get_transform(Transform trn, int u = 0, int v = 0) {
    switch (trn) {
        default:
        case Transform::none: return {1,0,0,1,u,v};
        case Transform::rotate_clockwise: return {0,1,-1,0,u,v};
        case Transform::rotate_counterclockwise: return {0,-1,1,0,u,v};
        case Transform::upsidedown: return {-1,0,0,-1,u,v};
        case Transform::mirror_h: return {-1,0,0,1,u,v};
        case Transform::mirror_v: return {1,0,0,-1,u,v};
    }
}



template<typename T>
static constexpr T fast_multiply_by_1_0(T val, int8_t coef) {
//#ifdef __AVR__
    return static_cast<T>(coef < 0?-val:coef>0?val:0);
/*#else
    return coef * val;
#endif*/
}

constexpr Point operator*(const Matrix &mx, const Point &pt) {
    return {
        fast_multiply_by_1_0(pt.x, mx.a) + fast_multiply_by_1_0(pt.y, mx.b) + mx.u,
        fast_multiply_by_1_0(pt.x, mx.c) + fast_multiply_by_1_0(pt.y, mx.d) + mx.v
    };
}

constexpr Direction operator*(const Matrix &mx, const Direction &pt) {
    return {
        fast_multiply_by_1_0(pt.x, mx.a) + fast_multiply_by_1_0(pt.y, mx.b),
        fast_multiply_by_1_0(pt.x, mx.c) + fast_multiply_by_1_0(pt.y, mx.d)
    };
}

constexpr Matrix operator~(const Matrix &m) {
    int det = fast_multiply_by_1_0(m.a , m.d) - fast_multiply_by_1_0(m.b , m.c);
    switch (det) {
        case 0:return {0,0,0,0,0,0};
        case 1:return {
            static_cast<int8_t>(m.d),
            static_cast<int8_t>(-m.b),
            static_cast<int8_t>(-m.c),
            static_cast<int8_t>(m.a),
            -(fast_multiply_by_1_0(m.u, m.d)+fast_multiply_by_1_0(m.v, -m.b)),
            -(fast_multiply_by_1_0(m.u, -m.c)+fast_multiply_by_1_0(m.v, m.a))};
        case -1: return {
            static_cast<int8_t>(-m.d),
            static_cast<int8_t>(m.b),
            static_cast<int8_t>(m.c),
            static_cast<int8_t>(-m.a),
            -(fast_multiply_by_1_0(m.u, -m.d )+fast_multiply_by_1_0(m.v, m.b)),
            -(fast_multiply_by_1_0(m.u, m.c)+fast_multiply_by_1_0(m.v, -m.a))};
        default: {
            Matrix inverse{static_cast<int8_t>(m.d/det), static_cast<int8_t>(-m.b/det), static_cast<int8_t>(-m.c/det), static_cast<int8_t>(m.a/det),0,0};
            inverse.u = -(fast_multiply_by_1_0(m.u, inverse.a)+fast_multiply_by_1_0(m.v, inverse.b));
            inverse.v = -(fast_multiply_by_1_0(m.u, inverse.c)+fast_multiply_by_1_0(m.v, inverse.d));
            return inverse;
        }
    }
}

constexpr Point operator+(const Point &pt, const Direction &dir) {
    return {pt.x + dir.x, pt.y + dir.y};
}

constexpr Point operator*(int constant, const Point &pt) {
    return Point{pt.x * constant, pt.y * constant};
}

constexpr Direction operator*(int constant, const Direction &dir) {
    return Direction{dir.x * constant, dir.y * constant};
}




}

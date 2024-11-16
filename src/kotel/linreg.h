#pragma once
#include <algorithm>
#include <numeric>

namespace kotel {


class LinReg {
public:

    LinReg() = default;
    template<typename Iter>
    LinReg(Iter from, Iter to) {
        if (from == to) {
            _slope = 0;
            _intercept = 0;
        } else {
            float mean_x;
            float mean_y = 0;
            int n = 0;
            std::for_each(from, to, [&](float yi){
                ++n;
                mean_y += yi;
            });
            if (n == 1) {
                _slope = 0;
                _intercept = mean_y;
            } else {
                mean_x = static_cast<float>(n - 1) / 2.0f;
                mean_y = mean_y / n;

                float numerator = 0.0f;
                float denominator = 0.0f;

                std::size_t i = 0;
                std::for_each(from, to, [&](float yi){
                    float xi = static_cast<float>(i);++i;
                    numerator += (xi - mean_x) * (yi - mean_y);
                    denominator += (xi - mean_x) * (xi - mean_x);
                });

                _slope = numerator / denominator;
                _intercept = mean_y - _slope * mean_x;
            }
        }
    }

    float operator()(int x) const {
        return _slope * x + _intercept;
    }

protected:
    float _slope = 0.0f;
    float _intercept = 0.0f;
};

}

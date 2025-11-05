#include <string_view>

template<int N>
constexpr auto init_string_constexpr(const char (&str)[N]) {
    return std::string_view(str, N-1);
}

constexpr auto project_version = init_string_constexpr("v2-8-g3845705");

#pragma once

#include <api/Stream.h>


template<typename Arg>
void print(Stream &s, const Arg &arg) {
    if constexpr(std::is_convertible_v<Arg, std::string_view>) {
        std::string_view str(arg);
        s.write(str.data(), str.size());
    } else {
        s.print(arg);
    }
}

template<typename Arg1,  typename Arg2, typename ... Args>
void print(Stream &s, const Arg1 &arg1, const Arg2 &arg2, const Args &... args) {
    print<Arg1>(s, arg1);
    print<Arg2, Args...>(s, arg2, args...);
}

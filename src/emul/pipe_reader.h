#pragma once
#include <cstdint>
#include <memory>
#include <string_view>

class PipeReader {
public:


    void open(std::string file);
    std::string_view read_line();
    void close();
    bool opened() const {return _fd != nullptr;}


protected:
    char buff[256];
    int rdpos = 0;
    int wrpos = 0;

    int getfd() const {
        return reinterpret_cast<std::uintptr_t>(_fd.get())-1;
    }

    struct Deleter {
        void operator()(void *p);
    };
    std::unique_ptr<void, Deleter> _fd;


};

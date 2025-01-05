#include "pipe_reader.h"
#include <unistd.h>
#include <fcntl.h>
#include <string>

void PipeReader::open(std::string file) {
    int fd = ::open(file.c_str(), O_NONBLOCK| O_CLOEXEC| O_RDONLY);
    _fd.reset(reinterpret_cast<void *>(fd + 1));
}

std::string_view PipeReader::read_line() {
    std::string_view s(buff+rdpos, wrpos-rdpos);
    auto sep = s.find('\n');
    if (sep == s.npos) {
        if (rdpos > 0) {
            if (wrpos > rdpos) {
                std::move(std::begin(buff)+rdpos, std::begin(buff)+wrpos, std::begin(buff));
            }
            wrpos -= rdpos;
            rdpos = 0;
        }
        if (wrpos == sizeof(buff)) {
            wrpos = 0;  //failsafe
        }
        int x = ::read(getfd(), buff+wrpos, sizeof(buff) - wrpos);
        if (x == -1) return {};
        wrpos += x;
        return read_line();
    }
    rdpos += sep+1;
    return s.substr(0,sep);
}

void PipeReader::close() {
    _fd.reset();
}

void PipeReader::Deleter::operator ()(void *p) {
    int i = reinterpret_cast<std::uintptr_t>(p);
    ::close(i-1);
}

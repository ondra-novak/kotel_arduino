#pragma once

#include <api/Stream.h>

namespace kotel {


class HasherStream: public Stream {
public:


    static const uint32_t FNV_PRIME = 16777619u;
    static const uint32_t OFFSET_BASIS = 2166136261u;

    uint32_t &hash;
    HasherStream(uint32_t &hash, bool init = true):hash(hash) {
        if (init) hash = OFFSET_BASIS;
    }

    virtual int available() override {return false;}
    virtual int read() override {return -1;}
    virtual int peek() override {return -1;}
    virtual size_t write(uint8_t x) {
       hash ^= x;
       hash *= FNV_PRIME;
       return 1;
    }


};


}

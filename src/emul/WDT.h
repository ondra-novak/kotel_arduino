
#pragma once
class WDTimer {
    public:

    int begin(uint32_t) {return 1;}
    void refresh() {}
    uint32_t getMaxTimeout() const {return 5500;}
};

inline WDTimer WDT;



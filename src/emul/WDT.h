
#pragma once
class WDTimer {
    public:

    int begin(uint32_t) {return 1;}
    void refresh() {}
};

inline WDTimer WDT;



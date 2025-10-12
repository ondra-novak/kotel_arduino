#pragma once
#ifndef SRC_KOTEL_PINREAD_H_
#define SRC_KOTEL_PINREAD_H_

#include <api/Common.h>

namespace kotel {

template<int pinId, unsigned short retry_count>
class PinReader {
public:

    short read() {
        short v = static_cast<short>(digitalRead(pinId));
        if (v != last_value) {
            if (failed_cnt++ == retry_count) {
                last_value = v;
                failed_cnt = 0;
            }
        } else {
            failed_cnt = 0;
        }
        return last_value;
    }


protected:
    short last_value = false;
    unsigned short failed_cnt = retry_count;
};


}




#endif /* SRC_KOTEL_PINREAD_H_ */

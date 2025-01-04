#pragma once

#include <Keyboard1W.h>
#include "constants.h"

namespace kotel {

/*

Keyboard circuit (https://www.falstad.com/circuit/circuitjs.html)

----------------------------------------------------------
$ 1 0.000005 10.20027730826997 65 5 50 5e-11
s 640 192 720 192 0 0 false
s 640 320 720 320 0 1 false
s 640 448 720 448 0 1 false
r 640 192 640 320 0 4700
r 640 320 640 448 0 4700
r 640 64 640 192 0 4700
r 640 448 640 576 0 4700
r 720 448 720 576 0 14100
w 640 576 720 576 0
g 640 576 640 608 0 0
R 640 64 640 16 0 0 40 5 0 0 0.5
p 832 320 832 416 3 0 0 10000000
w 832 416 832 576 0
w 832 576 720 576 0
w 720 320 720 448 0
w 720 192 720 320 0
w 720 320 832 320 0
----------------------------------------------------------



      ◯ +5V
      │
     ┌┴┐
     │ │
4K7  │ │    │
     └┬┘  ──┴──
      ⬤───⬤   ⬤───⬤────────⬤  A5
     ┌┴┐          │
     │ │          │
4K7  │ │    │     │
     └┬┘  ──┴──   │
      ⬤───⬤   ⬤───⬤
     ┌┴┐          │
     │ │          │
4K7  │ │    │     │
     └┬┘  ──┴──   │
      ⬤───⬤   ⬤───⬤
     ┌┴┐         ┌┴┐
     │ │         │ │
4K7  │ │         │ │ 14K1 (3x4K7)
     └┬┘         └┬┘
      ⬤───────────┘
     ┌┴┐
     │ │
4K7  │ │
     └┬┘
     ═╧═



 */


using MyKeyboard = Keyboard1W<3, 7>;

constexpr auto kbdcntr = MyKeyboard(keyboard_pin, {
        {0,{false,false,false}},
        {614,{true,false,false}},
        {382,{false,true,false}},
        {202,{false,false,true}},
        {558,{true,true,false}},
        {277,{false,true,true}},
        {437,{true,false,true}},
});

struct MyState: MyKeyboard::State {
    bool _utility_flag[3] = {};

    ///tests and resets utility flag associated with the key
    /**
     * @param idx button index
     * @retval true flag has been set (now it is reset)
     * @retval false flag was reset (and it is still reset)
     */
    bool test_and_reset_utility_flag(int idx) {
        if (_utility_flag[idx]) {
            _utility_flag[idx] = false;
            return true;
        }
        return false;
    }
    ///tests and resets utility flag associated with the key
    /**
     * @param st reference to state object. It must be state retrieved by function
     * get_state() - same reference
     * @retval true flag has been set (now it is reset)
     * @retval false flag was reset (and it is still reset)
     */
    bool test_and_reset_utility_flag(const MyKeyboard::KeyState &st) {
        return test_and_reset_utility_flag(&st - _states);
    }

    ///Sets utility flag
    /**
     * Utility flag can be used to implement multiple meanings on a single button
     * For example, if the button was long pressed, the utility flag can be
     * set to true. When button is released, an action is taken depend on
     * utility flag. If the utility flag is set, no action is taken, because action
     * has been performed on long press. If the utility flag is not set, then
     * long press did not happened and some action can be performed as result of
     * short press.
     *
     * @param idx button index
     */
    void set_utility_flag(int idx) {
        _utility_flag[idx] = true;
    }

    ///Sets utility flag
    /**
     * @param st reference to state object. It must be state retrieved by function
     * get_state() - same reference
     */
    void set_utility_flag(const MyKeyboard::KeyState &st) {
        _utility_flag[&st - _states] = true;
    }

};

constexpr int key_code_up = 0;
constexpr int key_code_stop = 1;
constexpr int key_code_down = 2;

}

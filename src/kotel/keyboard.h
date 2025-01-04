#pragma once

#include <Keyboard1W.h>
#include "constants.h"

namespace kotel {

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

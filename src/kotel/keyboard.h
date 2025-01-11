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
        {614,{false,false,true}},
        {382,{false,true,false}},
        {202,{true,false,false}},
        {558,{false,true,true}},
        {277,{true,true,false}},
        {437,{true,false,true}},
});


constexpr int key_code_up = 0;
constexpr int key_code_feeder = 0;
constexpr int key_code_stop = 1;
constexpr int key_code_full = 1;
constexpr int key_code_down = 2;
constexpr int key_code_fan = 2;

}


#pragma once
#include <cstdint>
#include <DotMatrix.h>

namespace kotel {

using WideIcon = DotMatrix::Bitmap<8,6>;
constexpr WideIcon icon_no_network =
        "O O O   "
        " OOO    "
        "  O  O O"
        "  O   O "
        "  O  O O"
        "        ";
constexpr WideIcon icon_network_ok =
        "O O O   "
        " OOO    "
        "  O    O"
        "  O O O "
        "  O  O  "
        "        ";
constexpr WideIcon icon_tray_open =
        "   #   #"
        "  #    #"
        " #     #"
        "#       "
        "#####  #"
        "#####   ";




}

#pragma once
#include <cstdint>
#include <DotMatrix.h>

namespace kotel {

using WideIcon = DotMatrix::Bitmap<8,6>;
constexpr auto icon_no_network = WideIcon(
        "O O O   "
        " OOO    "
        "  O  O O"
        "  O   O "
        "  O  O O"
        "        ");
constexpr auto icon_network_ok = WideIcon(
        "O O O   "
        " OOO    "
        "  O    O"
        "  O O O "
        "  O  O  "
        "        ");
constexpr auto icon_tray_open = WideIcon(
        "   #   #"
        "  #    #"
        " #     #"
        "#       "
        "#####  #"
        "#####   ");




}

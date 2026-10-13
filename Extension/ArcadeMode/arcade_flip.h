#pragma once
#include <cstdint>

namespace dingosdk::arcade_mode {
// Xbox X + direction names from Activision's THPS1+2 controls table:
// https://support.activision.com/tony-hawks-pro-skater-1-2/articles/controls-and-tricks-in-tony-hawks-pro-skater-1-2
// This is input selection only. Native Skate gesture profiles must be measured
// separately; selecting a name must never imply that the game performed it.
enum class FlipTrick : std::uint8_t {
    none, kickflip, heelflip, impossible, pop_shove_it, hardflip,
    inward_heelflip, varial_kickflip, varial_heelflip
};

constexpr FlipTrick flip_direction(std::uint16_t buttons, std::int16_t left_x,
                                  std::int16_t left_y) noexcept {
    constexpr std::uint16_t up = 0x0001, down = 0x0002, left = 0x0004, right = 0x0008;
    const auto pad = buttons & (up | down | left | right);
    // Physical D-pad wins over a steering stick. Invalid opposing directions
    // remain unselected rather than guessing a trick.
    int x = 0, y = 0;
    if (pad) {
        if ((pad & (left | right)) == (left | right) ||
            (pad & (up | down)) == (up | down)) return FlipTrick::none;
        x = (pad & right ? 1 : 0) - (pad & left ? 1 : 0);
        y = (pad & up ? 1 : 0) - (pad & down ? 1 : 0);
    } else {
        constexpr int threshold = 13000;
        x = left_x > threshold ? 1 : left_x < -threshold ? -1 : 0;
        y = left_y > threshold ? 1 : left_y < -threshold ? -1 : 0;
    }
    if (x == -1 && y == 0) return FlipTrick::kickflip;
    if (x == 1 && y == 0) return FlipTrick::heelflip;
    if (x == 0 && y == 1) return FlipTrick::impossible;
    if (x == 0 && y == -1) return FlipTrick::pop_shove_it;
    if (x == -1 && y == 1) return FlipTrick::hardflip;
    if (x == 1 && y == 1) return FlipTrick::inward_heelflip;
    if (x == -1 && y == -1) return FlipTrick::varial_kickflip;
    if (x == 1 && y == -1) return FlipTrick::varial_heelflip;
    return FlipTrick::none;
}
}

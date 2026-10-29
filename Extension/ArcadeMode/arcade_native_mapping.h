#pragma once
#include "arcade_authored_gestures.h"
#include "arcade_trick_table.h"
#include <span>

namespace dingosdk::arcade_mode {
// Native identifiers documented by the existing upstream Style Editor and
// confirmed current-game authored labels (Impossible: regular 36, nollie 38).
// Zero means no established exact mapping, never permission to substitute.
constexpr std::uint8_t native_flip_id(ArcadeTrick trick, bool nollie) noexcept {
    switch (trick) {
    case ArcadeTrick::kickflip: return nollie ? 17 : 2;
    case ArcadeTrick::impossible: return nollie ? 38 : 36;
    case ArcadeTrick::heelflip: return nollie ? 18 : 3;
    case ArcadeTrick::pop_shove_it: return nollie ? 19 : 4;
    case ArcadeTrick::hardflip: return nollie ? 20 : 9;
    case ArcadeTrick::inward_heelflip: return nollie ? 24 : 6;
    case ArcadeTrick::varial_kickflip: return nollie ? 23 : 5;
    case ArcadeTrick::varial_heelflip: return nollie ? 21 : 8;
    case ArcadeTrick::shove_it_360: return nollie ? 28 : 10;
    case ArcadeTrick::hardflip_360: return nollie ? 26 : 15;
    case ArcadeTrick::inward_heelflip_360: return nollie ? 30 : 12;
    case ArcadeTrick::flip_360: return nollie ? 29 : 11;
    case ArcadeTrick::heelflip_360: return nollie ? 27 : 14;
    default: return 0;
    }
}
// Only ordinary basic flips admitted by the same-flight proof may retain an
// existing Ollie trajectory. This grants neither trick admission nor an impulse.
constexpr bool ordinary_ollie_followup(std::int32_t id) noexcept {
    return id == 2 || id == 3 || id == 4;
}
inline const AuthoredGesture* exact_authored_gesture(std::span<const AuthoredGesture> catalog,
        std::uint8_t native_id, bool nollie, GestureContext context) noexcept {
    if (!native_id) return nullptr;
    for (const auto& gesture : catalog)
        if (gesture.native_id == native_id && gesture.nollie == nollie && gesture.context == context)
            return &gesture;
    return nullptr;
}
}

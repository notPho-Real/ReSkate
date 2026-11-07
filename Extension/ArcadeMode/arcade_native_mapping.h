#pragma once
#include "arcade_authored_gestures.h"
#include "arcade_trick_table.h"
#include <span>

namespace dingosdk::arcade_mode {
// Native identifiers documented by the existing upstream Style Editor and
// confirmed current-game authored labels (Impossible: regular 36, nollie 38).
// Zero means no established exact mapping, never permission to substitute.
constexpr std::uint8_t native_flip_id(ArcadeTrick trick, bool nollie) noexcept {
    const auto* definition=trick_definition(trick);
    return definition?(nollie?definition->nollie_flip:definition->native_flip):0;
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

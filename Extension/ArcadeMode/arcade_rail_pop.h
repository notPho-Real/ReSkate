#pragma once
#include "arcade_input_policy.h"

namespace dingosdk::arcade_mode {
// Admission for the installed game's exact _fromgrind stick recipe, not a
// velocity writer or a named-grind actor. 401 is the measured held grind;
// 403 is the independently captured fixed-rail contact/exit phase. Other
// members of the 400 family are exclusions, not verified launch admission.
inline bool rail_pop_input_ready(const InputNativeContext& native, std::uint64_t now) noexcept {
    return native.required && native.fresh(now) && native.rail_family && !native.airborne &&
        (native.flip.state==401 || native.flip.state==403) && native.flip.mode<=2;
}
inline bool rail_pop_input_continues(const InputNativeContext& native, std::uint64_t now) noexcept {
    // Native 402/404 are measured brief grind phases. They can retain an
    // already admitted hold without granting a new setup in an unknown phase.
    return native.required && native.fresh(now) && native.rail_family && !native.airborne &&
        native.flip.state>=401 && native.flip.state<=404 && native.flip.mode<=2;
}
}

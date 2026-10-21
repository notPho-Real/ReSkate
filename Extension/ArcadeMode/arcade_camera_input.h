#pragma once
#include "arcade_native_input_state.h"

namespace dingosdk::arcade_mode {
// Physical camera controls share gameplay's selected source and owner lease.
// No output/remapped stick or overlay's merged controller can supply this input.
struct CameraInputSnapshot {
    NativeInputOwner owner{};
    std::uint64_t owner_generation{}, route_generation{};
    unsigned slot{};
    std::uint16_t buttons{};
    short right_x{}, right_y{};
    bool valid{};
    bool same_binding(const CameraInputSnapshot& other) const noexcept {
        return valid && other.valid && owner == other.owner && owner_generation == other.owner_generation &&
            route_generation == other.route_generation && slot == other.slot;
    }
};
// Caller holds the same input mutex as both producers. Sampling does not renew
// either lease, select a controller, or establish native ownership.
template<class Route, class Input>
CameraInputSnapshot camera_input_snapshot(const Route& route, const Input& input,
                                          std::uint64_t now, bool enabled) noexcept {
    if (!enabled || !input.admitted(0,now)) return {};
    const auto pad = route.physical(now);
    const auto native = input.native_context(now);
    if (!pad.valid || !native.valid || !native.owner.valid() ||
        !native.owner_generation || native.owner_generation != input.owner_epoch()) return {};
    return {native.owner,native.owner_generation,route.generation(),route.selected_slot(),
        pad.buttons,pad.rx,pad.ry,true};
}
}

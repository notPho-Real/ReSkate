#pragma once
#include "Extension/AnimationStudio/anim_core.h"
#include <array>
#include <cmath>
#include <memory>
#include <stdexcept>

namespace dingosdk::arcade_mode::wallplant_animation {
// The native sequence's stationary phase, shared by its entry/release variants.
inline constexpr auto held_clip_name = "animation/dingo/c_proto_onb_grab_bs_1ft_judo_footplant_loop";
struct Clips { std::array<std::shared_ptr<const anim_studio::Clip>, 2> poses; };

// Keep asset resolution testable without a running game. A failed read or
// mirror cannot publish a partially populated stance set to the client thread.
template<class Library> Clips load_held_poses(const Library& library) {
    auto clip = library.load(held_clip_name);
    if (clip.empty() || !std::isfinite(clip.duration()) || clip.duration() <= 0)
        throw std::runtime_error("empty native footplant hold clip");
    Clips result;
    result.poses[1] = std::make_shared<const anim_studio::Clip>(library.mirror(clip));
    result.poses[0] = std::make_shared<const anim_studio::Clip>(std::move(clip));
    return result;
}
}

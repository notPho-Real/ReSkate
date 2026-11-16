#pragma once
#include "Extension/AnimationStudio/anim_core.h"
#include <algorithm>

namespace dingosdk::skater_animation {
// Playback replaces the body beneath the hips. Placement and the board remain
// native, and rotation-only tracks retain the current skater's bone lengths.
inline const std::array<bool, anim_studio::skeleton_joints>& body_joints() {
    static const auto mask = [] {
        std::array<bool, anim_studio::skeleton_joints> result{};
        for (std::uint16_t i = 0; i < result.size(); ++i)
            for (int at = i; at >= 0; at = anim_studio::parent(static_cast<std::uint16_t>(at)))
                if (at == anim_studio::hips_joint) { result[i] = true; break; }
        return result;
    }();
    return mask;
}
inline void blend_body_pose(std::span<anim_studio::Joint> native,
        std::span<const anim_studio::Joint> sample, const anim_studio::Clip& clip, float weight,
        bool displayed_hips_translation = false) {
    const auto& body = body_joints();
    const auto count = std::min({native.size(), sample.size(), body.size(), static_cast<std::size_t>(clip.joints)});
    for (std::size_t i = 0; i < count; ++i) {
        if (!body[i]) continue;
        auto& joint = native[i];
        const float joint_weight = weight * clip.joint_weight(i);
        joint.rotation = anim_studio::slerp(joint.rotation, sample[i].rotation, joint_weight);
        // A display turn changes the hips around the board pivot; all other
        // rotation-only tracks keep the native skater's bone lengths.
        if (clip.keeps_position(i) && !(i == anim_studio::hips_joint && displayed_hips_translation)) continue;
        for (std::size_t k = 0; k < 3; ++k)
            joint.position[k] += (sample[i].position[k] - joint.position[k]) * joint_weight;
    }
}
}

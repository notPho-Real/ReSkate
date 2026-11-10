#pragma once
#include "Extension/AnimationStudio/anim_core.h"
#include <cmath>

namespace dingosdk::skater_animation {
// Rotate only the displayed body and its placement-relative board. Native
// placement, board physics and every joint outside the hips subtree stay owned
// by the game. The caller applies this before blending its owned clip.
inline bool display_turn(std::span<anim_studio::Joint> pose, anim_studio::Joint* board,
                         float radians, anim_studio::Vec3 pivot = {}) noexcept {
    using namespace anim_studio;
    if (pose.size() != skeleton_joints || !std::isfinite(radians)) return false;
    for (float value : pivot) if (!std::isfinite(value) || std::abs(value) > 10) return false;
    if (radians == 0) return true;
    const auto placement = world(pose, placement_joint);
    const auto hip_parent = world(pose, static_cast<std::uint16_t>(parent(hips_joint)));
    const auto hips = world(pose, hips_joint);
    if (!finite(placement) || !finite(hip_parent) || !finite(hips) || (board && !finite(*board))) return false;
    const float scale = (placement.scale[0] + placement.scale[1] + placement.scale[2]) / 3;
    const float parent_scale = (hip_parent.scale[0] + hip_parent.scale[1] + hip_parent.scale[2]) / 3;
    if (scale < .001f || parent_scale < .001f) return false;
    const auto turn = axis_angle({0,1,0}, radians);
    const auto world_turn = multiply(multiply(placement.rotation, turn), conjugate(placement.rotation));
    auto center = rotate(placement.rotation, {pivot[0]*scale,pivot[1]*scale,pivot[2]*scale});
    for (unsigned i=0;i<3;++i) center[i] += placement.position[i];
    auto moved = rotate(world_turn, {hips.position[0]-center[0],hips.position[1]-center[1],hips.position[2]-center[2]});
    for (unsigned i=0;i<3;++i) moved[i] += center[i]-hip_parent.position[i];
    moved = rotate(conjugate(hip_parent.rotation), moved);
    auto& root = pose[hips_joint];
    for (unsigned i=0;i<3;++i) root.position[i] = moved[i]/parent_scale;
    root.rotation = normalised(multiply(conjugate(hip_parent.rotation), multiply(world_turn,hips.rotation)));
    if (board) {
        moved = rotate(turn, {board->position[0]-pivot[0],board->position[1]-pivot[1],board->position[2]-pivot[2]});
        for (unsigned i=0;i<3;++i) board->position[i] = pivot[i]+moved[i];
        board->rotation = normalised(multiply(turn,board->rotation));
    }
    return true;
}
}

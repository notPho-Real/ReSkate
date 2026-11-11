#pragma once
#include "pose_math.h"

// Shared cosmetic board geometry used by animation playback and authoring.
// These points estimate the displayed deck and wheels; they create no contacts.
namespace dingosdk::anim_studio::board_geometry {
using namespace pose_math;
struct Shape {
    std::array<Vec3, 6> points{};
    Vec3 up{0, 1, 0};
};
inline Shape from_pose(std::span<const Joint> pose,const Joint& board) {
    Shape shape;
    constexpr std::uint16_t left_foot_joint=343,right_foot_joint=10;
    const auto placed = world(pose, placement_joint);
    const auto between=rotate(conjugate(placed.rotation),
        sub(world(pose,left_foot_joint).position,world(pose,right_foot_joint).position));
    const Vec3 feet{between[0],0,between[2]};
    std::size_t up = 1, along = 0;
    float best_up = -1, best_along = -1;
    std::array<Vec3, 3> axes{};
    for (std::size_t i = 0; i < 3; ++i) {
        Vec3 e{};
        e[i] = 1;
        axes[i] = rotate(board.rotation, e);
        if (std::abs(axes[i][1]) > best_up) best_up = std::abs(axes[i][1]), up = i;
    }
    for (std::size_t i = 0; i < 3; ++i) {
        if (i == up) continue;
        const float d = std::abs(axes[i][0] * feet[0] + axes[i][2] * feet[2]);
        if (d > best_along) best_along = d, along = i;
    }
    const std::size_t side = 3 - up - along;
    const float up_sign = axes[up][1] >= 0 ? 1.0f : -1.0f;
    const auto point = [&](float l, float u, float w) {
        Vec3 v{};
        v[along] = l;
        v[up] = u * up_sign;
        v[side] = w;
        return v;
    };
    shape.up = point(0, 1, 0);
    constexpr float tip = 0.40f, kick = 0.04f, wheelbase = 0.18f, track = 0.09f, wheels = 0.087f;
    shape.points = {point(tip, kick, 0), point(-tip, kick, 0), point(wheelbase, -wheels, track), point(wheelbase, -wheels, -track),
                    point(-wheelbase, -wheels, track), point(-wheelbase, -wheels, -track)};
    return shape;
}
inline float lowest_point(const Joint& board,const Shape& shape) noexcept {
    float low=board.position[1];
    for(const auto& pt:shape.points)low=std::min(low,board.position[1]+rotate(board.rotation,pt)[1]);
    return low;
}
} // namespace dingosdk::anim_studio::board_geometry

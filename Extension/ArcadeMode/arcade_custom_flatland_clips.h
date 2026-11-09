#pragma once
#include "arcade_custom_flatland.h"
#include <stdexcept>

namespace dingosdk::arcade_mode {
// Original authored offsets, not copied animation data. The caller supplies an
// owned local Manual seed (395 parent-local joints and placement-relative board).
// This rough pose demonstrates the animation route; it is not contact/IK proof.
inline CustomTrickClips make_casper_clips(std::span<const anim_studio::Joint> seed,
        const anim_studio::Joint& board) {
    using namespace anim_studio;
    if(seed.size()!=skeleton_joints || !finite(board))throw std::invalid_argument("flatland seed");
    for(const auto& joint:seed)if(!finite(joint))throw std::invalid_argument("flatland joint");
    const auto make=[&](bool flip) {
        Animation animation;
        const float duration=flip?.6f:15.f;
        const std::array<float,7> times=flip?std::array<float,7>{0,.1f,.2f,.3f,.4f,.5f,.6f}:
            std::array<float,7>{0,.1f,.2f,.3f,14.7f,14.85f,15.f};
        for(auto [joint,degrees]:std::array<std::pair<std::uint16_t,Vec3>,8>{{
            {hips_joint,{0,0,-12}},{42,{8,0,0}},{341,{-28,0,0}},{342,{48,0,0}},
            {343,{-24,0,0}},{8,{18,0,0}},{9,{30,0,0}},{10,{-35,0,0}}}}) {
            const auto rotation=from_euler(degrees);
            if(flip) {
                set_key(animation,joint,0,{rotation,{}});
                set_key(animation,joint,.2f,{rotation,{}});
                set_key(animation,joint,duration,{});
            } else {
                set_key(animation,joint,0,{});
                set_key(animation,joint,.3f,{rotation,{}});
                set_key(animation,joint,14.7f,{rotation,{}});
                set_key(animation,joint,duration,{});
            }
        }
        auto clip=std::make_shared<Clip>();clip->joints=skeleton_joints;
        // Each adjacent board roll is <=90 degrees, preserving a full turn.
        for(float time:times) {
            clip->times.push_back(time);
            std::vector<Joint> pose(seed.begin(),seed.end());
            apply_edits(animation,time,pose);
            clip->frames.insert(clip->frames.end(),pose.begin(),pose.end());
            const float weight=flip?1.f-std::clamp((time-.2f)/.4f,0.f,1.f):
                std::min(std::clamp(time/.3f,0.f,1.f),std::clamp((15-time)/.3f,0.f,1.f));
            const float angle=flip?180+540*(time/duration):180*weight;
            auto b=board;
            b.rotation=multiply(b.rotation,axis_angle({0,0,1},angle*3.14159265358979f/180));
            b.position[1]+=.08f*weight; // render clearance only; no rigid body is moved.
            clip->board.push_back(b);
        }
        return clip;
    };
    return {make(false),make(true)};
}
}

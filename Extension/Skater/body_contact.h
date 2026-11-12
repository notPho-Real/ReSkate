#pragma once
#include "body_pose.h"
#include "arm_reach.h"
#include "skater_skeleton.h"
#include <cmath>

namespace dingosdk::skater_animation {
// A held authored body can inherit the physical placement's bank and pitch.
// Level its frame about the existing hips, then solve both feet: the observed
// surface endpoint and the other foot's original board-relative endpoint.
// This is render-pose math only. A failed reach publishes none of the changes.
inline bool level_body_contact(std::span<anim_studio::Joint> pose, std::uint16_t foot,
        const anim_studio::Vec3& point, const anim_studio::Quat& rotation, float weight) {
    using namespace anim_studio;
    if(pose.size()!=skeleton_joints || !std::isfinite(weight) || weight<0 || weight>1 ||
       (foot!=bonecam::joint::left_foot && foot!=bonecam::joint::right_foot))return false;
    const auto other=foot==bonecam::joint::left_foot?bonecam::joint::right_foot:bonecam::joint::left_foot;
    const auto placement=world(pose,placement_joint),hips=world(pose,hips_joint);
    const auto held=world(pose,other);
    if(!finite(placement) || !finite(hips) || !finite(held))return false;
    for(const auto& joint:pose) {
        float square{};
        for(const auto value:joint.rotation)square+=value*value;
        if(!finite(joint) || std::abs(square-1.f)>.001f || joint.scale[0]<=1e-6f ||
           std::abs(joint.scale[0]-joint.scale[1])>1e-5f ||
           std::abs(joint.scale[0]-joint.scale[2])>1e-5f)return false;
    }
    const auto forward=rotate(placement.rotation,{0,0,1});
    if(std::hypot(forward[0],forward[2])<.00001f)return false;
    const float half=std::atan2(forward[0],forward[2])*.5f;
    const Quat upright{0,std::sin(half),0,std::cos(half)};
    const auto parent_world=world(pose,static_cast<std::uint16_t>(parent(hips_joint)));
    const auto corrected=normalised(multiply(multiply(upright,conjugate(placement.rotation)),hips.rotation));
    std::array<Joint,skeleton_joints> result;
    std::copy(pose.begin(),pose.end(),result.begin());
    result[hips_joint].rotation=normalised(multiply(conjugate(parent_world.rotation),corrected));
    const auto board=arm_reach::reach_joint(result,other,held.position,1,held.rotation);
    if(!board.valid || !board.reached)return false;
    const auto planted=arm_reach::reach_joint(result,foot,point,1,rotation);
    if(!planted.valid || !planted.reached)return false;
    const auto& body=body_joints();
    for(std::size_t i=0;i<pose.size();++i)
        if(body[i])pose[i].rotation=slerp(pose[i].rotation,result[i].rotation,weight);
    return true;
}
}

#pragma once
#include "body_pose.h"
#include "arm_reach.h"
#include "skater_skeleton.h"
#include <cmath>

namespace dingosdk::skater_animation {
// A held contact uses the observed point, not the tangential coordinates of
// an authored ground foot. Projecting that low foot onto a wall can put the
// target beyond the leg even when the actual collision point is reachable.
inline std::optional<arm_reach::Placed> held_surface_target(const anim_studio::Joint& foot,
        const arm_reach::ContactFrame& frame, const anim_studio::Vec3& point,
        const anim_studio::Vec3& normal) {
    auto target=arm_reach::contact_target(foot,frame,point,normal);
    if(target) {
        const float length=std::hypot(normal[0],normal[1],normal[2]);
        for(unsigned axis=0;axis<3;++axis)
            target->position[axis]=point[axis]+normal[axis]/length*frame.clearance*foot.scale[0];
        for(const auto value:target->position)if(!std::isfinite(value))return {};
    }
    return target;
}
// Prepare the same body frame for contact selection, anchoring and drawing.
// Keep the board foot fixed while the hips lose inherited placement bank.
inline bool level_contact_pose(std::span<anim_studio::Joint> pose, std::uint16_t foot) {
    using namespace anim_studio;
    if(pose.size()!=skeleton_joints ||
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
    std::copy(result.begin(),result.end(),pose.begin());
    return true;
}
// Fix the endpoint from the held authored pose, before the entry blend. Using
// the first partially blended foot would lock a different anchor at each draw
// cadence and force the fully blended leg back into the incoming crouch.
inline std::optional<arm_reach::Placed> held_contact_target(
        std::span<const anim_studio::Joint> native, const anim_studio::Clip& clip,
        float time, std::uint16_t foot, const arm_reach::ContactFrame& frame,
        const anim_studio::Vec3& point, const anim_studio::Vec3& normal, bool level_body=false) {
    using namespace anim_studio;
    if(native.size()!=skeleton_joints || clip.joints!=skeleton_joints ||
       clip.times.empty() || clip.frames.size()/clip.joints<clip.times.size() ||
       !std::isfinite(time) || time<0 || time>clip.duration() ||
       (foot!=bonecam::joint::left_foot && foot!=bonecam::joint::right_foot))return {};
    std::array<Joint,skeleton_joints> pose,sample;
    std::copy(native.begin(),native.end(),pose.begin());
    sample=pose;
    clip.sample(time,sample);
    blend_body_pose(pose,sample,clip,1);
    if(level_body && !level_contact_pose(pose,foot))return {};
    return level_body?held_surface_target(world(pose,foot),frame,point,normal):
        arm_reach::contact_target(world(pose,foot),frame,point,normal);
}
// A held authored body can inherit the physical placement's bank and pitch.
// Level its frame about the existing hips, then solve both feet: the observed
// surface endpoint and the other foot's original board-relative endpoint.
// This is render-pose math only. A failed reach publishes none of the changes.
inline bool level_body_contact(std::span<anim_studio::Joint> pose, std::uint16_t foot,
        const anim_studio::Vec3& point, const anim_studio::Quat& rotation, float weight) {
    using namespace anim_studio;
    if(pose.size()!=skeleton_joints || !std::isfinite(weight) || weight<0 || weight>1 ||
       (foot!=bonecam::joint::left_foot && foot!=bonecam::joint::right_foot))return false;
    std::array<Joint,skeleton_joints> result;
    std::copy(pose.begin(),pose.end(),result.begin());
    if(!level_contact_pose(result,foot))return false;
    const auto planted=arm_reach::reach_joint(result,foot,point,1,rotation);
    if(!planted.valid || !planted.reached)return false;
    const auto& body=body_joints();
    for(std::size_t i=0;i<pose.size();++i)
        if(body[i])pose[i].rotation=slerp(pose[i].rotation,result[i].rotation,weight);
    return true;
}
// Solve the held authored pose before its entry/release blend. Solving an
// already blended pose uses different hips and board-foot positions from the
// pose that supplied the anchor, and applies the blend a second time.
inline bool blend_held_contact(std::span<anim_studio::Joint> native,
        const anim_studio::Clip& clip, float time, std::uint16_t foot,
        const anim_studio::Vec3& point, const anim_studio::Quat& rotation, float weight) {
    using namespace anim_studio;
    if(native.size()!=skeleton_joints || clip.joints!=skeleton_joints ||
       clip.times.empty() || clip.frames.size()/clip.joints<clip.times.size() ||
       !std::isfinite(time) || time<0 || time>clip.duration() ||
       !std::isfinite(weight) || weight<0 || weight>1)return false;
    std::array<Joint,skeleton_joints> posed,sample;
    std::copy(native.begin(),native.end(),posed.begin());sample=posed;
    clip.sample(time,sample);
    blend_body_pose(posed,sample,clip,1);
    if(!level_body_contact(posed,foot,point,rotation,1))return false;
    blend_body_pose(native,posed,clip,weight);
    return true;
}
// A surface endpoint is only a candidate until the full pose can reach it.
// Keep the first successful endpoint fixed; a refused first draw must not lock
// subsequent draws to an unreachable point from an earlier placement.
inline bool blend_held_surface_contact(std::span<anim_studio::Joint> native,
        const anim_studio::Clip& clip, float time, std::uint16_t foot,
        const arm_reach::ContactFrame& frame, const anim_studio::Vec3& point,
        const anim_studio::Vec3& normal, float weight,
        std::optional<arm_reach::Placed>& anchor,
        std::optional<arm_reach::Placed>* attempted=nullptr) {
    const auto candidate=anchor?anchor:held_contact_target(native,clip,time,foot,frame,point,normal,true);
    if(attempted)*attempted=candidate;
    if(!candidate || !blend_held_contact(native,clip,time,foot,candidate->position,candidate->rotation,weight))return false;
    anchor=candidate;
    return true;
}
}

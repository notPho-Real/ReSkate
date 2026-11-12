#pragma once
#include "arcade_wallplant_animation_measurement.h"
#include "Extension/Skater/arm_reach.h"
#include "Extension/Skater/body_pose.h"
#include "Extension/Skater/skater_skeleton.h"

namespace dingosdk::arcade_mode::wallplant_animation {
struct PlantContact {
    PlantFrame sample;
    arm_reach::ContactFrame frame;
};
inline bool valid_contact_pose(std::span<const anim_studio::Joint> pose) noexcept {
    for(const auto& joint:pose) {
        if(!anim_studio::finite(joint))return false;
        float square{};
        for(const auto value:joint.rotation)square+=value*value;
        if(std::abs(square-1.f)>.001f || joint.scale[0]<=1e-6f ||
           std::abs(joint.scale[0]-joint.scale[1])>1e-5f || std::abs(joint.scale[0]-joint.scale[2])>1e-5f)return false;
    }
    return true;
}
// The lower foot in the authored support frame supplies the contact. Evaluate
// its wall reach on the same observed native body geometry used by playback;
// a raised foot cannot win merely because it happens to face the wall.
inline std::optional<PlantContact> plant_contact(const anim_studio::Clip& clip,std::uint16_t ground,
        const anim_studio::Vec3& wall_direction,std::span<const anim_studio::Joint> native) {
    if(native.size()!=anim_studio::skeleton_joints || clip.joints!=native.size() ||
       ground>=clip.joints || clip.times.empty() || clip.frames.size()/clip.joints<clip.times.size() ||
       !valid_contact_pose(native))return {};
    std::optional<PlantFrame> best;
    std::optional<PlantContact> result;
    std::vector<anim_studio::Joint> pose(native.begin(),native.end());
    for(std::size_t i=0;i<clip.times.size();++i) {
        if(!std::isfinite(clip.times[i]) || clip.times[i]<0 || clip.times[i]>clip.duration())return {};
        const auto authored=clip.frame(i);
        if(!valid_contact_pose(authored))return {};
        const auto support=anim_studio::world(authored,ground);
        std::optional<arm_reach::ContactFrame> reference;
        std::uint16_t selected{};
        float height{};
        for(const auto joint:{bonecam::joint::left_foot,bonecam::joint::right_foot}) {
            const auto foot=anim_studio::world(authored,joint);
            const auto frame=arm_reach::contact_frame(foot,support);
            if(!frame)return {};
            const auto clearance=frame->clearance*foot.scale[0];
            if(!reference || clearance<height) {reference=frame;selected=joint;height=clearance;}
        }
        std::copy(native.begin(),native.end(),pose.begin());
        skater_animation::blend_body_pose(pose,authored,clip,1.f);
        const auto placement=anim_studio::world(pose,anim_studio::placement_joint);
        const auto scale=(placement.scale[0]+placement.scale[1]+placement.scale[2])/3;
        const auto foot=anim_studio::world(pose,selected);
        if(!anim_studio::finite(placement) || !anim_studio::finite(foot) || !std::isfinite(scale) || scale<=.00001f)return {};
        auto relative=anim_studio::rotate(anim_studio::conjugate(placement.rotation),
            {foot.position[0]-placement.position[0],foot.position[1]-placement.position[1],foot.position[2]-placement.position[2]});
        for(auto& value:relative)value/=scale;
        if(!consider_plant_frame(best,clip.times[i],selected,relative,wall_direction))return {};
        if(best->time==clip.times[i] && best->joint==selected)result=PlantContact{*best,*reference};
    }
    return result;
}
}

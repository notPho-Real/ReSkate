#pragma once
#include "arcade_custom_flatland.h"
#include "arcade_native_wall_contact.h"
#include <string_view>
namespace dingosdk::arcade_mode {
inline FlatlandSurface flatland_surface_kind(std::string_view name) noexcept {
    if(name=="Asphalt" || name=="Roofing_Asphalt" || name=="Earth" || name=="Grass" ||
       name=="Gravel" || name=="Sand" || name=="Concrete_Rough" || name=="Rubber" ||
       name=="Wood_Thin_Rough" || name=="Wood_Thick_Rough" || name=="Metal_Grate" ||
       name=="Metal_DiamondPlate")return FlatlandSurface::rough;
    if(name=="Wood_Thin_Smooth" || name=="Wood_Thick_Smooth" || name=="Plastic")return FlatlandSurface::sealed;
    if(name=="Metal" || name=="Metal_Thin" || name=="Metal_Rail" || name=="Marble" || name=="Glass")
        return FlatlandSurface::slick;
    if(name=="Default" || name=="Concrete" || name=="Brick" || name=="Bedrock")return FlatlandSurface::ordinary;
    return FlatlandSurface::unknown;
}
class FlatlandSurfaceJoin {
    NativeInputFrame previous_{};
    std::uint64_t serial_{};
public:
    FlatlandSurfaceEvidence observe(const NativeInputFrame& frame,
            const native_wall_contact::SupportMaterial& support) noexcept {
        if(frame.owner!=previous_.owner || frame.owner_generation!=previous_.owner_generation){previous_={};serial_=0;}
        if(!frame.valid || frame.physics_step<=previous_.physics_step) return {};
        const auto previous=previous_;const auto serial=serial_;
        previous_=frame;serial_=support.serial;
        const auto& owner=support.owner;
        if(!previous.valid || previous.owner!=frame.owner || previous.owner_generation!=frame.owner_generation ||
           !support.valid || !support.serial || support.serial==serial ||
           owner.base!=frame.owner.base || owner.entity!=frame.owner.entity ||
           owner.core!=frame.owner.core || owner.context!=frame.owner.context || owner.rig!=frame.owner.rig ||
           support.sampled_at<previous.sampled_at || support.sampled_at>frame.sampled_at ||
           frame.sampled_at-previous.sampled_at>150 ||
           !std::isfinite(support.seconds) || std::abs(support.seconds-frame.physics_seconds)>1e-6f)return {};
        const auto kind=flatland_surface_kind(support.material.label());
        return {kind,frame.owner,frame.owner_generation,frame.contact_generation,frame.physics_step,
            kind!=FlatlandSurface::unknown};
    }
};
}

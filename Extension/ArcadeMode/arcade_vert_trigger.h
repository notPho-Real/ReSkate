#pragma once
#include "arcade_vert_air.h"
#include "arcade_trigger_input.h"
namespace dingosdk::arcade_mode {
inline TriggerDecision qualify_vert_trigger(const VertAirController& controller,const TriggerPress& input,
        const InputNativeContext& current,std::uint64_t now) noexcept {
    if(!input || input.channel()>1 || input.press.token.source!=InputSource::triggers ||
       input.press.token.edge!=InputEdge::press || !input.native.fresh(input.press.at) || !current.fresh(now) ||
       (input.context!=InputContext::air && input.context!=InputContext::ground) ||
       (input.owner!=TriggerOwner::none && input.owner!=TriggerOwner::landing_revert && input.owner!=TriggerOwner::transfer))return {};
    const auto& frame=controller.observation().native;
    if(!frame.valid || current.generations.owner!=frame.owner_generation ||
       input.native.generations.owner!=frame.owner_generation || now<input.press.at)return {};
    const auto& landing=controller.landing();
    const auto& takeoff=landing.takeoff();
    if(input.owner!=TriggerOwner::transfer) {
        const auto deadline=landing.deadline(input.native,input.press.at,frame,now);
        if(deadline && current.airborne_episode==frame.airborne_episode &&
           current.air_departure==frame.air_departure && current.airborne==frame.airborne)
            return {TriggerOwner::landing_revert,deadline,landing.ready(frame,now) &&
                !current.airborne && current.flip.state==100 && current.flip.mode<=1};
        // The native producer can publish ground before the physical bridge
        // observes its wheel contacts. Retain that edge across this short gap;
        // it still cannot execute until the independent contact arrives.
        if(landing.contact_publication_pending(current,now) && input.press.at>=takeoff.sampled_at &&
           now-input.press.at<=VertLandingWindow::before_ms &&
           input.native.airborne_episode==takeoff.airborne_episode &&
           input.native.air_departure==takeoff.air_departure+(input.native.airborne?0u:1u))
            return {TriggerOwner::landing_revert,input.press.at+VertLandingWindow::before_ms+VertLandingWindow::after_ms,false};
    }
    if(input.owner==TriggerOwner::landing_revert)return {};
    if(input.context!=InputContext::air || !input.native.airborne || !current.airborne ||
       !controller.eligible(frame,now) || current.air_episode()!=input.native.air_episode() ||
       current.air_episode()!=InputNativeContext::from_frame(frame).air_episode() ||
       input.native.physics_step<takeoff.physics_step || input.press.at<takeoff.sampled_at ||
       now-input.press.at>150)return {};
    return {TriggerOwner::transfer,input.press.at+150,true};
}
}

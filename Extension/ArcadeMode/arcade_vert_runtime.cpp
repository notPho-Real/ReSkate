#include "arcade_vert_runtime.h"
#include "arcade_native_wall_contact.h"
#include <mutex>
#include <cstring>
#if defined(DINGOSDK_DEV_TOOLS)
#include "Engine/Core/Log/logging.h"
#endif
namespace dingosdk::arcade_mode::vert_runtime {
namespace {
// Controller state is short-lived and never calls back into input ownership.
// Query scopes and their input-context validation stay outside this mutex.
// Contention cannot revoke flight/trigger authority or enable ground fallback.
std::mutex mutex;
VertAirController controller;
VertPatch support;
NativeInputFrame support_frame;
}
void cancel() noexcept {std::lock_guard lock(mutex);controller.reset();support={};support_frame={};}
TriggerDecision qualify(void*,const TriggerPress& press,const InputNativeContext& current,std::uint64_t now) noexcept {
    // Contention is not evidence of ordinary ground. All query callbacks that
    // acquire input ownership run outside this mutex; locked owner validation
    // uses bounded memory reads only. Observe the completed controller proof
    // before allowing the shared dispatcher to fall through to ground controls.
    std::lock_guard lock(mutex);
    return qualify_vert_trigger(controller,press,current,now);
}
InputSurfaceState surface(std::uint64_t now,const NativeInputFrame& frame,InputSurfaceState other) noexcept {
    std::lock_guard lock(mutex);
    if(!controller.eligible(frame,now))return other;
    other.fresh=other.can_transfer=true;other.transfer_buttons=0;
    other.generation=frame.airborne_episode;other.physics_step=frame.physics_step;
    other.generations=InputNativeContext::from_frame(frame).generations;
    other.can_plant=other.can_push=false;return other;
}
InputDisposition request(const InputRequest& input,std::uint64_t now,const NativeInputFrame& frame) noexcept {
    std::lock_guard lock(mutex);
    if(input.action!=InputAction::transfer_command || input.context!=InputContext::air ||
       input.air_episode!=InputNativeContext::from_frame(frame).air_episode() ||
       input.generations.owner!=frame.owner_generation || input.first_input_at!=input.physical_pressed_at)
        return InputDisposition::cancelled;
    auto direction=controller.direction();
    if(input.direction>=1 && input.direction<=9) {
        const int lateral=(input.direction-1)%3-1;
        const int longitudinal=(input.direction-1)/3-1;
        const auto right=vert_math::cross(direction,{0,1,0});
        direction=vert_math::add(vert_math::scale(direction,longitudinal<0?.5f:1.f),
            vert_math::scale(right,static_cast<float>(lateral)*.65f));
    }
    return controller.request(frame,now,input.physical_pressed_at,input.input_sequence,direction) ?
        InputDisposition::started:InputDisposition::cancelled;
}
SurfaceGeometryVector alignment(const NativeInputFrame& frame,std::uint64_t now) noexcept {
    std::lock_guard lock(mutex);
    return controller.alignment(frame,now);
}
Motion step(VertObservation observation,std::uint64_t now,const NativeInputOwner& physical_owner,
        const char* frame_reason,const NativeInputCorrelation* correlation,const NativeRequestSample* live) noexcept {
    std::lock_guard lock(mutex);
    const auto& frame=observation.native;
    if(!frame.valid) {
        const bool transition=frame_reason && std::strcmp(frame_reason,"frame_correlation")==0 &&
            correlation && live && controller.publication_gap(*correlation,*live,physical_owner,now);
        if(transition || (frame_reason && std::strcmp(frame_reason,"frame_lock_busy")==0))
            controller.unavailable(now,physical_owner,observation.enabled,observation.contested);
        else controller.observe(observation);
#if defined(DINGOSDK_DEV_TOOLS)
        static std::uint64_t last_unavailable{};
        if(now<last_unavailable || now-last_unavailable>=250) {
            last_unavailable=now;
            logging::log(logging::Level::info,logging::Channel::skater,
                "ARCADE VERT UNAVAILABLE: reason={} phase={} previous_ms={} now={} supported={} up_y={}.",
                frame_reason?frame_reason:"unknown",static_cast<unsigned>(controller.phase()),
                controller.observation().native.sampled_at,now,observation.supported,observation.up[1]);
        }
#endif
        // Withholding the ordinary Arcade air shaper is not motion authority.
        // Native gravity/collision continue during this bounded history gap.
        return {{},false,false,controller.preserves_native()};
    }
    if(support.valid && frame.valid && frame.owner==support_frame.owner &&
       frame.owner_generation==support_frame.owner_generation && !frame.airborne && !support_frame.airborne &&
       frame.sampled_at>=support_frame.sampled_at && frame.sampled_at-support_frame.sampled_at<=100 &&
       vert_math::length(vert_math::sub(observation.position,support.center().point))<2)
        if(std::any_of(support.samples.begin(),support.samples.end(),[&](const auto& sample) {
            return sample.body==observation.contacted_shape;
        }))observation.support=support;
    if(frame.airborne && controller.origin().valid) {
        for(const auto& contact:controller.origin().contacts) {
            SurfaceGeometryWorldOwner current{};
            if(!native_wall_contact::static_world_owner(contact.shape,current) || current!=contact.owner) {
                controller.cancel();break;
            }
        }
    }
    if(controller.phase()==VertPhase::released) {
        const auto& released=controller.released_plan();
        for(const auto& sample:released.destination.samples)if(released.destination.valid) {
            SurfaceGeometryWorldOwner current{};
            if(!native_wall_contact::static_world_owner(sample.body,current) || current!=sample.world_owner) {
                controller.cancel();break;
            }
        }
    }
    [[maybe_unused]] const auto before=controller.phase();
    controller.observe(observation);
    auto plan=controller.consume(frame,frame.sampled_at,observation.velocity);
    if(plan.destination.valid)for(const auto& sample:plan.destination.samples) {
        SurfaceGeometryWorldOwner current{};
        if(!native_wall_contact::static_world_owner(sample.body,current) || current!=sample.world_owner) {
            controller.cancel();plan={};break;
        }
    }
#if defined(DINGOSDK_DEV_TOOLS)
    if(before!=controller.phase() || plan || (!frame.airborne && observation.up[1]<.8f && frame.physics_step%6==0))logging::log(logging::Level::info,logging::Channel::skater,
        "ARCADE VERT: step={} episode={} phase={} action={} position={},{},{} velocity={},{},{} planned={},{},{} supported={} patch={} native_ramp={} normal={},{},{} mode={}.",
        frame.physics_step,frame.airborne_episode,static_cast<unsigned>(controller.phase()),static_cast<unsigned>(plan.action),
        observation.position[0],observation.position[1],observation.position[2],
        observation.velocity[0],observation.velocity[1],observation.velocity[2],
        plan.velocity[0],plan.velocity[1],plan.velocity[2],observation.supported,observation.support.valid,observation.native_ramp.valid,
        observation.ramp().normal[0],observation.ramp().normal[1],observation.ramp().normal[2],frame.mode);
#endif
    return {plan,controller.phase()==VertPhase::native_air || controller.phase()==VertPhase::requested ||
        controller.phase()==VertPhase::released,controller.phase()==VertPhase::released};
}
void geometry(const SurfaceGeometryNativeApi& supplied,const SurfaceGeometryContext& context,
        const NativeInputFrame& frame,SurfaceGeometryVector board,SurfaceGeometryVector up) noexcept {
    auto api=supplied;
    api.static_world_owner=[](void*,const SurfaceGeometryBody& shape,SurfaceGeometryWorldOwner& owner) noexcept {
        return native_wall_contact::static_world_owner(shape,owner);
    };
    api.local_component_owner=[](void*,const SurfaceGeometryContext& ctx,const SurfaceGeometryBody& shape,
            SurfaceGeometryWorldOwner& owner) noexcept {
        return native_wall_contact::local_component_owner(shape,ctx.owner[1],owner);
    };
    if(!api.now || !api.current)return;
    if(!api.current(api.user,context)) {
#if defined(DINGOSDK_DEV_TOOLS)
        static std::uint64_t reported{};const auto failed_at=api.now(api.user);
        if(failed_at<reported || failed_at-reported>=250) {
            reported=failed_at;
            logging::log(logging::Level::info,logging::Channel::skater,
                "ARCADE VERT QUERY REJECT: reason={}.",api.current_failure?api.current_failure(api.user):"unknown");
        }
#endif
        return;
    }
    const auto now=api.now(api.user);
    if(!frame.airborne) {
        // The board basis chooses probe orientation; the measured normal and
        // independently observed wheel contacts, not that basis, admit takeoff.
        SurfaceGeometryTrace trace;
        const auto patch=query_vert_patch(api,context,board,up,.9f,&trace);
#if defined(DINGOSDK_DEV_TOOLS)
        static std::uint64_t reported{};
        if(now<reported || now-reported>=250) {
            reported=now;
            logging::log(logging::Level::info,logging::Channel::skater,
                "ARCADE VERT PATCH: step={} valid={} reason={} context={} ray={} hits={} up={},{},{}.",
                frame.physics_step,patch.valid,trace.reason,trace.context_reason,trace.ray,trace.hits,up[0],up[1],up[2]);
        }
#endif
        std::lock_guard lock(mutex);support=patch;support_frame=frame;return;
    }
    VertAirController snapshot;
    {
        std::unique_lock lock(mutex,std::try_to_lock);
        if(!lock.owns_lock() || (!controller.pending(frame,now) && !controller.landing().flight(frame)))return;
        snapshot=controller;
    }
    using namespace vert_math;
    const auto observation=snapshot.observation();const auto direction=snapshot.direction();
    if(snapshot.landing().flight(frame) && observation.velocity[1]<-.1f) {
        // A short footprint query reserves touchdown input only. The original
        // physics callback must still observe actual wheel support to execute.
        SurfaceGeometryRequest probe;
        constexpr std::array<std::array<float,2>,5> offsets{{{0,0},{-.025f,0},{.025f,0},{0,-.025f},{0,.025f}}};
        for(std::size_t i=0;i<5;++i)probe.segments[i]={{board[0]+offsets[i][0],board[1]+.05f,board[2]+offsets[i][1]},
            {board[0]+offsets[i][0],board[1]-1.5f,board[2]+offsets[i][1]}};
        const auto hit=query_surface_geometry(api,context,probe);
        if(hit.valid) {
            const auto patch=query_vert_patch(api,context,hit.point,hit.query_normal,.5f);
            bool current=patch.valid && api.current(api.user,context);
            for(const auto& sample:patch.samples)if(current && !revalidate_surface_geometry(api,context,sample))current=false;
            if(current) {std::lock_guard lock(mutex);(void)controller.landing().approach(frame,patch);}
        }
    }
    if(!snapshot.pending(frame,now))return;
    const auto right=cross(direction,{0,1,0});const float gravity=snapshot.gravity();
    VertPlan selected;
    if(gravity>0) {
        // A fixed travel-relative fan and fixed flight times provide stable
        // selection without depending on allocation order or a map's axes.
        constexpr std::array<float,5> angles{0,-.4f,.4f,-.8f,.8f};
        for(float reach:{.12f,.25f,.45f,.7f,1.f})for(float angle:angles) {
            const auto heading=add(scale(direction,std::cos(angle)),scale(right,std::sin(angle)));
            auto center=add(observation.position,scale(heading,std::max(.8f,length(observation.velocity)*reach)));
            center[1]+=.5f;
            // A downward cast finds the transition. A second footprint aligns
            // to its measured normal; curved and split meshes retain all owners.
            SurfaceGeometryRequest probe;
            constexpr std::array<std::array<float,2>,5> offsets{{{0,0},{-.025f,0},{.025f,0},{0,-.025f},{0,.025f}}};
            for(std::size_t i=0;i<5;++i)probe.segments[i]={{center[0]+offsets[i][0],center[1],center[2]+offsets[i][1]},
                {center[0]+offsets[i][0],center[1]-12,center[2]+offsets[i][1]}};
            const auto hit=query_surface_geometry(api,context,probe);
            if(!hit.valid)continue;
            const auto patch=query_vert_patch(api,context,hit.point,hit.query_normal,.5f);
            // Search distance and flight time independently: a nearly vertical
            // launch can clear a close spine after a long flight. Tying the
            // two misses those trajectories even when the solver admits them.
            std::array<SurfaceGeometrySample,3> crest{};
            bool crest_queried{},crest_valid{};
            for(float time:{.5f,.8f,1.1f,1.4f,1.8f}) {
                auto candidate=vert_destination(observation,snapshot.origin(),patch,direction,gravity,time);
                if(candidate.action==VertAction::none || (selected && candidate.cost>=selected.cost))continue;
                if(candidate.action==VertAction::spine || candidate.action==VertAction::hip) {
                    if(!crest_queried) {
                        crest_queried=true;crest_valid=true;
                        float highest=-100000;
                        for(std::size_t k=0;k<crest.size();++k) {
                            const float fraction=static_cast<float>(k+1)*.25f;
                            auto middle=add(snapshot.origin().point,
                                scale(sub(candidate.landing,snapshot.origin().point),fraction));
                            middle[1]=std::max(observation.position[1],candidate.landing[1])+3;
                            for(std::size_t i=0;i<5;++i)probe.segments[i]={{middle[0]+offsets[i][0],middle[1],middle[2]+offsets[i][1]},
                                {middle[0]+offsets[i][0],middle[1]-12,middle[2]+offsets[i][1]}};
                            crest[k]=query_surface_geometry(api,context,probe);
                            if(!crest[k].valid || crest[k].query_normal[1]<.05f){crest_valid=false;break;}
                            highest=std::max(highest,crest[k].point[1]);
                        }
                        // The connection must rise back toward the originating
                        // lip. Missing support anywhere across it rejects a gap;
                        // a sloped crest need not have a flat midpoint.
                        crest_valid=crest_valid && highest>=std::max(snapshot.origin().point[1],candidate.landing[1])-.25f;
                    }
                    if(!crest_valid)continue;
                    candidate.crest=crest;
                }
                candidate.clear=query_vert_clearance(api,context,observation.position,candidate.velocity,gravity,time);
                if(candidate)selected=candidate;
            }
        }
    }
    if(!selected) {
        selected=vert_level_out(observation,direction);
        // Missing gravity forbids extrapolating an unmeasured landing. Only a
        // short immediate clearance check is required for breaking vert.
        selected.clear=query_vert_clearance(api,context,observation.position,selected.velocity,gravity,.2f,direction);
    }
    if(!selected || !api.current(api.user,context))return;
    for(const auto& sample:selected.crest)
        if(sample.valid && !revalidate_surface_geometry(api,context,sample))return;
    for(const auto& sample:selected.destination.samples)
        if(selected.destination.valid && !revalidate_surface_geometry(api,context,sample))return;
    std::lock_guard lock(mutex);
    (void)controller.stage(selected,observation,api.now(api.user));
}
}

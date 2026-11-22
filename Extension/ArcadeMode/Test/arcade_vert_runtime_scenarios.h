#pragma once
// Uses the geometry test fixture to exercise the real runtime bridge. No native
// contact or landing is inferred from these procedural query responses.
#include "Extension/ArcadeMode/arcade_vert_runtime.h"
#include <condition_variable>
#include <future>
#include <mutex>

namespace {
struct VertOwnerReadPause {
    std::mutex mutex;
    std::condition_variable changed;
    bool entered{}, released{};
    static inline VertOwnerReadPause* active{};
    void wait() {
        std::unique_lock lock(mutex);
        entered=true;changed.notify_all();
        changed.wait(lock,[&] {return released;});
    }
};
}
#if defined(DINGOSDK_DEV_TOOLS)
#include "Engine/Core/Log/logging.h"
namespace dingosdk::logging {
bool enabled(Level) noexcept{return true;}
void write(Level,Context,Channel,std::string_view) noexcept{}
void write(Level,Channel,std::string_view) noexcept{}
}
#endif
namespace dingosdk::arcade_mode::native_wall_contact {
bool static_world_owner(const SurfaceGeometryBody& shape,SurfaceGeometryWorldOwner& owner) noexcept {
    if(VertOwnerReadPause::active)VertOwnerReadPause::active->wait();
    return Fixture::static_owner(Fixture::running,shape,owner);
}
bool local_component_owner(const SurfaceGeometryBody& shape,std::uintptr_t entity,SurfaceGeometryWorldOwner& owner) noexcept {
    return entity==Fixture::running->live.owner[1] && Fixture::component_owner(Fixture::running,Fixture::running->live,shape,owner);
}
}
void vert_runtime_scene_checks() {
    for(unsigned scenario=0;scenario<9;++scenario) {
        const unsigned fault=scenario<5?scenario:0;
        vert_runtime::cancel();Fixture f;f.allow_world_owner=f.unknown=true;f.curve_radius=4;
        VertObservation o;o.enabled=o.supported=true;o.native.valid=true;o.native.owner={1,2,3,4,5};
        o.native.owner_generation=o.native.contact_generation=o.native.action_generation=o.native.mode_generation=1;
        o.native.flip_known=true;o.native.flip_id=-1;o.native.physics_seconds=1.f/60;o.native.state=100;o.native.mode=1;
        o.native.sampled_at=1000;o.contacted_shape={7,1,9};
        const auto context=[&] {f.time=o.native.sampled_at;f.live.native=InputNativeContext::from_frame(o.native);};
        for(float angle:{.8f,1.1f,1.3f}) {
            ++o.native.physics_step;o.native.sampled_at+=17;context();
            o.position={4*std::sin(angle),4*(1-std::cos(angle)),0};
            o.up={-std::sin(angle),std::cos(angle),0};o.velocity={8*std::cos(angle),8*std::sin(angle),0};
            vert_runtime::geometry(f.api(),f.live,o.native,o.position,o.up);
            check(!vert_runtime::step(o,f.time,o.native.owner,"frame_valid").plan,"ground query never emits a transfer impulse");
        }
        auto missing=o;missing.native={};
        check(!vert_runtime::step(missing,f.time+5,o.native.owner,"frame_lock_busy").native_flight,"missing publication has no runtime motion authority");
        o.supported=false;
        for(unsigned i=0;i<2;++i) {
            ++o.native.physics_step;o.native.sampled_at+=17;context();o.position[1]+=.13f;
            (void)vert_runtime::step(o,f.time,o.native.owner,"frame_valid");
        }
        ++o.native.physics_step;o.native.sampled_at+=17;o.native.state=200;o.native.airborne=true;
        o.native.airborne_episode=1;o.native.airborne_started_known=true;o.native.airborne_started_at=o.native.sampled_at;context();
        auto motion=vert_runtime::step(o,f.time,o.native.owner,"frame_valid");
        check(motion.native_flight && !motion.plan,"actual runtime recognizes queried curved support through the native departure pattern without input");
        auto surface=vert_runtime::surface(f.time,o.native,{});check(surface.can_transfer,"runtime exposes fresh transfer context");
        if(!fault && scenario<5)for(unsigned sample=0;sample<4;++sample) {
            const unsigned channel=sample%2;const bool touchdown=sample>=2;
            if(sample==2)for(unsigned i=0;i<10;++i) {
                ++o.native.physics_step;o.native.sampled_at+=17;o.velocity={4,-3,0};context();
                (void)vert_runtime::step(o,f.time,o.native.owner,"frame_valid");
            }
            // Pause the existing bounded owner read while step holds its mutex.
            // A contending qualifier must see that completed proof, not report
            // ordinary ground merely because another thread is validating it.
            ++o.native.physics_step;o.native.sampled_at+=17;context();
            TriggerPress press;press.press.sequence=channel+1;
            press.press.token={InputSource::triggers,static_cast<std::int16_t>(channel),InputEdge::press};
            auto published=o.native;
            if(touchdown) {
                ++published.physics_step;++published.sampled_at;++published.air_departure;
                published.airborne=false;published.state=100;published.contacts=0xc000;
            }
            const auto current=InputNativeContext::from_frame(published);
            press.press.at=published.sampled_at;press.context=touchdown?InputContext::ground:InputContext::air;
            press.native=current;
            VertOwnerReadPause pause;VertOwnerReadPause::active=&pause;
            auto stepping=std::async(std::launch::async,[&] {
                return vert_runtime::step(o,f.time,o.native.owner,"frame_valid");
            });
            bool entered{};
            {
                std::unique_lock lock(pause.mutex);
                entered=pause.changed.wait_for(lock,std::chrono::seconds(2),[&] {return pause.entered;});
            }
            std::promise<void> started;
            auto qualifying=std::async(std::launch::async,[&] {
                started.set_value();
                return vert_runtime::qualify(nullptr,press,current,f.time+7);
            });
            started.get_future().wait();
            const auto status=qualifying.wait_for(std::chrono::milliseconds(20));
            {
                std::lock_guard lock(pause.mutex);pause.released=true;
            }
            pause.changed.notify_all();
            const auto completed=stepping.get();const auto decision=qualifying.get();
            VertOwnerReadPause::active=nullptr;
            check(entered && status==std::future_status::timeout,
                "contended trigger qualification waits for original physics proof");
            check(completed.native_flight && decision.owner==(touchdown?TriggerOwner::landing_revert:TriggerOwner::transfer) &&
                decision.ready!=touchdown,"both contended triggers retain transfer or pending touchdown ownership");
            check(decision.deadline==press.press.at+(touchdown?430:150),
                "waiting for controller proof never renews the original trigger deadline");
        }
        if(scenario>=5) {
            constexpr std::array<float,4> speeds{-5.f,-1.f,0.f,1.f};
            o.position[1]+=1;
            for(unsigned sample=0;sample<3;++sample) {
                ++o.native.physics_step;o.native.sampled_at+=17;
                o.velocity={0,speeds[scenario-5]+static_cast<float>(2-sample)*9.81f*o.native.physics_seconds,0};
                context();(void)vert_runtime::step(o,f.time,o.native.owner,"frame_valid");
                if(sample<2)o.position=vert_math::position(o.position,o.velocity,9.81f,o.native.physics_seconds);
            }
        }
        InputRequest input;input.action=InputAction::transfer_command;input.context=InputContext::air;
        input.air_episode=f.live.native.air_episode();input.generations=f.live.native.generations;
        input.physical_pressed_at=input.first_input_at=f.time;input.input_sequence=1;
        check(vert_runtime::request(input,f.time,o.native)==InputDisposition::started,"runtime accepts exact fresh edge");
        f.curve_radius=0;f.anchor=true;f.first=0;f.calls=f.pops=f.frees=0;
        if(fault==1)f.anchor_block_at=1;
        if(fault==2)f.no_context=true;
        vert_runtime::geometry(f.api(),f.live,o.native,o.position,o.up);
        if(fault==3) {
            missing=o;missing.native={};(void)vert_runtime::step(missing,f.time+1,o.native.owner,"frame_lock_busy");
            ++o.native.physics_step;o.native.sampled_at+=17;context();
        }
        if(fault==4)++o.native.owner_generation;
        motion=vert_runtime::step(o,f.time,o.native.owner,"frame_valid");
        check(bool(motion.plan)==(fault==0),"runtime clearance, query context, missing publication and owner gates control the one-shot plan");
        if(!fault)check(motion.plan.action==VertAction::level_out &&
            vert_math::length(motion.plan.velocity)<=vert_math::length(o.velocity)+.001f,"runtime fallback preserves energy");
        if(scenario>=5)check(o.velocity[1]==0 ? vert_math::length(motion.plan.velocity)==0 :
            motion.plan.velocity[0]>0,"real runtime fallback redirects falling motion and invents no apex energy");
        check(!vert_runtime::step(o,f.time,o.native.owner,"frame_valid").plan,"runtime cannot emit a second plan for held input");
        check(f.calls==f.pops && f.calls==f.frees,"runtime query allocations are released on success and rejection");
    }
    for(unsigned scene=0;scene<5;++scene)for(float yaw:{0.f,.8f,2.1f})for(float shift:{0.f,1000.f}) {
    vert_runtime::cancel();Fixture f;f.allow_world_owner=f.unknown=f.spine_scene=true;f.first=0;
    if(scene==1)f.scene_hip=1.04719755f;
    if(scene==2)f.scene_drop=2;
    if(scene==3)f.scene_gap=5;
    if(scene==4)f.scene_blocked=true;
    f.scene_yaw=yaw;f.scene_origin={shift,0,-shift};
    const auto vector=[&](SurfaceGeometryVector v) {
        return SurfaceGeometryVector{v[0]*std::cos(yaw)-v[2]*std::sin(yaw),v[1],v[0]*std::sin(yaw)+v[2]*std::cos(yaw)};
    };
    const auto point=[&](SurfaceGeometryVector p){return vert_math::add(vector(p),f.scene_origin);};
    VertObservation o;o.enabled=o.supported=true;o.native.valid=true;o.native.owner={1,2,3,4,5};
    o.native.owner_generation=o.native.contact_generation=o.native.action_generation=o.native.mode_generation=1;
    o.native.flip_known=true;o.native.flip_id=-1;o.native.physics_seconds=1.f/60;o.native.state=100;o.native.mode=1;
    o.native.sampled_at=1000;o.contacted_shape={7,1,9};
    const auto context=[&] {f.time=o.native.sampled_at;f.live.native=InputNativeContext::from_frame(o.native);};
    for(float angle:{.8f,1.1f,1.3f}) {
        ++o.native.physics_step;o.native.sampled_at+=17;context();
        o.position=point({4*std::sin(angle),4*(1-std::cos(angle)),0});o.up=vector({-std::sin(angle),std::cos(angle),0});
        o.velocity=vector({8*std::cos(angle),8*std::sin(angle),0});
        vert_runtime::geometry(f.api(),f.live,o.native,o.position,o.up);(void)vert_runtime::step(o,f.time,o.native.owner,"frame_valid");
    }
    const auto origin=query_vert_patch(f.api(),f.live,o.position,o.up,.5f);
    ++o.native.physics_step;o.native.sampled_at+=17;context();o.position=point({4,4.2f,0});o.velocity=vector({0,8,0});
    o.native.state=200;o.native.airborne=true;o.native.airborne_episode=1;o.native.airborne_started_known=true;
    o.native.airborne_started_at=o.native.sampled_at;o.supported=false;context();
    auto motion=vert_runtime::step(o,f.time,o.native.owner,"frame_valid");check(motion.native_flight,"compound scene genuine takeoff");
    if(scene==2){o.position=point({4,6,0});o.velocity=vector({4,-4,0});}
    for(unsigned i=0;i<3;++i) {
        ++o.native.physics_step;o.native.sampled_at+=17;o.position=vert_math::position(o.position,o.velocity,9.81f,1.f/60);
        o.velocity[1]-=9.81f/60;context();(void)vert_runtime::step(o,f.time,o.native.owner,"frame_valid");
    }
    InputRequest input;input.action=InputAction::transfer_command;input.context=InputContext::air;
    input.air_episode=f.live.native.air_episode();input.generations=f.live.native.generations;
    input.physical_pressed_at=input.first_input_at=f.time;input.input_sequence=1;
    check(vert_runtime::request(input,f.time,o.native)==InputDisposition::started,"compound scene fresh trigger");
    unsigned reachable{};
    for(float x:{4.9f,5.f,5.1f,5.2f})for(float time:{1.1f,1.4f,1.8f}) {
        const float xnormal=(8.3f-x)/4,ynormal=std::sqrt(1-xnormal*xnormal);
        const auto patch=query_vert_patch(f.api(),f.live,point({x,4-4*ynormal,0}),vector({xnormal,ynormal,0}),.5f);
        auto candidate=vert_destination(o,origin,patch,vector({1,0,0}),9.81f,time);
        if(candidate.action==VertAction::spine && query_vert_clearance(f.api(),f.live,o.position,candidate.velocity,9.81f,time))++reachable;
    }

    if(!scene)check(reachable>0,"scene has an independently cleared reachable spine trajectory");
    vert_runtime::geometry(f.api(),f.live,o.native,o.position,o.up);
    motion=vert_runtime::step(o,f.time,o.native.owner,"frame_valid");

    if(scene<3)check(motion.plan.action==(scene==0?VertAction::spine:scene==1?VertAction::hip:VertAction::acid_drop),
        "complete transformed scene selects its reachable contextual action");
    else check(motion.plan.action==VertAction::none || motion.plan.action==VertAction::level_out,
        "missing crest connection and obstructed flight never fabricate a transfer");
    if(scene<3) {
        o.velocity=motion.plan.velocity;
        auto absent=o;absent.native={};
        const auto suspended=vert_runtime::step(absent,f.time+1,o.native.owner,"frame_lock_busy");
        check(!suspended.plan && !suspended.native_flight &&
            vert_math::length(vert_runtime::alignment(o.native,f.time+1))==0,
            "busy original publication withdraws runtime motion and alignment authority");
        o.position=vert_math::position(o.position,o.velocity,9.81f,2.f/60);o.velocity[1]-=9.81f*2.f/60;
        o.native.physics_step+=2;o.native.sampled_at+=34;context();
        motion=vert_runtime::step(o,f.time,o.native.owner,"frame_valid");
        check(motion.native_flight && motion.aligning && !motion.plan &&
            vert_math::length(vert_runtime::alignment(o.native,f.time))>.9f,
            "fresh original trajectory resumes transformed spine, hip and acid alignment without a second impulse");
        (void)vert_runtime::step(absent,f.time+1,o.native.owner,"frame_correlation");
        ++o.native.physics_step;o.native.sampled_at+=17;context();
        motion=vert_runtime::step(o,f.time,o.native.owner,"frame_valid");
        check(!motion.native_flight && !motion.plan && vert_math::length(vert_runtime::alignment(o.native,f.time))==0,
            "correlation failure never receives the narrow busy-publication continuation");
    }
    check(f.calls==f.pops && f.calls==f.frees,"complete scene releases every native query allocation");
    }

}

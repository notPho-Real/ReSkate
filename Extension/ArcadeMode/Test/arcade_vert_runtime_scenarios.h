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
#if !defined(_WIN32)
DWORD GetLastError(){return 0;}
void SetLastError(DWORD){}
#endif
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
void vert_runtime_mode_gap_checks() {
    for(bool direct:{false,true})for(unsigned fault=0;fault<2;++fault) {
        vert_runtime::cancel();Fixture f;f.allow_world_owner=f.unknown=true;f.curve_radius=4;
        VertObservation o;o.enabled=o.supported=true;o.native.valid=true;o.native.owner={1,2,3,4,5};
        o.native.owner_generation=o.native.contact_generation=o.native.action_generation=o.native.mode_generation=1;
        o.native.flip_known=true;o.native.flip_id=-1;o.native.selection_object=99;
        o.native.physics_seconds=1.f/60;o.native.state=100;o.native.mode=1;o.native.contacts=0xc000;
        o.native.sampled_at=1000;o.contacted_shape={7,1,9};
        const auto context=[&] {f.time=o.native.sampled_at;f.live.native=InputNativeContext::from_frame(o.native);};
        for(float angle:{.8f,1.1f,1.3f}) {
            ++o.native.physics_step;o.native.sampled_at+=17;context();
            o.position={4*std::sin(angle),4*(1-std::cos(angle)),0};
            o.up={-std::sin(angle),std::cos(angle),0};o.velocity={8*std::cos(angle),8*std::sin(angle),0};
            vert_runtime::geometry(f.api(),f.live,o.native,o.position,o.up);
            (void)vert_runtime::step(o,f.time,o.native.owner,"frame_valid");
        }
        for(unsigned state:{200u,201u}) {
            ++o.native.physics_step;o.native.sampled_at+=17;o.native.state=state;o.native.airborne=true;
            o.native.contacts=0;o.native.airborne_episode=1;o.native.airborne_started_known=true;
            if(state==200)o.native.airborne_started_at=o.native.sampled_at;
            o.supported=false;context();
            check(vert_runtime::step(o,f.time,o.native.owner,"frame_valid").native_flight,"runtime mode fixture has real qualified flight history");
        }
        NativeInputCorrelation diagnostic{o.native,NativeInputCorrelation::mode_changed};
        NativeRequestSample live{};live.valid=true;live.state=201;live.crouch_mode=2;
        auto missing=o;missing.native={};
        const auto gap=vert_runtime::step(missing,f.time+1,o.native.owner,"frame_correlation",&diagnostic,&live);
        check(gap.preserve_native && !gap.native_flight && !gap.plan && !gap.aligning &&
            !vert_runtime::surface(f.time+1,o.native,{}).can_transfer,
            "runtime idle mode gap withholds all transfer authority and preserves native no-input trajectory");
        ++o.native.physics_step;o.native.sampled_at+=17;o.native.mode=2;if(fault)++o.native.selection_object;context();
        const auto resumed=vert_runtime::step(o,f.time,o.native.owner,"frame_valid");
        check(resumed.native_flight==(fault==0) && !resumed.plan && !resumed.aligning,
            "runtime resumes only a fully admitted matching idle actor after mode2 publication");
        if(fault)continue;
        for(unsigned i=0;i<10;++i) {
            ++o.native.physics_step;o.native.sampled_at+=17;o.velocity={4,-3,0};context();
            (void)vert_runtime::step(o,f.time,o.native.owner,"frame_valid");
        }
        diagnostic={o.native,NativeInputCorrelation::mode_changed|NativeInputCorrelation::contacts_changed};
        live.crouch_mode=1;live.words[1]=direct?0xc000u:0x8000u;missing=o;missing.native={};
        if(direct){live.state=100;diagnostic.mismatches|=NativeInputCorrelation::state_changed;}
        const auto contact_gap=vert_runtime::step(missing,f.time+1,o.native.owner,"frame_correlation",&diagnostic,&live);
        check(contact_gap.preserve_native && !contact_gap.native_flight && !contact_gap.plan && !contact_gap.aligning,
            "real runtime withdraws transfer motion while preserving bounded landing history");
        o.native.mode=1;o.velocity[1]=0;
        if(!direct) {
            ++o.native.physics_step;o.native.sampled_at+=17;o.native.contacts=0x8000;context();
            const auto partial=vert_runtime::step(o,f.time,o.native.owner,"frame_valid");
            check(partial.preserve_native && !partial.native_flight && !partial.plan && !partial.aligning &&
                !vert_runtime::surface(f.time,o.native,{}).can_transfer,
                "original AIR contact preserves native response without transfer or tilt authority");
        }
        ++o.native.physics_step;o.native.sampled_at+=17;o.native.state=100;o.native.airborne=false;
        ++o.native.air_departure;o.native.contacts=0xc000;o.up=o.contact_normal={0,1,0};context();
        (void)vert_runtime::step(o,f.time,o.native.owner,"frame_valid");
        TriggerPress presses[2];
        for(unsigned channel=0;channel<2;++channel) {
            auto& press=presses[channel];press.press.sequence=channel+1;
            press.press.token={InputSource::triggers,static_cast<std::int16_t>(channel),InputEdge::press};
            press.press.at=f.time;press.context=InputContext::ground;press.native=InputNativeContext::from_frame(o.native);
            const auto decision=vert_runtime::qualify(nullptr,press,press.native,f.time);
            check(decision.owner==TriggerOwner::landing_revert && !decision.ready,
                "ground publication alone reserves LT or RT but cannot execute Revert");
            press.owner=TriggerOwner::landing_revert;
        }
        ++o.native.physics_step;o.native.sampled_at+=17;o.supported=true;context();
        (void)vert_runtime::step(o,f.time,o.native.owner,"frame_valid");
        for(const auto& press:presses) {
            const auto decision=vert_runtime::qualify(nullptr,press,InputNativeContext::from_frame(o.native),f.time);
            check(decision.owner==TriggerOwner::landing_revert && decision.ready && decision.deadline<=press.press.at+430,
                "only original wheel support releases the same landing edge after native velocity response");
        }
    }
}
void vert_runtime_ramp_publication_checks() {
    for(unsigned fault=0;fault<3;++fault) {
        vert_runtime::cancel();Fixture f;f.allow_world_owner=f.unknown=true;
        VertObservation o;o.enabled=o.supported=true;o.native.valid=true;o.native.owner={1,2,3,4,5};
        o.native.owner_generation=o.native.contact_generation=o.native.action_generation=o.native.mode_generation=1;
        o.native.flip_known=true;o.native.flip_id=-1;o.native.selection_object=99;
        o.native.physics_seconds=1.f/60;o.native.state=100;o.native.mode=1;o.native.contacts=0xc000;
        o.native.sampled_at=1000;
        const auto contacts=[&] {
            std::array<VertRampContact,2> hits;
            for(unsigned i=0;i<2;++i)
                hits[i]={vert_math::add(o.position,{0,0,i?.22f:-.22f}),o.up,{7,1,9},f.world_owner};
            o.native_ramp=vert_ramp_contacts(hits,o.position,o.up);
        };
        const auto advance=[&] {
            ++o.native.physics_step;o.native.sampled_at+=17;
            f.time=o.native.sampled_at;f.live.native=InputNativeContext::from_frame(o.native);
        };
        for(float angle:{.8f,1.1f,1.3f}) {
            advance();o.position={4*std::sin(angle),4*(1-std::cos(angle)),0};
            o.up={-std::sin(angle),std::cos(angle),0};o.velocity={8*std::cos(angle),8*std::sin(angle),0};contacts();
            (void)vert_runtime::step(o,f.time,o.native.owner,"frame_valid");
        }
        for(unsigned i=0;i<8;++i) {
            advance();o.native.contacts=i%2?0x8000:0xc000;++o.native.contact_generation;
            o.position[1]+=.12f;contacts();
            auto physical=o;physical.native={};
            if(fault==1)physical.native_ramp={};
            if(fault==2)physical.native_ramp.contacts[1].owner.generation++;
            NativeInputCorrelation diagnostic{o.native,NativeInputCorrelation::contacts_changed};
            NativeRequestSample live{};live.valid=true;live.state=100;live.crouch_mode=1;
            live.words[1]=o.native.contacts==0xc000?0x8000:0xc000;
            const auto motion=vert_runtime::step(physical,f.time+1,o.native.owner,"frame_correlation",&diagnostic,&live);
            check(!motion.plan && !motion.native_flight && !motion.aligning &&
                  !vert_runtime::surface(f.time+1,o.native,{}).can_transfer,
                "runtime physical ramp continuation never publishes flight or motion authority");
            if(!fault)check(motion.preserve_native,"runtime keeps original ramp physics through fresh contact churn");
        }
        advance();o.native.state=200;o.native.contacts=0;o.native.airborne=true;
        o.native.airborne_episode=1;o.native.airborne_started_known=true;o.native.airborne_started_at=o.native.sampled_at;
        o.supported=false;o.native_ramp={};f.live.native=InputNativeContext::from_frame(o.native);
        const auto flight=vert_runtime::step(o,f.time,o.native.owner,"frame_valid");
        check(flight.native_flight==!fault && !flight.plan,
            "runtime admits subsequent native takeoff only after fresh same-owner physical ramp contacts");
    }
}
void vert_runtime_scene_checks() {
    vert_runtime_ramp_publication_checks();
    vert_runtime_mode_gap_checks();
    for(unsigned scenario=0;scenario<30;++scenario) {
        const unsigned fault=scenario<5?scenario:scenario==11?5:0;
        vert_runtime::cancel();Fixture f;f.allow_world_owner=f.unknown=true;f.curve_radius=4;
        VertObservation o;o.enabled=o.supported=true;o.native.valid=true;o.native.owner={1,2,3,4,5};
        o.native.owner_generation=o.native.contact_generation=o.native.action_generation=o.native.mode_generation=1;
        o.native.flip_known=true;o.native.flip_id=-1;o.native.selection_object=99;
        o.native.physics_seconds=(scenario==12 || scenario==18 || scenario==23)?1.f/30:
            (scenario==14 || scenario==19 || scenario==24)?1.f/120:1.f/60;o.native.state=100;o.native.mode=1;
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
        if(scenario==9){++o.native.physics_step;o.native.sampled_at+=17;context();}
        NativeInputCorrelation diagnostic{o.native,NativeInputCorrelation::contacts_changed};
        NativeRequestSample live{};live.valid=true;live.state=100;live.crouch_mode=1;live.words[1]=0x4000;
        o.native.contacts=0xc000;diagnostic.observed=o.native;
        const auto withheld=vert_runtime::step(missing,f.time+6,o.native.owner,"frame_correlation",&diagnostic,&live);
        check(withheld.preserve_native && !withheld.native_flight && !withheld.plan && !withheld.aligning,
            "actual runtime retains contact-only history while withholding input and physical authority");
        if(scenario==9)for(auto next: {0u,0x4000u}) {
            ++o.native.physics_step;o.native.sampled_at+=17;
            o.native.contacts=live.words[1];diagnostic.observed=o.native;live.words[1]=next;context();
            const auto retained=vert_runtime::step(missing,f.time+1,o.native.owner,"frame_correlation",&diagnostic,&live);
            check(retained.preserve_native && !retained.native_flight && !retained.plan && !retained.aligning &&
                !vert_runtime::surface(f.time+1,o.native,{}).can_transfer,
                "confirmed original contact chain preserves runtime history without granting physical or input authority");
        }
        o.native.contacts=0x4000;
        o.supported=false;
        for(unsigned i=0;i<(scenario==9?1u:2u);++i) {
            ++o.native.physics_step;o.native.sampled_at+=17;context();o.position[1]+=.13f;
            (void)vert_runtime::step(o,f.time,o.native.owner,"frame_valid");
        }
        ++o.native.physics_step;o.native.sampled_at+=17;o.native.state=200;o.native.airborne=true;
        o.native.airborne_episode=1;o.native.airborne_started_known=true;o.native.airborne_started_at=o.native.sampled_at;context();
        auto motion=vert_runtime::step(o,f.time,o.native.owner,"frame_valid");
        check(motion.native_flight && !motion.plan,"actual runtime recognizes queried curved support through the native departure pattern without input");
        o.native.contacts=0;diagnostic={o.native,NativeInputCorrelation::state_changed};
        live.state=201;live.words[1]=0;
        missing=o;missing.native={};
        const auto air_gap=vert_runtime::step(missing,f.time+1,o.native.owner,"frame_correlation",&diagnostic,&live);
        check(air_gap.preserve_native && !air_gap.native_flight && !air_gap.plan && !air_gap.aligning,
            "actual200-to201 gap preserves native trajectory without another flight command");
        check(!vert_runtime::surface(f.time+1,o.native,{}).can_transfer,"old frame cannot request transfer during publication gap");
        ++o.native.physics_step;o.native.sampled_at+=17;o.native.state=201;context();
        motion=vert_runtime::step(o,f.time,o.native.owner,"frame_valid");
        check(motion.native_flight && !motion.plan,"fresh201 resumes same native flight with no impulse");
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
        if((scenario>=5 && scenario<9) || scenario>=12) {
            constexpr std::array<float,4> speeds{-5.f,-1.f,0.f,1.f};
            o.position[1]+=1;
            for(unsigned sample=0;sample<3;++sample) {
                ++o.native.physics_step;o.native.sampled_at+=17;
                o.velocity={0,(scenario>=12?8.f:speeds[scenario-5])+static_cast<float>(2-sample)*9.81f*o.native.physics_seconds,0};
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
        // Model bounded but nonzero discovery cost. The retained native trial
        // paused publication for 50-62ms only while its transfer was pending.
        // This is a scheduling fixture, not a measured per-ray engine cost.
        if(scenario>=12){f.vertical_delay=3;f.vertical_calls=0;}
        // The p46 trace spent 17-19ms per six misses, 17ms on its last
        // miss plus clearance, and about33ms between client callbacks. The
        // query owner is the client callback, not the native physics clock.
        // Model both costs; omitting clearance hid the original deadline miss.
        if(scenario>=17){f.clearance_delay_us=200;f.time+=9;}
        if(scenario==20 || scenario==27)f.anchor_block_at=26;
        if(scenario==21)f.vertical_delay=12;
        auto callback_started=f.time;
        if(scenario==15)f.anchor_block_at=26;
        vert_runtime::geometry(f.api(),f.live,o.native,o.position,o.up);
        if(scenario>=12) {
            const auto first_query_calls=f.calls;
            if(scenario==16)vert_runtime::cancel();
            motion=vert_runtime::step(o,f.time,o.native.owner,"frame_valid");
            for(unsigned callback=0;callback<12 && !motion.plan && !((scenario==15 || scenario==20 || scenario==27) && f.calls>=26);++callback) {
                const auto step_ms=static_cast<std::uint64_t>(std::ceil(o.native.physics_seconds*1000));
                const auto next=scenario==29?f.time+15:
                    scenario>=17?std::max(f.time,callback_started+33):f.time+step_ms;
                const auto steps=std::max<std::uint64_t>(1,static_cast<std::uint64_t>(std::llround(
                    static_cast<double>(next-o.native.sampled_at)/(1000.0*o.native.physics_seconds))));
                const auto seconds=static_cast<float>(steps)*o.native.physics_seconds;
                o.native.physics_step+=steps;o.native.sampled_at=next;
                o.position=vert_math::position(o.position,o.velocity,9.81f,seconds);
                o.velocity[1]-=9.81f*seconds;context();callback_started=f.time;
                // Native client geometry can precede the motion observer for
                // this original frame. Query admission has the new frame;
                // the controller still owns the previous physical observation.
                if(scenario<22)(void)vert_runtime::step(o,f.time,o.native.owner,"frame_valid");
                vert_runtime::geometry(f.api(),f.live,o.native,o.position,o.up);
                if(f.calls==25) {
                    if(scenario==25)vert_runtime::cancel();
                    if(scenario==26){++o.native.owner_generation;f.live.native=InputNativeContext::from_frame(o.native);}
                    if(scenario==28){o.native.flip_id=2;f.live.native=InputNativeContext::from_frame(o.native);}
                }
                motion=vert_runtime::step(o,f.time,o.native.owner,"frame_valid");
            }
            const bool admitted=scenario<15 || (scenario>=17 && scenario<20) || (scenario>=22 && scenario<25) || scenario==29;
            check((motion.plan.action==VertAction::level_out)==admitted,
                "resumed search still rejects obstructed clearance and cancelled ownership");
            if(admitted)check(f.time-input.physical_pressed_at<=150,
                "bounded destination discovery leaves time for fresh Level Out within the original trigger deadline");
            if(admitted)check(f.calls==25+72 && f.vertical_calls==25,
                "neutral trigger checks the entire destination fan and every clearance ray at 30/60/120Hz");
            if(scenario==15 || scenario==20 || scenario==27)check(f.vertical_calls==25 && f.calls==26,"obstruction blocks the completed fan's Level Out");
            if(scenario==16)check(f.calls==first_query_calls && f.calls>0 && f.vertical_calls==f.calls && f.calls<25,
                "cancelled search cannot resume its retained cursor");
            if(scenario==21)check(f.time-input.physical_pressed_at>150 && !motion.plan,
                "work that cannot fit still expires the original input instead of extending its deadline");
            if(scenario==25 || scenario==26)check(f.calls==25 && f.vertical_calls==25,
                "completed miss cursor grants no clearance authority after cancellation or owner replacement");
            check(f.calls==f.pops && f.calls==f.frees,"resumed destination discovery releases every native result");
            continue;
        }
        if(fault==3) {
            missing=o;missing.native={};(void)vert_runtime::step(missing,f.time+1,o.native.owner,"frame_lock_busy");
            ++o.native.physics_step;o.native.sampled_at+=17;context();
        }
        if(fault==4)++o.native.owner_generation;
        // A delayed callback must honor query age using execution time. At the
        // inclusive boundary, its handoff lease begins when it actually writes.
        const auto execution_time=f.time+(scenario==10?50u:scenario==11?51u:0u);
        motion=vert_runtime::step(o,execution_time,o.native.owner,"frame_valid");
        check(bool(motion.plan)==(fault==0),"runtime clearance, query context, missing publication and owner gates control the one-shot plan");
        if(!fault)check(motion.plan.action==VertAction::level_out &&
            vert_math::length(motion.plan.velocity)<=vert_math::length(o.velocity)+.001f,"runtime fallback preserves energy");
        if(scenario>=5 && scenario<9)check(o.velocity[1]==0 ? vert_math::length(motion.plan.velocity)==0 :
            motion.plan.velocity[0]>0,"real runtime fallback redirects falling motion and invents no apex energy");
        check(!vert_runtime::step(o,execution_time,o.native.owner,"frame_valid").plan,"runtime cannot emit a second plan for held input");
        if(!fault && o.native.state==201) {
            // Fixture acknowledgement models the successful submit/write seam;
            // only native acceptance can prove adoption and physical landing.
            check(vert_runtime::submitted_native_level_out(o.native,execution_time+1,motion.plan),"runtime handoff uses actual release time without renewing query age");
            check(vert_math::length(vert_runtime::alignment(o.native,execution_time+1))==0,"runtime withdraws alignment at handoff");
            ++o.native.physics_step;o.native.sampled_at=execution_time+17;o.velocity[1]+=1.3f;o.velocity[2]-=.6f;context();
            const auto native_owned=vert_runtime::step(o,f.time,o.native.owner,"frame_valid");
            check(native_owned.native_flight && !native_owned.plan && !native_owned.aligning,
                "native AIR adoption remains unshaped by ordinary or vert motion writers");
        }
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
    if(!scene && yaw==0 && shift==0) {
        const auto expired=vert_runtime::step(o,f.time+51,o.native.owner,"frame_valid");
        check(!expired.plan && expired.native_flight,"late transfer use preserves flight without a stale impulse");
        o.native.physics_step+=4;o.native.sampled_at+=68;
        o.position=vert_math::position(o.position,o.velocity,9.81f,4*o.native.physics_seconds);
        o.velocity[1]-=9.81f*4*o.native.physics_seconds;context();
        (void)vert_runtime::step(o,f.time,o.native.owner,"frame_valid");
        vert_runtime::geometry(f.api(),f.live,o.native,o.position,o.up);
        // A previously selected transfer cannot become a skipped fan on retry.
        // Its geometry and clearance must be queried again under the same edge.
    }
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

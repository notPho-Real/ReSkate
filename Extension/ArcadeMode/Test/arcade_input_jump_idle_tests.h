#pragma once
#include "Extension/ArcadeMode/arcade_gesture.h"
#include "Extension/ArcadeMode/arcade_native_readiness.h"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>
namespace jump_idle_tests {
using namespace dingosdk::arcade_mode;
static_assert(InputTiming::ground_pair_ms == 220 && InputTiming::ground_sample_slack_ms == 55 &&
    InputTiming::air_pair_ms == 400 && InputTiming::air_request_ms == 700);
unsigned checks{};
void check(bool value,const char* reason) {
    ++checks;
    if(!value){std::cerr<<"FAIL: "<<reason<<'\n';std::exit(1);}
}
struct Fixture {
    ArcadeInput input{true};NativeInputState producer;NativeInputReadiness readiness;
    NativeInputOwner owner{1,2,3,4,5};
    std::vector<PreparedGesture> recipes{PreparedGesture{2,false,GestureContext::ground,
        std::vector<AuthoredStickStep>{{0,-25000,0},{18,-25000,0},{21,0,0},{24,0,0}}}};
    unsigned starts{},recognized{},cancelled{},nonneutral{};
    NativeInputFrame frame{};
    Fixture(){input.enable(true);input.set_recognition_capture(0,true);}
    NativeGesture::Output poll(std::uint64_t now,std::uint32_t state,std::int32_t selection,PhysicalInputSample physical={},
            std::uint32_t mode=2,std::uint32_t stance=0,std::uintptr_t object=16,bool known=true,bool permission=true,bool permit_flip=true) {
        NativeRequestSample sample{};sample.valid=true;sample.state=state;sample.crouch_mode=mode;
        sample.words[0]=stance;sample.words[1]=state==100?0xc000u:0u;
        frame=producer.observe(now,owner,sample,{known,selection,object});
        readiness.publish(frame,{true,false,false,false,false,false,false,false,true,0});
        input.publish_native_context(frame);input.publish_authority(now,frame.owner_generation,1,true);
        const auto admission=readiness.sample(now,frame);
        const auto out=input.poll(0,now,physical.buttons,physical.x,physical.y,0,0,permission,
            false,false,0,false,false,true,now/17,false,recipes,admission.mirror_x,false,
            frame.airborne?SkatePose::air:SkatePose::riding,false,false,false,false,admission.flip_ready && permit_flip,false,0,0,
            ActionExperiment::classic,admission.action,admission.mirror_x,false);
        const auto* d=input.input_diagnostic(0);
        for(std::size_t i=0;i<d->recognition_count;++i)recognized+=d->recognition[i].request.action==InputAction::flip?1u:0u;
        for(std::size_t i=0;i<d->dispatched;++i)if(d->dispatch[i].request.action==InputAction::flip)
            cancelled+=d->dispatch[i].disposition==InputDisposition::cancelled?1u:0u;
        starts+=out.requested_id==2?1u:0u;nonneutral+=out.right_x || out.right_y?1u:0u;
        return out;
    }
};
void receipt_sequence() {
    auto h=std::make_unique<Fixture>();h->poll(349299020,201,1);
    const auto pressed=h->poll(349299055,201,1,{0x4004});
    check(h->recognized==1 && pressed.command_busy && !pressed.authored_playback && pressed.buttons==0,
        "early post-jump face and direction are recognized once and wait without owning native output");
    h->poll(349299127,201,1);
    const auto started=h->poll(349299160,201,-1);
    check(started.requested_id==2 && started.authored_playback && h->starts==1 && !h->cancelled,
        "fresh idle selection starts the original released pending flip through the existing adapter");
    const auto* d=h->input.input_diagnostic(0);
    check(d->dispatched==1 && d->dispatch[0].request.recognized_at==349299055 &&
        d->dispatch[0].request.first_input_at==349299055 && d->dispatch[0].request.last_input_at==349299055 &&
        d->dispatch[0].request.generations.action+1==h->frame.action_generation,
        "readiness does not renew original timing or rewrite immutable generation evidence");
    h->poll(349299177,201,-1);h->poll(349299194,201,-1);h->poll(349299211,201,-1);
    check(h->nonneutral!=0,"qualified readiness reaches nonneutral adapter output after its existing startup schedule");
}
void sampled_orderings() {
    for(unsigned hz:{30u,60u})for(bool pad:{false,true})
    for(unsigned press:{966u,983u,1000u,1017u,1034u,1051u,1100u,1150u})for(int offset:{-34,0,34}) {
        auto h=std::make_unique<Fixture>();std::uint64_t sampled_press{},first_verified_air{};
        for(unsigned i=0;i<=48;++i) {
            const auto now=std::uint64_t{800}+std::uint64_t{i}*1000/hz;if(now>1600)break;
            const auto state=now<1000?100u:now<1017?200u:now<1500?201u:100u;
            if((state==200 || state==201) && !first_verified_air)first_verified_air=now;
            const bool held=now>=press && now<press+72;
            if(held && !sampled_press)sampled_press=now;
            const auto direction_at=static_cast<unsigned>(static_cast<int>(press)+offset);
            const bool direction=now>=direction_at && now<press+140;
            PhysicalInputSample physical{static_cast<std::uint16_t>((held?0x4000u:0u)|(pad && direction?4u:0u)),
                static_cast<std::int16_t>(!pad && direction?-25000:0)};
            h->poll(now,state,now<960 || now>=1160?-1:1,physical,state==100?1u:2u);
        }
        // Verified AIR includes the initial native phase. The physics producer
        // qualifies only its forward progression, not a reversed or missed flight.
        const bool eligible=sampled_press>=first_verified_air;
        if(h->starts!=(eligible?1u:0u))std::cerr<<"ordering hz="<<hz<<" pad="<<pad<<" press="<<press<<" offset="<<offset<<" sampled="<<sampled_press<<" air="<<first_verified_air<<" starts="<<h->starts<<" recognized="<<h->recognized<<" cancelled="<<h->cancelled<<'\n';
        check(h->starts==(eligible?1u:0u),"sampled face before/with/after direction preserves verified early-air input and rejects ground borrowing");
        check(eligible?h->nonneutral!=0:h->nonneutral==0,"only the qualified queued flip produces actual adapter stick output");
    }
}
void negative_transitions() {
    for(unsigned variant=0;variant<13;++variant) {
        auto h=std::make_unique<Fixture>();h->poll(1000,201,1);h->poll(1020,201,1,{0x4004});
        std::uint32_t state=201,mode=2,stance{};std::uintptr_t object=16;bool known=true,permission=true;
        if(variant==0)++h->owner.core;
        if(variant==1)object=24;
        if(variant==2)known=false;
        if(variant==3)state=200;
        if(variant==4)mode=0;
        if(variant==5)stance=0x20;
        if(variant==6)state=100;
        if(variant==7)state=300;
        if(variant==8)h->input.enable(false);
        if(variant==9)permission=false;
        if(variant==10)h->poll(1040,201,2,{0x4004}); // An intervening native action is not jump readiness.
        if(variant==12) {
            NativeRequestSample missed{};missed.valid=true;missed.state=201;missed.crouch_mode=2;
            h->producer.observe(1040,h->owner,missed,{true,2,16}); // Physics saw another action between input polls.
        }
        const auto now=variant==11?1171u:1060u; // Provider gap beyond150ms.
        h->poll(now,state,-1,{0x4004},mode,stance,object,known,permission);
        check(!h->starts && !h->nonneutral,"owner/object/unknown/state/mode/stance/landing/bail/OFF/authority/intervening-action/gap changes cannot reuse pending flip");
        if(variant==8)h->input.enable(true);
        h->poll(now+17,201,-1,{0x4004});h->poll(now+34,201,-1,{0x4004});
        check(!h->starts && !h->nonneutral,"recovery cannot turn a held-through cancelled face into a new flip");
        h->poll(now+51,201,-1);h->poll(now+68,201,-1,{0x4004});
        check(h->starts==1,"a physical release and fresh press rearm after the negative boundary");
    }
}
void negative_origins() {
    for(unsigned variant=0;variant<3;++variant) {
        auto h=std::make_unique<Fixture>();const std::int32_t origin=variant==0?1:variant==1?2:16;
        const std::uintptr_t object=variant==0?0u:16u;
        h->poll(1000,201,origin,{},2,0,object);h->poll(1020,201,origin,{0x4004},2,0,object);
        h->poll(1060,201,-1,{},2,0,object);
        check(!h->starts,"a missing selection identity or a different native action cannot borrow the jump-to-idle exception");
    }
}
void other_faces_keep_boundary() {
    UnifiedInput input;NativeInputState producer;NativeInputOwner owner{1,2,3,4,5};
    InputEnvironment environment{InputContext::air,true};environment.named_grabs=true;
    struct Sink {
        unsigned grabs{};
        static InputDisposition receive(void* self,const InputRequest& request) noexcept {
            if(request.action==InputAction::grab)++static_cast<Sink*>(self)->grabs;
            return InputDisposition::unsupported;
        }
    } sink;
    const auto poll=[&](std::uint64_t now,std::int32_t selection,PhysicalInputSample physical,bool enabled) {
        NativeRequestSample sample{};sample.valid=true;sample.state=201;sample.crouch_mode=2;
        environment.native_context=InputNativeContext::from_frame(producer.observe(now,owner,sample,{true,selection,16}));
        environment.auxiliary_enabled=enabled;
        input.poll(now,physical,environment,{&sink,Sink::receive});
    };
    poll(1000,1,{},false);poll(1020,1,{0x2000},false);poll(1040,1,{},false);
    poll(1060,-1,{0,-25000},true);
    check(!sink.grabs,"jump retirement cannot carry an unrelated unconsumed grab face across the action boundary");
}
void intervening_mode_and_contact() {
    for(bool prefix:{false,true})for(bool idle_first:{false,true})for(bool mode_excursion:{false,true}) {
        auto h=std::make_unique<Fixture>();h->poll(1000,201,1);
        h->poll(1020,201,1,{0x4000,static_cast<std::int16_t>(prefix?0:-25000)});
        if(idle_first)h->poll(1040,201,-1,{0x4000},2,0,16,true,true,false);
        const auto origin=h->frame;
        NativeRequestSample sample{};sample.valid=true;sample.state=201;sample.crouch_mode=mode_excursion?0u:2u;
        sample.words[1]=mode_excursion?0u:0x4000u;
        h->producer.observe(1050,h->owner,sample,{true,idle_first?-1:1,16});
        sample.crouch_mode=2;sample.words[1]=0;
        h->producer.observe(1060,h->owner,sample,{true,idle_first?-1:1,16});
        h->poll(1080,201,-1,{0x4004});
        check(h->frame.contact_generation>origin.contact_generation,
            "the native producer records both hidden mode and wheel-contact excursions");
        check(origin.mode_generation && (mode_excursion ? h->frame.mode_generation>origin.mode_generation :
            h->frame.mode_generation==origin.mode_generation),
            "mode continuity advances only for mode changes, not wheel-contact samples");
        check(h->starts==(mode_excursion?0u:1u),
            "a hidden mode excursion cancels queued and prefix flip intent while wheel-contact changes preserve it");
        if(mode_excursion) {
            h->poll(1100,201,-1,{4});h->poll(1120,201,-1,{0x4004});
            check(h->starts==1,"fresh physical X rearms after mode continuity is lost without reviving the old command");
        }
    }
}
void mode_proof_and_active_playback() {
    NativeInputState producer;NativeInputOwner owner{1,2,3,4,5};NativeRequestSample sample{};
    sample.valid=true;sample.state=201;sample.crouch_mode=2;
    const auto before=InputNativeContext::from_frame(producer.observe(1000,owner,sample,{true,1,16}));
    auto after=InputNativeContext::from_frame(producer.observe(1020,owner,sample,{true,-1,16}));
    check(after.jump_selection_retired(before.generations,before.air_episode(),before.flip),
        "unchanged observed mode continuity qualifies the existing jump retirement");
    auto missing=before.flip;missing.mode_generation=0;
    check(!after.jump_selection_retired(before.generations,before.air_episode(),missing),
        "missing original mode continuity cannot qualify retirement");
    ++after.flip.mode_generation;
    check(!after.jump_selection_retired(before.generations,before.air_episode(),before.flip),
        "mode continuity mismatch rejects retirement even with equal endpoint mode");
    after.flip.mode_generation=0;
    check(!after.jump_selection_retired(before.generations,before.air_episode(),before.flip),
        "missing current mode continuity cannot qualify retirement");
    for(bool mode_excursion:{false,true}) {
        auto h=std::make_unique<Fixture>();const auto state=mode_excursion?201u:200u;
        h->poll(1000,state,-1);check(h->poll(1020,state,-1,{0x4004}).authored_playback,
            "active playback control begins before native progression");
        sample.state=mode_excursion?201u:200u;sample.crouch_mode=mode_excursion?0u:2u;
        h->producer.observe(1040,h->owner,sample,{true,2,16});sample.state=201;sample.crouch_mode=2;
        h->producer.observe(1060,h->owner,sample,{true,2,16});
        const auto active=h->poll(1080,201,2,{0x4004});
        check(active.authored_playback && h->starts==1,
            "mode/state acknowledgement cannot withdraw already-admitted same-flight native playback");
    }
}
void forward_wait() {
 for(unsigned hz:{30u,60u})for(bool pad:{false,true})for(bool idle_at_transition:{false,true}) {
  auto h=std::make_unique<Fixture>();
  const auto dt=std::uint64_t{1000}/hz;
  h->poll(1000,200,1);
  const PhysicalInputSample press{static_cast<std::uint16_t>(0x4000u|(pad?4u:0u)),static_cast<std::int16_t>(pad?0:-25000)};
  const auto requested=h->poll(1000+dt,200,1,press);
  check(h->recognized==1 && !requested.authored_playback && requested.command_busy,"early ordinary flip waits without output in initial native air phase");
  const auto original=h->input.input_diagnostic(0)->recognition[0].request;
  auto out=h->poll(1000+2*dt,201,idle_at_transition?-1:1);
  if(!idle_at_transition){check(!out.authored_playback && !h->cancelled,"forward air phase alone preserves waiting intent but cannot assert readiness");out=h->poll(1000+3*dt,201,-1);}
  check(out.requested_id==2 && out.authored_playback && h->starts==1 && !h->cancelled,"same-flight forward phase and observed idle deliver the original request exactly once");
  const auto* d=h->input.input_diagnostic(0);
  check(d->dispatch[0].request==original,"native phase readiness preserves original immutable request and timing");
  h->poll(1000+4*dt,201,-1,press);h->poll(1000+5*dt,201,-1,press);
  check(h->starts==1,"held input is never reissued by a native phase transition");
 }
}
void missed_excursions() {
 for(unsigned fault=0;fault<11;++fault) {
  auto h=std::make_unique<Fixture>();h->poll(1000,200,1);h->poll(1020,200,1,{0x4004});
  NativeRequestSample s{};s.valid=true;s.state=201;s.crouch_mode=2;
  if(fault==0){s.state=100;s.words[1]=0xc000u;}
  if(fault==1)s.crouch_mode=0;
  if(fault==2)s.words[0]=0x20;
  const auto id=fault==3?2:1;const auto object=std::uintptr_t{fault==4?24u:16u};
  if(fault<5)h->producer.observe(1030,h->owner,s,{true,id,object});
  if(fault==5){s.state=201;h->producer.observe(1030,h->owner,s,{true,1,16});s.state=200;h->producer.observe(1040,h->owner,s,{true,1,16});}
  if(fault==6){h->producer.observe(1030,h->owner,s,{false,-1,0});}
  if(fault==7)h->producer.invalidate();
  if(fault==8)++h->owner.rig;
  if(fault==9)h->input.enable(false);
  const auto now=fault==10?1200u:1060u;
  h->poll(now,201,-1,{0x4004});
  check(!h->starts,"missed departure/mode/stance/action/object/reverse/unknown/gap/owner/OFF cancels early-air request");
 }
}
void readiness_and_context() {
 auto h=std::make_unique<Fixture>();h->poll(1000,200,1);h->poll(1020,200,1,{0x4004});
 auto out=h->poll(1040,201,-1,{},2,0,16,true,true,false);
 check(!out.authored_playback && !h->starts && out.command_busy,"continuity proof never substitutes for native readiness");
 out=h->poll(1060,201,-1);
 check(out.requested_id==2 && h->starts==1,"independently admitted readiness releases retained request");
 NativeInputState producer;NativeInputOwner owner{1,2,3,4,5};NativeRequestSample s{};s.valid=true;s.state=200;s.crouch_mode=2;
 auto a=producer.observe(100,owner,s,{true,1,16});s.state=201;auto b=producer.observe(120,owner,s,{true,1,16});
 auto context=InputNativeContext::from_frame(b);auto before=InputNativeContext::from_frame(a);
 check(context.ordinary_flip_wait_continues(before.generations,before.air_episode(),before.flip),"actual producer supplies phase continuity");
 before.flip.wait_generation=0;
 check(!context.ordinary_flip_wait_continues(before.generations,before.air_episode(),before.flip),"older provider without continuity capability fails closed for phase extension");
}
int run(){forward_wait();missed_excursions();readiness_and_context();mode_proof_and_active_playback();intervening_mode_and_contact();receipt_sequence();sampled_orderings();negative_transitions();negative_origins();other_faces_keep_boundary();std::cout<<checks<<" jump-idle input checks passed\n";return 0;}
}

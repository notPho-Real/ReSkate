#include "Extension/ArcadeMode/arcade_input_session.h"
#include "Extension/ArcadeMode/arcade_native_input_state.h"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <type_traits>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why) {++checks;if(!value){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
void native_boundaries() {
    for (unsigned missing:{0u,1u,2u,3u,4u}) {
        HeldFlipObservation native{};native.available=true;native.sampled_at=1000;
        const auto sampled=sample_held_flip_state(native,[&](unsigned index,auto& value) noexcept {
            if(index==missing)return false;
            using T=std::remove_reference_t<decltype(value)>;
            value=static_cast<T>(index==0?2:0);return true;
        });
        native.valid=sampled;
        HeldFlipInput input;
        check(input.start(1000,2,native)==(missing>=1 && missing<=3),
            "first-use missing native flags permit gesture preparation; missing flip identity or rotation count still refuses input");
        check(!native.prepare && !native.holding && !input.acknowledged(),
            "unallocated flags cannot acknowledge a native held action or actor");
    }
    for (int id:{2,3}) {
        HeldFlipInput input;
        HeldFlipObservation native{true,true,true,false,false,id,0,1000};
        check(input.start(1000,id,native),"fresh original PrepareHeldFlip admits matching base ID");
        native.sampled_at=1034;native.holding=true;
        check(input.hold(1034,native,true),"original held entry continues input before extra rotation");
        native.sampled_at=1068;native.extra_rotations=1;
        check(!input.hold(1068,native,true) && !input.acknowledged(),"counter alone withdraws input but cannot acknowledge actor execution");
        native={true,true,true,false,false,id,0,2000};
        check(input.start(2000,id,native),"a new lifecycle can start independently");
        native.sampled_at=2034;native.holding=true;native.actor_advanced=true;
        check(input.hold(2034,native,true),"exact initialized advancing loop supports continued native ownership");
        native.sampled_at=2068;native.extra_rotations=1;
        check(!input.hold(2068,native,true) && input.acknowledged(),"one extra native rotation requests double catch");
        check(!input.hold(2102,native,true),"completed hold cannot restart itself");
        native={true,true,false,false,false,id,0,3000};
        check(input.start(3000,id,native),"second physical press can prepare the original gesture before its native admission window");
        native.sampled_at=3034;
        check(input.hold(3034,native,true),"preparation retains input while the original animation approaches its admission window");
        native.sampled_at=3100;native.prepare=true;native.holding=true;native.actor_advanced=true;
        check(input.hold(3100,native,true),"native window and original held actor admit the prepared gesture");
        native.sampled_at=3300;native.extra_rotations=1;
        check(!input.hold(3300,native,true) && input.acknowledged(),"prepared gesture still catches only at the independently observed extra rotation");
        native={true,true,false,false,false,id,0,4000};
        check(input.start(4000,id,native),"unadmitted preparation starts bounded input only");
        native.sampled_at=4201;
        check(!input.hold(4201,native,true) && !input.acknowledged(),"missing native window withdraws preparation at the input deadline");
        native={true,true,true,false,false,id,0,5000};
        check(input.start(5000,id,native),"late physical repetition starts only matching native input");
        native.sampled_at=5088;native.prepare=false;
        check(input.hold(5088,native,true) && !input.acknowledged(),
            "the observed short Prepare window cannot prematurely withdraw an unfinished gesture or acknowledge admission");
        native.sampled_at=5201;
        check(!input.hold(5201,native,true) && !input.acknowledged(),
            "seeing a Prepare window cannot grant an unadmitted gesture the longer held-actor lifetime");
    }
    for (unsigned failure=0;failure<9;++failure) {
        HeldFlipInput input;HeldFlipObservation native{true,true,true,false,false,2,0,1000};
        check(input.start(1000,2,native),"negative control starts a valid request");
        native.sampled_at=1034;bool flight=true;std::uint64_t now=1034;
        if(failure==0)native.valid=false;
        if(failure==1)native.id=3;
        if(failure==2)flight=false;
        if(failure==3)now=999;
        if(failure==4)native.sampled_at=800;
        if(failure==5){native.prepare=false;now=1201;native.sampled_at=1201;}
        if(failure==6)native.extra_rotations=2;
        if(failure==7)native.extra_rotations=std::nanf("");
        if(failure==8){now=2201;native.sampled_at=2201;}
        check(!input.hold(now,native,flight) && !input.acknowledged(),"unknown/stale/foreign/landed/overshot/timed-out native evidence withdraws without false double acknowledgment");
    }
}
struct Route {
    InputSession session;NativeInputState producer;NativeInputOwner owner{1,2,3,4,5};
    InputAdmission admission{};NativeInputFrame frame{};unsigned bridge_calls{},repeats{};
    std::int32_t requested{};std::vector<PreparedGesture> recipes;bool held_available{true},bridge_accept{true},switched{};
    InputRequest repeated_request{};
    std::uintptr_t selection_object{16};
    dingosdk::NativeOrientation native_orientation{true,false};
    Route() {
        AuthoredGesture kick{2,false,GestureContext::ground,90,180,.85f,{{280,30,.5f,1,0},{90,150,.5f,1,.1f}}};
        AuthoredGesture heel{3,false,GestureContext::ground,180,270,.85f,{{330,80,.5f,1,0},{210,270,.5f,1,.1f}}};
        const std::array catalog{kick,heel};recipes=prepare_gestures(catalog);
        recipes.emplace_back(std::uint8_t{1},false,GestureContext::ground,std::vector<AuthoredStickStep>{{0,0,-32767},{18,28000,28000},{21,0,0},{24,0,0}});
        admission.gameplay=admission.authored=admission.action_fresh=true;
        admission.may_start=admission.inspected_stance=true;admission.action_mode=ActionExperiment::classic;
        admission.catalog=recipes;admission.held_flip.available=true;
        admission.normal_flip={this,[](void* p,std::int32_t id) noexcept {
            auto& self=*static_cast<Route*>(p);if(!self.bridge_accept)return false;
            ++self.bridge_calls;self.requested=id;return true;
        }};
    }
    NativeGesture::Output poll(std::uint64_t now,PhysicalInputSample physical={},std::uint32_t state=201,std::int32_t id=2,
                               bool prepare=true,float extra=0,bool held=false,bool actor=false,int animation_id=-999,bool advance=true) {
        NativeRequestSample sample{};sample.valid=true;sample.state=state;sample.crouch_mode=state==100?1:(state==200 || state==103)?0:2;
        sample.words[1]=state==100?0xc000u:0;
        sample.words[0]=switched?0x20u:0;
        if(advance)frame=producer.observe(now,owner,sample,{true,id,selection_object},false,0,native_orientation);
        admission.native_context=InputNativeContext::from_frame(frame);
        admission.pose=frame.airborne?SkatePose::air:SkatePose::riding;
        admission.flip_ready=id==-1;admission.tick=now*60/1000;
        admission.held_flip={held_available,frame.airborne,prepare,held,actor,animation_id==-999?id:animation_id,extra,now};
        const auto out=session.poll(now,physical,0,0,admission);
        const auto& d=session.diagnostic();
        for(std::size_t i=0;i<d.dispatched;++i)
            if(d.dispatch[i].request.form==InputForm::held_repeat && d.dispatch[i].disposition==InputDisposition::started) {
                ++repeats;repeated_request=d.dispatch[i].request;
            }
        return out;
    }
    void first(std::int16_t direction=0) {
        poll(1000,{},100,-1);poll(1017,{0x1000},100,-1);poll(1051,{0x1000},100,1);
        poll(1085,{},100,1);poll(1102,{},200,1);poll(1119,{},201,1);poll(1136,{},201,1);
        poll(1153,{0x4000,direction,0},201,1);
        check(bridge_calls==1 && requested==(direction>0?3:2),"first physical X retains the original single native bridge");
        poll(1170,{},201,requested,false);
    }
};
void queued_first_repetition() {
    for(unsigned hz:{30u,60u,120u}) for(std::int16_t direction:{std::int16_t{0},std::int16_t{-25000},std::int16_t{25000}})
    for(bool switched:{false,true}) for(bool physics_retired:{false,true})
    for(unsigned first_phase:{103u,200u,201u}) for(bool idle_retired:{false,true}) {
        auto h=std::make_unique<Route>();const auto dt=(1000u+hz-1)/hz;
        h->switched=switched;h->bridge_accept=false;
        h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);
        h->poll(1051,{0x1000},100,1);h->poll(1085,{},100,1);
        h->poll(1102,{},first_phase==103?103:200,1);h->poll(1119,{},first_phase,1,false);
        h->poll(1119+dt,{0x4000,direction,0},first_phase,1,false);
        const auto queued_selection=idle_retired?-1:1;
        h->poll(1119+2*dt,{},201,queued_selection,false);
        const auto second=1119+3*dt;
        h->poll(second,{0x4000,static_cast<std::int16_t>(direction?0:25000),0},201,queued_selection,false);
        h->poll(1119+4*dt,{},201,queued_selection,false);
        check(!h->bridge_calls && !h->repeats,"queued physical X pair cannot bypass the first native flip admission");
        h->bridge_accept=true;h->poll(1119+5*dt,{},201,queued_selection,false);
        const auto id=direction>0?3:2;
        check(h->bridge_calls==1 && h->requested==id && !h->repeats,
              "waiting pair first admits exactly the requested original single flip");
        const auto physics_id=physics_retired?id:queued_selection;
        const auto out=h->poll(1119+6*dt,{},201,physics_id,false,0,false,false,id);
        check(h->repeats==1 && out.authored_playback && h->bridge_calls==1,
              "second physical X before first admission prepares the matching held native flip");
        check(h->repeated_request.physical_pressed_at==second &&
              h->repeated_request.physical_released_at==0 &&
              h->repeated_request.deadline==second+InputTiming::readiness_ms && !h->repeated_request.unbounded,
              "queued held repeat retains its recognition-edge timestamp and immutable deadline");
        h->poll(1119+7*dt,{},201,physics_id,true,0,true,true,id);
        const auto caught=h->poll(1119+8*dt,{},201,physics_id,true,1,true,true,id);
        check(caught.held_input_released && caught.held_actor_acknowledged && !caught.authored_playback,
              "queued repetition releases only at independently supplied held actor and extra rotation");
        h->poll(1119+9*dt,{},201,physics_id,false,1,false,false,id);
        check(h->bridge_calls==1 && h->repeats==1,"queued pair never replays another pop, flip or held request");
    }
    for(unsigned hz:{30u,60u,120u})for(unsigned fault=0;fault<10;++fault) {
        auto h=std::make_unique<Route>();const auto dt=(1000u+hz-1)/hz;
        h->bridge_accept=false;
        h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);
        h->poll(1051,{0x1000},100,1);h->poll(1085,{},100,1);
        h->poll(1102,{},200,1);h->poll(1119,{},201,1,false);
        h->poll(1119+dt,{0x4000},201,1,false);h->poll(1119+2*dt,{},201,1,false);
        h->poll(1119+3*dt,{0x4000},201,1,false);
        if(fault==0)++h->owner.rig;
        if(fault==1)h->admission.external_owner=true;
        if(fault==2)h->admission.gameplay=false;
        if(fault==5)h->switched=true;
        if(fault==6)h->selection_object=32;
        if(fault==7)h->held_available=false;
        h->poll(1119+4*dt,{},fault==3?100:fault==4?300:201,1,false);
        h->bridge_accept=true;h->poll(1119+5*dt,{},201,1,false);
        for(unsigned n=6;n<12;++n)h->poll(1119+n*dt,{},201,1,fault==8,0,false,false,fault==9?3:2);
        check(!h->repeats,
              "queued repeat rejects owner, conflict, focus, landing, bail, stance, selector, missing observer, late Prepare or wrong actor");
    }
    for(bool late_first:{false,true})for(unsigned delay:{199u,200u}) {
        auto h=std::make_unique<Route>();h->bridge_accept=false;
        h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);
        h->poll(1051,{0x1000},100,1);h->poll(1085,{},100,1);
        h->poll(1102,{},200,1);h->poll(1119,{},201,1,false);
        h->poll(1136,{0x4000},201,1,false);h->poll(1153,{},201,1,false);
        h->poll(1170,{0x4000},201,1,false);h->poll(1187,{},201,1,false);
        if(!late_first)h->bridge_accept=true;
        h->poll(1204,{},201,1,false);h->poll(1250,{},201,1,false);
        h->poll(1300,{},201,1,false);h->bridge_accept=true;
        h->poll(1336,{},201,1,false);h->poll(1170+delay,{},201,1,false,0,false,false,2);
        check(h->repeats==(delay==199?1u:0u) && h->bridge_calls==1,
              "first admission retains its ordinary policy without renewing the queued repeat's original deadline");
    }
}
void preair_queued_repetition() {
    for(unsigned hz:{30u,60u,120u})for(unsigned phase:{100u,103u})
    for(std::int16_t direction:{std::int16_t{0},std::int16_t{-25000},std::int16_t{25000}})
    for(bool switched:{false,true}) {
        auto h=std::make_unique<Route>();h->bridge_accept=false;h->switched=switched;
        const auto dt=(1000u+hz-1)/hz;
        h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);
        h->poll(1051,{0x1000},100,1);h->poll(1085,{},100,1);
        h->poll(1085+dt,{0x4000,direction,0},phase,1,false);
        h->poll(1085+2*dt,{},phase,1,false);
        const auto second=1085+3*dt;
        h->poll(second,{0x4000},phase,1,false);
        check(!h->bridge_calls && !h->repeats,"pre-AIR pair cannot execute before native takeoff");
        h->poll(second+dt,{},200,1,false);h->bridge_accept=true;
        h->poll(second+2*dt,{},201,1,false);
        const auto id=direction>0?3:2;
        const auto out=h->poll(second+3*dt,{},201,1,false,0,false,false,id);
        check(h->bridge_calls==1 && h->requested==id && h->repeats==1 && out.authored_playback,
              "two pre-AIR X edges on the same released pop prepare the original held flip after takeoff");
        check(h->repeated_request.pop_release_at==1085 &&
              h->repeated_request.physical_pressed_at==second && !h->repeated_request.unbounded &&
              h->repeated_request.deadline==second+InputTiming::readiness_ms,
              "pre-AIR repeat keeps exact pop ownership and second-edge deadline");
        h->poll(second+4*dt,{},201,1,true,0,true,true,id);
        const auto caught=h->poll(second+5*dt,{},201,1,true,1,true,true,id);
        check(caught.held_actor_acknowledged && caught.held_input_released,
              "pre-AIR repeat completion still requires independent native held actor and rotation evidence");
    }
    for(unsigned delay:{199u,200u,222u}) {
        auto h=std::make_unique<Route>();h->bridge_accept=false;
        h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);
        h->poll(1051,{0x1000},100,1);h->poll(1085,{},100,1);
        h->poll(1102,{0x4000},103,1,false);h->poll(1136,{},103,1,false);
        const auto second=1170u;h->poll(second,{0x4000},103,1,false);
        h->poll(1204,{},200,1,false);h->bridge_accept=true;
        h->poll(1238,{},201,1,false);h->poll(1288,{},201,1,false);
        h->poll(1338,{},201,1,false);
        h->poll(second+delay,{},201,1,false,0,false,false,2);
        check(h->bridge_calls==1 && h->repeats==(delay==199?1u:0u),
              "ground preload cannot renew the repeat at200ms or a longer222ms native base delay");
    }
    for(unsigned hz:{30u,60u,120u})for(unsigned fault=0;fault<11;++fault) {
        auto h=std::make_unique<Route>();h->bridge_accept=false;
        const auto dt=(1000u+hz-1)/hz;
        h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);
        h->poll(1051,{0x1000},100,1);h->poll(1085,{},100,1);
        h->poll(1085+dt,{0x4000},103,1,false);
        h->poll(1085+2*dt,{},103,1,false);
        const auto second=1085+3*dt;h->poll(second,{0x4000},103,1,false);
        if(fault==0)++h->owner.rig;
        if(fault==1)h->admission.gameplay=false;
        if(fault==2)h->admission.external_owner=true;
        if(fault==3)h->switched=true;
        if(fault==4)h->selection_object=32;
        if(fault==5)h->held_available=false;
        h->poll(second+dt,{},200,1,false);
        h->poll(second+2*dt,fault==8?PhysicalInputSample{0x1000}:PhysicalInputSample{},
                (fault==6 || fault==8)?100:fault==7?300:201,1,false);
        h->bridge_accept=true;
        h->poll(second+3*dt,{},201,1,fault==9);
        for(unsigned n=4;n<9;++n)h->poll(second+n*dt,{},201,1,fault==9,0,false,false,fault==10?3:2);
        check(!h->repeats,
              "pre-AIR repeat rejects owner, focus, conflict, stance, selection, observer, landing, bail, new pop, late Prepare and wrong actor");
    }
    {
        auto h=std::make_unique<Route>();h->bridge_accept=false;
        h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);
        h->poll(1051,{0x1000},100,1);
        // A remains held throughout: the X taps cannot invent its release.
        h->poll(1102,{0x5000},103,1,false);h->poll(1119,{0x1000},103,1,false);
        h->poll(1136,{0x5000},103,1,false);h->poll(1153,{0x1000},200,1,false);
        h->bridge_accept=true;h->poll(1170,{0x1000},201,1,false);
        h->poll(1187,{0x1000},201,1,false,0,false,false,2);
        check(!h->repeats,"ground X pair cannot invent a qualified pop release");
    }
}
void preair_successor_dependency() {
    auto h=std::make_unique<Route>();h->bridge_accept=false;
    h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);
    h->poll(1051,{0x1000},100,1);h->poll(1085,{},100,1);
    h->poll(1102,{0x4000},103,1,false);h->poll(1136,{},103,1,false);
    h->poll(1170,{0x4000},103,1,false);h->poll(1204,{},200,1,false);
    h->poll(1238,{},201,1,false);h->poll(1272,{},201,1,false);
    h->poll(1304,{},201,1,false);h->poll(1336,{},201,1,false);
    h->bridge_accept=true;h->poll(1360,{},201,1,false);
    check(h->bridge_calls==1 && !h->repeats,"late qualified parent starts before the child deadline");
    const auto out=h->poll(1394,{},201,1,false,0,false,false,2);
    check(h->repeats==1 && out.authored_playback,
          "pre-AIR queued repeat survives one native successor after parent admission at190ms");
    check(h->repeated_request.physical_pressed_at==1170 && h->repeated_request.deadline==1370 &&
          !h->repeated_request.unbounded && !out.held_actor_acknowledged,
          "dependency admission retains original edge deadline and grants no native completion");
}

struct DependencyQueue {
    InputRequestQueue queue;
    InputRequest parent{},child{};
    InputNativeContext native{};
    unsigned bases{},repeats{},waiting{},expired{},mutation{};
    bool wait_child{};
    DependencyQueue() {
        native.required=native.valid=native.airborne=true;
        native.generations={1,2,3};native.physics_step=10;native.sampled_at=1360;
        native.airborne_episode=1;native.flip={true,1,16,201,2,0,10,1,1};
        native.orientation={true,false};
        parent.action=InputAction::flip;parent.context=InputContext::ground;parent.session=1;
        parent.form=InputForm::neutral_press;parent.air.trick=ArcadeTrick::kickflip;
        parent.physical_pressed_at=1102;parent.pop_release_at=1085;parent.input_sequence=10;
        parent.generations=native.generations;parent.generation_mask=1;parent.unbounded=true;
        child=parent;child.form=InputForm::held_repeat;child.air=air_selection(TrickFamily::flip,4,2);
        child.physical_pressed_at=1170;child.input_sequence=20;child.parent_input_sequence=10;
        child.presses=2;child.deadline=1370;child.unbounded=false;
    }
    void enqueue(bool include_child=true) {check(queue.push(parent),"queue parent");if(include_child)check(queue.push(child),"queue exact child");}
    void dispatch(std::uint64_t now) {
        queue.dispatch(now,1,InputContext::air,{this,[](void* p,const InputRequest& r) noexcept {
            auto& q=*static_cast<DependencyQueue*>(p);
            if(r.presses==1) {
                ++q.bases;
                if(q.mutation==1)q.queue.cancel(InputAction::flip);
                if(q.mutation==2)q.queue.clear_ordinary();
                if(q.mutation==3)q.queue.clear();
                if(q.mutation>=1 && q.mutation<=4)q.queue.push(q.child);
                return InputDisposition::started;
            }
            if(q.wait_child){++q.waiting;return InputDisposition::waiting;}
            ++q.repeats;return InputDisposition::started;
        }},[&](const InputDispatchEvent& e) {
            if(e.disposition==InputDisposition::expired)++expired;
            if(mutation==5 && e.disposition==InputDisposition::started && e.request.presses==1) {
                queue.cancel(InputAction::flip);queue.push(child);
            }
        },native,false,{},1085);
    }
    void next(std::uint64_t now) {++native.physics_step;++native.flip.step;native.sampled_at=now;}
};
void successor_dependency_boundaries() {
    for(unsigned first_age:{190u,196u,199u})for(unsigned dt:{9u,17u,34u}) {
        DependencyQueue q;q.enqueue();q.native.sampled_at=1170+first_age;q.dispatch(1170+first_age);
        q.next(1170+first_age+dt);q.dispatch(1170+first_age+dt);
        check(q.bases==1 && q.repeats==1,"only exact first successor can carry timely accepted parent dependency");
    }
    for(unsigned age:{199u,200u,222u}) {
        DependencyQueue q;q.enqueue();q.native.sampled_at=1170+age;q.dispatch(1170+age);
        q.next(1204+age);q.dispatch(1204+age);
        check(q.repeats==(age==199?1u:0u),"parent at or beyond immutable second-edge deadline cannot grant allowance");
    }
    for(unsigned delay:{150u,151u}) {
        DependencyQueue q;q.enqueue();q.dispatch(1360);
        for(unsigned now:{1370u,1390u,1420u,1460u})q.dispatch(now);
        check(q.repeats==0,"same accepted native sample cannot execute beyond physical deadline");
        q.next(1360+delay);q.dispatch(1360+delay);
        check(q.repeats==(delay==150?1u:0u),"original native sample lease is inclusive150ms and never refreshed by polls");
    }
    for(unsigned fault=0;fault<23;++fault) {
        DependencyQueue q;
        if(fault==0)q.child.parent_input_sequence=11;
        if(fault==1)q.child.input_sequence=0;
        if(fault==2)q.child.pop_release_at=1086;
        if(fault==3)q.child.air=air_selection(TrickFamily::flip,6,2);
        if(fault==4)q.child.nollie=true;
        if(fault==5)q.child.presses=3;
        if(fault==6)q.child.context=InputContext::air;
        if(fault==7)q.parent.form=InputForm::repeated_direction;
        if(fault==20)q.native.orientation.valid=false;
        q.enqueue();q.dispatch(1360);q.next(1394);
        if(fault==8)++q.native.physics_step;
        if(fault==9)++q.native.generations.owner;
        if(fault==10)++q.native.airborne_episode;
        if(fault==11)q.native.flip.object=32;
        if(fault==12)q.native.flip.stance=0x20;
        if(fault==13)q.native.orientation.fakie=true;
        if(fault==14)q.native.sampled_at=1359;
        if(fault==15)q.native.flip.state=300;
        if(fault==16)q.native.airborne=false;
        if(fault==17)q.native.flip.id=3;
        if(fault==18)q.native.valid=false;
        if(fault==19)q.native.orientation.valid=false;
        if(fault==21)--q.native.generations.contact;
        if(fault==22)--q.native.physics_step; // A changed timestamp on the same step is not a new native sample.
        q.dispatch(1394);
        check(q.repeats==0,"invalid physical parent/native successor cannot borrow dependency admission");
    }
    {
        DependencyQueue q;q.native.orientation={};q.enqueue();q.native.sampled_at=1238;q.dispatch(1238);
        q.next(1272);q.dispatch(1272);
        check(q.repeats==1,"ordinary timely admission does not require the exceptional orientation proof");
    }
    {
        DependencyQueue q;q.enqueue();q.dispatch(1360);q.next(1394);q.wait_child=true;q.dispatch(1394);
        check(q.waiting==1 && q.expired==1,"waiting at the one successor opportunity consumes it without native credit");
        q.wait_child=false;q.dispatch(1395);q.next(1428);q.dispatch(1428);
        check(!q.repeats,"waiting successor never retries even with newly ready actor");
    }
    {
        DependencyQueue q;q.enqueue();q.native.sampled_at=1238;q.dispatch(1238);
        q.wait_child=true;q.next(1272);q.dispatch(1272);
        q.wait_child=false;q.native.sampled_at=1370;q.dispatch(1370);
        check(q.waiting==1 && !q.repeats,"early parent consumes its successor before deadline and cannot renew allowance later");
    }
    {
        DependencyQueue q;q.enqueue();q.dispatch(1360);q.wait_child=true;q.dispatch(1359);
        q.wait_child=false;q.next(1394);q.dispatch(1394);
        check(!q.repeats,"clock rollback retires dependency even if original request later becomes current");
    }
    for(unsigned mutation:{1u,2u,3u,4u,5u}) {
        DependencyQueue q;q.mutation=mutation;q.enqueue(mutation!=4);q.dispatch(1360);
        q.next(1394);q.dispatch(1394);
        check(!q.repeats,"adapter/report cancellation, clear or reentrant insertion cannot fabricate a preexisting child");
    }
}
void composed_successor_negatives() {
    for(unsigned fault=0;fault<15;++fault) {
        auto h=std::make_unique<Route>();h->bridge_accept=false;
        h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);h->poll(1051,{0x1000},100,1);
        h->poll(1085,{},100,1);h->poll(1102,{0x4000},103,1,false);h->poll(1136,{},103,1,false);
        h->poll(1170,{0x4000},103,1,false);h->poll(1204,{},200,1,false);
        h->poll(1238,{},201,1,false);h->poll(1272,{},201,1,false);h->poll(1304,{},201,1,false);
        h->poll(1336,{},201,1,false);h->bridge_accept=true;h->poll(1360,{},201,1,false);
        if(fault==0)++h->owner.rig;
        if(fault==1)h->admission.external_owner=true;
        if(fault==2)h->admission.gameplay=false;
        if(fault==5)h->switched=true;
        if(fault==6)h->selection_object=32;
        if(fault==7)h->held_available=false;
        if(fault==13)h->native_orientation.fakie=true;
        if(fault==14)h->native_orientation.valid=false;
        PhysicalInputSample input{};if(fault==11)input.lt=255;if(fault==12)input.buttons=0x8000;
        const auto out=h->poll(1394,input,fault==3?100:fault==4?300:201,1,fault==8,fault==10?1.0f:0.0f,false,false,fault==9?3:2);
        check(!h->repeats && !out.held_actor_acknowledged,
              "successor still rejects foreign owner, focus, conflict, landing, bail, stance, object, missing observer, Prepare, wrong actor, count and buttons");
        h->poll(1428,{},201,1,false,0,false,false,2);
        check(!h->repeats,"failed successor opportunity cannot replay on recovery");
    }
    auto h=std::make_unique<Route>();h->bridge_accept=false;
    h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);h->poll(1051,{0x1000},100,1);
    h->poll(1085,{},100,1);h->poll(1102,{0x4000},103,1,false);h->poll(1136,{},103,1,false);
    h->poll(1170,{0x4000},103,1,false);h->poll(1204,{},200,1,false);
    h->poll(1238,{},201,1,false);h->poll(1272,{},201,1,false);h->poll(1304,{},201,1,false);
    h->poll(1336,{},201,1,false);h->bridge_accept=true;h->poll(1369,{},201,1,false);
    for(unsigned now:{1370u,1371u,1380u,1390u})h->poll(now,{},201,1,false,0,false,false,2,false);
    check(!h->repeats,"duplicate client polls after deadline do not spend successor opportunity or admit early");
    h->poll(1403,{},201,1,false,0,false,false,2);
    check(h->repeats==1,"same native sample polling preserves one real successor opportunity");
}
void physical_repetition() {
    for(unsigned hz:{30u,60u,120u}) for(std::int16_t direction:{std::int16_t{0},std::int16_t{25000}}) {
        auto h=std::make_unique<Route>();h->first(direction);const auto id=h->requested;
        const auto step=1000/hz;
        h->poll(1200,{},201,id,true);
        const auto out=h->poll(1200+step,{0x4000},201,id,true);
        check(!h->repeats && h->bridge_calls==1 && !out.authored_playback &&
            !out.right_x && !out.right_y,
            "a new repeat cannot begin its multi-frame gesture inside the already-open native consumption window");
        h->poll(1300,{},201,id,false);
        h->poll(1334,{0x4000},201,id,true);
        check(!h->repeats && h->bridge_calls==1,
            "a later preparation flag cannot turn a missed repeat into a disconnected flip");
    }
    for(unsigned hz:{30u,60u,120u}) for(std::int16_t direction:{std::int16_t{0},std::int16_t{25000}}) {
        auto h=std::make_unique<Route>();h->first(direction);const auto id=h->requested;
        const auto step=1000/hz;
        h->poll(1200,{},201,id,true);
        h->poll(1200+step,{},201,id,false);
        const auto out=h->poll(1200+2*step,{0x4000},201,id,false);
        check(!h->repeats && h->bridge_calls==1 && !out.authored_playback &&
            !out.right_x && !out.right_y,
            "a second X after the observed native preparation window cannot start a disconnected gesture");
        h->poll(1300,{},201,id,true);
        h->poll(1317,{0x4000},201,id,true);
        check(!h->repeats && h->bridge_calls==1,
            "a later native flag cannot reopen the first flip repetition window");
    }
    for(bool takeoff:{false,true}) {
        auto h=std::make_unique<Route>();
        h->poll(1000,{},100,-1);h->poll(1017,{0x1000},100,-1);h->poll(1051,{0x1000},100,1);
        h->poll(1085,{},100,1);h->poll(1102,{},200,1);
        h->poll(1119,{0x4000},takeoff?200:201,1,false);
        check(h->bridge_calls==1,"captured takeoff X reaches original flip bridge");
        h->poll(1136,{},201,1,false,0,false,false,2);
        h->poll(1187,{0x4000},201,1,false,0,false,false,2);
        h->poll(1204,{},201,1,true,0,false,false,2);
        check(h->repeats==1,"native animation Kickflip admits repeated X while physics retains the original Ollie selection");
        h->poll(1237,{},201,1,true,0,true,true,2);
        const auto caught=h->poll(1270,{},201,1,true,1,true,true,2);
        check(h->bridge_calls==1 && caught.held_actor_acknowledged && caught.held_input_released &&
            !caught.authored_playback && !caught.right_x && !caught.right_y,
            "animation-owned extra rotation catches without changing physics selection or issuing another flip");
    }
    {
        auto single=std::make_unique<Route>();single->first();
        for(unsigned now=1200;now<=1700;now+=25)single->poll(now);
        check(single->bridge_calls==1 && !single->repeats,"single X never starts held input without a second physical edge");
    }
    for(unsigned hz:{30u,60u,120u}) for(std::int16_t direction:{std::int16_t{0},std::int16_t{-25000},std::int16_t{25000}}) {
        auto h=std::make_unique<Route>();h->first(direction);const auto id=h->requested;
        h->poll(1204,{0x4000},201,id,false);
        check(h->repeats==1,"second physical press prepares native gesture before the short held admission window");
        h->poll(1220,{},201,id,false);
        auto out=h->poll(1237,{},201,id,true);
        check(h->repeats==1 && out.authored_playback,"same physical repetition keeps one prepared input path through native admission");
        bool endpoint=false;
        for(unsigned n=1;n<=hz/3;++n) {
            const auto now=1237+std::uint64_t{n}*1000/hz;
            out=h->poll(now,{},201,id,true,0,n>2,n>3);
            endpoint=endpoint || (out.authored_playback && out.right_y>0 && out.right_x!=0);
        }
        check(endpoint && h->repeats==1 && h->bridge_calls==1,"held input reaches authored endpoint without a second native flip or pop request");
        out=h->poll(1600,{},201,id,true,1,true,true);
        check(!out.authored_playback && !out.right_x && !out.right_y && out.held_input_released && out.held_actor_acknowledged,
            "one extra native rotation releases RS for original catch with independent actor evidence");
        h->poll(1634,{},100,0);h->poll(1668,{0x4000},100,0);
        check(h->repeats==1 && h->bridge_calls==1,"landing rejects repeat and cannot borrow ground X");
    }
    for(unsigned failure=0;failure<8;++failure) {
        auto h=std::make_unique<Route>();h->first();
        if(failure==0)++h->owner.rig;
        if(failure==1)h->admission.external_owner=true;
        if(failure==2)h->held_available=false;
        if(failure>=6) {
            NativeRequestSample intervening{};intervening.valid=true;intervening.state=201;
            intervening.crouch_mode=failure==6?2u:1u;
            h->producer.observe(1180,h->owner,intervening,{true,failure==6?3:2,16});
            intervening.crouch_mode=2;
            h->producer.observe(1190,h->owner,intervening,{true,2,16});
        }
        const auto time=failure==3?1500u:1204u;
        if(failure==3){h->poll(1250);h->poll(1350);h->poll(1450);}
        h->poll(time,{0x4000},failure==4?100u:201u,failure==5?3:2,true);
        check(!h->repeats,"owner/conflict/missing observation/window/landing/wrong base/intervening native action cannot extend a flip");
    }
}
}
int main(){native_boundaries();preair_successor_dependency();successor_dependency_boundaries();composed_successor_negatives();preair_queued_repetition();queued_first_repetition();physical_repetition();std::cout<<checks<<" held-flip input checks passed; native animation observations are fixtures\n";}

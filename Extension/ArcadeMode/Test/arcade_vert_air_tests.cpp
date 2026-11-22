#include "Extension/ArcadeMode/arcade_vert_air.h"
#include "Extension/ArcadeMode/arcade_vert_trigger.h"
#include "Extension/ArcadeMode/arcade_air_motion.h"
#include "Extension/ArcadeMode/arcade_input_session.h"
#include "Extension/ArcadeMode/arcade_native_input_state.h"
#include "Engine/Game/Abi/orientation_target.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
using namespace vert_math;
namespace {
unsigned checks{};
void check(bool value,const char* why) {++checks;if(!value){std::cerr<<why<<'\n';std::exit(1);}}
VertPatch patch(Vec point,Vec normal) {
    VertPatch p;p.valid=true;
    for(auto& s:p.samples){s.valid=true;s.point=point;s.query_normal=normal;s.body={7,8,9};s.world_owner.value=10;}
    return p;
}
VertObservation ground() {
    VertObservation o;o.native.valid=true;o.native.owner={1,2,3,4,5};o.native.owner_generation=1;o.native.contact_generation=1;o.native.action_generation=1;o.native.mode_generation=1;
    o.native.flip_known=true;o.native.flip_id=-1;
    o.native.physics_step=2;o.native.sampled_at=1000;o.native.state=100;o.native.contacts=0xc000;
    o.native.physics_seconds=.01f;o.native.mode=1;o.position={0,5,0};o.velocity={2,8,0};
    o.up={-.9701425f,.2425356f,0};o.support=patch(o.position,o.up);o.supported=o.enabled=true;
    return o;
}
VertObservation fly(VertObservation o) {
    ++o.native.physics_step;o.native.sampled_at+=10;o.native.state=200;o.native.contacts=0;
    ++o.native.contact_generation;++o.native.action_generation;o.native.airborne=true;o.native.airborne_episode=1;o.native.airborne_started_known=true;
    o.native.airborne_started_at=o.native.sampled_at;o.supported=false;o.support={};return o;
}
void warm(VertAirController& c,VertObservation o) {
    o.native.sampled_at-=20;o.native.physics_step=1;o.position[1]-=.2f;
    o.up={-.8660254f,.5f,0};o.velocity={4,6.928203f,0};o.support=patch(o.position,o.up);
    c.observe(o);
}
void original_contact_support() {
    for(float yaw:{0.f,.8f,2.4f})for(float shift:{0.f,1000.f}) {
        const auto turn=[&](Vec v){return Vec{v[0]*std::cos(yaw)-v[2]*std::sin(yaw),v[1],v[0]*std::sin(yaw)+v[2]*std::cos(yaw)};};
        VertAirController c;auto o=ground();
        for(unsigned i=0;i<3;++i) {
            const float angle=.9f+.2f*static_cast<float>(i);
            o.native.physics_step=i+1;o.native.sampled_at=1000+17*i;
            o.position=add(turn({4*std::sin(angle),4*(1-std::cos(angle)),0}),{shift,0,shift});
            o.up=turn({-std::sin(angle),std::cos(angle),0});o.velocity=turn({8*std::cos(angle),8*std::sin(angle),0});
            std::array<VertRampContact,2> hits;
            for(unsigned j=0;j<2;++j)hits[j]={add(o.position,turn({0,0,j?.22f:-.22f})),o.up,{7,j+1,9},{}};
            for(auto& hit:hits)hit.owner.value=10;
            o.native_ramp=vert_ramp_contacts(hits,o.position,o.up);o.support={};
            check(o.native_ramp.valid,"distinct original wheel contacts validate a map-independent origin");c.observe(o);
            auto duplicate=hits;duplicate[1]=duplicate[0];
            check(!vert_ramp_contacts(duplicate,o.position,o.up).valid,"duplicate contact rows cannot fabricate footprint support");
            auto unknown=hits;unknown[1].owner.value=0;
            check(!vert_ramp_contacts(unknown,o.position,o.up).valid,"unknown static incarnation cannot qualify ramp");
        }
        o=fly(o);c.observe(o);check(c.eligible(o.native,o.native.sampled_at),"actual curved contacts recognize takeoff without a query patch");
        check(!c.consume(o.native,o.native.sampled_at,o.velocity),"contact-based recognition leaves no-input trajectory native");
    }
}
void retained_rear_wheel() {
    for(unsigned fault=0;fault<5;++fault) {
        VertAirController c;auto o=ground();warm(c,o);c.observe(o);
        const auto original=o.support.center();o.support={};o.native_ramp={};
        o.contact_normal=original.query_normal;o.contacted_shape=original.body;o.contact_owner=original.world_owner;
        for(unsigned i=0;i<8;++i) {
            ++o.native.physics_step;o.native.sampled_at+=17;o.position[1]+=.12f;o.contact_point=o.position;
            if(fault==1)o.contact_owner.value++;
            if(fault==2)o.contact_normal={0,1,0};
            if(fault==3)o.supported=false;
            if(fault==4)o.contacted_shape.generation++;
            c.observe(o);
        }
        o.supported=false;++o.native.physics_step;o.native.sampled_at+=17;c.observe(o);
        o=fly(o);c.observe(o);
        check(c.eligible(o.native,o.native.sampled_at)==(fault==0),
            "only real same-ramp rear-wheel support continues the measured long lip departure");
    }
}
void trigger_qualification() {
    for(unsigned channel:{0u,1u}) {
        VertAirController c;auto o=ground();warm(c,o);c.observe(o);o=fly(o);c.observe(o);
        TriggerPress press;press.press.sequence=1;press.press.token={InputSource::triggers,static_cast<std::int16_t>(channel),InputEdge::press};
        press.press.at=o.native.sampled_at;press.context=InputContext::air;press.native=InputNativeContext::from_frame(o.native);
        auto decision=qualify_vert_trigger(c,press,press.native,press.press.at);
        check(decision.owner==TriggerOwner::transfer && decision.ready,"LT and RT share eligible transfer behavior");
        auto held=press;held.context=InputContext::ground;held.native=InputNativeContext::from_frame(ground().native);
        check(qualify_vert_trigger(c,held,press.native,press.press.at).owner==TriggerOwner::none,"ground-held setup cannot become a transfer");
        for(unsigned i=0;i<20;++i) {
            ++o.native.physics_step;o.native.sampled_at+=10;o.velocity={4,-3,0};o.position={0,.3f,0};c.observe(o);
        }
        press.press.at=o.native.sampled_at;press.native=InputNativeContext::from_frame(o.native);
        check(c.landing().approach(o.native,patch({0,0,0},{0,1,0})),"landing geometry corroborates descending approach");
        decision=qualify_vert_trigger(c,press,press.native,press.press.at);
        check(decision.owner==TriggerOwner::landing_revert && !decision.ready,"near-touchdown edge reserves Revert ahead of transfer");
        press.owner=TriggerOwner::landing_revert;
        auto touchdown=o;++touchdown.native.physics_step;touchdown.native.sampled_at+=10;
        touchdown.native.airborne=false;touchdown.native.state=100;++touchdown.native.air_departure;
        touchdown.native.contacts=0xc000;touchdown.supported=true;touchdown.up={0,1,0};touchdown.contact_normal={0,1,0};touchdown.contacted_shape={7,8,9};
        const auto published=InputNativeContext::from_frame(touchdown.native);
        decision=qualify_vert_trigger(c,press,published,touchdown.native.sampled_at);
        check(decision.owner==TriggerOwner::landing_revert && !decision.ready,"ground publication before original support retains reserved edge");
        auto ground_edge=press;ground_edge.owner=TriggerOwner::none;ground_edge.context=InputContext::ground;
        ground_edge.native=published;ground_edge.press.at=touchdown.native.sampled_at;
        decision=qualify_vert_trigger(c,ground_edge,published,touchdown.native.sampled_at);
        check(decision.owner==TriggerOwner::landing_revert && !decision.ready,"fresh ground edge during publication gap cannot be stolen by Switch");
        c.observe(touchdown);
        decision=qualify_vert_trigger(c,press,published,touchdown.native.sampled_at);
        check(decision.owner==TriggerOwner::landing_revert && decision.ready,"real supported touchdown releases reserved native Revert");
        press.owner=TriggerOwner::transfer;
        check(qualify_vert_trigger(c,press,published,touchdown.native.sampled_at).owner==TriggerOwner::none,"transfer-consumed edge cannot request touchdown Revert");
        press.owner=TriggerOwner::landing_revert;press.context=InputContext::balance;
        check(qualify_vert_trigger(c,press,published,touchdown.native.sampled_at).owner==TriggerOwner::none,"vert qualifier never steals established Manual ownership");
        c.reset();check(qualify_vert_trigger(c,ground_edge,published,touchdown.native.sampled_at).owner==TriggerOwner::none,"ordinary ground cannot borrow a cancelled vert window");
    }
}
void landing_windows() {
    for(unsigned hz:{30u,60u,120u})for(unsigned fault=0;fault<10;++fault) {
        auto o=fly(ground());VertLandingWindow window;window.begin(o);
        const auto takeoff=o.native.sampled_at;
        const auto interval=1000u/hz;
        for(unsigned ms=interval;ms<250;ms+=interval) {
            ++o.native.physics_step;o.native.sampled_at=takeoff+ms;o.velocity={4,-3,0};o.position={0,.3f,0};
            window.observe(o);
        }
        const auto nearby=patch({0,0,0},{0,1,0});
        check(!window.available(o.native,o.native.sampled_at),"descending AIR alone cannot reserve landing input");
        check(window.approach(o.native,nearby),"fresh near-surface descending approach qualifies reservation");
        check(window.available(o.native,o.native.sampled_at),"reservation is available before actual contact");
        check(!window.contact().valid,"geometry never manufactures native landing");
        const auto air_origin=InputNativeContext::from_frame(o.native);
        const auto edge=o.native.sampled_at;
        check(window.deadline(air_origin,edge,o.native,edge)==edge+430,"reservation deadline is anchored to original edge");
        check(!window.ready(o.native,edge),"predicted approach cannot execute Revert");
        auto old_origin=InputNativeContext::from_frame(ground().native);
        check(!window.deadline(old_origin,edge,o.native,edge),"pre-takeoff ground origin cannot become landing input");
        auto bad=nearby;bad.valid=false;
        check(!window.approach(o.native,bad),"missing footprint cannot update landing proof");
        auto landing=o;++landing.native.physics_step;landing.native.sampled_at+=interval;
        landing.native.airborne=false;landing.native.state=100;++landing.native.air_departure;
        landing.native.contacts=0xc000;landing.supported=true;landing.up={0,1,0};
        landing.contact_normal={0,1,0};landing.contacted_shape={7,8,9};
        if(fault==1)landing.enabled=false;
        if(fault==2)landing.contested=true;
        if(fault==3)++landing.native.owner_generation;
        if(fault==4)landing.native.state=500;
        if(fault==5)landing.native.air_departure+=1;
        if(fault==6)landing.contact_normal={1,0,0};
        if(fault==7)landing.up={0,-1,0};
        if(fault==8)landing.native.sampled_at+=101;
        if(fault==9)landing.supported=false;
        window.observe(landing);
        check(window.contact().valid==(fault==0),"only original supported same-flight touchdown opens execution window");
        if(fault==0) {
            const auto contact=landing.native.sampled_at;
            check(window.deadline(air_origin,edge,landing.native,contact)==contact+250,"air edge survives actual touchdown publication");
            check(window.ready(landing.native,contact),"actual supported riding authorizes native Revert actuator");
            auto ground_origin=InputNativeContext::from_frame(landing.native);
            check(window.deadline(ground_origin,contact,landing.native,contact)==contact+250,"fresh touchdown ground edge qualifies");
            ++ground_origin.air_departure;
            check(!window.deadline(ground_origin,contact,landing.native,contact),"different departure cannot borrow landing window");
            for(unsigned ms=interval;ms<=250;ms+=interval) {
                ++landing.native.physics_step;landing.native.sampled_at=contact+ms;window.observe(landing);
                check(window.available(landing.native,landing.native.sampled_at),"touchdown buffer survives ordinary frame rates");
                check(window.contact().sampled_at==contact,"repeated support never refreshes landing deadline");
            }
            ++landing.native.physics_step;landing.native.sampled_at=contact+251;window.observe(landing);
            check(!window.available(landing.native,landing.native.sampled_at),"landing window expires into ordinary ground control");
        }
    }
    VertLandingWindow unqualified;auto o=ground();unqualified.observe(o);
    check(!unqualified.available(o.native,o.native.sampled_at),"ordinary ground never manufactures a vert Revert");
}
void low_speed_level_out() {
    for(float yaw:{0.f,.8f,2.4f})for(float vy:{-8.f,-1.f,-.01f,0.f,.01f,1.f,8.f}) {
        const Vec direction{std::cos(yaw),0,std::sin(yaw)};
        auto o=ground();o.velocity={0,vy,0};const auto plan=vert_level_out(o,direction);
        check(plan.action==VertAction::level_out,"valid direction admits Level Out on either side of the apex");
        const float speed=length(o.velocity);
        check(length(plan.velocity)<=speed+.00001f &&
            length(sub(plan.velocity,o.velocity))<=std::min(8.f,speed*.65f)+.00001f,
            "low-speed Level Out retains the existing kinetic and impulse bounds");
        check(plan.velocity[1]*vy>=0 && std::abs(plan.velocity[1])<=std::abs(vy),
            "descending Level Out never becomes an upward relaunch");
        check(vy==0 ? length(plan.velocity)==0 : dot(plan.velocity,direction)>0,
            "nonzero vertical motion redirects physically; exact rest invents no momentum");
        check(!plan.clear,"apex admission does not bypass the existing clearance query");
    }
    for(unsigned hz:{30u,60u,120u})for(unsigned channel:{0u,1u})for(float vy:{-5.f,-1.f,0.f,1.f}) {
        auto o=ground();o.native.physics_seconds=1.f/static_cast<float>(hz);
        VertAirController c;warm(c,o);c.observe(o);o=fly(o);o.velocity={0,9.81f+vy,0};c.observe(o);
        const auto begin=o;const auto start=o.native.sampled_at;
        for(unsigned step=1;step<=hz;++step) {
            ++o.native.physics_step;o.native.sampled_at=start+(1000u*step)/hz;
            const float elapsed=static_cast<float>(step)/static_cast<float>(hz);
            o.position=position(begin.position,begin.velocity,9.81f,elapsed);
            o.velocity={0,begin.velocity[1]-9.81f*elapsed,0};c.observe(o);
            check(!c.consume(o.native,o.native.sampled_at,o.velocity),"no-input native ascent/apex/descent has no Level Out impulse");
        }
        TriggerPress press;press.press.sequence=1;press.press.token={InputSource::triggers,static_cast<std::int16_t>(channel),InputEdge::press};
        press.press.at=o.native.sampled_at;press.context=InputContext::air;press.native=InputNativeContext::from_frame(o.native);
        const auto decision=qualify_vert_trigger(c,press,press.native,press.press.at);
        check(decision.owner==TriggerOwner::transfer && decision.ready && decision.deadline==press.press.at+150,
            "both low-speed triggers retain the original fresh-edge owner and deadline");
        check(c.request(o.native,press.press.at,press.press.at,1),"same-flight low-speed request accepted");
        auto plan=vert_level_out(o,c.direction());check(!c.stage(plan,o,press.press.at),"zero-speed physical leveling still requires clearance");
        plan.clear=true;check(c.stage(plan,o,press.press.at),"cleared low-speed plan stages through the existing controller");
        const auto released=c.consume(o.native,press.press.at,o.velocity);
        check(released.action==VertAction::level_out && c.phase()==VertPhase::released,
            "near-apex input executes within its original timestamp lifetime");
        check(!c.consume(o.native,press.press.at,o.velocity),"low-speed action cannot consume one physical edge twice");
        o.position=position(o.position,released.velocity,9.81f,o.native.physics_seconds);
        o.velocity=released.velocity;o.velocity[1]-=9.81f*o.native.physics_seconds;
        ++o.native.physics_step;o.native.sampled_at+=1000u/hz;c.observe(o);
        check(c.phase()==VertPhase::released && !c.consume(o.native,o.native.sampled_at,o.velocity),
            "native gravity continues after the sole bounded Level Out impulse");
    }
    auto o=ground();o.position={0,5,0};o.velocity={.7f,-.7f,0};
    const float time=1.4f;const auto target=position(o.position,o.velocity,9.81f,time);
    const auto landing=patch(target,{.9f,std::sqrt(.19f),0});
    const auto drop=vert_destination(o,VertRampSupport{o.support},landing,{1,0,0},9.81f,time);
    check(drop.action==VertAction::acid_drop && length(sub(drop.velocity,o.velocity))<.00001f,
        "a low-speed entry can reach a lower transition without added energy");
    auto blocked=landing;blocked.valid=false;
    check(vert_destination(o,VertRampSupport{o.support},blocked,{1,0,0},9.81f,time).action==VertAction::none,
        "removing the initial speed floor does not admit unsupported destinations");
}
void lifecycle() {
    auto o=ground();VertAirController c;warm(c,o);c.observe(o);check(c.phase()==VertPhase::rising_support,"steep support qualifies");
    o=fly(o);c.observe(o);check(c.eligible(o.native,1010),"genuine takeoff qualifies");
    check(!c.consume(o.native,1010,o.velocity),"no input never writes motion");
    check(!c.request(o.native,1010,1000,1),"pre-takeoff press cannot request transfer");
    for(unsigned i=0;i<4;++i){++o.native.physics_step;o.native.sampled_at+=10;o.velocity[1]-=.0981f;c.observe(o);}
    check(std::abs(c.gravity()-9.81f)<.01f,"gravity measured from native step cadence");
    check(c.request(o.native,1050,1050,2),"fresh air input accepted");
    check(!c.request(o.native,1050,1050,3),"same episode admits one request");
    auto plan=vert_level_out(o,c.direction());check(plan.action==VertAction::level_out,"no destination selects break vert");
    check(length(plan.velocity)<=length(o.velocity)+.001f && length(sub(plan.velocity,o.velocity))<=8.001f,"level out bounded without energy injection");
    check(!c.stage(plan,o,1050),"uncleared flight cannot stage");plan.clear=true;
    check(c.stage(plan,o,1050),"cleared flight staged");
    check(bool(c.consume(o.native,1050,o.velocity)),"one native physical impulse");
    check(!c.consume(o.native,1050,o.velocity),"duplicate physics call cannot repeat");
    auto landing=ground();landing.native.physics_step=8;landing.native.sampled_at=1070;landing.up={0,1,0};landing.support=patch({0,0,0},{0,1,0});
    c.observe(landing);check(c.phase()==VertPhase::landed,"actual supported riding establishes landing");
    for(unsigned negative=0;negative<7;++negative){
        c.reset();o=ground();warm(c,o);
        if(negative==0){o.support=patch(o.position,{0,1,0});o.up={0,1,0};}
        if(negative==1)o.supported=false;
        if(negative==2)o.native.state=401;
        if(negative==3)o.support.valid=false;
        if(negative==4)o.contested=true;
        c.observe(o);o=fly(o);
        if(negative==5)o.native.owner_generation++;
        if(negative==6)o.native.sampled_at+=151;
        c.observe(o);check(!c.eligible(o.native,o.native.sampled_at),"flat, rail, missing support, contested, stale owner and gap reject");
    }
    for(unsigned negative=0;negative<5;++negative){
        c.reset();o=ground();warm(c,o);c.observe(o);o=fly(o);c.observe(o);
        check(c.request(o.native,1010,1010,1),"request before cancellation");
        if(negative==0)o.enabled=false;
        if(negative==1)o.contested=true;
        if(negative==2)o.native.owner.core++;
        if(negative==3)o.native.valid=false;
        if(negative==4){o.native.airborne=false;o.native.state=500;o.supported=false;}
        ++o.native.physics_step;c.observe(o);
        check(!c.pending(o.native,1010) && c.phase()!=VertPhase::landed,"cancellation never fabricates landing");
    }
}
Vec rotate(Vec v,float a){return {v[0]*std::cos(a)-v[2]*std::sin(a),v[1],v[0]*std::sin(a)+v[2]*std::cos(a)};}
void contact_departure_delay() {
    for(bool curved:{false,true}) {
        VertAirController c;auto o=ground();if(curved){warm(c,o);c.observe(o);}
        ++o.native.physics_step;o.native.sampled_at+=17;o.position[1]+=.13f;
        o.up={-1,0,0};o.velocity={0,8,0};o.support=patch(o.position,o.up);c.observe(o);
        o=fly(o);c.observe(o);check(c.eligible(o.native,o.native.sampled_at)==curved,
            "vertical upper transition qualifies only after actual curved climbing support");
    }
    { VertAirController c;auto o=ground();warm(c,o);c.observe(o);
      ++o.native.physics_step;o.native.sampled_at+=17;o.support={};c.observe(o);
      o=fly(o);c.observe(o);check(c.eligible(o.native,o.native.sampled_at),
          "partial lip footprint does not erase a still-recent validated transition"); }
    for(unsigned fault=0;fault<8;++fault) {
        VertAirController c;auto o=ground();warm(c,o);c.observe(o);
        o.supported=false;o.support={};
        for(unsigned i=0;i<2;++i) {
            ++o.native.physics_step;o.native.sampled_at+=17;o.position[1]+=.13f;
            if(fault==1)o.native.state=401;
            if(fault==2)o.native.mode=4;
            if(fault==3)o.native.owner_generation++;
            if(fault==4)o.position[0]+=3;
            if(fault==5)o.velocity[1]=-1;
            if(fault==6)o.up={0,1,0};
            if(fault==7)o.native.sampled_at+=60;
            c.observe(o);
        }
        o=fly(o);c.observe(o);
        check(c.eligible(o.native,o.native.sampled_at)==(fault==0),
            "measured contact-to-AIR delay preserves only a recent same-owner rising departure");
        check(!c.consume(o.native,o.native.sampled_at,o.velocity),"departure delay cannot create a no-input impulse");
    }
    VertAirController c;auto o=ground();warm(c,o);c.observe(o);
    o.supported=false;o.support={};
    for(unsigned i=0;i<8;++i){++o.native.physics_step;o.native.sampled_at+=17;o.position[1]+=.1f;c.observe(o);}
    o=fly(o);c.observe(o);check(!c.eligible(o.native,o.native.sampled_at),"unsupported riding cannot renew the real support deadline");
}
void unavailable_snapshots() {
    // Replay the observed missing-frame pattern with procedural support. The
    // recording establishes the gap, not the geometry supplied by this test.
    for(unsigned fault=0;fault<6;++fault) {
        VertAirController c;auto o=ground();warm(c,o);c.observe(o);
        auto owner=o.native.owner;if(fault==1)++owner.core;
        const auto now=fault==2?1101u:fault==3?999u:1005u;
        c.unavailable(now,owner,fault!=4,fault==5);
        check(!c.request({},now,now,1),"unavailable frame never admits input");
        check(!c.consume({},now,o.velocity),"unavailable frame never writes motion");
        if(!fault)check(c.observation().native.sampled_at==1000,"gap never refreshes observation age");
        o=fly(o);c.observe(o);
        check(c.eligible(o.native,1010)==(fault==0),"only a short same-owner gap preserves genuine takeoff");
        check(!c.consume(o.native,1010,o.velocity),"no trigger after gap keeps native flight");
    }
    VertAirController c;auto o=ground();warm(c,o);c.observe(o);
    for(unsigned now:{1020u,1040u,1060u,1080u,1101u})c.unavailable(now,o.native.owner,true,false);
    check(!c.observation().native.valid,"repeated absence cannot extend the observation lifetime");
    c.reset();warm(c,o);c.observe(o);o=fly(o);c.observe(o);
    check(c.request(o.native,1010,1010,1),"fresh request before missing publication");
    auto plan=vert_level_out(o,{1,0,0});plan.clear=true;
    check(c.stage(plan,o,1010),"query staged before gap");
    c.unavailable(1015,o.native.owner,true,false);
    check(!c.pending(o.native,1015) && !c.stage(plan,o,1015),"in-flight old query cannot reacquire authority after gap");
    check(!c.consume(o.native,1015,o.velocity),"staged impulse withdrawn on gap");
    ++o.native.physics_step;o.native.sampled_at=1020;c.observe(o);
    check(c.pending(o.native,1020) && !c.consume(o.native,1020,o.velocity),"fresh frame alone cannot revive discarded plan");
    check(c.stage(plan,o,1020) && bool(c.consume(o.native,1020,o.velocity)),"fresh query and admitted frame can execute within original deadline");
    c.unavailable(1025,o.native.owner,true,false);
    check(c.phase()==VertPhase::released && length(c.alignment(o.native,1025))==0,"released transfer suspends alignment until original physics can prove continuation");
    ++o.native.physics_step;o.native.sampled_at=1030;c.observe(o);
    check(!c.request(o.native,1030,1030,2),"missing observation never rearms a second transfer");
    c.reset();o=ground();warm(c,o);c.observe(o);o=fly(o);c.observe(o);
    check(c.request(o.native,1010,1010,1),"expiry request");
    c.unavailable(1020,o.native.owner,true,false);
    ++o.native.physics_step;o.native.sampled_at=1030;c.observe(o);
    check(!c.pending(o.native,1161),"gap does not renew original trigger deadline");
}
void released_snapshot_gaps() {
    for(unsigned rate:{30u,60u,120u})for(unsigned gap:{1u,2u})for(unsigned fault=0;fault<11;++fault) {
        VertAirController c;auto o=ground();o.native.physics_seconds=1.f/static_cast<float>(rate);
        const auto milliseconds=static_cast<std::uint64_t>(std::ceil(1000.f/static_cast<float>(rate)));
        warm(c,o);c.observe(o);o=fly(o);c.observe(o);
        const auto advance=[&](unsigned steps) {
            const float seconds=static_cast<float>(steps)*o.native.physics_seconds;
            o.position=position(o.position,o.velocity,9.81f,seconds);o.velocity[1]-=9.81f*seconds;
            o.native.physics_step+=steps;o.native.sampled_at+=milliseconds*steps;
        };
        for(unsigned i=0;i<4;++i){advance(1);c.observe(o);}
        const auto edge=o.native.sampled_at;
        check(c.request(o.native,edge,edge,1),"gap fixture owns one original transfer edge");
        auto plan=vert_level_out(o,c.direction());plan.clear=true;
        check(c.stage(plan,o,edge),"gap fixture stages one clear impulse");
        const auto applied=c.consume(o.native,edge,o.velocity);check(bool(applied),"gap fixture releases one impulse");
        o.velocity=applied.velocity;
        c.unavailable(edge+1,o.native.owner,true,false);
        auto duplicate=o;duplicate.native.sampled_at=edge+2;c.observe(duplicate);
        check(length(c.alignment(duplicate.native,edge+2))==0,"new timestamp without newer original step cannot resume alignment");
        check(!c.request(duplicate.native,edge+2,edge+2,2) && !c.consume(duplicate.native,edge+2,o.velocity),
            "suspension admits no second trigger or physical impulse");
        advance(gap);
        if(fault==1)o.velocity[0]+=.6f;
        if(fault==2)o.velocity[1]+=.8f;
        if(fault==3)o.position[0]+=.4f;
        if(fault==4)++o.native.owner.core;
        if(fault==5)++o.native.owner_generation;
        if(fault==6)++o.native.airborne_episode;
        if(fault==7)o.native.physics_seconds*=2;
        if(fault==8)o.native.sampled_at+=101;
        if(fault==9)o.contested=true;
        if(fault==10)o.enabled=false;
        c.observe(o);
        check((c.phase()==VertPhase::released)==(fault==0),"only matching bounded original trajectory resumes released transfer");
        check((length(c.alignment(o.native,o.native.sampled_at))>.9f)==(fault==0),
            "collision, displacement, owner, episode, cadence, stale frame, bail or OFF denies alignment");
        check(!c.request(o.native,o.native.sampled_at,o.native.sampled_at,2) &&
              !c.consume(o.native,o.native.sampled_at,o.velocity),"resumption never rearms an edge or repeats the impulse");
        if(fault)continue;
        while(o.velocity[1]>=-.5f || o.native.sampled_at-c.landing().takeoff().sampled_at<150) {
            advance(1);c.observe(o);
        }
        check(c.phase()==VertPhase::released,"fresh original flight continues after a proven gap");
        auto target=o.position;target[1]-=.01f;
        check(c.landing().approach(o.native,patch(target,{0,1,0})),"original descending approach may reserve touchdown input");
        c.unavailable(o.native.sampled_at+1,o.native.owner,true,false);
        std::array<TriggerPress,2> landing_inputs{};
        for(unsigned channel:{0u,1u}) {
            auto& press=landing_inputs[channel];press.press.sequence=channel+2;
            press.press.token={InputSource::triggers,static_cast<std::int16_t>(channel),InputEdge::press};
            press.press.at=o.native.sampled_at+2;press.context=InputContext::air;
            press.native=InputNativeContext::from_frame(o.native);
            auto decision=qualify_vert_trigger(c,press,press.native,press.press.at);
            check(decision.owner==TriggerOwner::landing_revert && !decision.ready && decision.deadline==press.press.at+430,
                "suspended transfer permits only original landing reservation, never ready execution");
            press.owner=TriggerOwner::landing_revert;
            decision=qualify_vert_trigger(c,press,press.native,press.press.at+3);
            check(decision.owner==TriggerOwner::landing_revert && !decision.ready && decision.deadline==press.press.at+430,
                "repeated suspended observations preserve original landing edge deadline");
            auto published=o.native;++published.physics_step;published.sampled_at+=milliseconds;
            published.airborne=false;published.state=100;published.contacts=0xc000;++published.air_departure;
            decision=qualify_vert_trigger(c,press,InputNativeContext::from_frame(published),published.sampled_at);
            check(decision.owner==TriggerOwner::landing_revert && !decision.ready && decision.deadline==press.press.at+430,
                "ground publication without original contact cannot execute reserved Revert or fall through to Switch");
            auto consumed=press;consumed.owner=TriggerOwner::transfer;
            check(qualify_vert_trigger(c,consumed,press.native,press.press.at).owner==TriggerOwner::none,
                "suspension cannot recycle the already transfer-consumed edge");
        }
        advance(1);o.native.airborne=false;o.native.state=100;o.native.contacts=0xc000;++o.native.air_departure;
        o.supported=true;o.up=o.contact_normal={0,1,0};o.contacted_shape={7,8,9};
        c.observe(o);
        check(c.phase()==VertPhase::landed && c.landing().ready(o.native,o.native.sampled_at),
            "actual supported touchdown after a short busy gap retains independent landing Revert lineage");
        for(const auto& press:landing_inputs) {
            const auto decision=qualify_vert_trigger(c,press,InputNativeContext::from_frame(o.native),o.native.sampled_at);
            check(decision.owner==TriggerOwner::landing_revert && decision.ready && decision.deadline<=press.press.at+430,
                "only fresh original contact releases reserved LT or RT Revert with no renewed deadline");
        }
        check(!c.request(o.native,o.native.sampled_at,o.native.sampled_at,2),"actual touchdown cannot replay transfer");
    }
}
void occupied_actor_expiry() {
    VertAirController c;auto o=ground();warm(c,o);c.observe(o);o=fly(o);c.observe(o);
    const auto edge=o.native.sampled_at;
    check(c.request(o.native,edge,edge,1),"fresh transfer before existing native flip observation");
    ++o.native.action_generation;o.native.flip_id=2;
    auto plan=vert_level_out(o,{1,0,0});plan.clear=true;
    for(unsigned i=0;i<17;++i) {
        ++o.native.physics_step;o.native.sampled_at+=10;c.observe(o);
        check(!c.stage(plan,o,o.native.sampled_at) && !c.consume(o.native,o.native.sampled_at,o.velocity),
            "native flip owns its trajectory throughout transfer request expiry");
    }
    check(c.phase()==VertPhase::native_air && c.landing().flight(o.native),
        "expired transfer retains genuine vert flight and independent landing lineage");
    check(!c.request(o.native,o.native.sampled_at,edge,1),"expired original trigger cannot rearm after actor retirement");
    ++o.native.physics_step;o.native.sampled_at+=10;o.velocity={4,-3,0};c.observe(o);
    ++o.native.physics_step;o.native.sampled_at+=10;o.native.airborne=false;o.native.state=100;
    ++o.native.air_departure;o.native.contacts=0xc000;o.native.flip_id=-1;o.supported=true;
    o.up=o.contact_normal={0,1,0};o.contacted_shape={7,8,9};c.observe(o);
    check(c.landing().ready(o.native,o.native.sampled_at),
        "actual qualifying touchdown can still admit a new Revert after a safely refused transfer");
}
void destinations() {
    unsigned found{};
    for(float gravity:{9.81f,20.f,34.29f})for(float speed:{6.f,10.f,14.f})for(float angle:{0.f,.7f,1.6f,3.1f})for(float shift:{0.f,1000.f}) {
        auto o=ground();o.position={shift,6,shift};o.velocity=rotate({speed,3,0},angle);
        const auto origin=patch(o.position,rotate({-.98f,.1989975f,0},angle));
        for(float landing_time:{.8f,1.2f,1.6f}) {
            const auto point=position(o.position,o.velocity,gravity,landing_time);
            auto arrival=o.velocity;arrival[1]-=gravity*landing_time;
            if(arrival[1]>=-.5f)continue;
            // Surface tangent follows the descending ballistic approach, with
            // small positive normal impact. This constructs reachable ramps.
            auto normal=rotate({-arrival[1],speed,0},angle);unit(normal);
            const auto target=patch(point,normal);
            const auto plan=vert_destination(o,origin,target,rotate({1,0,0},angle),gravity,landing_time);
            if(normal[1]<.15f || normal[1]>.92f)continue;
            // Add a small approach angle instead of exact tangency.
            auto adjusted=normal;adjusted[1]+=.04f;unit(adjusted);
            const auto valid=vert_destination(o,origin,patch(point,adjusted),rotate({1,0,0},angle),gravity,landing_time);
            if(valid.action!=VertAction::none){
                ++found;check(valid.action==VertAction::spine,"opposing ramp is spine under translation and yaw");
                check(length(sub(position(o.position,valid.velocity,gravity,valid.seconds),valid.landing))<.001f,"analytic trajectory meets geometry");
                check(!valid,"geometric candidate alone is never clearance or a landing");
                auto invalid=target;invalid.valid=false;
                check(vert_destination(o,origin,invalid,rotate({1,0,0},angle),gravity,landing_time).action==VertAction::none,"missing patch rejected");
                check(vert_destination(o,origin,target,rotate({-1,0,0},angle),gravity,landing_time).action==VertAction::none,"ramp behind intent rejected");
                invalid=patch(add(point,{100,0,100}),adjusted);
                check(vert_destination(o,origin,invalid,rotate({1,0,0},angle),gravity,landing_time).action==VertAction::none,"unreachable destination rejected");
            }
            (void)plan;
        }
    }
    check(found>20,"parametric valid destination coverage");
    auto o=ground();o.position={0,6,0};o.velocity={8,0,0};
    auto target=patch(position(o.position,o.velocity,9.81f,1),{.7f,.7141428f,0});
    auto hip=vert_destination(o,patch(o.position,{0,.2f,-.9797959f}),target,{1,0,0},9.81f,1);
    check(hip.action==VertAction::hip,"adjacent transition uses hip action");
    o.velocity[1]=-1;target=patch(position(o.position,o.velocity,9.81f,1),{.7f,.7141428f,0});
    auto acid=vert_destination(o,patch(o.position,{-.9797959f,.2f,0}),target,{1,0,0},9.81f,1);
    check(acid.action==VertAction::acid_drop,"lower aligned transition uses acid drop");
}
void staged_races_and_orientation() {
    for(unsigned failure=0;failure<6;++failure) {
        VertAirController c;auto o=ground();warm(c,o);c.observe(o);o=fly(o);c.observe(o);
        check(c.request(o.native,1010,1010,1),"race request admitted");
        auto plan=vert_level_out(o,{1,0,0});plan.clear=true;
        check(c.stage(plan,o,1010),"race plan staged from immutable source");
        ++o.native.physics_step;o.native.sampled_at+=10;o.position=add(o.position,scale(o.velocity,.01f));
        if(failure==1)o.velocity[0]+=3;
        if(failure==2)o.position[0]+=1;
        if(failure==3)o.native.owner_generation++;
        if(failure==4)o.native.flip_id=2;
        if(failure==5)o.native.flip_known=false;
        c.observe(o);
        check(bool(c.consume(o.native,1020,o.velocity))==(failure==0),"collision, displacement, owner or native actor replacement revokes staged impulse");
    }
    for(float dt:{1.f/30,1.f/60,1.f/120})for(float yaw:{0.f,.8f,2.f}) {
        std::array<float,16> target{std::cos(yaw),0,-std::sin(yaw),0,0,1,0,0,std::sin(yaw),0,std::cos(yaw),0,3,4,5,1};
        const auto before=target;
        check(vert_align_target(target,{.8f,.6f,0},dt),"physical target tilts toward destination");
        check(dingosdk::game::orientation_target::basis_valid(target),"orientation stays orthonormal");
        check(dingosdk::game::orientation_target::error(before,target)<=2.5f*dt+.0001f,"orientation rate bounded at multiple frame rates");
        check(target[12]==3 && target[13]==4 && target[14]==5,"orientation never moves native position");
    }
}
void destination_contact_landing() {
    for(unsigned fault=0;fault<4;++fault) {
        VertAirController c;auto o=ground();warm(c,o);c.observe(o);o=fly(o);c.observe(o);
        check(c.request(o.native,1010,1010,1),"landing request");
        auto plan=vert_level_out(o,{1,0,0});plan.action=VertAction::spine;plan.clear=true;
        plan.landing={4,1,0};plan.normal={.6f,.8f,0};plan.destination=patch(plan.landing,plan.normal);
        check(c.stage(plan,o,1010) && bool(c.consume(o.native,1010,o.velocity)),"landing plan executed");
        auto landed=ground();landed.native.physics_step=4;landed.native.sampled_at=1020;
        landed.position=plan.landing;landed.up=plan.normal;landed.velocity={4,-3,0};
        landed.support=plan.destination;landed.contacted_shape=plan.destination.center().body;
        if(fault==1)landed.position[0]+=20;
        if(fault==2)++landed.contacted_shape.index;
        if(fault==3)landed.supported=false;
        c.observe(landed);
        check((c.phase()==VertPhase::landed)==(fault==0),
            "only actual destination support near the prediction establishes a transfer landing");
    }
}
void unchanged_motion() {
    AirMotion c;AirMotionIntent intent;intent.enabled=true;intent.sampled_at=1000;intent.generation=1;
    AirMotionFrame f;f.now=1000;f.owner=1;f.state=100;f.dt=.01f;f.ordinary_ground=true;
    (void)c.step(intent,f);f.state=200;f.velocity_y=8;f.native_vert_air=true;
    auto output=c.step(intent,f);check(!output.vertical && !output.horizontal,"qualified vert suppresses ordinary gravity and launch shaping");
}
struct Requests {unsigned count{};InputRequest last{};};
TriggerRouting routing_for(VertAirController& controller) {
    return {&controller,[](void* value,const TriggerPress& press,const InputNativeContext& current,std::uint64_t now) noexcept {
        return qualify_vert_trigger(*static_cast<VertAirController*>(value),press,current,now);
    }};
}
void session_trigger_ownership() {
    InputSession session;Requests requests;InputAdmission a;
    a.gameplay=true;a.pose=SkatePose::air;a.action_mode=ActionExperiment::classic;
    a.extensions={&requests,[](void* p,const InputRequest& r) noexcept {
        if(r.action!=InputAction::transfer_command)return InputDisposition::unsupported;
        auto& out=*static_cast<Requests*>(p);++out.count;out.last=r;return InputDisposition::started;
    }};
    VertAirController controller;auto observation=ground();warm(controller,observation);controller.observe(observation);
    observation=fly(observation);controller.observe(observation);a.triggers=routing_for(controller);
    auto frame=observation.native;
    a.native_context=InputNativeContext::from_frame(frame);
    a.surface={1,frame.physics_step,true,false,false,true,0,a.native_context.generations};
    a.tick=1;(void)session.poll(1010,{},0,0,a);
    ++frame.physics_step;frame.sampled_at=1020;a.native_context=InputNativeContext::from_frame(frame);a.tick=2;
    observation.native=frame;controller.observe(observation);
    PhysicalInputSample pad;pad.lt=255;
    auto result=session.poll(1020,pad,0,0,a);
    check(result.trigger_owned && requests.count==1,"accepted LT owns native trigger channel");
    controller.cancel();a.surface.can_transfer=false;++frame.physics_step;frame.sampled_at=1030;a.native_context=InputNativeContext::from_frame(frame);a.tick=3;
    result=session.poll(1030,pad,0,0,a);
    check(result.trigger_owned && requests.count==1,"trigger ownership persists after request while held");
    pad.lt=0;++frame.physics_step;frame.sampled_at=1040;a.native_context=InputNativeContext::from_frame(frame);a.tick=4;
    result=session.poll(1040,pad,0,0,a);check(!result.trigger_owned,"physical release retires transfer trigger ownership");
}
void transfer_preserves_grab_producer() {
    for(unsigned channel:{0u,1u})for(bool qualified:{false,true}) {
        InputSession session;InputAdmission a;
        a.gameplay=a.action_fresh=a.inspected_stance=true;a.pose=SkatePose::air;a.action_mode=ActionExperiment::classic;
        VertAirController controller;auto o=ground();warm(controller,o);controller.observe(o);o=fly(o);controller.observe(o);
        if(qualified)a.triggers=routing_for(controller);
        struct Producer {unsigned reads{},requests{};} producer;
        a.normal_grab.context=&producer;
        a.normal_grab.controls=[](void* state) noexcept {
            ++static_cast<Producer*>(state)->reads;return native_grab::Control{2u};
        };
        a.extensions={&producer,[](void* state,const InputRequest& input) noexcept {
            if(input.action!=InputAction::transfer_command)return InputDisposition::unsupported;
            ++static_cast<Producer*>(state)->requests;return InputDisposition::started;
        }};
        const auto poll=[&](std::uint64_t now,PhysicalInputSample pad) {
            ++o.native.physics_step;o.native.sampled_at=now;controller.observe(o);
            a.native_context=InputNativeContext::from_frame(o.native);
            a.surface={1,o.native.physics_step,true,false,false,true,0,a.native_context.generations};a.tick=now/17;
            return session.poll(now,pad,0,0,a);
        };
        check(poll(1020,{}).action.native_grab_triggers,"existing qualified grab producer owns its native hand channel");
        const auto before=producer.reads;
        PhysicalInputSample pad;if(channel)pad.rt=255;else pad.lt=255;
        auto output=poll(1037,pad);
        check(output.action.native_grab_triggers==qualified && producer.reads==before+(qualified?1u:0u),
            "shared transfer claim preserves grab producer in the same poll; unclaimed trigger still conflicts");
        check(output.vert_trigger_owned==(qualified?1u<<channel:0u) && producer.requests==(qualified?1u:0u),
            "one contextual trigger claims one trajectory request without replacing grab ownership");
        output=poll(1054,pad);
        check(output.action.native_grab_triggers==qualified && producer.requests==(qualified?1u:0u),
            "held transfer does not repeat or suppress ongoing grab production");
    }
}
void vert_trigger_manual_entry() {
    // Input adapter fixture: genuine vert/landing proof is covered above. No
    // Manual request or synthetic native state is a combo/scoring acceptance.
    for(unsigned phase:{0u,1u,2u})for(unsigned channel:{0u,1u})for(bool nose:{false,true})for(bool conflict:{false,true}) {
        const bool landing=phase==1;
        InputSession session;NativeInputState producer;InputAdmission a;
        a.gameplay=a.action_fresh=a.inspected_stance=true;a.action_mode=ActionExperiment::classic;
        struct Proof {unsigned channel{};bool landing{};} proof{channel,landing};
        a.triggers={&proof,[](void* state,const TriggerPress& press,const InputNativeContext&,std::uint64_t) noexcept {
            const auto& value=*static_cast<Proof*>(state);
            return press.channel()==value.channel?TriggerDecision{
                value.landing?TriggerOwner::landing_revert:TriggerOwner::transfer,press.press.at+150,true}:TriggerDecision{};
        }};
        a.extensions={nullptr,[](void*,const InputRequest& input) noexcept {
            return input.action==InputAction::transfer_command?InputDisposition::started:InputDisposition::unsupported;
        }};
        const auto poll=[&](std::uint64_t now,PhysicalInputSample pad,unsigned state) {
            NativeRequestSample sample{};sample.valid=true;sample.state=state;sample.crouch_mode=1;
            sample.words[1]=state==200?0u:0xc000u;
            const auto frame=producer.observe(now,{1,2,3,4,5},sample,{true,-1,16});
            a.native_context=InputNativeContext::from_frame(frame);a.pose=frame.airborne?SkatePose::air:SkatePose::riding;
            a.manual_safe=state!=101;a.manual_prepare=frame.airborne;a.tick=now/17;
            a.surface={1,frame.physics_step,true,false,false,true,0,a.native_context.generations};
            return session.poll(now,pad,0,0,a);
        };
        const unsigned initial=landing?100u:200u;
        (void)poll(1000,{},initial);
        PhysicalInputSample pad;if(phase!=2){if(channel)pad.rt=255;else pad.lt=255;}
        pad.y=nose?-25000:25000;
        const auto start=poll(1017,pad,initial);
        check(start.vert_trigger_owned==(phase==2?0u:1u<<channel),"first Manual direction retains exact concurrent vert trigger claim");
        if(phase==2) {
            pad.y=nose?25000:-25000;
            check(poll(1034,pad,200).manual.owns,"existing AIR Manual owns its prepared input before transfer");
            if(channel)pad.rt=255;else pad.lt=255;
            if(conflict)pad.lt=pad.rt=255;
            const auto held=poll(1051,pad,200);
            if(held.manual.owns!=!conflict || held.vert_trigger_owned!=(1u<<channel))std::cerr<<"AIR manual transfer: channel="<<channel<<" nose="<<nose<<" conflict="<<conflict<<" owned="<<held.manual.owns<<" mask="<<unsigned(held.vert_trigger_owned)<<"\n";
            check(held.manual.owns==!conflict && held.vert_trigger_owned==(1u<<channel),
                "fresh same-poll transfer preserves existing AIR Manual; unclaimed channel still cancels");
            continue;
        }
        if(landing)check(start.action.requested && start.action.pad.owns_left_stick,
            "qualified landing starts the existing native Revert input before Manual handoff");
        if(conflict)pad.lt=pad.rt=255;
        pad.y=nose?25000:-25000;
        auto output=poll(1034,pad,landing?101u:200u);
        if(landing) {
            check((session.pending_manual().action!=InputAction::none)==!conflict,
                "Manual pair during landing Revert retains its own input despite held consumed trigger");
            for(unsigned at=1051;at<1731;at+=17)(void)poll(at,pad,101);
            output=poll(1731,pad,100);
        }
        check(output.manual.owns==!conflict && (!output.manual.owns || output.right_y==(nose?20000:-20000)),
            "consumed vert trigger permits Manual/Nose preparation; unrelated trigger remains a conflict");
        if(!conflict)check(!output.manual.confirmed,"input delivery alone never certifies a native Manual or combo");
    }
}
void held_transfer_intent() {
    UnifiedInput input;Requests received;
    InputAdapter adapter{&received,[](void* p,const InputRequest& r) noexcept {
        if(r.action==InputAction::transfer_command){auto& out=*static_cast<Requests*>(p);++out.count;out.last=r;}
        return InputDisposition::started;
    }};
    InputEnvironment e;e.gameplay=true;e.context=InputContext::air;
    VertAirController controller;auto observation=ground();warm(controller,observation);controller.observe(observation);
    observation=fly(observation);controller.observe(observation);e.triggers=routing_for(controller);
    auto native=observation.native;
    PhysicalInputSample pad;pad.buttons=8;
    for(unsigned i=0;i<30;++i) {
        native.sampled_at+=20;++native.physics_step;observation.native=native;controller.observe(observation);
        e.native_context=InputNativeContext::from_frame(native);
        e.surface={1,native.physics_step,true,false,false,true,0,e.native_context.generations};
        if(i==29)pad.rt=255;
        input.poll(native.sampled_at,pad,e,adapter);
    }
    check(received.count==1 && received.last.direction==InputEvents::encode(1,0),
        "transfer uses still-held direction after the trick sequence window expires");
}
void input_edges() {
    for(bool right:{false,true})for(bool held_before:{false,true}) {
        UnifiedInput input;Requests received;
        InputAdapter adapter{&received,[](void* p,const InputRequest& r) noexcept {
            auto& out=*static_cast<Requests*>(p);if(r.action==InputAction::transfer_command){++out.count;out.last=r;}
            return InputDisposition::started;
        }};
        InputEnvironment e;e.gameplay=true;e.context=InputContext::ground;
        VertAirController controller;auto observation=ground();warm(controller,observation);controller.observe(observation);
        e.triggers=routing_for(controller);
        auto native=observation.native;e.native_context=InputNativeContext::from_frame(native);
        PhysicalInputSample physical;
        input.poll(1000,physical,e,adapter);
        if(held_before){if(right)physical.rt=255;else physical.lt=255;}
        native.sampled_at=1005;++native.physics_step;observation.native=native;controller.observe(observation);
        e.native_context=InputNativeContext::from_frame(native);
        input.poll(1005,physical,e,adapter);
        observation=fly(ground());observation.native.physics_step=4;controller.observe(observation);native=observation.native;
        e.context=InputContext::air;e.native_context=InputNativeContext::from_frame(native);
        e.surface.fresh=e.surface.can_transfer=true;e.surface.generation=1;e.surface.physics_step=native.physics_step;
        e.surface.generations=e.native_context.generations;
        input.poll(1010,physical,e,adapter);
        if(right)physical.rt=255;else physical.lt=255;
        native.sampled_at=1020;++native.physics_step;observation.native=native;controller.observe(observation);
        e.native_context=InputNativeContext::from_frame(native);
        e.surface.generations=e.native_context.generations;
        input.poll(1020,physical,e,adapter);
        check(received.count==(held_before?0u:1u),"only fresh airborne trigger recognizes transfer");
        input.poll(1021,physical,e,adapter);check(received.count==(held_before?0u:1u),"held trigger never repeats transfer");
        if(!held_before)check(received.last.input_sequence && received.last.physical_pressed_at==1020 && received.last.air_episode.number==1,"edge timestamp and native flight retained");
    }
}
}
int main(){retained_rear_wheel();original_contact_support();trigger_qualification();landing_windows();low_speed_level_out();lifecycle();contact_departure_delay();unavailable_snapshots();released_snapshot_gaps();occupied_actor_expiry();destinations();staged_races_and_orientation();destination_contact_landing();unchanged_motion();session_trigger_ownership();transfer_preserves_grab_producer();vert_trigger_manual_entry();held_transfer_intent();input_edges();std::cout<<checks<<" vert checks passed\n";}

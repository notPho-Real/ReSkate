#include "Extension/ArcadeMode/arcade_vert_air.h"
#include "Extension/ArcadeMode/arcade_vert_trigger.h"
#include "Extension/ArcadeMode/arcade_air_motion.h"
#include "Extension/ArcadeMode/arcade_input_session.h"
#include "Extension/ArcadeMode/arcade_native_input_state.h"
#include "Engine/Game/Abi/orientation_target.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>
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
struct PushBoundaryFixture {
    VertAirController controller; NativeInputState publisher; VertObservation o; NativeRequestSample live;
    unsigned dt;
    explicit PushBoundaryFixture(unsigned rate,bool pushing=true):dt((1000+rate-1)/rate) {
        live.valid=true;live.state=100;live.crouch_mode=1;live.words[0]=pushing?0x40:0;live.words[1]=0xc000;
        o.enabled=o.supported=true;o.support.valid=true;
        for(unsigned i=0;i<3;++i) {
            const float angle=1.1f+.1f*static_cast<float>(i);
            o.position={0,2+.15f*static_cast<float>(i),0};o.up={0,std::cos(angle),std::sin(angle)};
            o.velocity={0,8*std::sin(angle),-8*std::cos(angle)};
            for(auto& s:o.support.samples){s.valid=true;s.point=o.position;s.query_normal=o.up;s.body={7,8,9};s.world_owner.value=10;}
            o.native=publisher.observe(1000+i*dt,{1,2,3,4,5},live,{true,-1,99},false,1.f/static_cast<float>(rate));
            controller.observe(o);
        }
        check(controller.phase()==VertPhase::rising_support,"synthetic curvature establishes real support before the recorded push/contact boundary");
    }
};
void push_clear_boundary() {
    for(bool pushing:{false,true})for(unsigned rate:{30u,60u,120u})for(unsigned fault=0;fault<17;++fault) {
        PushBoundaryFixture x(rate,pushing);auto now=x.o.native.sampled_at+1;auto owner=x.o.native.owner;
        x.live.words[0]=pushing?0:0x40;x.live.words[1]=0x4000;
        NativeInputCorrelation d{};check(!x.publisher.correlate(now,owner,x.live,&d).valid,"mismatched push/contact snapshot never acquires native input authority");
        if(fault==1)x.live.words[0]=0x20;
        if(fault==2){d.observed.stance_and_push^=0x20;}
        if(fault==3)++owner.core;
        if(fault==4)++d.observed.owner_generation;
        if(fault==5)now+=101;
        if(fault==6)d.observed.physics_step+=3;
        if(fault==7)d.observed.flip_known=false;
        if(fault==8)d.observed.flip_id=3;
        if(fault==9)d.observed.selection_object=100;
        if(fault==10)x.live.state=300;
        if(fault==11)x.live.state=500;
        if(fault==12)x.live.crouch_mode=2;
        if(fault==13)x.controller.cancel();
        if(fault==14)x.live.valid=false;
        if(fault==15)d.observed.rail_family=true;
        if(fault==16)d.mismatches|=NativeInputCorrelation::mode_changed;
        check(x.controller.publication_gap(d,x.live,owner,now)==(fault==0),"only a push edge with unchanged stance and idle owner retains proven ramp history");
    }
    for(bool pushing:{false,true})for(unsigned rate:{30u,60u,120u})for(unsigned fault=0;fault<8;++fault) {
        PushBoundaryFixture x(rate,pushing);auto now=x.o.native.sampled_at+1;const auto owner=x.o.native.owner;
        x.live.words[0]=pushing?0:0x40;x.live.words[1]=0x4000;NativeInputCorrelation d{};
        check(!x.publisher.correlate(now,owner,x.live,&d).valid,"confirmation fixture begins with an unadmitted publication gap");
        check(x.controller.publication_gap(d,x.live,owner,now),"proven ramp push edge retains history until original confirmation");x.controller.unavailable(now,owner,true,false);
        check(!x.controller.eligible(x.o.native,now) && !x.controller.consume(x.o.native,now,x.o.velocity),"publication gap cannot consume a transfer or grant stale input eligibility");
        x.o.native=x.publisher.observe(now+x.dt,owner,x.live,{true,-1,99},false,1.f/static_cast<float>(rate));
        if(fault==1)x.o.native.stance_and_push^=0x40;
        if(fault==2)x.o.native.stance_and_push=0x20;
        if(fault==3)x.o.native.selection_object=100;
        if(fault==4)x.o.native.flip_id=1;
        if(fault==5)x.o.native.rail_family=true;
        if(fault==6)x.o.native.state=500;
        if(fault==7)x.o.native.mode=3;
        x.controller.observe(x.o);check((x.controller.phase()==VertPhase::rising_support)==(fault==0),"the next original frame must confirm the observed push edge and unchanged actor");
        x.live.state=200;x.live.words[1]=0;x.o.support={};x.o.supported=false;
        x.o.native=x.publisher.observe(now+2*x.dt,owner,x.live,{true,-1,99},false,1.f/static_cast<float>(rate));
        x.controller.observe(x.o);
        check(x.controller.eligible(x.o.native,x.o.native.sampled_at)==(fault==0),"only independently confirmed original AIR qualifies after push completion");
        check(!x.controller.consume(x.o.native,x.o.native.sampled_at,x.o.velocity),"no trigger preserves native flight after the push/contact handoff");
    }
}

// fbb09e16 no-trigger Deck capture, original publisher times and measured
// support/velocity rows. Actor and shape identities below are fixture tokens:
// the capture did not record their atomic addresses. This is a boundary replay,
// not proof that an offline observation is a native landing.
void recorded_push_onset_takeoff() {
    for(bool vertical:{false,true}) {
        VertAirController unqualified;auto plain=ground();plain.native.selection_object=99;
        plain.up=vertical?Vec{0,0,1}:Vec{0,1,0};plain.support=patch(plain.position,plain.up);
        unqualified.observe(plain);
        NativeInputCorrelation gap{plain.native,NativeInputCorrelation::stance_changed};
        NativeRequestSample live{};live.valid=true;live.state=100;live.crouch_mode=1;live.words[0]=0x40;live.words[1]=0xc000;
        check(!unqualified.publication_gap(gap,live,plain.native.owner,plain.native.sampled_at+1),
            "flatground or standalone vertical support cannot become vert from push onset");
    }
    struct Row {unsigned step;std::uint64_t ms;Vec position,velocity,normal;};
    const Row rows[]{
        {9636,1801798049,{146.4234f,1.23778f,-99.47458f},{-.030875f,8.406275f,-6.2341266f},{0,.634393f,.773011f}},
        {9639,1801798097,{146.42183f,1.672956f,-99.72305f},{-.030442f,8.881953f,-4.200911f},{0,.5141023f,.8577289f}},
        {9642,1801798145,{146.4204f,2.115732f,-99.880135f},{-.0281083f,8.878125f,-2.5017233f},{0,.3368897f,.9415442f}},
        {9648,1801798253,{146.4175f,2.9614563f,-99.97101f},{-.0267408f,8.055744f,.00008679f},{0,.07356458f,.9972905f}},
        {9651,1801798301,{146.41621f,3.34919f,-99.971f},{-.0254456f,7.6775093f,-.00003936f},{0,0,1}},
        {9654,1801798347,{146.41492f,3.718358f,-99.98103f},{-.0254171f,7.2389617f,-.1713165f},{0,0,1}}
    };
    VertAirController c;auto o=ground();o.native.selection_object=99;o.native.owner_generation=1711;
    for(const auto& row:rows) {
        if(row.step==9651) {
            auto original=o.native;original.physics_step=9650;original.sampled_at=1801798285;
            NativeInputCorrelation gap{original,NativeInputCorrelation::stance_changed};
            NativeRequestSample live{};live.valid=true;live.state=100;live.crouch_mode=1;live.words[0]=0x40;live.words[1]=0xc000;
            check(c.publication_gap(gap,live,original.owner,1801798294),"captured push onset retains established curved-ramp history");
            c.unavailable(1801798294,original.owner,true,false);
            check(!c.eligible(original,1801798294) && !c.consume(original,1801798294,o.velocity),"push onset gap grants no transfer authority");
        }
        o.native.physics_step=row.step;o.native.sampled_at=row.ms;o.native.stance_and_push=row.step>=9651?0x40:0;
        o.native.contacts=row.step>=9654?0x4000:0xc000;
        o.position=row.position;o.velocity=row.velocity;o.up=row.normal;o.support={};
        std::array<VertRampContact,2> contacts;
        for(unsigned i=0;i<2;++i){contacts[i]={add(row.position,{i?.2f:-.2f,0,0}),row.normal,{7,i+1,9},{}};contacts[i].owner.value=10;}
        o.native_ramp=vert_ramp_contacts(contacts,o.position,o.up);c.observe(o);
    }
    o=fly(o);o.native.physics_step=9656;o.native.sampled_at=1801798379;o.native.airborne_started_at=o.native.sampled_at;
    o.native_ramp={};o.position={146.41408f,3.9528522f,-99.98055f};o.velocity={-.0244861f,6.9787397f,.17991157f};c.observe(o);
    check(c.eligible(o.native,o.native.sampled_at),"captured real AIR follows original curved support and confirmed push onset");
    o.native.physics_step=9658;o.native.sampled_at=1801798417;o.native.state=201;c.observe(o);
    NativeInputCorrelation gap{o.native,NativeInputCorrelation::mode_changed|NativeInputCorrelation::stance_changed};
    NativeRequestSample live{};live.valid=true;live.state=201;live.crouch_mode=2;
    check(c.publication_gap(gap,live,o.native.owner,1801798418),"idle AIR mode publication may complete the recorded native push");
    c.unavailable(1801798418,o.native.owner,true,false);
    o.native.physics_step=9659;o.native.sampled_at=1801798435;o.native.mode=2;o.native.stance_and_push=0;c.observe(o);
    check(c.eligible(o.native,o.native.sampled_at),"original mode2 confirms idle actor after push completion");
    check(!c.consume(o.native,o.native.sampled_at,o.velocity),"recorded no-trigger flight never submits trajectory motion");
}

struct LandingFixture {
    VertAirController c;VertObservation o;unsigned dt;
    LandingFixture(unsigned rate,bool released):o(ground()),dt((1000+rate-1)/rate) {
        o.native.physics_seconds=1.f/static_cast<float>(rate);o.native.selection_object=99;
        warm(c,o);c.observe(o);o=fly(o);c.observe(o);o.native.state=201;o.native.mode=2;
        for(unsigned i=0;i<4;++i)advance();
        if(released) {
            check(c.request(o.native,o.native.sampled_at,o.native.sampled_at,1),"genuine flight can own transfer before landing");
            auto plan=vert_level_out(o,c.direction());plan.clear=true;
            check(c.stage(plan,o,o.native.sampled_at),"clear physical plan stages before contact");
            auto applied=c.consume(o.native,o.native.sampled_at,o.velocity);check(bool(applied),"one physical plan delivered before synthetic collision");
            o.velocity=applied.velocity;
        }
        while(o.velocity[1]>=-.5f || o.native.sampled_at-c.landing().takeoff().sampled_at<150)advance();
        auto target=o.position;target[1]-=.05f;
        check(c.landing().approach(o.native,patch(target,{0,1,0})),"independent descending approach before contact publication");
    }
    void advance() {
        o.position=position(o.position,o.velocity,9.81f,o.native.physics_seconds);o.velocity[1]-=9.81f*o.native.physics_seconds;
        ++o.native.physics_step;o.native.sampled_at+=dt;c.observe(o);
    }
    NativeInputCorrelation gap() const {return {o.native,NativeInputCorrelation::mode_changed|NativeInputCorrelation::contacts_changed};}
    NativeRequestSample live() const {NativeRequestSample s{};s.valid=true;s.state=201;s.crouch_mode=1;s.words[1]=0x8000;return s;}
};
void recorded_contact_publications() {
    // fdf original capture15640..15644: c000/0x40 ->4000/0 ->0/0
    // ->4000/0 ->AIR. Geometry/actor tokens and callback ordering below
    // deliberately model consecutive mismatches; the trace lacks the last
    // admitted body frame, so this is not a claim about its exact scheduling.
    VertAirController c;auto o=ground();o.native.selection_object=99;o.native.stance_and_push=0x40;
    o.native.owner_generation=4343;o.native.physics_step=15639;o.native.sampled_at=1809762458;
    warm(c,o);c.observe(o);check(c.phase()==VertPhase::rising_support,"contact-chain fixture has proven curvature");
    struct Edge {unsigned step;std::uint64_t ms,now;std::uint32_t original_contacts,original_stance,live_contacts;};
    const Edge edges[]{{15640,1809762473,1809762480,0xc000,0x40,0x4000},
        {15641,1809762489,1809762495,0x4000,0,0},{15642,1809762507,1809762520,0,0,0x4000}};
    for(const auto& edge:edges) {
        auto f=o.native;f.physics_step=edge.step;f.sampled_at=edge.ms;
        f.contacts=edge.original_contacts;f.stance_and_push=edge.original_stance;
        NativeRequestSample live{};live.valid=true;live.state=100;live.crouch_mode=1;live.words[1]=edge.live_contacts;
        NativeInputCorrelation gap{f,NativeInputCorrelation::contacts_changed|
            (edge.original_stance?NativeInputCorrelation::stance_changed:0u)};
        if(edge.step==15642) {
            for(unsigned fault=0;fault<14;++fault) {
                auto rejected=c;auto bad=gap;auto owner=f.owner;auto at=edge.now;
                if(fault==0)++owner.core;
                if(fault==1)++bad.observed.owner_generation;
                if(fault==2)bad.observed.contacts=0x4000;
                if(fault==3)bad.observed.stance_and_push=0x40;
                if(fault==4)bad.observed.flip_id=3;
                if(fault==5)++bad.observed.selection_object;
                if(fault==6)bad.observed.rail_family=true;
                if(fault==7)bad.observed.physics_seconds=1.f/30;
                if(fault==8)bad.observed.sampled_at=o.native.sampled_at+101;
                if(fault==9)bad.observed.physics_step+=6;
                if(fault==10)bad.observed.mode=2;
                if(fault==11)bad.observed.sampled_at=1809762489;
                if(fault==12)++bad.observed.airborne_episode;
                if(fault==13)++bad.observed.air_departure;
                check(!rejected.publication_gap(bad,live,owner,at),
                    "contact chain requires confirming original contact, idle actor, owner, cadence and original deadline");
            }
            auto expired=c;expired.unavailable(o.native.sampled_at+101,o.native.owner,true,false);
            check(!expired.preserves_native(),"retained original publications never renew the physical history deadline");
        }
        check(c.publication_gap(gap,live,f.owner,edge.now),"confirmed original contact publications retain bounded ramp history");
        c.unavailable(edge.now,f.owner,true,false);
        check(!c.eligible(f,edge.now) && !c.consume(f,edge.now,o.velocity),"contact-publication chain grants no motion or transfer input");
        check(c.observation().native.physics_step==15639,"contact-only history never replaces physical observation or lip deadline");
    }
    o.native.physics_step=15643;o.native.sampled_at=1809762525;o.native.stance_and_push=0;o.native.contacts=0x4000;
    o.supported=false;o.support={};c.observe(o);
    o=fly(o);o.native.physics_step=15644;o.native.sampled_at=1809762539;o.native.airborne_started_at=o.native.sampled_at;c.observe(o);
    check(c.eligible(o.native,o.native.sampled_at),"only correlated original AIR establishes flight after the publication chain");
    check(!c.consume(o.native,o.native.sampled_at,o.velocity),"no input after contact chain preserves native trajectory");
}
void confirmed_takeoff_publication() {
    // 096 capture: admitted30082, original30085/state200 versus live201,
    // push64->0, mismatch288, same generation5727, elapsed60ms. The journal
    // contains every original callback. The retained live edge/actor/episode
    // were not logged, so their confirmation below is a conditional fixture,
    // not a claim that this completely replays the native scheduler.
    for(unsigned fault=0;fault<18;++fault) {
        VertAirController c;auto o=ground();o.native.selection_object=99;
        o.native.physics_seconds=1.f/60;o.native.stance_and_push=0x40;
        o.native.owner_generation=5727;o.native.physics_step=30082;o.native.sampled_at=1825241225;
        o.native.airborne_episode=1;o.native.air_departure=1;
        warm(c,o);c.observe(o);
        auto prior=o.native;prior.physics_step=30083;prior.sampled_at=1825241237;prior.contacts=0x4000;
        NativeRequestSample live{};live.valid=true;live.state=200;live.crouch_mode=1;live.words[0]=0x40;
        if(fault==13)live.state=100;
        NativeInputCorrelation gap{prior,NativeInputCorrelation::state_changed|NativeInputCorrelation::contacts_changed};
        if(fault==13)gap.mismatches=NativeInputCorrelation::contacts_changed;
        check(c.publication_gap(gap,live,prior.owner,1825241249),"takeoff fixture retains a real prior publication without admitting AIR");
        c.unavailable(1825241249,prior.owner,true,false);
        auto f=prior;f.physics_step=30085;f.sampled_at=1825241270;f.state=200;f.contacts=0;
        f.airborne=true;f.airborne_episode=2;f.airborne_started_known=true;f.airborne_started_at=1825241253;
        live.state=201;live.words[0]=0;
        auto owner=f.owner;auto now=std::uint64_t{1825241285};
        if(fault==1)++owner.core;
        if(fault==2)++f.owner_generation;
        if(fault==3)f.flip_id=3;
        if(fault==4)++f.selection_object;
        if(fault==5)f.rail_family=true;
        if(fault==6)++f.airborne_episode;
        if(fault==7)++f.air_departure;
        if(fault==8)f.airborne_started_known=false;
        if(fault==9)f.airborne_started_at=prior.sampled_at-1;
        if(fault==10)f.airborne_started_at=f.sampled_at+1;
        if(fault==11)f.physics_seconds=1.f/30;
        if(fault==12)f.physics_step++;
        if(fault==14)f.contacts=0x4000;
        if(fault==15)live.words[0]=0x20;
        if(fault==16)now=f.sampled_at+101;
        if(fault==17)f.sampled_at=o.native.sampled_at+101;
        gap={f,NativeInputCorrelation::state_changed|NativeInputCorrelation::stance_changed};
        const bool retained=c.publication_gap(gap,live,owner,now);
        check(retained==(fault==0),"confirmed ground-to200 publication must preserve ramp history only with exact lineage and cleared push");
        if(fault)continue;
        c.unavailable(now,owner,true,false);
        check(c.phase()==VertPhase::rising_support && !c.eligible(f,now) && !c.consume(f,now,o.velocity),
            "confirmed publication cannot manufacture flight or grant trigger or motion authority");
        check(c.observation().native.physics_step==30082,"publication does not replace last physical ramp observation");
        o.native=f;o.native.state=201;o.native.stance_and_push=0;o.native.physics_step=30086;
        o.native.sampled_at=1825241287;o.supported=false;o.support={};
        for(unsigned resumed_fault=0;resumed_fault<9;++resumed_fault) {
            auto rejected=c;auto changed=o;
            if(resumed_fault==0)++changed.native.airborne_episode;
            if(resumed_fault==1)++changed.native.air_departure;
            if(resumed_fault==2)++changed.native.airborne_started_at;
            if(resumed_fault==3)changed.native.flip_id=3;
            if(resumed_fault==4)++changed.native.selection_object;
            if(resumed_fault==5)changed.native.contacts=0x4000;
            if(resumed_fault==6)changed.native.physics_step+=2;
            if(resumed_fault==7)changed.native.rail_family=true;
            if(resumed_fault==8)changed.native.sampled_at=1825241326;
            rejected.observe(changed);
            check(!rejected.eligible(changed.native,changed.native.sampled_at),
                "fresh physical continuation cannot substitute another flight, actor, contact or stale history");
        }
        c.observe(o);
        check(c.eligible(o.native,o.native.sampled_at),"only fresh correlated AIR establishes flight after confirmed takeoff publication");
        check(!c.consume(o.native,o.native.sampled_at,o.velocity),"no input after takeoff chain preserves native motion");
    }
}
void publication_rejection_evidence() {
    LandingFixture x(60,false);auto gap=x.gap();auto live=x.live();const char* reason{};
    // Diagnostics must identify the missing boundary without widening it.
    gap.observed.physics_step+=3;
    check(!x.c.publication_gap(gap,live,x.o.native.owner,x.o.native.sampled_at+18,&reason) &&
        std::string_view(reason)=="original_step_gap","more than two unobserved native steps still refuse history");
    gap=x.gap();++gap.observed.owner_generation;
    check(!x.c.publication_gap(gap,live,x.o.native.owner,x.o.native.sampled_at+18,&reason) &&
        std::string_view(reason)=="generation","generation rejection is independently observable");
    gap=x.gap();
    check(!x.c.publication_gap(gap,live,x.o.native.owner,x.o.native.sampled_at+101,&reason) &&
        std::string_view(reason)=="original_age","stale original publication is independently observable");
}
void direct_landing_contact_boundary() {
    // Native fdf trace: step5929 idle201/mode2/0 contacts was followed by
    // live100/mode1/c00018ms later (mismatch224), with unchanged stance.
    // Identity and flight setup below are fixture tokens, not captured pointers.
    for(unsigned fault=0;fault<9;++fault) {
        LandingFixture x(60,false);auto d=x.gap();auto live=x.live();const auto original=x.o.native;
        d.mismatches|=NativeInputCorrelation::state_changed;live.state=100;live.words[1]=0xc000;
        auto owner=original.owner;auto now=original.sampled_at+18;
        if(fault==1)++owner.core;
        if(fault==2)++d.observed.owner_generation;
        if(fault==3)live.state=500;
        if(fault==4)d.mismatches&=~NativeInputCorrelation::state_changed;
        if(fault==5)live.words[1]=0;
        if(fault==6)live.words[0]=0x20;
        if(fault==7)now=original.sampled_at+101;
        if(fault==8)d.observed.flip_id=3;
        check(x.c.publication_gap(d,live,owner,now)==(fault==0),
            "captured direct idle AIR to riding contact retains only qualified landing lineage");
        if(fault)continue;
        x.c.unavailable(now,owner,true,false);
        check(!x.c.eligible(original,now) && !x.c.consume(original,now,x.o.velocity) &&
            length(x.c.alignment(original,now))==0 && !x.c.landing().ready(original,now),
            "live touchdown mismatch cannot execute motion or Revert");
        ++x.o.native.physics_step;x.o.native.sampled_at=now+1;
        x.o.native.airborne=false;x.o.native.state=100;x.o.native.mode=1;x.o.native.contacts=0xc000;
        ++x.o.native.air_departure;x.o.supported=false;x.o.velocity={0,0,-2.2928247f};
        x.o.up=x.o.contact_normal={0,1,0};x.o.contacted_shape={7,8,9};x.c.observe(x.o);
        check(!x.c.landing().ready(x.o.native,x.o.native.sampled_at),"original riding without support cannot establish Revert");
        ++x.o.native.physics_step;x.o.native.sampled_at+=x.dt;x.o.supported=true;x.c.observe(x.o);
        check(x.c.landing().ready(x.o.native,x.o.native.sampled_at),"original wheel support confirms direct touchdown");
        for(unsigned channel=0;channel<2;++channel) {
            TriggerPress press;press.press.sequence=channel+1;press.press.at=original.sampled_at;
            press.press.token={InputSource::triggers,static_cast<std::int16_t>(channel),InputEdge::press};
            press.context=InputContext::air;press.native=InputNativeContext::from_frame(original);
            const auto current=InputNativeContext::from_frame(x.o.native);
            const auto decision=qualify_vert_trigger(x.c,press,current,x.o.native.sampled_at);
            check(decision.owner==TriggerOwner::landing_revert && decision.ready,"LT and RT retain bounded direct-touchdown Revert ownership");
            press.owner=TriggerOwner::transfer;
            check(qualify_vert_trigger(x.c,press,current,x.o.native.sampled_at).owner==TriggerOwner::none,
                "transfer edge cannot replay as direct-touchdown Revert");
        }
    }
}
void landing_contact_boundary() {
    for(unsigned rate:{30u,60u,120u})for(bool released:{false,true})for(unsigned fault=0;fault<13;++fault) {
        LandingFixture x(rate,released);auto d=x.gap();auto live=x.live();auto owner=x.o.native.owner;auto now=x.o.native.sampled_at+1;
        if(fault==1)++owner.core;
        if(fault==2)++d.observed.owner_generation;
        if(fault==3)live.words[0]=0x20;
        if(fault==4){x.o.velocity[1]=2;++x.o.native.physics_step;x.o.native.sampled_at+=x.dt;x.c.observe(x.o);d=x.gap();now=x.o.native.sampled_at+1;}
        if(fault==5)live.words[1]=0;
        if(fault==6)live.crouch_mode=0;
        if(fault==7)live.state=500;
        if(fault==8)now+=101;
        if(fault==9)d.observed.contacts=0x8000;
        if(fault==10)d.observed.flip_id=3;
        if(fault==11)d.observed.selection_object=0;
        if(fault==12)d.observed.sampled_at=x.c.landing().takeoff().sampled_at+100;
        check(x.c.publication_gap(d,live,owner,now)==(fault==0),"only same-owner descending idle201 return-to-support timing preserves history");
    }
    for(unsigned rate:{30u,60u,120u})for(bool released:{false,true})for(unsigned fault=0;fault<12;++fault) {
        LandingFixture x(rate,released);const auto original=x.o.native;auto d=x.gap();auto live=x.live();const auto edge=original.sampled_at+1;
        check(x.c.publication_gap(d,live,original.owner,edge),"recorded landing publication boundary retains lineage");
        x.c.unavailable(edge,original.owner,true,false);
        check(x.c.preserves_native() && !x.c.eligible(original,edge) && !x.c.pending(original,edge) &&
            !x.c.consume(original,edge,x.o.velocity) && length(x.c.alignment(original,edge))==0 &&
            !x.c.landing().ready(original,edge),"partial contact has no motion, tilt or Revert execution authority");
        TriggerPress presses[2];
        for(unsigned channel=0;channel<2;++channel) {
            auto& p=presses[channel];p.press.sequence=channel+2;p.press.token={InputSource::triggers,static_cast<std::int16_t>(channel),InputEdge::press};
            p.press.at=edge;p.context=InputContext::air;p.native=InputNativeContext::from_frame(original);
            const auto decision=qualify_vert_trigger(x.c,p,p.native,edge);
            check(decision.owner==TriggerOwner::landing_revert && !decision.ready && decision.deadline==edge+430,"fresh LT and RT can retain original landing reservation only");
            p.owner=TriggerOwner::landing_revert;
            auto consumed=p;consumed.owner=TriggerOwner::transfer;
            check(qualify_vert_trigger(x.c,consumed,p.native,edge).owner==TriggerOwner::none,"a consumed transfer edge cannot become landing Revert");
        }
        ++x.o.native.physics_step;x.o.native.sampled_at+=x.dt;x.o.native.mode=1;x.o.native.contacts=0x8000;
        if(fault==1)x.o.native.sampled_at=original.sampled_at+101;
        if(fault==2)++x.o.native.owner.core;
        if(fault==3)++x.o.native.owner_generation;
        if(fault==4)x.o.native.selection_object=100;
        if(fault==5)x.o.native.stance_and_push=0x20;
        if(fault==6)x.o.native.contacts=0;
        if(fault==7)x.o.native.state=500;
        if(fault==8)x.o.native.mode=2;
        if(fault==9)x.o.enabled=false;
        if(fault==10)x.o.velocity[1]=0;
        if(fault==11)x.o.velocity[1]=2;
        const bool accepted=fault==0 || fault>=10;
        x.c.observe(x.o);
        check(x.c.landing_contact_pending()==accepted,"only original compatible partial contact continues the bounded landing history");
        check(!x.c.landing().ready(x.o.native,x.o.native.sampled_at) && !x.c.request(x.o.native,x.o.native.sampled_at,x.o.native.sampled_at,5),"AIR contact cannot execute Revert or rearm transfer");
        x.o.native=original;x.o.native.physics_step+=2;x.o.native.sampled_at+=2*x.dt;
        x.o.native.airborne=false;x.o.native.state=100;x.o.native.mode=1;x.o.native.contacts=0xc000;++x.o.native.air_departure;
        x.o.enabled=true;x.o.supported=false;x.o.up=x.o.contact_normal={0,1,0};x.o.contacted_shape={7,8,9};x.c.observe(x.o);
        check(!x.c.landing().ready(x.o.native,x.o.native.sampled_at),"ground publication alone never establishes actual landing");
        ++x.o.native.physics_step;x.o.native.sampled_at=original.sampled_at+3000/rate;x.o.supported=true;x.c.observe(x.o);
        check(x.c.landing().ready(x.o.native,x.o.native.sampled_at)==accepted,"only independent original wheel support establishes Revert readiness");
        for(const auto& p:presses) {
            const auto decision=qualify_vert_trigger(x.c,p,InputNativeContext::from_frame(x.o.native),x.o.native.sampled_at);
            check((decision.owner==TriggerOwner::landing_revert && decision.ready)==accepted,"original LT/RT landing edge survives only proven touchdown");
            if(accepted)check(decision.deadline<=edge+430,"contact timing never renews the physical trigger deadline");
        }
    }
    for(unsigned rate:{30u,60u,120u})for(bool released:{false,true}) {
        LandingFixture x(rate,released);const auto source=x.o.native;auto diagnostic=x.gap();auto live=x.live();
        check(x.c.publication_gap(diagnostic,live,source.owner,source.sampled_at+1),"expiry fixture has bounded contact history");
        x.c.unavailable(source.sampled_at+1,source.owner,true,false);
        ++x.o.native.physics_step;x.o.native.sampled_at+=x.dt;x.o.native.mode=1;x.o.native.contacts=0x8000;
        x.c.observe(x.o);
        x.c.unavailable(source.sampled_at+101,source.owner,true,false);
        check(!x.c.preserves_native() && !x.c.landing().takeoff().valid,
            "new partial-contact observations cannot renew the original100ms landing bridge deadline");
    }
}

void publication_contact_boundary() {
    for(unsigned rate:{30u,60u,120u})for(unsigned fault=0;fault<12;++fault) {
        const auto dt=static_cast<std::uint64_t>(std::ceil(1000.f/static_cast<float>(rate)));
        VertAirController c;auto o=ground();NativeInputState publisher;
        NativeRequestSample live{};live.valid=true;live.state=100;live.crouch_mode=1;live.words[1]=0xc000;
        const auto owner=o.native.owner;
        for(unsigned i=0;i<3;++i) {
            const float angle=.9f+.2f*static_cast<float>(i);
            o.position={4*std::sin(angle),4*(1-std::cos(angle)),0};
            o.up={-std::sin(angle),std::cos(angle),0};o.velocity={8*std::cos(angle),8*std::sin(angle),0};
            o.support=patch(o.position,o.up);
            o.native=publisher.observe(1000+i*dt,owner,live,{true,-1,99},false,1.f/static_cast<float>(rate));
            c.observe(o);
        }
        check(c.phase()==VertPhase::rising_support,"record-shaped curved riding establishes history before the contact edge");
        auto now=o.native.sampled_at+1;auto observed_owner=owner;
        live.words[1]=0x4000;
        if(fault==1)++observed_owner.core;
        if(fault==2)live.crouch_mode=2;
        if(fault==3)live.words[0]=0x20;
        if(fault==4)now+=151;
        if(fault==5)now=o.native.sampled_at-1;
        if(fault==6)live.state=500;
        if(fault==7)live.valid=false;
        if(fault==8)live.state=300;
        NativeInputCorrelation diagnostic{};
        check(!publisher.correlate(now,observed_owner,live,&diagnostic).valid,"contact/publication mismatch never admits a native input frame");
        if(fault==9)++diagnostic.observed.owner_generation;
        if(fault==10)diagnostic.observed.physics_step+=3;
        if(fault==11)diagnostic.mismatches=0;
        const bool permitted=c.publication_gap(diagnostic,live,observed_owner,now);
        check(permitted==(fault==0),"only fresh same-owner contact timing retains vert history; bail, rail, mode and stance reject");
        if(!permitted)continue;
        c.unavailable(now,owner,true,false);
        check(c.preserves_native() && !c.eligible(o.native,now) && !c.consume(o.native,now,o.velocity),
            "contact gap withholds the ordinary shaper but has no stale motion or trigger authority");
        const auto source=o.support.center();o.support={};o.supported=true;
        o.contact_point=o.position;o.contact_normal=source.query_normal;o.contacted_shape=source.body;o.contact_owner=source.world_owner;
        check(vert_riding_contact_mask(live.state,live.words[1]),"observed0x4000 rear-wheel contact reaches original-contact inspection");
        o.native=publisher.observe(now+dt,owner,live,{true,-1,99},false,1.f/static_cast<float>(rate));c.observe(o);
        check(c.phase()==VertPhase::rising_support,"original rear contact continues the measured ramp");
        live.state=200;live.words[1]=0;
        check(!publisher.correlate(now+dt+1,owner,live,&diagnostic).valid && c.publication_gap(diagnostic,live,owner,now+dt+1),
            "native ground-to-air publication gap preserves history without a synthetic takeoff");
        c.unavailable(now+dt+1,owner,true,false);
        o.supported=false;o.native=publisher.observe(now+2*dt,owner,live,{true,-1,99},false,1.f/static_cast<float>(rate));c.observe(o);
        check(c.eligible(o.native,o.native.sampled_at),"only fresh original AIR establishes the genuine episode after rear contact");
        live.state=201;
        check(!publisher.correlate(now+2*dt+1,owner,live,&diagnostic).valid && c.publication_gap(diagnostic,live,owner,now+2*dt+1),
            "native200-to201 publication retains the same flight without admitting the mismatched frame");
        c.unavailable(now+2*dt+1,owner,true,false);
        o.native=publisher.observe(now+3*dt,owner,live,{true,-1,99},false,1.f/static_cast<float>(rate));c.observe(o);
        check(c.eligible(o.native,o.native.sampled_at) && !c.consume(o.native,o.native.sampled_at,o.velocity),
            "no-input flight remains native after the current201 publication");
        const auto deadline=o.native.sampled_at;
        for(unsigned gap=20;gap<=120;gap+=20)c.unavailable(deadline+gap,owner,true,false);
        check(!c.preserves_native(),"repeated publication gaps cannot renew the historical owner deadline");
    }
    for(auto mask:{0u,0x4000u,0x8000u,0xc000u,0x4001u})for(auto state:{100u,103u,200u,201u,300u,500u})
        check(vert_riding_contact_mask(state,mask)==((state==100 || state==103) && (mask==0x4000 || mask==0x8000 || mask==0xc000)),
            "contact inspection cannot admit unrelated airborne, rail, bail or unknown contact masks");
}
void idle_air_mode_boundary() {
    for(bool pushing:{false,true})for(unsigned fault=0;fault<12;++fault) {
        VertAirController c;auto o=ground();o.native.selection_object=99;o.native.stance_and_push=pushing?0x40:0;warm(c,o);c.observe(o);o=fly(o);c.observe(o);
        ++o.native.physics_step;o.native.sampled_at+=10;o.native.state=201;c.observe(o);
        NativeInputCorrelation diagnostic{o.native,NativeInputCorrelation::mode_changed|(pushing?NativeInputCorrelation::stance_changed:0u)};
        NativeRequestSample live{};live.valid=true;live.state=201;live.crouch_mode=2;
        auto owner=o.native.owner;auto now=o.native.sampled_at+1;
        if(fault==1)live.crouch_mode=3;
        if(fault==2){diagnostic.observed.mode=2;live.crouch_mode=1;}
        if(fault==3)diagnostic.observed.flip_id=8;
        if(fault==4)++owner.core;
        if(fault==5)++diagnostic.observed.airborne_episode;
        if(fault==6)live.words[1]=0x4000;
        if(fault==7)diagnostic.observed.flip_known=false;
        if(fault==8){live.state=200;diagnostic.mismatches|=NativeInputCorrelation::state_changed;}
        if(fault==9)check(c.request(o.native,now,now,1),"mode boundary negative has a pending transfer");
        if(fault==10)now+=101;
        if(fault==11)live.words[0]=0x20;
        check(c.publication_gap(diagnostic,live,owner,now)==(fault==0),
            "only the recorded idle201 mode1-to2 boundary preserves no-input history");
    }
    for(bool pushing:{false,true})for(unsigned fault=0;fault<9;++fault) {
        VertAirController c;auto o=ground();o.native.selection_object=99;o.native.stance_and_push=pushing?0x40:0;warm(c,o);c.observe(o);o=fly(o);c.observe(o);
        ++o.native.physics_step;o.native.sampled_at+=10;o.native.state=201;c.observe(o);
        const auto before=o.native;
        NativeInputCorrelation diagnostic{before,NativeInputCorrelation::mode_changed|(pushing?NativeInputCorrelation::stance_changed:0u)};
        NativeRequestSample live{};live.valid=true;live.state=201;live.crouch_mode=2;
        check(c.publication_gap(diagnostic,live,o.native.owner,o.native.sampled_at+1),"recorded idle mode boundary staged");
        c.unavailable(o.native.sampled_at+1,o.native.owner,true,false);
        check(c.preserves_native() && !c.eligible(before,before.sampled_at+1) && !c.consume(before,before.sampled_at+1,o.velocity),
            "mode gap never executes a plan or admits a trigger using old publication");
        ++o.native.physics_step;o.native.sampled_at+=10;o.native.mode=2;o.native.stance_and_push=0;
        if(fault==1)o.native.flip_id=8;
        if(fault==2)o.native.flip_known=false;
        if(fault==3)++o.native.selection_object;
        if(fault==4)++o.native.airborne_episode;
        if(fault==5)o.native.mode=1;
        if(fault==6)o.native.contacts=0x4000;
        if(fault==7){o.native.state=100;o.native.airborne=false;}
        if(fault==8)o.native.sampled_at+=91;
        c.observe(o);
        check(c.eligible(o.native,o.native.sampled_at)==(fault==0),
            "only fully admitted same-episode idle201 mode2 can resume; changed actor/contact/state cancels");
    }
    VertAirController c;auto o=ground();warm(c,o);c.observe(o);
    NativeInputCorrelation diagnostic{o.native,NativeInputCorrelation::contacts_changed};
    NativeRequestSample live{};live.valid=true;live.state=100;live.crouch_mode=1;
    check(c.publication_gap(diagnostic,live,o.native.owner,1001),"zero-contact riding can withdraw authority briefly");
    c.unavailable(1001,o.native.owner,true,false);o.support={};o.supported=false;
    for(unsigned i=0;i<8;++i){++o.native.physics_step;o.native.sampled_at+=17;c.observe(o);}
    o=fly(o);c.observe(o);
    check(!c.eligible(o.native,o.native.sampled_at),"unsupported ground after a publication gap cannot mint a later vert launch");
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
void recorded_curved_landing_speed() {
    // 5f9 original supported callback6672. Owner/up/timing below are fixtures;
    // velocity and curved support normal are the captured physical values.
    for(float yaw:{0.f,.8f,2.4f})for(unsigned scenario=0;scenario<9;++scenario) {
        const auto turn=[&](Vec v){return Vec{v[0]*std::cos(yaw)-v[2]*std::sin(yaw),v[1],v[0]*std::sin(yaw)+v[2]*std::cos(yaw)};};
        VertLandingWindow landing;auto o=fly(ground());landing.begin(o);
        for(unsigned i=0;i<3;++i) {
            ++o.native.physics_step;o.native.sampled_at+=60;o.velocity={0,-9,0};landing.observe(o);
        }
        ++o.native.physics_step;o.native.sampled_at+=17;o.native.airborne=false;o.native.state=100;
        ++o.native.air_departure;o.native.contacts=0xc000;o.supported=true;o.contacted_shape={7,8,9};
        o.contact_normal={0,.24298f,.9700313f};o.velocity={-.059908796f,-9.087554f,.9837277f};
        if(scenario==1)o.velocity=scale(o.contact_normal,-9);
        if(scenario==2){o.contact_normal={0,1,0};o.velocity={0,-9,0};}
        if(scenario==3){o.contact_normal={0,1,0};o.velocity={1.1f,0,0};}
        if(scenario==4){o.contact_normal={0,1,0};o.velocity={.9f,-9,0};}
        if(scenario==5)o.contact_normal={0,0,1};
        o.up=turn(o.contact_normal);
        if(scenario==6)o.contact_normal=scale(o.contact_normal,2);
        if(scenario==7)o.contact_normal={};
        if(scenario==8)o.contact_normal[1]=std::numeric_limits<float>::quiet_NaN();
        o.velocity=turn(o.velocity);o.contact_normal=turn(o.contact_normal);landing.observe(o);
        check(landing.ready(o.native,o.native.sampled_at)==(scenario==0 || scenario==3 || scenario==6),
            "real curved landing uses ramp-tangential speed; normal impact, slow flat contact and walls do not qualify");
    }
}
void native_level_out_handoff() {
    // 5f9 original callback17020 ->17021, measured on the same native owner.
    // Curvature/owner tokens and the later interpolation are fixture inputs;
    // the two physical packets are recorded, not a simulated native success.
    for(unsigned fault=0;fault<12;++fault) {
        VertAirController c;auto o=ground();o.native.sampled_at=2320;o.native.physics_step=17018;
        o.native.physics_seconds=1.f/60;o.native.selection_object=99;
        warm(c,o);c.observe(o);o=fly(o);o.native.state=201;o.native.mode=2;c.observe(o);
        ++o.native.physics_step;o.native.sampled_at=2350;
        o.position={146.26212f,6.6123023f,-99.4684f};o.velocity={.08514405f,2.5308897f,-.19912721f};c.observe(o);
        check(c.request(o.native,2350,2350,1),"recorded Level Out edge owns one request");
        auto plan=vert_level_out(o,{0,0,-1});plan.clear=true;plan.velocity={.046924613f,1.4732242f,-1.6020749f};
        check(c.stage(plan,o,2350),"recorded cleared packet stages");
        plan=c.consume(o.native,2350,o.velocity);check(bool(plan),"recorded packet releases once");
        auto receipt=o.native;auto acknowledged=plan;auto now=2350ull;
        if(fault==1)receipt.owner.core++;
        if(fault==2)receipt.owner_generation++;
        if(fault==3)receipt.airborne_episode++;
        if(fault==4)receipt.physics_step++;
        if(fault==5)receipt.selection_object++;
        if(fault==6)receipt.flip_id=3;
        if(fault==7)receipt.state=200;
        if(fault==8)acknowledged.velocity[0]+=.1f;
        if(fault==9)acknowledged.action=VertAction::spine;
        if(fault==10)now+=51;
        // A missing/failed native submit never calls this boundary.
        const bool handed=fault!=11 && c.submitted_native_level_out(receipt,now,acknowledged);
        check(handed==(fault==0),"only the exact completed AIR Level Out can relinquish motion ownership");
        if(handed) {
            check(!c.submitted_native_level_out(receipt,now,plan),"native handoff receipt cannot replay");
            check(length(c.alignment(receipt,now))==0,"handoff immediately withdraws our alignment writer");
            c.unavailable(now+1,receipt.owner,true,false);
            check(c.preserves_native() && !c.consume(receipt,now+1,o.velocity),
                "busy publication retains native-owned history without replaying a write");
        }
        ++o.native.physics_step;o.native.sampled_at=2367;
        o.position={146.2627f,6.63415f,-99.49571f};o.velocity={.24536134f,2.7779105f,-.751648f};c.observe(o);
        check((c.phase()==VertPhase::native_owned)==handed,"recorded native articulation does not erase qualified landing lineage after handoff");
        if(!handed) {check(c.phase()==VertPhase::cancelled,"unacknowledged release retains its strict unexpected-motion rejection");continue;}
        check(c.landing().flight(o.native) && c.preserves_native(),"handoff tracks genuine flight without adding motion");
        check(!c.request(o.native,2367,2367,2) && !c.consume(o.native,2367,o.velocity) &&
            length(c.alignment(o.native,2367))==0,"native-owned flight cannot request, repeat or align another transfer");
        const auto continuing=c;const auto flight=o;
        for(unsigned cancellation=0;cancellation<5;++cancellation) {
            auto copy=continuing;auto changed=flight;++changed.native.physics_step;changed.native.sampled_at+=17;
            if(cancellation==0)changed.enabled=false;
            if(cancellation==1)changed.contested=true;
            if(cancellation==2)changed.native.owner_generation++;
            if(cancellation==3)changed.native.airborne_episode++;
            if(cancellation==4){changed.native.state=500;changed.native.airborne=false;}
            copy.observe(changed);check(!copy.landing().flight(changed.native) && !copy.preserves_native(),
                "native handoff retains owner, episode, bail, contest and OFF cancellation");
        }
        // Synthetic intervening steps test the observer, not landing physics.
        while(o.native.sampled_at<3335) {
            o.position=position(o.position,o.velocity,9.81f,o.native.physics_seconds);o.velocity[1]-=9.81f*o.native.physics_seconds;
            ++o.native.physics_step;o.native.sampled_at+=17;c.observe(o);
        }
        auto press=TriggerPress{};press.press.token={InputSource::triggers,0,InputEdge::press};
        press.press.at=o.native.sampled_at;press.press.sequence=19;press.context=InputContext::air;
        press.native=InputNativeContext::from_frame(o.native);
        auto approach=o.position;approach[1]-=.05f;
        check(c.landing().approach(o.native,patch(approach,{0,1,0})),"handoff retains independent near-touchdown reservation");
        check(qualify_vert_trigger(c,press,press.native,press.press.at).owner==TriggerOwner::landing_revert,
            "new physical edge near native-owned landing belongs to Revert");
        press.owner=TriggerOwner::transfer;
        check(qualify_vert_trigger(c,press,press.native,press.press.at).owner==TriggerOwner::none,
            "transfer-consumed edge cannot become landing Revert after handoff");
        auto contact_gap=c;
        NativeInputCorrelation diagnostic{o.native,NativeInputCorrelation::mode_changed|
            NativeInputCorrelation::contacts_changed|NativeInputCorrelation::state_changed};
        NativeRequestSample live{};live.valid=true;live.state=100;live.crouch_mode=1;live.words[1]=0xc000;
        check(contact_gap.publication_gap(diagnostic,live,o.native.owner,o.native.sampled_at+1),
            "native-owned flight retains the measured direct touchdown publication boundary");
        contact_gap.unavailable(o.native.sampled_at+1,o.native.owner,true,false);
        ++o.native.physics_step;o.native.sampled_at+=17;o.native.state=100;o.native.mode=1;o.native.airborne=false;
        ++o.native.air_departure;o.native.contacts=0xc000;o.supported=true;o.contact_normal={0,1,0};
        o.contacted_shape={7,8,9};o.up={.00053092f,.99999976f,.00036382f};
        o.position={146.5140076f,3.2789941f,-100.792885f};o.velocity={.26463276f,-.00005719f,-1.8089862f};
        auto unsupported=o;unsupported.supported=false;auto copy=c;copy.observe(unsupported);
        check(copy.phase()!=VertPhase::landed && !copy.landing().ready(unsupported.native,unsupported.native.sampled_at),
            "submission and expected position cannot synthesize native landing");
        contact_gap.observe(o);
        check(contact_gap.landing().ready(o.native,o.native.sampled_at) && length(contact_gap.alignment(o.native,o.native.sampled_at))==0,
            "original contact after publication gap proves Revert without reviving alignment");
        c.observe(o);check(c.phase()==VertPhase::landed && c.landing().ready(o.native,o.native.sampled_at),
            "fresh supported native contact establishes landing and independent Revert window");
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
int main(){confirmed_takeoff_publication();recorded_curved_landing_speed();native_level_out_handoff();recorded_contact_publications();publication_rejection_evidence();direct_landing_contact_boundary();recorded_push_onset_takeoff();landing_contact_boundary();push_clear_boundary();idle_air_mode_boundary();publication_contact_boundary();retained_rear_wheel();original_contact_support();trigger_qualification();landing_windows();low_speed_level_out();lifecycle();contact_departure_delay();unavailable_snapshots();released_snapshot_gaps();occupied_actor_expiry();destinations();staged_races_and_orientation();destination_contact_landing();unchanged_motion();session_trigger_ownership();transfer_preserves_grab_producer();vert_trigger_manual_entry();held_transfer_intent();input_edges();std::cout<<checks<<" vert checks passed\n";}

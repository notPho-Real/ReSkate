#include "Extension/ArcadeMode/arcade_wallplant.h"
#include "Extension/ArcadeMode/arcade_wallplant_visual.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* message) {
    ++checks;if(!value){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}
}
bool near(float a,float b){return std::abs(a-b)<.0001f;}
struct Fixture {
    WallplantPolicy policy;
    WallplantFrame frame{};
    WallplantContact contact{};
    WallplantPress press{};
    Fixture() {
        frame.now=1000;frame.velocity={-12,-2,0};frame.ground_clearance=2;
        frame.enabled=frame.focused=frame.controller_owned=frame.takeoff_observed=frame.clearance_known=true;
        frame.native.valid=true;frame.native.owner={1,2,3,4,5};frame.native.owner_generation=1;
        frame.native.contact_generation=2;frame.native.action_generation=3;frame.native.physics_step=20;
        frame.native.sampled_at=1000;frame.native.airborne=true;frame.native.airborne_episode=1;
        frame.native.airborne_started_known=true;frame.native.airborne_started_at=800;
        frame.geometry.owner={1,2,3,4,5};frame.geometry.client_context=6;frame.geometry.world=7;
        frame.geometry.world_generation=1;frame.geometry.self_entity=2;frame.geometry.self_generation=1;
        frame.geometry.phase=SurfaceGeometryPhase::client;
        contact.body={7,10,1};contact.point={0,2,0};contact.normal={1,0,0};contact.identity=1;
        contact.collision=contact.static_world=true;press.sequence=1;press.fresh_a=true;
        sync();
    }
    void sync() {
        auto& n=frame.geometry.native;n.required=n.valid=true;n.generations={frame.native.owner_generation,
            frame.native.contact_generation,frame.native.action_generation};
        n.physics_step=frame.native.physics_step;n.sampled_at=frame.native.sampled_at;
        n.airborne=frame.native.airborne;n.airborne_episode=frame.native.airborne_episode;n.air_departure=frame.native.air_departure;
        contact.native=frame.native;contact.geometry=frame.geometry;contact.sampled_at=frame.now;
        contact.approach_velocity=frame.velocity;press.native=n;press.pressed_at=frame.now;
    }
    WallplantCommand run(){return policy.step(frame,contact,press);}
    void advance(std::uint64_t amount) {
        frame.now+=amount;frame.native.sampled_at=frame.now;++frame.native.physics_step;sync();
    }
};
}
int main() {
    for(unsigned fault=0;fault<12;++fault) {
        Fixture f;f.frame.native.state=201;f.sync();
        f.frame.now=1030;f.contact.sampled_at=1028;f.press.pressed_at=1010;
        f.contact.approach_velocity={-12,-2,5};f.frame.velocity={.25f,-2,5};
        NativeRequestSample live{};live.valid=true;live.state=201;
        WallplantCollisionResponseProof proof{{f.frame.native,live,false},live,1,1,7,1028};
        if(fault==1)proof.raw=0;
        if(fault==2)++proof.input;
        if(fault==3)++proof.contact;
        if(fault==4)--proof.sampled_at;
        if(fault==5)proof.live.state=504;
        if(fault==6)++proof.execution.native.owner_generation;
        if(fault==7)++proof.execution.native.physics_step;
        if(fault==8)proof.live.crouch_mode=1;
        if(fault==9)proof.live.words[1]=0x8000;
        if(fault==10)f.frame.velocity[0]=7;
        if(fault==11)f.frame.now=1151;
        WallplantPolicyDecision decision{};
        const auto command=f.policy.step(f.frame,f.contact,f.press,nullptr,&decision,&proof);
        check(bool(command)==(fault==0),"only the exact fresh collision response can corroborate already outward solver velocity");
        if(!fault) {
            check(decision.collision_response&&!decision.reconstruction,
                  "collision response does not manufacture a native100 chooser result");
            check(near(command.after[0],6.7246154f)&&near(command.after[1],12.7f)&&near(command.after[2],2.8019231f)&&
                  near(command.before[0]+command.motion.delta_x,command.after[0]),
                  "preimpact reflection uses scalar damping and only the difference from actual solver velocity");
            check(!f.policy.step(f.frame,f.contact,f.press,nullptr,nullptr,&proof),
                  "same native collision and A cannot apply a second response");
        }
    }
    {
        const auto setup=[](Fixture& f) {
            f.frame.native.state=201;f.frame.native.mode=2;f.frame.native.contacts=0;f.sync();
            f.contact.normal={0,0,1};f.contact.approach_velocity={-5.1398554f,-1.3409811f,-9.839134f};
            f.frame.velocity={-5.6444554f,-1.2829007f,.13112003f};
            NativeRequestSample imported{};imported.valid=true;imported.state=201;
            imported.crouch_mode=1;imported.words[1]=0x8000;
            auto reconstructed=imported;reconstructed.state=100;
            return WallplantReconstructionProof{{7,f.press.sequence,10,201,100,true},
                {f.frame.native,imported,true},reconstructed,10,201,f.contact.identity};
        };
        Fixture f;const auto reconstruction=setup(f);WallplantPolicyDecision decision{};
        const auto command=f.policy.step(f.frame,f.contact,f.press,&reconstruction,&decision);
        check(bool(command)&&decision.reconstruction&&near(decision.current_normal,.13112003f),
            "recorded native reconstruction corroborates its small original solver response");
        check(near(command.after[2],4.7736523f)&&near(command.after[1],12.7f)&&near(command.after[0],-2.4937035f)&&
            command.after[2]>command.before[2],"measured preimpact vector reflects and loses scalar speed independently of solver response");
        check(!f.policy.step(f.frame,f.contact,f.press,&reconstruction),"same physical input cannot replay reconstructed response");
        ++f.press.sequence;
        auto repeated=reconstruction;repeated.ticket.input=f.press.sequence;
        check(!f.policy.step(f.frame,f.contact,f.press,&repeated),"fresh A cannot repeat the same unresolved contact");
        for(unsigned fault=0;fault<21;++fault) {
            Fixture invalid;auto proof=setup(invalid);
            switch(fault) {
            case 0:proof={};break;
            case 1:++proof.ticket.input;break;
            case 2:++proof.contact;break;
            case 3:++proof.execution.native.physics_step;break;
            case 4:proof.ticket.chosen=201;break;
            case 5:++proof.selector;break;
            case 6:proof.previous=100;break;
            case 7:proof.execution.imported_contact=false;break;
            case 8:proof.execution.live.state=100;break;
            case 9:proof.live.state=201;break;
            case 10:++proof.execution.native.owner.core;break;
            case 11:proof.ticket.selected=false;break;
            case 12:proof.ticket.before=200;break;
            case 13:++proof.execution.native.airborne_episode;break;
            case 14:++proof.execution.native.sampled_at;break;
            case 15:proof.live.crouch_mode=2;break;
            case 16:proof.live.words[1]=0xc000;break;
            case 17:proof.execution.native.valid=false;break;
            case 18:proof.contact=0;break;
            case 19:proof.ticket.selector=0;break;
            case 20:proof.ticket.invocation=0;break;
            }
            WallplantPolicyDecision rejected{};
            check(!invalid.policy.step(invalid.frame,invalid.contact,invalid.press,&proof,&rejected)&&
                !rejected.reconstruction,"malformed or unrelated reconstruction never admits outward motion");
        }
        for(float speed:{5.08f,6.0f}) {
            Fixture faster;const auto proof=setup(faster);faster.frame.velocity[2]=speed;
            WallplantPolicyDecision rejected{};
            check(!faster.policy.step(faster.frame,faster.contact,faster.press,&proof,&rejected)&&
                rejected.reconstruction&&std::string_view(rejected.reason)=="policy_exit_already_reached",
                "native outward speed at or above computed exit cannot receive a second kick");
        }
        Fixture ordinary;setup(ordinary);WallplantPolicyDecision rejected{};
        check(!ordinary.policy.step(ordinary.frame,ordinary.contact,ordinary.press,nullptr,&rejected)&&
            std::string_view(rejected.reason)=="policy_outward_unproven",
            "ordinary outward velocity remains rejected with its exact same-call reason");
        // The established independent second approach is still admitted after
        // separation, a different collision and a new physical press.
        f.frame.separated=true;f.advance(1001);f.frame.velocity={-5,-2,-10};f.sync();
        f.frame.separated=true;f.contact.identity=2;f.press.sequence=3;
        check(bool(f.run()),"legitimate separated new contact and press remain eligible");
    }
    {
        NativeInputState producer;const NativeInputOwner owner{1,2,3,4,5};
        NativeRequestSample sample{};sample.valid=true;sample.state=201;sample.crouch_mode=2;
        WallplantCommand completed{};completed.motion.ready=true;completed.input_sequence=9;
        completed.native=producer.observe(1000,owner,sample,{});completed.issued_at=1034;
        WallplantVisual visual;
        visual.completed(completed,false);
        check(!visual.pending(),"unsuccessful body write creates no visual handoff");
        visual.completed(completed,true);
        check(visual.pending() && !visual.take(completed.native,1034),"completion alone cannot fabricate resumed AIR");
        sample.state=100;sample.crouch_mode=1;sample.words[1]=0x8000;
        const auto ground=producer.observe(1046,owner,sample,{});
        visual.observe(ground,1046);
        check(visual.pending() && !visual.take(ground,1047),"first genuine native100 preserves only pending visual receipt");
        const auto second_ground=producer.observe(1062,owner,sample,{});
        visual.observe(second_ground,1062);
        check(visual.pending(),"same short contact100 phase may span original physics steps");
        sample.state=200;sample.words[1]=0;
        const auto resumed=producer.observe(1098,owner,sample,{});
        visual.observe(resumed,1098);
        const auto played=visual.take(resumed,1100);
        check(played.input_sequence==9 && wallplant_same_observation(played.native,completed.native),
            "observed100 then newAIR admits original immutable occurrence exactly once");
        check(!visual.take(resumed,1101) && !visual.pending(),"visual ticket cannot replay");
        for(unsigned fault=0;fault<15;++fault) {
            visual.completed(completed,true);visual.observe(ground,1046);
            auto changed=resumed;auto now=std::uint64_t{1098};
            switch(fault) {
            case 0:changed.valid=false;break;
            case 1:++changed.owner.core;break;
            case 2:++changed.owner_generation;break;
            case 3:changed.rail_family=true;break;
            case 4:changed.state=504;changed.airborne=false;break;
            case 5:changed.state=103;changed.airborne=false;break;
            case 6:changed.state=300;changed.airborne=false;break;
            case 7:++changed.airborne_episode;break;
            case 8:changed.airborne_episode=completed.native.airborne_episode;break;
            case 9:++changed.air_departure;break;
            case 10:changed.air_departure=completed.native.air_departure;break;
            case 11:now=1185;changed.sampled_at=now;break;
            case 12:changed.sampled_at=1100;break;
            case 13:changed.physics_step=ground.physics_step;break;
            case 14:changed.sampled_at=1045;break;
            }
            visual.observe(changed,now);
            check(!visual.pending() && !visual.take(resumed,1100),"owner/time/state/episode discontinuity drops visual without relaxing native admission");
        }
        visual.completed(completed,true);visual.observe(ground,1046);visual.observe(resumed,1098);
        auto landed=resumed;landed.state=100;landed.airborne=false;++landed.physics_step;
        ++landed.air_departure;landed.sampled_at=1110;
        visual.observe(landed,1110);
        check(!visual.pending(),"a second actual landing cancels resumed visual even inside original lease");
        for(const auto* reason:{"off","focus","owner","trick_interrupt"}) {
            visual.completed(completed,true);visual.observe(ground,1046);visual.cancel(reason);
            visual.observe(resumed,1098);
            check(!visual.pending() && !visual.take(resumed,1100),"hard cancellation cannot be revived by subsequent AIR");
        }
        visual.completed(completed,true);visual.observe(ground,1046);visual.observe(resumed,1098);
        check(!visual.take(resumed,1185) && !visual.pending(),"client delay does not renew original command lease");
        visual.completed(completed,true);visual.observe(ground,1046);visual.observe(resumed,1098);
        auto replaced=resumed;++replaced.physics_step;
        check(!visual.take(replaced,1100) && !visual.pending(),"client must request from the exact last original observation");
        visual.completed(completed,true);
        auto direct=completed.native;++direct.physics_step;direct.sampled_at=1046;
        visual.observe(direct,1046);
        check(bool(visual.take(direct,1047)),"ordinary same-flight visual completion remains supported");
        auto invalid=completed;invalid.native.state=100;
        visual.completed(invalid,true);
        check(!visual.pending(),"ground command cannot create a visual receipt");
    }
    {
        NativeInputState producer;NativeInputOwner owner{1,2,3,4,5};
        NativeRequestSample air{};air.valid=true;air.state=201;air.crouch_mode=2;
        const auto observed=producer.observe(1000,owner,air,{});
        auto imported=air;imported.crouch_mode=1;imported.words[1]=0x8000;
        const WallplantExecutionProof proof{observed,imported,true};
        WallplantSelectionTicket ticket{7,9,10,201,0,false};
        auto ground=imported;ground.state=100;
        check(!wallplant_reconstruction_current(ticket,proof,owner,10,201,ground,1035),
            "native previous AIR alone does not authorize a ground write");
        check(ticket.finish(7,10,201,100),"exact original chooser result finishes this invocation");
        check(wallplant_reconstruction_current(ticket,proof,owner,10,201,ground,1035),
            "recorded AIR wall contact can finish after original native100 reconstruction");
        const auto once=ticket.take();
        check(!ticket.take().invocation && once.invocation==7,"handoff is removed before callbacks and cannot replay");
        for(unsigned fault=0;fault<18;++fault) {
            auto t=once;auto p=proof;auto o=owner;auto live=ground;
            std::uintptr_t selector=10;std::uint32_t previous=201;std::uint64_t now=1035;
            switch(fault) {
            case 0:t.invocation=0;break;case 1:t.input=0;break;case 2:t.selected=false;break;
            case 3:selector=11;break;case 4:t.before=100;break;case 5:live.state=504;break;
            case 6:previous=100;break;case 7:p.imported_contact=false;break;
            case 8:p.native.valid=false;break;case 9:p.native.airborne=false;break;
            case 10:++o.core;break;case 11:live.valid=false;break;
            case 12:now=999;break;case 13:now=1151;break;case 14:live.crouch_mode=2;break;
            case 15:live.words[0]=0x20;break;case 16:live.words[1]=0xc000;break;
            case 17:p.live.valid=false;break;
            }
            check(!wallplant_reconstruction_current(t,p,o,selector,previous,live,now),
                "changed owner/state/callback/native contact or stale original proof cancels handoff");
        }
        for(unsigned fault=0;fault<5;++fault) {
            WallplantSelectionTicket t{7,9,10,201,0,false};
            check(!t.finish(fault==0?8:7,fault==1?11:10,fault==2?200:201,fault==3?504:fault==4?200:100),
                "wrong chooser invocation and unrelated state transitions cannot finish a ticket");
            check(!t.invocation,"rejected chooser result discards staged authority");
        }
        check(wallplant_same_observation(observed,observed),"original frame identity is retained");
        for(unsigned fault=0;fault<9;++fault) {
            auto changed=observed;
            switch(fault) {
            case 0:++changed.physics_step;break;case 1:++changed.sampled_at;break;
            case 2:++changed.owner_generation;break;case 3:++changed.contact_generation;break;
            case 4:++changed.action_generation;break;case 5:++changed.airborne_episode;break;
            case 6:++changed.air_departure;break;case 7:++changed.owner.rig;break;
            case 8:changed.airborne=false;break;
            }
            check(!wallplant_same_observation(observed,changed),"next observation or cancelled episode cannot revive handoff");
        }
        Fixture f;f.contact.normal={0,0,1};f.contact.approach_velocity={-5.3687f,-1.4435f,-8.9189f};
        f.frame.velocity={-5.81f,-2.3358f,-8.9f};
        const auto command=f.run();
        check(bool(command),"single command calculated from actual post-native reconstruction velocity");
        std::array<SurfaceGeometryVector,2> velocities{f.frame.velocity,SurfaceGeometryVector{-4.81f,-1.3358f,-7.9f}};
        std::array<SurfaceGeometryVector,2> spins{};
        check(WallplantPolicy::apply_bodies(command,velocities,spins),"one final transaction applies after reconstruction");
        check(near(velocities[0][1],12.7f)&&velocities[0][2]>0&&near(velocities[1][1]-velocities[0][1],1),
            "final kick upward and outward preserves articulated body differences");
    }
    for(unsigned contacts:{0x4000u,0x8000u}) {
        NativeInputState producer;NativeInputOwner owner{1,2,3,4,5};
        NativeRequestSample original{};original.valid=true;original.state=201;original.crouch_mode=2;
        const auto observed=producer.observe(1000,owner,original,{});
        auto live=original;live.crouch_mode=1;live.words[1]=contacts;
        NativeInputCorrelation correlation{};
        check(!producer.correlate(1035,owner,live,&correlation).valid,"ordinary correlation remains strict at imported wall-wheel contact");
        check(wallplant_imported_contact_phase(correlation,live),"only measured imported contact phase admits either native stance-swapped axle");
        const WallplantExecutionProof proof{correlation.observed,live,true};
        check(proof.native.mode==2&&proof.native.contacts==0&&proof.native.sampled_at==1000&&
            proof.native.physics_step==observed.physics_step,"execution proof keeps original physics observation unchanged");
        check(wallplant_execution_current(proof,owner,live,1035),"same live imported sample revalidates before the body transaction");
        auto invalid=proof;invalid.native.state=100;
        check(!wallplant_execution_current(invalid,owner,live,1035),"execution proof cannot pair different original and live selected states");
        auto changed=live;changed.crouch_mode=2;
        check(!wallplant_execution_current(proof,owner,changed,1035),"later mode change cannot use imported execution proof");
        changed=live;changed.words[1]^=0xc000;
        check(!wallplant_execution_current(proof,owner,changed,1035),"later wheel identity change cancels execution");
        check(!wallplant_execution_current(proof,owner,live,1151)&&!wallplant_execution_current(proof,owner,live,999),
            "imported phase never extends or backdates the original frame lease");
        auto replaced=owner;++replaced.context;
        check(!wallplant_execution_current(proof,replaced,live,1035),"execution retains exact original local owner");
        for(unsigned fault=0;fault<14;++fault) {
            auto input=live,base=original;auto reader=owner;std::uint64_t now=1035;
            NativeInputState state;
            switch(fault) {
            case 0:input.state=100;break;
            case 1:input.state=200;break;
            case 2:input.state=504;break;
            case 3:input.crouch_mode=0;break;
            case 4:input.crouch_mode=2;break;
            case 5:input.words[1]=0xc000;break;
            case 6:input.words[1]=0;break;
            case 7:input.words[0]=0x20;break;
            case 8:input.valid=false;break;
            case 9:++reader.rig;break;
            case 10:now=1151;break;
            case 11:now=999;break;
            case 12:base.state=200;input.state=200;break;
            case 13:base.crouch_mode=0;break;
            }
            state.observe(1000,owner,base,{});NativeInputCorrelation rejected{};
            (void)state.correlate(now,reader,input,&rejected);
            check(!wallplant_imported_contact_phase(rejected,input),"other native transitions and additional mismatches cannot borrow wallplant execution");
        }
    }
    {
        Fixture f;f.frame.native.state=201;f.sync();
        auto owner=f.frame.native.owner;
        const auto admitted=[&](std::uint32_t current=201,std::uint32_t selected=201,
                std::uintptr_t selector=6,std::uintptr_t owned_selector=6,std::uintptr_t context=4) {
            return wallplant_selection_current(f.frame.native,owner,selector,owned_selector,context,
                current,selected,f.frame.now);
        };
        check(admitted(),"observed AIR owner admitted before native chooser commits its next state");
        check(!admitted(100,100) && !admitted(504,504),"chooser handoff cannot re-air ground or offboard state");
        check(!admitted(201,100) && !admitted(200,201),"current argument and committed native state must agree");
        check(!admitted(201,201,6,7) && !admitted(201,201,0,0),"foreign or absent selector cannot consume local work");
        check(!admitted(201,201,6,6,8),"selector must retain the verified local context");
        ++owner.core;check(!admitted(),"replacement owner cannot borrow admitted flight");owner=f.frame.native.owner;
        f.frame.now=1151;check(!admitted(),"preselector seam does not extend original frame lease");
        f.frame.now=999;check(!admitted(),"future original observation cannot grant handoff");f.frame.now=1000;
        f.frame.native.valid=false;check(!admitted(),"invalid native sample blocks handoff");f.frame.native.valid=true;
        f.frame.native.airborne=false;check(!admitted(),"AIR state number alone cannot establish airborne eligibility");
        f.frame.native.airborne=true;
        const auto command=f.run();check(bool(command),"qualified preselector command consumes original physical press");
        check(!f.run(),"later ordinary writer cannot replay the same wallplant input");
    }
    {
        const auto parameters=WallplantParameters::reference_prototype();
        const float launch=GameplayProfile{}.launch_speed(200,LaunchDescriptor::ordinary());
        const float apex=launch*launch/(2*AirMotion::ordinary_gravity);
        const SurfaceGeometryVector root{10,apex,20};
        const auto query=wallplant_clearance_request(root,parameters);
        for(const auto& segment:query.segments) {
            check(segment.end[1]<=0 && segment.start[1]-segment.end[1]<=20,
                  "wallplant-only clearance ray covers existing launch apex within native query safety bound");
            check(segment.start[0]==segment.end[0] && segment.start[2]==segment.end[2],
                  "clearance footprint remains downward and does not query a competing support direction");
        }
    }
    for(unsigned rate:{30u,60u,120u})for(float side:{-1.0f,1.0f})for(float tangent:{0.0f,7.0f}) {
        Fixture f;f.frame.native.physics_seconds=1.0f/static_cast<float>(rate);
        f.contact.normal={side,0,0};f.frame.velocity={-12*side,-2,tangent};f.sync();
        const auto command=f.run();check(bool(command),"head-on and angled collision admit in both directions and cadences");
        check(near(command.after[0]*side,tangent==0?6.285f:7.0635036f) &&
            near(command.after[2],tangent==0?0.f:4.1203771f),"horizontal reflection applies scalar speed loss to both normal and tangent");
        check(near(command.after[1],500*GameplayProfile::metres_per_inch),"source-backed vertical exit target");
        check(f.policy.current(command,f.frame,f.contact),"command retains exact current native transaction identity");
        std::array<SurfaceGeometryVector,2> velocities{f.frame.velocity,SurfaceGeometryVector{f.frame.velocity[0]+.2f,-1,tangent+.3f}};
        std::array<SurfaceGeometryVector,2> spins{SurfaceGeometryVector{1,2,3},SurfaceGeometryVector{2,3,4}};
        const auto old_spins=spins;check(WallplantPolicy::apply_bodies(command,velocities,spins),"finite articulated batch applies");
        check(near(velocities[1][0]-velocities[0][0],.2f) && near(velocities[1][1]-velocities[0][1],1),"relative body velocity survives kick");
        check(spins==old_spins,"rebound does not steal native spin");
        check(!WallplantPolicy::apply_bodies(command,velocities,spins),"already-applied root cannot repeat same delta");
        check(!f.run(),"same input cannot produce another occurrence");
        ++f.press.sequence;check(!f.run(),"fresh input on unresolved contact cannot retrigger");
    }
    for(unsigned fault=0;fault<41;++fault) {
        Fixture f;
        switch(fault) {
        case 0:f.frame.enabled=false;break;
        case 1:f.frame.focused=false;break;
        case 2:f.frame.controller_owned=false;break;
        case 3:f.frame.competing_owner=true;break;
        case 4:f.frame.takeoff_observed=false;break;
        case 5:f.frame.clearance_known=false;break;
        case 6:f.frame.ground_clearance=.1f;break;
        case 7:f.frame.native.valid=false;break;
        case 8:f.frame.native.airborne=false;break;
        case 9:f.frame.native.rail_family=true;break;
        case 10:f.frame.native.airborne_started_known=false;break;
        case 11:f.frame.now+=151;break;
        case 12:f.press.fresh_a=false;break;
        case 13:f.press.sequence=0;break;
        case 14:f.press.pressed_at=1001;break;
        case 15:f.press.pressed_at=799;break;
        case 16:++f.press.native.generations.owner;break;
        case 17:++f.press.native.airborne_episode;break;
        case 18:f.contact.collision=false;break;
        case 19:f.contact.static_world=false;break;
        case 20:f.contact.rail=true;break;
        case 21:f.contact.skater=true;break;
        case 22:f.contact.normal={0,1,0};break;
        case 23:f.contact.normal={0,-1,0};break;
        case 24:f.contact.normal={.8f,.6f,0};break;
        case 25:f.contact.normal={0,0,0};break;
        case 26:f.contact.normal={2,0,0};break;
        case 27:f.contact.point[0]=std::numeric_limits<float>::quiet_NaN();break;
        case 28:f.contact.sampled_at=849;break;
        case 29:++f.contact.geometry.world_generation;break;
        case 30:++f.contact.native.owner_generation;break;
        case 31:f.contact.body.world=99;break;
        case 32:f.contact.approach_velocity={12,0,0};break;
        case 33:f.frame.velocity={1,-2,0};break;
        case 34:f.contact.approach_velocity={-1,0,20};break;
        case 35:f.contact.native.sampled_at=849;break;
        case 36:++f.frame.geometry.owner[0];break;
        case 37:++f.frame.geometry.native.generations.action;break;
        case 38:f.contact.geometry.self_generation=0;break;
        case 39:f.contact.body.index=UINT32_MAX;break;
        case 40:f.contact.approach_velocity={0,0,3};break;
        }
        check(!f.run(),"missing ownership, evidence, input or inappropriate geometry fail closed");
        f.frame=Fixture{}.frame;f.contact=Fixture{}.contact;f.press=Fixture{}.press;
        if(fault!=13)check(!f.run(),"rejected physical edge cannot be promoted by later readiness");
    }
    {
        Fixture f;f.frame.velocity={0,-2,0};
        check(bool(f.run()),"fresh genuine incoming collision still works after native response stops inward motion");
        f.policy.cancel();check(!f.run(),"cancellation cannot replay an old physical edge");
        f.policy.reset();check(bool(f.run()),"explicit journal/session reset restarts its sequence namespace");
        f.policy.reset();f.contact.body.generation=0;
        check(bool(f.run()),"allocated native shape generation zero is a valid identity");
    }
    {
        Fixture f;const auto first=f.run();check(bool(first),"first wall executes");
        f.advance(1100);++f.contact.identity;++f.press.sequence;
        check(!f.run(),"time and changed contact identity alone cannot prove separation");
        f.frame.separated=true;f.press.fresh_a=false;f.run();f.frame.separated=false;
        f.advance(17);++f.contact.identity;++f.press.sequence;f.press.fresh_a=true;
        f.contact.body.index=11;f.contact.normal={-1,0,0};f.frame.velocity={12,-1,3};f.sync();
        check(bool(f.run()),"different wall with separation, new approach and fresh edge executes");
    }
    {
        Fixture f;f.run();f.advance(17);f.frame.separated=true;f.press.fresh_a=false;f.run();
        f.frame.separated=false;f.advance(17);++f.press.sequence;++f.contact.identity;f.press.fresh_a=true;
        check(!f.run(),"concave corner cannot bypass source-backed repeat bound");
    }
    {
        Fixture f;auto command=f.run();f.frame.velocity[0]+=1;
        check(!f.policy.current(command,f.frame,f.contact),"native velocity takeover invalidates cached command");
        f.frame.velocity=command.before;++f.contact.body.generation;
        check(!f.policy.current(command,f.frame,f.contact),"body generation changed before commit invalidates command");
        std::array<SurfaceGeometryVector,2> velocities{command.before,SurfaceGeometryVector{0,0,std::numeric_limits<float>::infinity()}};
        std::array<SurfaceGeometryVector,2> spins{};const auto old=velocities;
        check(!WallplantPolicy::apply_bodies(command,velocities,spins) && velocities==old,"one invalid body rejects batch without partial writes");
    }
    {
        Fixture f;f.frame.velocity={-900,-2,10};f.sync();auto command=f.run();
        check(bool(command) && near(std::hypot(command.after[0],command.after[2]),894.34055f),
              "large inward speed loses the same scalar amount without an unrelated riding cap");
        check(near(command.after[2],9.936504f),"scalar loss scales tangent with the reflected whole vector");
        WallplantParameters invalid=WallplantParameters::reference_prototype();invalid.vertical_exit_speed=std::numeric_limits<float>::quiet_NaN();
        f.policy=WallplantPolicy(invalid);check(!f.run(),"invalid tuning never emits velocity");
    }
    {
        // This 5-12-13 triangle has a known horizontal reflection and scalar
        // exit speed7.285. A solver can alter current velocity, not that target.
        SurfaceGeometryVector expected{};
        for(const auto current:std::array{SurfaceGeometryVector{-12,-2,5},SurfaceGeometryVector{0,8,-40},
                SurfaceGeometryVector{-.1f,-500,20}}) {
            Fixture f;f.contact.approach_velocity={-12,-123,5};f.frame.velocity=current;
            const auto command=f.run();
            check(bool(command) && near(command.after[0],6.7246154f) && near(command.after[2],2.8019231f),
                "same incoming vector gives same full reflection after different native solver reconstructions");
            check(command.before==current && command.incoming==f.contact.approach_velocity &&
                command.reflected==SurfaceGeometryVector{12,0,5},"receipt keeps distinct preimpact, current and reflected vectors");
            check(near(current[0]+command.motion.delta_x,command.after[0]) &&
                near(current[2]+command.motion.delta_z,command.after[2]),"single body transaction applies difference from current solver velocity");
            expected=command.after;
        }
        for(float vertical:{-999.f,0.f,999.f}) {
            Fixture f;f.contact.approach_velocity={-12,vertical,5};
            const auto command=f.run();
            check(bool(command) && command.after==expected && near(command.after[1],12.7f),
                "incoming vertical speed does not change horizontal reflection or fixed upward release");
        }
        Fixture rotated;rotated.frame.velocity={-10,-2,0};rotated.sync();rotated.contact.normal={.6f,0,.8f};
        const auto command=rotated.run();
        check(bool(command) && near(command.reflected[0],-2.8f) && near(command.reflected[2],9.6f),
            "rotated wall reflects about actual normal rather than a world axis");
        check(near(command.after[0],-1.4224f) && near(command.after[2],4.8768f) &&
            near(std::hypot(command.after[0],command.after[2]),5.08f),"minimum exit applies to reflected speed, not normal component alone");
        Fixture tilted;tilted.contact.normal={.9987492f,.05f,0};const auto tilted_command=tilted.run();
        check(bool(tilted_command) && tilted_command.contact_normal==tilted.contact.normal &&
            near(tilted_command.normal[1],0),"actual collision plane retained separately from horizontal reflection normal");
        for(float size:{2.e-6f,3.e-6f}) {
            Fixture f;f.frame.velocity={-.6f*size,900,.8f*size};f.sync();
            const auto tiny=f.run();
            check(bool(tiny) && near(tiny.after[0],3.048f) &&
                near(tiny.after[2],size<.00000254f?-4.064f:4.064f),
                "below source zero threshold fallback reverses incoming direction; above it uses reflected direction");
            check(near(tiny.after[1],12.7f),"tiny horizontal fallback cannot inherit incoming vertical speed");
        }
        for(const auto profile:std::array{GameplayProfile{GameplayStats::standard},GameplayProfile{}}) {
            Fixture f;f.policy=WallplantPolicy(WallplantParameters::reference_prototype(profile));
            f.frame.velocity={-100,999,0};f.sync();const auto uncapped=f.run();
            check(bool(uncapped) && near(uncapped.after[0],94.285f) && near(uncapped.after[1],12.7f),
                "ordinary profile speed and Ollie caps do not change collision release");
        }
        for(float invalid:{-1000.f,1000.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
            Fixture frame;frame.frame.velocity[1]=invalid;
            check(!frame.run(),"current body components retain strict finite abs-below1000 guard");
            Fixture incoming;incoming.contact.approach_velocity[1]=invalid;
            check(!incoming.run(),"incoming components retain strict finite abs-below1000 guard");
        }
        Fixture too_large;too_large.frame.velocity={-999,0,-999};too_large.sync();
        too_large.contact.normal={.9238795f,0,.3826834f};
        check(!too_large.run(),"valid incoming components cannot emit a reflected component outside actual body safety bound");
    }
    std::cout<<checks<<" wallplant policy checks passed; synthetic observations are not hardware acceptance\n";
}

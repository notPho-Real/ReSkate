#include "pose_fixture.h"
#include "Extension/Skater/display_turn.h"
#include "surface_tests.h"
#include "Extension/ArcadeMode/arcade_custom_flatland_clips.h"
#include "Extension/ArcadeMode/arcade_custom_flatland_playback.h"
#include "Extension/ArcadeMode/arcade_input_session.h"
#include "Extension/ArcadeMode/arcade_manual_episode.h"
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool ok,const char* why){++checks;if(!ok){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
bool near(float a,float b,float tolerance=.0001f){return std::abs(a-b)<tolerance;}
// A simulated native player endpoint. This does not establish hook/render acceptance.
struct Player {
    std::uint64_t serial{},live{};unsigned starts{},cancels{};bool occupied{};
    CustomTrickPlayback adapter(){return {this,
        [](void* p,std::shared_ptr<const dingosdk::anim_studio::Clip>)->std::uint64_t {
            auto& self=*static_cast<Player*>(p);if(self.occupied || self.live)return 0;
            ++self.starts;return self.live=++self.serial;
        },
        [](void* p,std::uint64_t ticket){return static_cast<Player*>(p)->live==ticket;},
        [](void* p,std::uint64_t ticket){auto& self=*static_cast<Player*>(p);
            if(self.live==ticket){self.live=0;++self.cancels;}}};}
};
CustomTrickClips clips(){
    auto seed=flatland_pose_fixture();
    seed[1].position={3,0,4};
    return make_casper_clips(seed,{});
}
struct Rig {
    NativeInputState observer;NativeInputOwner owner{1,2,3,4,5};NativeInputFrame frame{};
    Player player;CustomFlatland trick{clips(),player.adapter()};
    ManualEpisode episode;ComboSession combo;InputSession session;
    float dt{1.f/60};std::uint64_t now{1000};
    CustomFlatlandContext context() const {return {frame,now,episode.serial(),true,true,true,false,false};}
    void next(std::uint64_t at,unsigned state=100,unsigned contacts=0x4000u){
        now=at;NativeRequestSample sample{};sample.valid=true;sample.state=state;sample.crouch_mode=1;sample.words[1]=contacts;
        frame=observer.observe(now,owner,sample,{true,-1},false,dt);
    }
    InputAdmission admission(){
        InputAdmission a{};a.gameplay=a.manual_safe=a.manual_balance_admitted=a.branch_ready=true;
        a.may_start=true;a.pose=SkatePose::manual;a.tick=now;a.custom_flatland=true;
        a.native_context=InputNativeContext::from_frame(frame);a.balance_generations=a.native_context.generations;
        a.balance_controls=InputBalanceControls::vertical();a.balance_controls.generations=a.balance_generations;
        a.extensions=trick.adapter();a.custom_flatland_branch=trick.branch();a.branch_ready=trick.branch_ready();
        return a;
    }
    Rig(){
        next(1000);episode.admit(frame,{SkatePose::manual,true,true});
        check(episode.established(),"fixture independently admits a Manual episode");
        auto a=admission();session.poll(now,{},0,0,a);
        ManualInputDiagnostic command{};command.owns=command.safe=true;command.request=-1;command.request_at=900;
        command.recognized_at=900;command.recognition_id=7;
        session.resume_manual_episode(command,a.native_context);
        combo.step(SkatePose::manual,0,0,false,nullptr,false,true);
    }
    NativeGesture::Output poll(std::uint64_t at,std::uint16_t buttons=0,bool enabled=true){
        next(at);auto c=context();c.enabled=enabled;c.ollie=(buttons&0x1000u)!=0;trick.observe(c);
        auto a=admission();a.gameplay=enabled;
        auto output=session.poll(at,{buttons},0,0,a);
        if(!output.manual.owns){c.manual_admitted=false;trick.observe(c);}
        return output;
    }
    void enter(){
        check(poll(1010,0x4000).manual.owns,"first X preserves Manual before gesture completion");
        check(poll(1030).manual.owns,"releasing first X preserves Manual");
        check(poll(1100,0x8000).manual.owns,"X Y preserves actual production Manual actuator");
        check(trick.phase()==CustomFlatland::Phase::entry && player.starts==1,"shared recognizer starts authored entry once");
        check(poll(1120,0x8000).manual.owns,"held accepted Y cannot cancel next poll");
        poll(1140);
    }
    void hold(){
        for(unsigned i=0;i<22;++i){next(now+17);trick.step(context(),{0,0,5});}
        check(trick.phase()==CustomFlatland::Phase::hold,"native elapsed time advances entry to hold");
    }
};
void input_and_continuity(){
    Rig r;const auto episode=r.episode.serial();const auto initial=r.combo.model().state();r.enter();r.hold();
    auto at=r.now+10;r.poll(at,0x4000);r.poll(at+30);auto out=r.poll(at+100,0x4000);
    check(out.manual.owns && out.manual.request_at==900,"flip keeps original Manual delivery lineage");
    check(r.trick.phase()==CustomFlatland::Phase::flip_exit && r.player.starts==2,"X X invokes newly authored flip exit");
    for(unsigned i=0;i<40;++i){r.next(r.now+17);r.trick.step(r.context(),{0,0,5});}
    check(!r.trick.active() && !r.player.live,"bounded flip returns to native Manual and releases animation");
    check(r.poll(r.now+17).manual.owns,"Manual actuator survives full vertical slice");
    check(r.episode.serial()==episode && r.episode.established(),"presentation subtype cannot replace native episode");
    const auto after=r.combo.model().state();
    check(after.multiplier==initial.multiplier && after.combo_active==initial.combo_active &&
        after.balance_kind==initial.balance_kind,"no invented scoring initiation or bank");
    Rig back;back.enter();back.hold();at=back.now+10;back.poll(at,0x4000);back.poll(at+10);
    out=back.poll(at+60,0x8000);
    check(out.manual.owns && back.trick.phase()==CustomFlatland::Phase::direct_exit,"contextual X Y starts an authored return without ending the Manual episode");
    check(back.poll(at+80,0x8000).manual.owns,"held return Y remains consumed");
    for(unsigned i=0;i<20;++i){back.next(back.now+17);const auto drag=back.trick.step(back.context(),{0,0,5});
        check(!drag.admitted,"direct return releases custom drag while its pose recovers");}
    check(!back.trick.active() && !back.player.live,"authored direct exit releases animation ownership");
}
void sequence_boundaries(){
    for(unsigned age:{299u,300u,301u}){
        Rig r;r.poll(1010,0x4000);r.poll(1030);r.poll(1130);r.poll(1230);
        const auto out=r.poll(1010+age,0x8000);
        check(r.trick.active()==(age<300),"entry strict 300ms query-time expiry");
        check(out.manual.owns==(age<300),"expired Y immediately follows ordinary cancellation policy");
    }
    for(unsigned age:{199u,200u,201u}){
        Rig r;r.enter();r.hold();const auto at=r.now+10;r.poll(at,0x4000);r.poll(at+10);r.poll(at+100);r.poll(at+age,0x4000);
        check((r.trick.phase()==CustomFlatland::Phase::flip_exit)==(age<200),"flip strict 200ms expiry");
    }
    {Rig r;check(!r.poll(1010,0x8000).manual.owns && !r.trick.active(),"plain Y has no prefix delay");}
    {Rig r;r.poll(1010,0x8000);r.poll(1020);r.poll(1050,0x4000);check(!r.trick.active(),"wrong order cannot enter");}
    {Rig r;r.poll(1010,0xc000);check(!r.trick.active(),"simultaneous XY cannot masquerade as a prior prefix");}
    {Rig r;r.player.occupied=true;r.poll(1010,0x4000);r.poll(1020);const auto out=r.poll(1060,0x8000);
        check(!r.trick.active() && !out.manual.owns,"busy player refuses trick and Y reservation");}
    {Rig r;r.poll(1010,0x4000);r.poll(1020);r.poll(1050,0x1000);r.poll(1080,0x8000);
        check(!r.trick.active(),"A cannot be swallowed by flatland prefix");}
    {Rig r;r.enter();r.poll(1200,0,false);check(!r.trick.active() && !r.player.live,"Arcade OFF cancels presentation immediately");}
    {Rig r;r.poll(1010,0x4000);r.poll(1020);r.next(1080);r.trick.observe(r.context());auto a=r.admission();a.custom_flatland=false;
        auto out=r.session.poll(r.now,{0x8000},0,0,a);check(!out.manual.owns,"default OFF capability preserves existing Y behavior");}
    {Rig r;r.poll(1010,0x4000);r.poll(1400);r.poll(1420,0x8000);check(!r.trick.active(),"delayed polling cannot renew source window");}
}
float drag_trace(unsigned hz,FlatlandSurface kind,bool verified,unsigned duplicates,bool reverse=false){
    Rig r;r.dt=1.f/static_cast<float>(hz);r.enter();
    std::array<float,3> velocity{reverse?-3.f:3.f,0,reverse?-4.f:4.f};
    const auto start=r.now;
    for(unsigned i=1;i<=hz;++i){
        r.next(start+static_cast<std::uint64_t>(std::llround(1000.0*i/hz)));
        FlatlandSurfaceEvidence surface{kind,r.frame.owner,r.frame.owner_generation,r.frame.contact_generation,r.frame.physics_step,verified};
        for(unsigned j=0;j<duplicates;++j){
            const auto before=std::hypot(velocity[0],velocity[2]);const auto delta=r.trick.step(r.context(),velocity,surface);
            velocity[0]+=delta.x;velocity[2]+=delta.z;
            check(std::hypot(velocity[0],velocity[2])<=before+.00001f,"drag cannot accelerate");
            check((reverse?velocity[0]<=.00001f:velocity[0]>=-.00001f) && near(velocity[1],0),"drag cannot reverse or change vertical velocity");
        }
    }
    return std::hypot(velocity[0],velocity[2]);
}
void friction(){
    float previous_slick=-1;
    for(unsigned hz:{30u,60u,120u}){
        const auto ordinary=drag_trace(hz,FlatlandSurface::ordinary,true,1);
        const auto slick=drag_trace(hz,FlatlandSurface::slick,true,1);
        check(near(ordinary,0) && slick>3,"ordinary stops rapidly while verified slick carries speed");
        check(near(slick,drag_trace(hz,FlatlandSurface::slick,true,4)),"duplicate observations spend no additional impulses");
        check(near(ordinary,drag_trace(hz,FlatlandSurface::slick,false,1)),"unverified slick always defaults to ordinary drag");
        check(near(slick,drag_trace(hz,FlatlandSurface::slick,true,1,true)),"fakie/backward motion loses equal speed");
        if(previous_slick>=0)check(near(previous_slick,slick,.08f),"30/60/120Hz results agree within one baseline step and discretized native wind");
        previous_slick=slick;
    }
    Rig r;r.enter();r.next(1160);auto c=r.context();r.trick.step(c,{3,0,4});r.next(1180);c=r.context();
    const auto stopped=r.trick.step(c,{0,0,0});check(stopped.x==0 && stopped.z==0,"stationary drag never creates motion");
    FlatlandSurfaceEvidence s{FlatlandSurface::slick,r.frame.owner,r.frame.owner_generation,r.frame.contact_generation,r.frame.physics_step,true};
    check(flatland_friction(r.frame,s).rolling<flatland_friction(r.frame).rolling,"verified contact chooses lower drag");
    ++s.contact_generation;check(flatland_friction(r.frame,s).rolling==flatland_friction(r.frame).rolling,"stale material proof defaults");
    r.trick.cancel();const auto cancelled=r.trick.step(c,{3,0,4});check(!cancelled.admitted,"cancel removes custom friction immediately");
    GroundResistance resistance;const auto friction=flatland_friction(r.frame);resistance.step(1.f/60,5,false,false,true,friction);
    check(resistance.step(1.f/60,3,false,false,true,friction)==0,"large native collision loss is never refunded or charged twice");
}
void cancellation(){
    for(unsigned reason=0;reason<12;++reason){
        Rig r;r.enter();auto c=r.context();
        switch(reason){
            case 0:c.enabled=false;break;case 1:c.frame.state=300;break;case 2:c.ollie=true;break;
            case 3:++c.frame.owner.core;break;case 4:++c.manual_episode;break;case 5:c.frame.state=103;break;
            case 6:c.frame.rail_family=true;break;case 7:c.balance_active=false;break;
            case 8:c.competing_owner=true;break;case 9:c.frame.valid=false;break;
            case 10:c.now+=151;break;case 11:++c.frame.owner_generation;break;
        }
        r.trick.observe(c);check(!r.trick.active() && r.player.live==0,"lifetime boundary revokes animation and drag");
    }
    {Rig r;r.enter();r.player.live=++r.player.serial;r.trick.observe(r.context());
        check(!r.trick.active() && r.player.live!=0,"old cancellation cannot stop newer playback owner");}
    {Rig r;r.enter();r.next(1200);auto c=r.context();c.frame.physics_seconds=std::numeric_limits<float>::quiet_NaN();
        r.trick.step(c,{0,0,5});check(!r.trick.active(),"invalid physics clock fails closed");}
    {Rig r;r.enter();for(unsigned i=0;i<905;++i){r.next(r.now+17);r.trick.step(r.context(),{0,0,0});}
        check(!r.trick.active() && !r.player.live,"finite hold automatically cleans up");}
    {Rig r;r.enter();r.next(1160);r.trick.step(r.context(),{0,0,5});r.next(1180);
        const auto c=r.context();const auto spent=r.trick.step(c,{0,0,5});
        check(spent.z<0,"test spends one native step");r.trick.cancel();
        InputRequest enter{};enter.action=InputAction::balance_branch_one;enter.variant=2;enter.context=InputContext::balance;
        enter.generations=InputNativeContext::from_frame(r.frame).generations;
        r.trick.observe(c);const auto adapter=r.trick.adapter();adapter.request(adapter.state,enter);
        const auto repeated=r.trick.step(c,{0,0,5});check(repeated.z==0,"cancel/reentry cannot spend the same physics step twice");}
}
void balance_continuity(){
    Rig r;ManualPhysics balance,control;balance.begin(false);control.begin(false);
    ComboSession plain;plain.step(SkatePose::manual,0,0,false,nullptr,false,true);
    const auto serial=r.episode.serial();
    for(unsigned i=1;i<=90;++i){
        const std::uint16_t button=i==2 || i==35 || i==40?0x4000u:i==6?0x8000u:0;
        const auto out=r.poll(1000+17*i,button);check(out.manual.owns,"balanced slice retains native RS owner");
        r.episode.observe(r.frame,0);
        const auto before=control.snapshot();
        const float correction=std::clamp(-3*before.position-2*before.velocity,-1.f,1.f);
        const auto actual=balance.sample(r.dt,correction),expected=control.sample(r.dt,correction);
        r.combo.step(SkatePose::manual,r.dt,correction,false,&actual,false,true);
        plain.step(SkatePose::manual,r.dt,correction,false,&expected,false,true);
        auto c=r.context();c.balance_active=actual.active;r.trick.step(c,{0,0,3});
        check(actual.active && actual.position==expected.position && actual.velocity==expected.velocity &&
            actual.elapsed==expected.elapsed && actual.repeat_pressure==expected.repeat_pressure,
            "Manual balance dynamics, fatigue and RNG continue across visual transitions");
        const auto a=r.combo.balance_display_state(),b=plain.balance_display_state();
        check(a.balance_kind==b.balance_kind && a.balance==b.balance && a.manual_fatigue==b.manual_fatigue &&
            a.multiplier==b.multiplier && a.raw_points==b.raw_points && a.combo_age_s==b.combo_age_s,
            "HUD balance and combo progression match the ordinary Manual control");
    }
    check(r.player.starts==2 && !r.trick.active() && r.episode.serial()==serial,"balanced slice completes without a second episode");
    check(!r.combo.take_score_settlement(),"visual custom trick never invents native score settlement");
    r.next(r.now+17,300);r.episode.observe(r.frame,0,true);r.combo.step(SkatePose::bail,r.dt,0);
    check(r.episode.ended() && !r.combo.model().state().combo_active,"real observed bailout retains normal episode/combo termination");
}
void display_rotation(){
    using namespace dingosdk::anim_studio;
    std::array<Joint,skeleton_joints> pose{};
    pose[0].rotation=from_euler({15,33,8});pose[0].position={5,2,7};
    pose[placement_joint].position={1,.5f,2};
    pose[hips_joint].position={.2f,1,.3f};
    pose[343].position={.3f,-.6f,.2f};
    Joint board;board.position={.1f,-.2f,.3f};
    const auto original=pose;const auto original_board=board;
    check(dingosdk::skater_animation::display_turn(pose,&board,1.2f),"display transform accepts finite owned pose");
    for(unsigned i=0;i<pose.size();++i)if(i!=hips_joint)
        check(pose[i].position==original[i].position && pose[i].rotation==original[i].rotation,
            "display turn leaves native placement and all non-root local joints unchanged");
    check(dingosdk::skater_animation::display_turn(pose,&board,-1.2f),"inverse display transform accepted");
    for(unsigned i=0;i<3;++i){
        check(near(pose[hips_joint].position[i],original[hips_joint].position[i],.0001f),"turn is reversible under nontrivial placement");
        check(near(board.position[i],original_board.position[i],.0001f),"body and board share the same reversible turn");
    }
    check(!dingosdk::skater_animation::display_turn(pose,&board,std::numeric_limits<float>::infinity()),"invalid display angle rejected");
}

void animation(){
    using namespace dingosdk::anim_studio;const auto c=clips();
    std::array<Joint,skeleton_joints> pose{};Joint board;
    c.entry_hold->sample(.3f,pose);c.entry_hold->sample_board(.3f,board);
    check(near(pose[1].position[0],3) && near(pose[1].position[2],4),"native placement seed remains unchanged");
    check(std::abs(pose[342].rotation[0])>.1f,"new authored body pose changes knee joint");
    check(rotate(board.rotation,{0,1,0})[1]<-.5f,"held rendered board is inverted and nose elevated");
    std::array<Joint,skeleton_joints> flip_start{};
    Joint flip_board;
    c.flip_exit->sample(0,flip_start);c.flip_exit->sample_board(0,flip_board);
    for(std::size_t i=0;i<pose.size();++i){
        check(pose[i].position==flip_start[i].position && pose[i].scale==flip_start[i].scale,
            "owned phase replacement preserves every body position and scale");
        float dot=0;for(unsigned j=0;j<4;++j)dot+=pose[i].rotation[j]*flip_start[i].rotation[j];
        check(near(std::abs(dot),1),"owned phase replacement preserves every body rotation");
    }
    check(board.position==flip_board.position && board.scale==flip_board.scale,
        "owned phase replacement preserves board placement");
    float board_dot=0;for(unsigned j=0;j<4;++j)board_dot+=board.rotation[j]*flip_board.rotation[j];
    check(near(std::abs(board_dot),1),"owned phase replacement preserves the inverted board rotation");
    c.flip_exit->sample_board(.6f,board);check(rotate(board.rotation,{0,1,0})[1]>.99f,"flip returns board upright");
    c.flip_exit->sample(.6f,pose);check(near(pose[342].rotation[3],1),"flip body exits to Manual seed");
    float total=0;Quat previous{};bool first=true;
    for(unsigned i=0;i<=60;++i){c.flip_exit->sample_board(static_cast<float>(i)/100,board);
        check(finite(board),"authored clip samples finite board poses");
        if(!first){float dot=0;for(unsigned j=0;j<4;++j)dot+=previous[j]*board.rotation[j];
            total+=2*std::acos(std::clamp(std::abs(dot),0.f,1.f));}
        first=false;previous=board.rotation;
    }
    check(total>3*3.14159265f && total<5*3.14159265f,"shortest arc interpolation retains the combined spin and roll path");
    const auto roundtrip=clip_from_bytes(clip_bytes(*c.entry_hold));check(roundtrip.times==c.entry_hold->times,"authored clip uses existing portable serialization");
}
}
int main(){surface_tests();input_and_continuity();sequence_boundaries();friction();cancellation();balance_continuity();display_rotation();animation();
    std::cout<<checks<<" custom flatland checks passed (simulated native observations/player; no gameplay acceptance)\n";}

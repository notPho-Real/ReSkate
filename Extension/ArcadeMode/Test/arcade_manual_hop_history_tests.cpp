#include "Extension/ArcadeMode/arcade_manual_hop.h"
#include "Extension/ArcadeMode/arcade_native_events.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace { unsigned checks{};void check(bool value,const char* why){++checks;if(!value){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}} }
int main(){
    for(bool nose:{false,true})for(float dt:{1.0f/30,1.0f/60}){
        ManualPhysics physics;const auto pose=nose?SkatePose::nose_manual:SkatePose::manual;
        ManualPhysics expected;
        PhysicalBalanceSample prior{};for(unsigned i=0;i<4;++i){prior=manual_physics_pose(physics,pose,dt,{});expected.begin(nose);expected.sample(dt,PhysicalManualControl{});}
        ManualHopContinuity hop;NativeInputFrame frame{};frame.valid=true;frame.owner={1,2,3,4,5};frame.owner_generation=frame.contact_generation=frame.action_generation=frame.physics_step=1;frame.sampled_at=1000;frame.state=100;frame.mode=1;frame.contacts=nose?0x8000:0x4000;
        NativeGesture::Output setup{};setup.authored_playback=true;setup.requested_id=nose?16:1;setup.jump.nollie=nose;setup.jump.pressed_at=1000;setup.jump.setup_started=true;
        hop.command(1000,frame,frame,pose,true,setup,true,true);
        check(hop.active(),"only an admitted Manual/Nose setup seeds hop continuity");
        frame.state=103;
        const auto crouch=observed_pose({true,false,false,false,103,0,0xc000});
        check(crouch==SkatePose::riding,"native103 is a grounded hop preparation rather than airborne proof");
        const bool preserve=hop.preserve(frame,crouch,true);
        check(preserve,"native crouch preserves history without claiming takeoff");
        manual_physics_pose(physics,crouch,dt,{},preserve);
        frame.state=200;check(!hop.preserve(frame,SkatePose::air,true)&&!hop.active(),"actual air retires the setup token and uses the existing airborne stop path");
        manual_physics_pose(physics,SkatePose::air,dt,{});
        const auto resumed=manual_physics_pose(physics,pose,dt,{});
        check(resumed.elapsed>prior.elapsed,"admitted manual hop must retain physical repeat history through native crouch before takeoff");
        expected.stop();expected.begin(nose);const auto oracle=expected.sample(dt,PhysicalManualControl{});
        check(resumed.position==oracle.position&&resumed.velocity==oracle.velocity&&resumed.elapsed==oracle.elapsed&&resumed.repeat_pressure==oracle.repeat_pressure,
              "crouch handoff retains exactly the established airborne stop history without adding balance updates");
    }
    for(bool nose:{false,true}) {
        const auto pose=nose?SkatePose::nose_manual:SkatePose::manual;
        NativeInputFrame origin{};origin.valid=true;origin.owner={1,2,3,4,5};origin.owner_generation=origin.contact_generation=origin.action_generation=origin.physics_step=1;
        origin.state=100;origin.mode=1;origin.sampled_at=1000;origin.contacts=nose?0x8000:0x4000;
        NativeGesture::Output setup{};setup.authored_playback=true;setup.requested_id=nose?16:1;setup.jump.nollie=nose;setup.jump.pressed_at=1000;setup.jump.setup_started=true;
        for(unsigned fault=0;fault<17;++fault){
            ManualHopContinuity rejected;auto proof=origin;auto frame=origin;auto request=setup;bool balance=true,held=true,eligible=true;
            if(fault==0)balance=false;if(fault==1)held=false;if(fault==2)eligible=false;
            if(fault==3)request.authored_playback=false;if(fault==4)request.requested_id=nose?1:16;
            if(fault==5)proof.owner.rig=6;if(fault==6)++proof.contact_generation;if(fault==7)++proof.action_generation;
            if(fault==8)proof.sampled_at=1001;if(fault==9)proof.sampled_at=849;if(fault==10)frame.state=200;if(fault==11)frame.owner_generation=0;
            if(fault==12)request.jump.setup_started=false;if(fault==13)request.jump.nollie=!nose;
            if(fault==14)request.jump.from_rail=true;if(fault==15)request.jump.released=true;if(fault==16)request.jump.pressed_at=1001;
            rejected.command(1000,frame,proof,pose,balance,request,held,eligible);
            check(!rejected.active(),"unadmitted, unavailable, stale or mismatched setup cannot preserve balance history");
        }
        for(unsigned fault=0;fault<8;++fault){
            ManualHopContinuity hop;hop.command(1000,origin,origin,pose,true,setup,true,true);
            auto frame=origin;frame.state=103;frame.sampled_at=1017;++frame.physics_step;++frame.contact_generation;++frame.action_generation;
            NativeGesture::Output release{};release.authored_playback=true;release.jump={true,true,nose,false,1000,1017};
            if(fault==0)release.jump.setup_admitted=false;if(fault==1)release.jump.nollie=!nose;if(fault==2)release.jump.from_rail=true;
            if(fault==3)release.jump.pressed_at=1001;if(fault==4)release.jump.released_at=999;if(fault==5)release.jump.released_at=1018;
            hop.command(1017,frame,{},SkatePose::riding,false,release,false,true);
            if(fault<6)check(!hop.active(),"invalid release metadata cannot retain the original hop episode");
            else {
                check(hop.preserve(frame,SkatePose::riding,true),"valid release survives legitimate native contact/action generation changes during crouch");
                release.jump={};release.authored_playback=false;
                hop.command(1034,frame,{},SkatePose::riding,false,release,fault==6,true);
                check(hop.active()==(fault==7),"new held jump after release cannot reuse old balance history, while released native crouch may finish");
                frame.state=100;check(!hop.preserve(frame,SkatePose::riding,true)&&!hop.active(),"actual ordinary ground ends a finished or failed hop setup");
            }
        }
        for(unsigned fault=0;fault<8;++fault){
            ManualHopContinuity hop;hop.command(1000,origin,origin,pose,true,setup,true,true);auto frame=origin;frame.state=103;
            auto observed=SkatePose::riding;bool valid=true;
            if(fault==0)++frame.owner.entity;if(fault==1)++frame.owner_generation;if(fault==2)valid=false;if(fault==3)frame.valid=false;
            if(fault==4)observed=SkatePose::unknown;if(fault==5)observed=SkatePose::bail;if(fault==6)frame.state=401;if(fault==7)hop.reset();
            check(!hop.preserve(frame,observed,valid)&&!hop.active(),"owner, invalidity, bail, unrelated state and OFF reset passive hop continuity");
        }
        ManualPhysics ordinary;const auto prior=manual_physics_pose(ordinary,pose,1.0f/60,{});
        manual_physics_pose(ordinary,SkatePose::riding,1.0f/60,{});
        const auto fresh=manual_physics_pose(ordinary,pose,1.0f/60,{});
        check(fresh.elapsed==prior.elapsed,"ordinary landing without an admitted hop still clears repeat history");
    }
    for(bool nose:{false,true})for(unsigned dt:{17u,34u})for(unsigned admission_delay:{0u,17u,34u}){
        NativeInputState state;NativeRequestSample raw{};raw.valid=true;raw.state=100;raw.crouch_mode=1;raw.words[1]=nose?0x8000:0x4000;
        const NativeInputOwner owner{1,2,3,4,5};auto frame=state.observe(1000+admission_delay,owner,raw,{true,-1});
        const auto pose=nose?SkatePose::nose_manual:SkatePose::manual;
        NativeGesture::Output setup{};setup.authored_playback=true;setup.requested_id=nose?16:1;
        setup.jump.nollie=nose;setup.jump.pressed_at=1000;setup.jump.setup_started=true;
        ManualHopContinuity hop;hop.command(1000+admission_delay,frame,frame,pose,true,setup,true,true);
        check(hop.active(),"existing admitted setup starts a passive token with original physical press");
        for(unsigned i=1;i<=3;++i){
            const auto now=1000+admission_delay+i*dt;frame=state.observe(now,owner,raw,{true,-1});
            setup.jump={};hop.command(now,frame,frame,pose,true,setup,true,true);
            check(hop.active(),"repeated held delivery cannot renew the passive setup episode");
        }
        const auto now=1000+admission_delay+4*dt;raw.state=103;raw.crouch_mode=2;raw.words[1]=0xc000;
        frame=state.observe(now,owner,raw,{true,-1});
        NativeGesture::Output release{};release.authored_playback=true;release.jump={true,true,nose,false,1000,now};
        hop.command(now,frame,{},SkatePose::riding,false,release,false,true);
        check(hop.active()&&hop.preserve(frame,SkatePose::riding,true),"original physical release matches after repeated held polls and delayed admission");
        raw.state=200;frame=state.observe(now+dt,owner,raw,{true,-1});
        check(!hop.preserve(frame,SkatePose::air,true)&&!hop.active(),"actual takeoff retires passive setup without inventing flight");
    }
    std::cout<<checks<<" manual hop history checks passed\n";
}

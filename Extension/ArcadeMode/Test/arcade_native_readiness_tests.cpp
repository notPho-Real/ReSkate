#include "Extension/ArcadeMode/arcade_native_readiness.h"
#include "Extension/ArcadeMode/arcade_gesture.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why){++checks;if(!value){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
NativeRequestSample sample(unsigned state=100,unsigned stance=0,unsigned mode=1){NativeRequestSample s{};s.valid=true;s.state=state;s.crouch_mode=mode;s.words[0]=stance;s.words[1]=0xc000;return s;}
}
int main(){
    const NativeInputOwner owner{1,2,3,4,5};
    const NativeInputPermissions all{true,true,true,true,true,true,true,true,true,7};
    for(unsigned bit:{0u,0x20u}) {
        NativeInputState source;NativeInputReadiness readiness;
        const auto old=source.observe(100,owner,sample(100,bit),{true,-1});readiness.publish(old,all);
        const auto unchanged=source.observe(110,owner,sample(100,bit),{true,-1});
        const auto admitted=readiness.sample(110,unchanged);
        check(admitted.gameplay&&admitted.matched&&admitted.start&&admitted.push&&admitted.board&&admitted.drive&&
            admitted.manual&&admitted.prepare&&admitted.releasing&&admitted.action&&admitted.speed==7,"unchanged generation retains all permissions without requiring a new client tick");
        check(admitted.mirror_x==!bool(bit&0x20u),"stance comes from the same validated frame as current authority");
        const auto opposite=source.observe(120,owner,sample(100,bit^0x20u),{true,-1});
        const auto pending=readiness.sample(120,opposite);
        check(pending.gameplay&&!pending.matched&&!pending.start&&!pending.push&&!pending.board&&!pending.drive&&
            pending.manual&&!pending.prepare&&!pending.releasing&&pending.speed==0,"changed native stance cannot inherit old client-derived permissions");
        check(pending.mirror_x==bool(bit&0x20u),"opposite current stance cannot be paired with the old stance");
        readiness.publish(opposite,{true,false,false,false,false,false,false,false,true,0});
        check(!readiness.sample(121,opposite).start,"explicit new not-ready publication replaces old ready within its time lease");
        auto changed=owner;++changed.entity;
        const auto replacement=source.observe(125,changed,sample(100,bit),{true,-1});
        check(!readiness.sample(125,replacement).gameplay,"new owner authority cannot borrow old owner readiness or stance");
        readiness.publish(replacement,all);
        check(readiness.sample(275,replacement).gameplay&&!readiness.sample(276,replacement).gameplay,"original physical observation retains inclusive150ms lease");
        check(!readiness.sample(124,replacement).gameplay,"future-dated proof fails closed");
        readiness.reset();check(!readiness.sample(125,replacement).gameplay,"OFF invalidation removes the permission snapshot");
    }
    NativeInputState source;NativeInputReadiness readiness;
    auto air=source.observe(10,owner,sample(200),{true,-1,11});readiness.publish(air,all);
    check(readiness.sample(10,air).flip_ready,"idle verified air selection allows the existing flip adapter");
    const auto ack=source.observe(15,owner,sample(201),{true,2,12});const auto active=readiness.sample(15,ack);
    check(active.gameplay&&active.action&&!active.flip_ready&&!active.start,"own selection acknowledgment retains actor context but removes idle-selection readiness");
    ArcadeInput transported(true);transported.enable(true);
    transported.publish_native_context(ack);transported.publish_authority(15,ack.owner_generation,1,true);
    check(transported.native_context(15).airborne_episode==ack.airborne_episode &&
        transported.native_context(15).air_departure==ack.air_departure,"existing facade exposes original physics flight identity without restamping");
    const auto unavailable=source.observe(20,owner,sample(103,0,0),{true,-1});
    check(!readiness.sample(20,unavailable).action,"new native not-ready mode cannot reuse old ready action deadline");
    check(!readiness.sample(20,unavailable).manual,"new not-ready mode cannot reuse generic Manual safety");
    auto bad=unavailable;bad.valid=false;check(!readiness.sample(20,bad).gameplay,"invalid current context cannot borrow a valid proof");
    {
        ArcadeInput input(true);input.enable(true);NativeInputState physics;NativeInputReadiness gate;
        auto frame=physics.observe(100,owner,sample(200),{true,-1});gate.publish(frame,all);
        auto poll=[&](std::uint64_t now,short y){
            input.publish_native_context(frame);input.publish_authority(now,frame.owner_generation,1,true);
            const auto a=gate.sample(now,frame);
            return input.poll(0,now,0,0,y,0,0,a.gameplay,a.start,false,0,false,false,false,now/17,false,{},a.mirror_x,
                a.manual,SkatePose::air,false,a.prepare,a.offboard,false,a.flip_ready,false,0,0,ActionExperiment::classic,a.action,a.mirror_x);
        };
        poll(100,0);poll(110,25000);check(poll(120,-25000).manual.owns,"real facade admits airborne direction-pair preparation");
        physics.observe(130,owner,sample(100),{true,-1});frame=physics.observe(140,owner,sample(200),{true,-1});
        check(poll(150,0).manual.owns,"prepared Manual survives an unpolled landing/rebound without renewed request or safety lease");
        frame=physics.observe(151,owner,sample(103,0,0),{true,-1});
        check(!poll(151,0).manual.owns,"actual unsafe crouch mode still withdraws Manual actuation immediately");
    }
    // Exercise actual facade/session physical edge lifetime around an authority
    // replacement, using the same readiness selection as production remapping.
    for(unsigned stance:{0u,0x20u}) {
        ArcadeInput input(true);input.enable(true);NativeInputState observer;NativeInputReadiness gate;
        NativeInputOwner current_owner=owner;
        auto frame=observer.observe(1000,current_owner,sample(100,stance),{true,-1});gate.publish(frame,all);
        const std::array prepared{PreparedGesture{1,false,GestureContext::ground,
            std::vector<AuthoredStickStep>{{0,1000,-32767},{18,25000,28000},{21,0,0},{24,0,0}}}};
        auto poll=[&](std::uint64_t now,std::uint16_t buttons){
            input.publish_native_context(frame);input.publish_authority(now,frame.owner_generation,1,true);
            const auto a=gate.sample(now,frame);
            return input.poll(0,now,buttons,0,0,0,0,a.gameplay,a.start,a.push,a.speed,false,a.board,true,now/17,false,
                prepared,a.mirror_x,a.manual,SkatePose::riding,false,a.prepare,a.offboard,false,a.flip_ready,false,0,0,
                ActionExperiment::classic,a.action,a.mirror_x);
        };
        poll(1000,0);check(poll(1017,classic_a).authored_playback,"fresh authorized press starts actual facade crouch");
        ++current_owner.rig;frame=observer.observe(1034,current_owner,sample(100,stance^0x20u),{true,-1});
        const auto blocked=poll(1034,classic_a);
        check(!blocked.authored_playback&&!blocked.jump.released,"authority replacement clears old held setup before new readiness exists");
        gate.publish(frame,{true,false,false,false,false,false,false,false,true,0});
        check(!poll(1051,classic_a).authored_playback,"new not-ready owner cannot restart the held press");
        gate.publish(frame,all);check(!poll(1068,classic_a).authored_playback,"new ready publication cannot manufacture a fresh physical press");
        poll(1085,0);const auto fresh=poll(1102,classic_a);
        check(fresh.authored_playback&&fresh.requested_id==1,"release then fresh press admits the new owner");
        check((fresh.right_x<0)==(stance!=0),"new owner authored lateral output uses the current opposite stance");
    }
    std::cout<<checks<<" coherent native readiness checks passed\n";
}

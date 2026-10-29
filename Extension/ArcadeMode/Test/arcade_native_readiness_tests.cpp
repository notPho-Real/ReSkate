#include "Extension/ArcadeMode/arcade_native_readiness.h"
#include "Extension/ArcadeMode/arcade_gesture.h"
#include "Extension/ArcadeMode/arcade_camera_input.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why){++checks;if(!value){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
NativeRequestSample sample(unsigned state=100,unsigned stance=0,unsigned mode=1){NativeRequestSample s{};s.valid=true;s.state=state;s.crouch_mode=mode;s.words[0]=stance;s.words[1]=0xc000;return s;}
void control_ownership() {
 NativeInputReadiness readiness;NativeInputState state;NativeInputOwner owner{1,2,3,4,5};
 const auto proof=state.observe(1000,owner,sample(),{true,1,16});
 readiness.publish(proof,{true,true,true,true,true,true,false,false,true,0});
 check(readiness.owns_controls(1050,proof.owner_generation,{}),"missing action sample retains only bounded original control reservation");
 check(!readiness.sample(1050,{}).gameplay,"reservation cannot manufacture gameplay/action readiness");
 check(!readiness.owns_controls(1151,proof.owner_generation,{}),"control reservation expires at original150ms boundary");
 check(!readiness.owns_controls(999,proof.owner_generation,{}),"clock rollback cannot retain controls");
 auto changed=proof;++changed.owner.rig;
 check(!readiness.owns_controls(1050,proof.owner_generation,changed),"known owner change restores fallback");
 check(!readiness.owns_controls(1050,proof.owner_generation+1,proof),"authority epoch mismatch restores fallback");
 changed=proof;changed.state=dingosdk::game::build::v20260929::arcade_native_probe::offboard_state;
 check(!readiness.owns_controls(1050,proof.owner_generation,changed),"offboard controls retain native meanings");
 readiness.reset();check(!readiness.owns_controls(1050,proof.owner_generation,proof),"reset cannot revive masked authority");
}
void client_phase_gap() {
    struct Route { ArcadeInput::Physical pad{true,1000,0,0,0,0,0,0,0};
        auto physical(std::uint64_t now)const{return now>=pad.at&&now-pad.at<=150?pad:ArcadeInput::Physical{};}
        unsigned selected_slot()const{return 1u;}unsigned generation()const{return 7u;}} route;
    const NativeInputOwner owner{1,2,3,4,5};
    for(unsigned state:{101u,103u,200u,201u}) {
        NativeInputState physics;ArcadeInput input(true);input.enable(true);NativeInputReadiness readiness;
        const auto original=physics.observe(1000,owner,sample(),{true,1,16});
        input.publish_native_context(original);input.publish_authority(1000,original.owner_generation,1,true);
        readiness.publish(original,{true,true,true,true,true,true,false,false,true,0});
        const auto before=camera_input_snapshot(route,input,1000,true);
        auto next=sample(state,0,state==201?2:0);
        check(!physics.correlate(1010,owner,next).valid,"actual client-ahead native phase has no full correlation");
        const bool defer=physics.defer_client_phase(1010,owner,next,input.native_context(1010),input.admitted(0,1010));
        check(defer,"same-owner original proof defers destructive client publication");
        // Production skips publication, cancellation and permission stores on
        // this result. Neither the original frame nor its lease is restamped.
        check(before.same_binding(camera_input_snapshot(route,input,1010,true)),"camera keeps the same source/owner through an uncorrelated phase read");
        check(input.native_context(1010).sampled_at==1000 && readiness.observed_proof().sampled_at==1000,
            "deferral retains original timestamps instead of granting the newer phase");
        check(!physics.defer_client_phase(1151,owner,next,input.native_context(1151),true),"original150ms expiry cannot be deferred");
        check(!camera_input_snapshot(route,input,1151,true).valid,"camera expiry still restores rather than retaining stale input");
        for(unsigned fault=0;fault<5;++fault) {
            auto changed=owner;auto current=next;auto admitted=original;bool permitted=true;
            if(fault==0)++changed.rig;if(fault==1)current.valid=false;if(fault==2)permitted=false;
            if(fault==3)++admitted.owner_generation;if(fault==4)admitted.valid=false;
            check(!physics.defer_client_phase(1010,changed,current,admitted,permitted),"owner/invalid/OFF-focus-source/epoch/absent admission withdraw immediately");
        }
        check(!physics.defer_client_phase(999,owner,next,original,true),"clock rollback cannot retain authority");
        for(unsigned excluded:{300u,400u,504u,701u})
            check(!physics.defer_client_phase(1010,owner,sample(excluded),original,true),"positive nonordinary/bail/rail/offboard exit cannot defer withdrawal");
    }
}
void held_air_manual() {
    for(unsigned hz:{30u,60u})for(int direction:{-1,1})for(bool pad:{false,true})for(unsigned fault=0;fault<7;++fault) {
        ArcadeInput input(true);input.enable(true);NativeInputState physics;NativeInputReadiness readiness;
        NativeInputOwner owner{1,2,3,4,5};NativeGesture::Output output{};std::uint64_t recognized{};
        const auto poll=[&](std::uint64_t now,int held,unsigned state=201,unsigned contacts=0,bool allowed=true){
            auto raw=sample(state,0,state==100?1:2);raw.words[1]=contacts;
            const auto frame=physics.observe(now,owner,raw,{true,-1,16});
            readiness.publish(frame,{true,false,false,false,false,true,false,false,true,0});
            input.publish_native_context(frame);input.publish_authority(now,frame.owner_generation,1,allowed);
            const bool contact=state==100 && (contacts==0x4000 || contacts==0x8000);
            if(contact)input.publish_balance_controls(InputBalanceControls::vertical(direction<0?InputBalanceKind::manual:InputBalanceKind::nose),frame);
            const auto a=readiness.sample(now,frame);
            const auto pose=state!=100?SkatePose::air:contacts==0x4000?SkatePose::manual:contacts==0x8000?SkatePose::nose_manual:SkatePose::riding;
            output=input.poll(0,now,pad?(held>0?1u:held<0?2u:0u):0,0,pad?0:static_cast<short>(held*25000),0,0,
                allowed&&a.gameplay,false,false,0,false,false,false,now/17,false,{},false,a.manual,pose,
                false,false,false,false,a.flip_ready,false,0,0,ActionExperiment::classic,a.action,false,contact);
        };
        poll(1000,0);poll(1100,-direction);poll(1250,direction);recognized=output.manual.recognized_at;
        check(output.manual.owns&&recognized==1250,"actual AIR pair starts the existing Manual/Nose RS actor");
        check(output.suppress_reverse,"the held AIR entry modifier cannot simultaneously reach native LY while the RS actor owns acquisition");
        for(unsigned step=1;step<=hz*2;++step) {
            const auto now=1250+std::uint64_t{step}*1000/hz;
            int held=direction;bool allowed=true;
            if(fault==1&&now>=1600)held=0;
            if(fault==2&&now>=1600&&now<1800)held=0; // Rehold cannot renew.
            if(fault==3&&step==hz/2)++owner.rig;
            if(fault==4&&now>=1600&&now<1800)allowed=false;
            if(fault==5&&now>=1600&&now<1800)continue;
            poll(now,held,fault==6&&step==hz/2?300:201,0,allowed);
        }
        check(output.manual.owns==(fault==0),"only uninterrupted fresh same-owner held direction survives beyond released700ms/ground1200ms");
        if(fault==0) {
            check(output.manual.recognized_at==recognized&&output.manual.request_at==recognized,"held input never renews recognition or original request time");
            poll(3284,direction,100,0xc000);check(output.manual.owns&&output.manual.request_at==3284&&output.suppress_reverse,"first qualified landing retains vertical reservation and starts the separate original1200ms acquisition");
            poll(3318,direction,100,direction<0?0x4000:0x8000);
            check(output.manual.owns&&output.manual.confirmed&&!output.suppress_reverse,"confirmed landing contact hands vertical input to existing balance correction without a new pair");
        }
    }
}
}
int main(){control_ownership();
    client_phase_gap();held_air_manual();
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

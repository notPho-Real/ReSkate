#include "Extension/ArcadeMode/arcade_gesture.h"
#include "Extension/ArcadeMode/arcade_air_motion.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why){++checks;if(!value){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
}
int main(int argc,char** argv){
    for(bool nose:{false,true})for(bool modifier:{false,true})for(unsigned frame:{17u,34u})for(unsigned hold:{0u,17u,34u,50u,100u,150u,200u,250u,500u}){
        if(argc>1 && (argv[1][0]=='n' ? !nose : nose || !modifier)) continue;
        ArcadeInput input(true,InputPolicy::production());input.enable(true);NativeInputState observer;
        NativeRequestSample native{};native.valid=true;native.state=100;native.crouch_mode=1;native.words[1]=nose?0x8000:0x4000;
        const std::array prepared{PreparedGesture{1,false,GestureContext::ground,
            std::vector<AuthoredStickStep>{{0,0,-32767},{18,25000,28000},{21,0,0},{24,0,0}}},
            PreparedGesture{16,true,GestureContext::ground,
            std::vector<AuthoredStickStep>{{0,0,32767},{18,25000,-28000},{21,0,0},{24,0,0}}}};
        auto poll=[&](std::uint64_t now,std::uint16_t buttons,bool admitted=true){
            const auto frame=observer.observe(now,{1,2,3,4,5},native,{true,-1});
            input.publish_native_context(frame);input.publish_authority(now,frame.owner_generation,1,true);
            const bool balancing=native.state==100;
            input.publish_balance_controls(balancing?InputBalanceControls::vertical(nose?InputBalanceKind::nose:InputBalanceKind::manual):InputBalanceControls{},frame);
            return input.poll(0,now,buttons,0,25000,0,0,true,admitted,false,0,modifier,false,true,now/17,modifier,prepared,false,true,
                balancing?(nose?SkatePose::nose_manual:SkatePose::manual):SkatePose::riding,false,false,false,false,true,false,modifier?255:0,0,ActionExperiment::classic,true,true,balancing);
        };
        poll(1000,0);const auto start=poll(1017,0x1000);
        check(start.authored_playback&&start.requested_id==(nose?16:1),"admitted Manual/Nose selects its source hop identity independently of the physical LT modifier");
        native.state=103;native.crouch_mode=0;native.words[1]=0xc000;
        for(std::uint64_t at=1117;at<1017+hold;at+=100)poll(at,0x1000);

        const auto released=poll(1017+hold,0);
        check(released.jump.released&&released.jump.setup_admitted&&released.jump.pressed_at==1017&&released.jump.released_at==1017+hold&&released.jump.nollie==nose,
              "manual hop carries actual physical timestamps rather than balance/setup latency");
        NativeCharge charge;const auto token=charge.semantic_release(1017+hold,released.jump.released,released.jump.setup_admitted,
            released.jump.from_rail,released.jump.pressed_at,released.jump.released_at,true);
        check(token.sequence&&token.charge_ms==std::min(hold,200u),"manual hop uses the shared capped ordinary launch token");
        AirMotion air;AirMotionIntent intent{true,true,1000,0,0,0,0,1};
        const float dt=static_cast<float>(frame)/1000;
        air.step(intent,{1000,1,100,dt,5,0,0,true});
        for(std::uint64_t at=1117;at<1017+hold;at+=100){intent.sampled_at=at;air.step(intent,{at,1,103,dt,5,0,0,true});}
        intent={true,true,1017+hold,token.sequence,token.ms,token.charge_ms,0,1};
        air.step(intent,{1017+hold,1,103,dt,5,0,0,true});
        const auto launch=air.step(intent,{1034+hold,1,200,dt,5,30,0,true});
        check(launch.vertical&&std::abs(launch.velocity_y-GameplayProfile{}.jump_speed(hold))<.0001f,
              "actual takeoff compensates one selected jump impulse after Manual/Nose setup");
        std::array<std::array<float,3>,3> velocities{{{2,30,3},{3,31,4},{4,32,5}}},spins{};
        check(AirMotion::apply_bodies(launch,velocities,spins)&&velocities[0][0]==2&&velocities[0][2]==3&&
            velocities[1][0]==3&&velocities[1][2]==4&&std::abs(velocities[1][1]-velocities[0][1]-1)<.0001f,
            "Manual/Nose launch preserves horizontal velocity and articulated relative momentum");
        native.state=200;check(!poll(1034+hold,0).jump.released,"airborne hop cannot replay a consumed physical release");
        input.enable(false);const auto off=poll(1051+hold,0x1000);
        check(off.buttons==0x1000&&!off.authored_playback&&!off.jump.released,"Arcade OFF yields manual-hop controls without semantic launch metadata");
    }
    if(argc==1)for(bool nose:{false,true})for(bool modifier:{false,true}) {
        ArcadeInput input(true,InputPolicy::production());input.enable(true);NativeInputState observer;NativeRequestSample native{};
        native.valid=true;native.state=100;native.crouch_mode=1;native.words[1]=0xc000;
        const std::array prepared{PreparedGesture{1,false,GestureContext::ground,
            std::vector<AuthoredStickStep>{{0,0,-32767},{18,25000,28000},{21,0,0},{24,0,0}}},
            PreparedGesture{16,true,GestureContext::ground,
            std::vector<AuthoredStickStep>{{0,0,32767},{18,25000,-28000},{21,0,0},{24,0,0}}}};
        bool admitted{};NativeInputFrame current{};
        auto poll=[&](std::uint64_t now,std::uint16_t buttons,short y) {
            current=observer.observe(now,{1,2,3,4,5},native,{true,-1});input.publish_native_context(current);input.publish_authority(now,current.owner_generation,1,true);
            input.publish_balance_controls(admitted?InputBalanceControls::vertical(nose?InputBalanceKind::nose:InputBalanceKind::manual):InputBalanceControls{},current);
            return input.poll(0,now,buttons,0,y,0,0,true,true,false,0,modifier&&bool(buttons&0x1000),false,true,now/17,
                modifier,prepared,false,true,admitted?(nose?SkatePose::nose_manual:SkatePose::manual):SkatePose::riding,
                false,false,false,false,true,false,(modifier&&(buttons&0x1000))?255:0,0,ActionExperiment::classic,true,true,admitted);
        };
        poll(2000,0,0);poll(2017,0,nose?-25000:25000);
        check(poll(2034,0,nose?25000:-25000).manual.owns,"existing direction-pair adapter owns the pre-hop Manual request");
        native.words[1]=nose?0x8000:0x4000;admitted=true;
        check(poll(2051,0,0).manual.confirmed,"independently admitted matching contact confirms the pre-hop manual actuator");
        const auto setup=poll(2068,0x1000,0);
        check(setup.authored_playback&&setup.requested_id==(nose?16:1)&&!setup.manual.owns,
              "one fresh A replaces the old manual stick owner with the source-selected hop setup");
        native.state=103;native.crouch_mode=0;admitted=false;
        const auto release=poll(2102,0,0);
        check(release.jump.released&&release.jump.setup_admitted&&release.jump.nollie==nose&&!release.manual.owns,
              "physical release from real manual ownership retains the hop identity and no stale manual stick");
        native.state=200;
        NativeGesture::Output finished{};unsigned releases=1;
        for(std::uint64_t now=2119;now<2425;now+=17){finished=poll(now,0,0);if(finished.jump.released)++releases;}
        check(releases==1&&!finished.manual.owns&&!finished.authored_playback&&finished.right_x==0&&finished.right_y==0,
              "completed native-air handoff emits one semantic release and restores neutral synthetic channels");
    }
    if(argc==1)for(bool nose:{false,true}) {
        ArcadeInput input(true);input.enable(true);NativeInputState observer;NativeRequestSample native{};
        native.valid=true;native.state=100;native.crouch_mode=1;native.words[1]=nose?0x8000:0x4000;
        const std::array prepared{PreparedGesture{1,false,GestureContext::ground,
            std::vector<AuthoredStickStep>{{0,0,-32767},{18,25000,28000},{21,0,0},{24,0,0}}},
            PreparedGesture{16,true,GestureContext::ground,
            std::vector<AuthoredStickStep>{{0,0,32767},{18,25000,-28000},{21,0,0},{24,0,0}}}};
        auto frame=observer.observe(3000,{1,2,3,4,5},native,{true,-1,0x200000});
        input.publish_native_context(frame);input.publish_authority(3000,frame.owner_generation,1,true);
        input.publish_balance_controls(InputBalanceControls::vertical(nose?InputBalanceKind::nose:InputBalanceKind::manual),frame);
        auto poll=[&](std::uint64_t now,std::uint16_t buttons){return input.poll(0,now,buttons,0,0,0,0,true,true,false,0,false,false,true,now/17,false,prepared,
            false,true,nose?SkatePose::nose_manual:SkatePose::manual,false,false,false,false,true,false,0,0,ActionExperiment::classic,true,true,true);};
        poll(3000,0);
        frame=observer.observe(3017,{1,2,3,4,5},native,{true,-1,0x300000});input.publish_native_context(frame);input.publish_authority(3017,frame.owner_generation,1,true);
        const auto stale=poll(3017,0x1000);
        check(!stale.authored_playback&&!stale.requested_id,"stale balance admission cannot silently fall back to a different hop family");
    }
    if(argc==1)for(bool nose:{false,true})for(bool admitted:{false,true})for(bool modifier:{false,true}) {
        ArcadeInput input(true);input.enable(true);NativeInputState observer;NativeRequestSample native{};
        native.valid=true;native.state=100;native.crouch_mode=1;native.words[1]=nose?0x8000:0x4000;
        const std::array full{PreparedGesture{1,false,GestureContext::ground,
            std::vector<AuthoredStickStep>{{0,0,-32767},{18,25000,28000},{21,0,0},{24,0,0}}},
            PreparedGesture{16,true,GestureContext::ground,
            std::vector<AuthoredStickStep>{{0,0,32767},{18,25000,-28000},{21,0,0},{24,0,0}}}};
        const auto catalog=admitted?std::span<const PreparedGesture>(full).subspan(nose?0:1,1):std::span<const PreparedGesture>(full);
        auto poll=[&](std::uint64_t now,std::uint16_t buttons){
            const auto frame=observer.observe(now,{1,2,3,4,5},native,{true,-1});input.publish_native_context(frame);input.publish_authority(now,frame.owner_generation,1,true);
            input.publish_balance_controls(admitted?InputBalanceControls::vertical(nose?InputBalanceKind::nose:InputBalanceKind::manual):InputBalanceControls{},frame);
            return input.poll(0,now,buttons,0,0,0,0,true,true,false,0,modifier,false,true,now/17,modifier,catalog,
                false,true,nose?SkatePose::nose_manual:SkatePose::manual,false,false,false,false,true,false,modifier?255:0,0,ActionExperiment::classic,true,true,admitted);
        };
        poll(4000,0);const auto output=poll(4017,0x1000);
        if(admitted)check(output.unsupported&&!output.authored_playback&&!output.requested_id,"missing exact balance-hop catalog entry never substitutes the other jump family");
        else check(output.requested_id==(modifier?16:1),"unadmitted wheel pose cannot choose a Nose/Manual hop identity");
    }
    std::cout<<checks<<" composed native manual hop checks passed\n";
}

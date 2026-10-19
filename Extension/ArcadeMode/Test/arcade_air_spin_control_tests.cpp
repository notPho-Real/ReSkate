#include "Extension/ArcadeMode/arcade_rotation.h"
#include "Extension/ArcadeMode/arcade_gesture.h"
#include "Extension/ArcadeMode/arcade_air_motion.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why){++checks;if(!value){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
template<class Input> bool permits(const Input& input,unsigned slot,bool authored){
    if constexpr(requires{input.allows_air_rotation(slot,authored);}) return input.allows_air_rotation(slot,authored);
    return !authored;
}
}
int main(int argc,char**argv){
    if(argc==1 || argv[1][0]=='s')for(auto side:{BoardChord::lb,BoardChord::rb})for(auto duration:{0u,17u,34u,80u,120u,121u,250u}){
        AirRotation spin;spin.poll(1000,0,true,SkatePose::air,false,false,false,true);
        auto out=spin.poll(1017,side,true,SkatePose::air,false,false,false,true);
        check(out.owns_left&&out.x==0&&out.y==0&&out.yaw_rate==(side==BoardChord::lb?-7.75f:7.75f),"supported airborne shoulder begins selected10/10 angular rate on its first eligible sample");
        out=spin.poll(1017+duration,side,true,SkatePose::air,false,false,false,true);
        check(out.owns_left&&out.yaw_rate!=0,"held native spin has no directional-delay or fixed-duration recipe");
        check(!spin.poll(1018+duration,0,true,SkatePose::air,false,false,false,true).owns_left,"physical shoulder release stops the input rate immediately");
    }
    if(argc==1 || argv[1][0]=='j') {
        ArcadeInput input(true);input.enable(true);NativeInputState physics;
        NativeRequestSample native{};native.valid=true;native.state=100;native.crouch_mode=1;native.words[1]=0xc000;
        const std::array catalog{PreparedGesture{1,false,GestureContext::ground,{{0,0,-32767},{18,25000,28000},{21,0,0},{54,0,0}}}};
        auto poll=[&](std::uint64_t now,std::uint16_t buttons){
            auto f=physics.observe(now,{1,2,3,4,5},native,{true,-1});input.publish_native_context(f);input.publish_authority(now,f.owner_generation,1,true);
            return input.poll(0,now,buttons,0,0,0,0,true,true,false,0,false,false,true,now/17,false,catalog,false,true,
                native.state==200?SkatePose::air:SkatePose::riding,false,false,false,false,true,false,0,0,ActionExperiment::classic,true,true);
        };
        poll(1000,0);auto setup=poll(1017,classic_a);
        check(setup.authored_playback,"ordinary jump setup actually owns its authored playback");
        native.state=103;native.crouch_mode=0;auto released=poll(1051,0);
        check(released.jump.released&&released.jump.setup_admitted,"actual ordinary jump release retains physical admission");
        native.state=200;auto air=poll(1068,BoardChord::rb);
        check(air.authored_playback&&permits(input,0,air.authored_playback),"ordinary jump playback tail does not suppress independent airborne shoulder rotation");
        input.cancel_commands();check(!permits(input,0,true),"command cancellation cannot leave an old jump exemption for future authored actions");
    }
    if(argc==1) {
        for(auto side:{BoardChord::lb,BoardChord::rb})for(unsigned lead:{0u,17u,120u,500u}) {
            AirRotation spin;spin.poll(1000,0,true,SkatePose::riding,false,false,false,true);
            spin.poll(1017,side,true,SkatePose::riding,true,false,false,true);
            const auto air=spin.poll(1017+lead,side,true,SkatePose::air,false,false,false,true);
            check(air.owns_left&&std::abs(air.yaw_rate)==7.75f,"ground-held shoulder selects full source rate on actual takeoff without restarting a prewind delay");
            check(!spin.poll(1034+lead,side,true,SkatePose::riding,false,false,false,true).owns_left,
                "actual landing ends the airborne shoulder episode immediately");
            check(!spin.poll(1051+lead,side,true,SkatePose::air,false,false,false,true).owns_left,
                "held shoulder cannot silently resume after a seen landing");
            spin.poll(1068+lead,0,true,SkatePose::air,false,false,false,true);
            check(spin.poll(1085+lead,side,true,SkatePose::air,false,false,false,true).owns_left,
                "release then fresh shoulder press restores ordinary-air rotation");
        }
        for(auto side:{BoardChord::lb,BoardChord::rb})for(unsigned delay:{0u,17u,120u,121u,250u}) {
            AirRotation spin;spin.poll(1000,0,true,SkatePose::air,false,false,false,true);
            spin.poll(1017,side,true,SkatePose::air,false,false,false,true);
            check(!spin.poll(1017+delay,BoardChord::pair,true,SkatePose::air,false,false,false,true).owns_left &&
                !spin.poll(1034+delay,side,true,SkatePose::air,false,false,false,true).owns_left,
                "simultaneous or late both-shoulder chord cancels spin and partial release cannot restart it");
        }
        for(unsigned fault=0;fault<5;++fault) {
            AirRotation spin;spin.poll(1000,0,true,SkatePose::air,false,false,false,true);
            spin.poll(1017,BoardChord::rb,true,SkatePose::air,false,false,false,true);
            check(!spin.poll(fault==4?1000:1034,BoardChord::rb,fault!=0,
                fault==1?SkatePose::bail:fault==2?SkatePose::grind:SkatePose::air,false,false,fault==3,true).owns_left,
                "OFF/bail/rail/competing ownership/rollback cancel the physical spin request");
        }
        for(unsigned rate:{30u,60u})for(unsigned turns:{1u,2u,3u,4u})for(auto side:{BoardChord::lb,BoardChord::rb}) {
            const float dt=1.0f/static_cast<float>(rate);AirRotation input;AirMotion motion;
            input.poll(1000,0,true,SkatePose::air,false,false,false,true);
            AirMotionIntent intent{true,true,1000,0,0,0,0,1};AirMotionFrame frame{1000,1,100,dt,5,0,0,true};motion.step(intent,frame);
            const double pi=3.14159265358979323846;
            const auto frames=static_cast<unsigned>(std::ceil(static_cast<double>(turns)*pi/(7.75*dt)));
            std::array<float,16> matrix{1,0,0,0,0,1,0,0,0,0,1,0,0,5,0,1};double total{};
            for(unsigned i=0;i<frames;++i) {
                const auto now=1017+i*(rate==30?34u:17u);
                auto spin=input.poll(now,side,true,SkatePose::air,false,false,false,true);
                intent.sampled_at=frame.now=now;frame.state=200;intent.yaw_rate=spin.yaw_rate;
                const auto out=motion.step(intent,frame);frame.velocity_y=out.velocity_y;
                check(out.angular&&std::abs(out.spin_y)==7.75f,"source held shoulder rate reaches the existing ordinary-air physical controller each step");
                AirMotion::rotate_target(matrix,out.spin_y*dt);total+=out.spin_y*dt;
            }
            check(std::abs(total)>=turns*pi && std::abs(total)-turns*pi<7.75*dt+.00001,
                "180/360/540/720 crossing is bounded by one physical sample rather than a fixed-duration recipe");
            check(std::abs(matrix[8]-std::sin(total))<.0001&&std::abs(matrix[10]-std::cos(total))<.0001,
                "physical local-up heading agrees with independently unwrapped signed spin integral");
            const auto now=1017+frames*(rate==30?34u:17u);
            const auto stopped=input.poll(now,0,true,SkatePose::air,false,false,false,true);
            intent.yaw_rate=stopped.yaw_rate;intent.sampled_at=frame.now=now;
            auto release=motion.step(intent,frame);
            check(release.angular&&release.spin_y==0,"physical release delivers one zero angular target without prescribing a completion angle");
        }
        {
            AirMotion motion;AirMotionIntent intent{true,true,10,0,0,0,7.75f,1};
            AirMotionFrame f{10,1,100,1.0f/60,5,0,0,true};motion.step(intent,f);
            f.now=intent.sampled_at=17;f.state=200;check(motion.step(intent,f).angular,"preheld rate can start the first actual flight");
            f.now=20;f.state=100;motion.step(intent,f);f.now=30;f.state=200;
            check(!motion.step(intent,f).angular,"physics-only landing/relaunch cannot replay a stale previous-flight angular intent");
            f.now=intent.sampled_at=40;intent.yaw_rate=0;motion.step(intent,f);
            f.now=intent.sampled_at=57;intent.yaw_rate=7.75f;
            check(motion.step(intent,f).angular,"a new neutral/rate publication rearms spin after the actual flight boundary");
        }
        NativeInputState observer;NativeRequestSample s{};s.valid=true;s.state=200;s.crouch_mode=1;
        auto frame=observer.observe(1000,{1,2,3,4,5},s,{true,-1});
        check(native_rotation_pose(frame,SkatePose::riding,false)==SkatePose::air,"fresh native takeoff overrides lagging ground pose for shoulder initiation");
        s.state=100;frame=observer.observe(1017,{1,2,3,4,5},s,{true,-1});
        check(native_rotation_pose(frame,SkatePose::air,false)==SkatePose::riding,"fresh native landing cannot retain stale air spin pose");
        check(native_rotation_pose(frame,SkatePose::manual,true)==SkatePose::manual,"independently admitted Manual cannot become ordinary ground spin");
        frame.valid=false;check(native_rotation_pose(frame,SkatePose::air,false)==SkatePose::unknown,"invalid native frame cannot prolong airborne spin");
    }
    std::cout<<checks<<" airborne spin control checks passed\n";
}

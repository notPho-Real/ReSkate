#include "Extension/ArcadeMode/arcade_air_motion.h"
#include "Extension/ArcadeMode/arcade_input_events.h"
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool condition,const char* why){++checks;if(!condition){std::cerr<<why<<'\n';std::exit(1);}}
bool near(float a,float b){return std::abs(a-b)<.00002f;}
}
int main(){
    check(!GroundControl{}.manual_vertical_reserved && !GroundControl::decode(0).manual_vertical_reserved,
        "unavailable control payload does not reserve an ordinary channel");
    for(unsigned bits=0;bits<512;++bits){
        const auto original=GroundControl::decode(bits);
        auto reserved=original;reserved.manual_vertical_reserved=true;
        auto free=original;free.manual_vertical_reserved=false;
        check((reserved.encode()^free.encode())==(1u<<8),"reservation uses only its dedicated unused payload bit");
        check(GroundControl::decode(reserved.encode()).manual_vertical_reserved &&
            !GroundControl::decode(free.encode()).manual_vertical_reserved,"reservation survives coherent ground control transport");
        check(reserved.known==free.known && reserved.down==free.down && reserved.left==free.left &&
            reserved.right==free.right && reserved.velocity_lock==free.velocity_lock &&
            reserved.can_kick(1,false)==free.can_kick(1,false) &&
            reserved.trying_to_brake(1,1)==free.trying_to_brake(1,1),
            "reservation preserves physical steering brake and kick facts without granting admission");
    }
    const GameplayProfile profile;
    for(bool automatic:{false,true})for(bool square:{false,true})for(bool down:{false,true})
    for(bool turn:{false,true})for(float speed:{1.2699f,1.27f,1.2701f,10.f})for(float dot:{-1.f,0.f,1.f}){
        GroundControl controls{true,automatic,square,down,turn,false};
        const bool slow=speed<50*.0254f;
        const bool expected=(!automatic&&!square&&slow)||(down&&(slow||!turn||dot<0));
        check(controls.trying_to_brake(speed,dot)==expected,"physical brake truth table follows strict50-inch speed, turn and reverse-facing branches");
        check(controls.trying_to_brake(speed,dot,false)==(!automatic&&!square&&slow),"Down preference leaves the independent low-speed no-autokick brake intact");
        check(GroundControl::decode(controls.encode()).encode()==controls.encode(),"all control flags survive the existing coherent transport representation");
        for(bool crouched:{false,true}){
            check(controls.can_kick(speed,crouched)==((automatic||square)&&!down&&speed<=profile.kick_speed(crouched)),"kick permission follows physical Down and auto/Square policy");
            check(!controls.can_kick(speed,crouched,profile,true),"known force-disabled kick policy cannot acquire a budget");
        }
    }
    for(bool crouched:{false,true}){
        GroundControl controls{true,true};const float threshold=profile.kick_speed(crouched);
        check(controls.can_kick(threshold,crouched)&&!controls.can_kick(std::nextafter(threshold,100.f),crouched),"kick threshold is inclusive without rounding a speed above it into eligibility");
    }
    GroundControl unknown;check(!unknown.can_kick(0,false)&&!unknown.trying_to_brake(0,0),"unavailable controls cannot claim source brake or kick readiness");
    check(!GroundControl{true,true,false,true}.trying_to_brake(10,std::numeric_limits<float>::quiet_NaN()),"unknown facing is unavailable instead of reverse-facing");
    for(short x:{short(-13001),short(-13000),short(-12999),short(0),short(12999),short(13000),short(13001)})
    for(short y:{short(-13001),short(-13000),short(-12999),short(0),short(12999),short(13000),short(13001)})
    for(auto raw_pad:{0u,1u,2u,4u,8u,5u,6u,9u,10u}){
        const auto pad=static_cast<std::uint16_t>(raw_pad);
        InputEvents events;events.sample(100,{pad,x,y},false,false,InputPolicy::production());
        const auto controls=GroundControl::physical(true,pad,x,y);const auto direction=events.held_direction();
        const auto normalized=InputEvents::normalize({pad,x,y},100);
        const auto code=InputEvents::encode(controls.right?1:controls.left?-1:0,controls.down?-1:bool(normalized.buttons&1u));
        if(!controls.known||code!=direction)std::cerr<<"x="<<x<<" y="<<y<<" pad="<<pad<<" expected="<<direction<<" got="<<code<<"\n";
        check(controls.known&&code==direction,"ground controls use the existing production physical quantization and pad precedence");
    }
    for(auto pad:{3u,12u,0x10u,0x20u})check(!GroundControl::physical(true,static_cast<std::uint16_t>(pad),0,-32768).known,"opposing or menu controls cannot create physical eligibility");
    for(float dt:{.017f,.034f})for(float coefficient:{0.f,.5f,1.f,1.5f})for(bool automatic:{false,true}){
        GroundResistance resistance;GroundFriction friction{automatic,false,coefficient};
        const float wind=.00001f/.0254f*60*dt*100;
        const float expected=10-wind-coefficient*.0254f*60*dt;
        check(near(resistance.selected_speed(10,dt,false,false,true,true,friction),expected),"rolling overrides apply converted60-inch coefficient acceleration at both physics cadences");
        resistance.step(dt,10,false,false,false,friction);
        for(float native_loss:{0.f,.01f,1.f}){
            GroundResistance episode;episode.step(dt,10,false,false,false,friction);
            const float current=10-native_loss,delta=episode.step(dt,current,false,false,false,friction);
            check(delta<=0&&near(current+delta,std::min(current,expected)),"additional resistance credits native drag and cannot restore collision loss");
        }
        const float impulse=900*.0254f*dt;
        check(resistance.selected_speed(2*impulse-.00001f,dt,false,true,true,false)==0,"brake snaps only below twice the selected impulse");
        check(near(resistance.selected_speed(2*impulse,dt,false,true,true,false),impulse),"exact brake snap boundary still subtracts one impulse");
        GroundResistance manual;manual.step(dt,10,false,false,true,friction);
        check(near(manual.step(dt,10,true,true,true,friction),expected-10),"admitted Manual rejects crouched and braking budgets while retaining explicitly selected coast friction");
        check(resistance.selected_speed(.000001f,dt,false,false,true,true,friction)>=0,"rolling resistance cannot reverse motion");
    }
    for(float dt:{.017f,.034f}){
        GroundResistance resistance;GroundFriction disabled{false,false};
        check(resistance.selected_speed(10,dt,false,false,true,true,disabled)==10,"no-autokick with default coefficient bypasses both wind and rolling friction");
        disabled.bailing=true;check(resistance.selected_speed(10,dt,false,false,true,true,disabled)<10,"known bail does not select the default no-autokick bypass");
    }
    NativeInputFrame current{};current.valid=true;current.owner={1,2,3,4,5};current.owner_generation=8;current.physics_step=4;current.sampled_at=100;
    auto proof=NativeAirSpinProof::from_frame(current);
    check(proof.same_owner_observation(current,250)&&!proof.same_owner_observation(current,251),"ground controls retain their original inclusive150ms physics lease");
    ++current.owner_generation;check(!proof.same_owner_observation(current,100),"reused pointers cannot borrow a different owner generation");
    std::cout<<checks<<" ground control checks passed\n";
}

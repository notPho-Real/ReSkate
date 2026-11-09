#include "Extension/ArcadeMode/arcade_custom_flatland_runtime.h"
#include "Extension/ArcadeMode/arcade_input_session.h"
#include "Extension/Skater/skater_animation.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk;
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool ok,const char* why){++checks;if(!ok){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
struct Player {
    std::uint64_t serial{},ticket{},now{},until{};
    unsigned starts{},cancels{},replacements{};
    float time{};
    bool occupied{},seed_available{true},seed_wrong_owner{},reject_replace{};
    std::uintptr_t entity{2};
} player;
}
// Fake ONLY the native client endpoint; compile and execute the actual runtime
// mailbox, shared recognizer, custom episode and animation authoring code.
namespace dingosdk::skater_animation {
void prepare_playback() noexcept {}
bool can_play() noexcept {return !player.occupied && !player.ticket;}
bool owns_play(std::uint64_t id) noexcept {return id && id==player.ticket && player.now<player.until;}
bool controlled_play_current(std::uint64_t id) noexcept {return owns_play(id);}
void cancel_play(std::uint64_t id){if(id && id==player.ticket){player.ticket=0;++player.cancels;}}
std::uint64_t try_play_controlled(Play play){
    if(!can_play() || !play.clip || play.clip->empty())return 0;
    ++player.starts;player.until=player.now+150;return player.ticket=++player.serial;
}
bool replace_owned(std::uint64_t id,std::shared_ptr<const Clip> clip,float seconds){
    if(!owns_play(id) || player.reject_replace || !clip || clip->empty() ||
        seconds<0 || seconds>clip->duration())return false;
    ++player.replacements;player.time=seconds;player.until=player.now+150;return true;
}
bool seek_owned(std::uint64_t id,float seconds){
    if(!owns_play(id))return false;
    player.time=seconds;player.until=player.now+150;return true;
}
std::optional<NativePose> native_pose(){
    if(!player.seed_available)return {};
    NativePose p;p.base=1;p.entity=player.seed_wrong_owner?99:player.entity;
    p.joints.resize(anim_studio::skeleton_joints);p.age=.001;p.sequence=player.now;return p;
}
}
namespace {
struct Rig {
    NativeInputState observer;
    NativeInputOwner owner{1,++player.entity,3,4,5};
    NativeInputFrame frame{};
    InputSession session;
    std::uint64_t now{player.now+1000};
    float dt{1.f/60};
    bool enabled{true},balanced{true};
    Rig(){
        custom_flatland_runtime::set_enabled(true);
        custom_flatland_runtime::cancel();player.occupied=false;player.seed_available=true;player.seed_wrong_owner=false;player.reject_replace=false;
        next();tick();next();tick();
        check(binding().enabled,"a fresh matching native seed enables the dev bridge");
        InputAdmission a=admission();session.poll(now,{},0,0,a);
        ManualInputDiagnostic d{};d.owns=d.safe=true;d.request=-1;d.request_at=now-100;d.recognized_at=d.request_at;d.recognition_id=7;
        session.resume_manual_episode(d,a.native_context);
    }
    CustomFlatlandContext context()const{return {frame,now,owner.entity,enabled,true,balanced,false,false};}
    CustomFlatlandBinding binding(){return custom_flatland_runtime::observe(context());}
    InputAdmission admission(){
        auto binding=this->binding();InputAdmission a{};
        a.gameplay=enabled;a.may_start=a.manual_safe=a.manual_balance_admitted=true;a.pose=SkatePose::manual;a.tick=now;
        a.native_context=InputNativeContext::from_frame(frame);a.balance_generations=a.native_context.generations;
        a.balance_controls=InputBalanceControls::vertical();a.balance_controls.generations=a.balance_generations;
        a.custom_flatland=binding.enabled;a.custom_flatland_branch=binding.branch;a.branch_ready=binding.ready;
        a.extensions={nullptr,[](void*,const InputRequest& r) noexcept {return custom_flatland_runtime::request(r);}};
        return a;
    }
    void next(unsigned ms=17){
        now+=ms;player.now=now;NativeRequestSample s{};s.valid=true;s.state=100;s.crouch_mode=1;s.words[1]=0x4000u;
        frame=observer.observe(now,owner,s,{true,-1},false,dt);
    }
    void tick(){custom_flatland_runtime::tick(context());}
    auto step(std::array<float,3> v={3,0,4}){return custom_flatland_runtime::consume(frame,now,v,balanced);}
    auto poll(std::uint16_t buttons){
        next();auto c=context();c.ollie=(buttons&0x1000u)!=0;custom_flatland_runtime::observe(c);
        auto a=admission(); // For A use the cancelled binding, without starting another gesture.
        auto out=session.poll(now,{buttons},0,0,a);
        if(!out.manual.owns)custom_flatland_runtime::cancel();
        return out;
    }
    void enter(){
        check(poll(0x4000).manual.owns,"live adapter keeps first X inside Manual");
        poll(0);check(poll(0x8000).manual.owns,"live adapter accepts Y without releasing native pressure");
        check(binding().branch==2,"accepted gesture changes only the custom branch");
        check(!step().admitted,"queued animation cannot authorize drag");
        tick();check(player.ticket!=0,"client submits authored pose through owned player");
        poll(0);
    }
    void hold(unsigned count=24){
        for(unsigned i=0;i<count;++i){next();auto d=step();tick();check(d.admitted,"active native writer consumes custom drag");}
        check(binding().ready && player.time==.3f,"entry settles into native-clock held pose");
    }
};
void slice(){
    Rig r;const auto starts=player.starts;r.enter();r.hold(1000);
    check(player.starts==starts+1,"holding beyond 15 seconds never restarts or duplicates animation");
    r.next();const auto first=r.step();const auto repeat=r.step();
    check(first.admitted && !repeat.admitted && repeat.x==0 && repeat.z==0,"one original step cannot apply drag twice");r.tick();
    const auto ticket=player.ticket;const auto replacements=player.replacements;const auto cancels=player.cancels;
    r.poll(0x4000);r.poll(0);check(r.poll(0x4000).manual.owns,"flip retains existing Manual actuator");
    check(!r.step().admitted,"pending replacement cannot authorize drag before client acceptance");
    r.tick();check(player.starts==starts+1 && player.ticket==ticket && player.replacements==replacements+1 &&
        player.cancels==cancels,"flip replaces the owned clip without reborrowing or restarting its blend");
    for(unsigned i=0;i<40;++i){r.next();r.step();r.tick();}
    check(!player.ticket && r.binding().branch==0 && r.poll(0).manual.owns,"flip blends to seed and returns to continuing Manual");
}
void replacement_loss(){
    for(bool stolen:{false,true}){
        Rig r;r.enter();r.hold();
        r.poll(0x4000);r.poll(0);r.poll(0x4000);
        const auto starts=player.starts;const auto replacements=player.replacements;
        const auto original=player.ticket;
        if(stolen){
            player.ticket=++player.serial;player.occupied=true;
            check(!r.binding().enabled,"lost player withdraws input readiness before the next client tick");
        }
        else player.reject_replace=true;
        r.tick();
        check(!r.step().admitted && r.binding().branch==0,"failed replacement withdraws the custom phase and drag");
        check(player.starts==starts && player.replacements==replacements,"failed replacement cannot fall back to borrowing a new player");
        if(stolen){check(player.ticket!=0 && player.ticket!=original,"replacement failure preserves the competing animation");player.ticket=0;player.occupied=false;}
        else check(!player.ticket,"rejected replacement retires only its own old clip");
    }
}
void cleanup(){
    for(unsigned reason=0;reason<8;++reason){
        Rig r;r.enter();r.hold();const auto ticket=player.ticket;
        if(reason==0)r.enabled=false;
        if(reason==1)r.balanced=false;
        if(reason==2)++r.owner.core;
        if(reason==3){r.next(151);}else r.next();
        if(reason==4){player.ticket=++player.serial;player.occupied=true;}
        if(reason==5){auto c=r.context();c.ollie=true;custom_flatland_runtime::observe(c);}
        if(reason==6)custom_flatland_runtime::cancel();
        if(reason==7)custom_flatland_runtime::set_enabled(false);
        custom_flatland_runtime::observe(r.context());
        const auto motion=r.step();check(!motion.admitted,"OFF/bail/owner/stale/player/Ollie/reset revoke friction immediately");
        r.tick();check(player.ticket!=ticket,"client cleans up the cancelled animation ticket");
        if(reason==4){check(player.ticket!=0,"cleanup preserves a newer unrelated animation");player.ticket=0;player.occupied=false;}
    }
}
void seed_and_pending(){
    Rig r;custom_flatland_runtime::cancel();r.owner.entity=++player.entity;player.seed_wrong_owner=true;
    r.next();r.tick();r.next();r.tick();check(!r.binding().enabled,"mismatched seed entity cannot enable gesture");
    player.seed_wrong_owner=false;r.next();r.tick();check(r.binding().enabled,"matching native seed enables recovered owner");
    r.poll(0x4000);r.poll(0);r.poll(0x8000);
    player.occupied=true;r.tick();check(!r.step().admitted,"another animation winning queued submission prevents drag");
    player.occupied=false;
}
}
int main(){
    check(!custom_flatland_runtime::status().enabled,"dev experiment defaults OFF");
    slice();replacement_loss();cleanup();seed_and_pending();std::cout<<checks<<" live adapter checks passed (simulated native endpoint)\n";}

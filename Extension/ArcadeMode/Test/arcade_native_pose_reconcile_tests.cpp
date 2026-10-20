#include "Extension/ArcadeMode/arcade_gesture.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why){++checks;if(!value){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
struct Fixture {
    ArcadeInput input{true,InputPolicy::production()};NativeInputState observer;NativeRequestSample native{};
    NativeFlipSelection flip{true,-1,0x200000};SkatePose caller{SkatePose::air};bool prepare{};
    NativeInputOwner owner{1,2,3,4,5};std::int16_t left_x{-25000};
    const std::array<PreparedGesture,2> prepared{PreparedGesture{1,false,GestureContext::ground,
        std::vector<AuthoredStickStep>{{0,0,-32767},{18,25000,28000},{21,0,0},{24,0,0}}},
        PreparedGesture{2,false,GestureContext::ground,
        std::vector<AuthoredStickStep>{{0,0,-32767},{18,-25000,28000},{21,0,0},{24,0,0}}}};
    Fixture(){input.enable(true);native.valid=true;native.state=200;native.crouch_mode=1;}
    NativeGesture::Output poll(std::uint64_t now,std::uint16_t buttons=0,short y=0){
        const auto frame=observer.observe(now,owner,native,flip);
        input.publish_native_context(frame);input.publish_authority(now,frame.owner_generation,1,true);
        return input.poll(0,now,buttons,left_x,y,321,-456,true,true,true,10,false,false,true,now/17,false,prepared,
            false,true,caller,false,prepare,false,false,true,false,0,0,ActionExperiment::classic,true,true,false);
    }
};
enum class PreparationCase { cardinal, diagonal, diagonal_prefix, diagonal_terminal, pad_override, owner_change, unknown_gap, observation_gap };
void preparation_directions(){
    using Case=PreparationCase;
    for(std::uint64_t step:{17u,34u})for(int sign:{-1,1})for(auto kind:{Case::cardinal,Case::diagonal,
            Case::diagonal_prefix,Case::diagonal_terminal,Case::pad_override,Case::owner_change,Case::unknown_gap,Case::observation_gap}){
        Fixture f;f.native.state=101;f.prepare=true;f.left_x=0;
        const auto first_y=static_cast<std::int16_t>(sign*25000);
        const auto last_y=static_cast<std::int16_t>(-sign*25000);
        const bool pad=kind==Case::pad_override;
        const auto first_buttons=static_cast<std::uint16_t>(pad?(sign>0?1u:2u):0u);
        const auto last_buttons=static_cast<std::uint16_t>(pad?(sign>0?2u:1u):0u);
        f.poll(5000);f.input.set_recognition_capture(0,true);
        f.left_x=kind==Case::diagonal || kind==Case::diagonal_prefix || pad ? std::int16_t{-25000}:std::int16_t{0};
        const auto first=InputEvents::normalize({first_buttons,f.left_x,first_y},5000+step);
        check((first.direction==8 || first.direction==2)==(kind!=Case::diagonal && kind!=Case::diagonal_prefix),
            "preparation fixture distinguishes cardinal commands from diagonal held components");
        f.poll(5000+step,first_buttons,first_y);
        f.left_x=kind==Case::diagonal || kind==Case::diagonal_terminal || pad ? std::int16_t{-25000}:std::int16_t{0};
        const auto waiting=f.poll(5000+2*step,last_buttons,last_y);
        check(!waiting.manual.owns && !waiting.authored_playback && !waiting.action.owned,
            "native101 preparation never starts an actor before independently observed readiness");
        const bool cardinal=kind!=Case::diagonal && kind!=Case::diagonal_prefix && kind!=Case::diagonal_terminal;
        const auto* pending=f.input.input_diagnostic(0);
        bool recognized=false;
        for(unsigned i=0;i<pending->recognition_count;++i){
            const auto& event=pending->recognition[i];
            if(event.request.action!=InputAction::manual && event.request.action!=InputAction::nose_manual)continue;
            recognized=true;
            check(event.queued && event.request.first_input_at==5000+step &&
                event.request.last_input_at==5000+2*step && event.request.recognized_at==5000+2*step,
                "preparation queues the actual cardinal edges with their original clocks before readiness");
        }
        check(recognized==cardinal,"native-bound preparation recognizes exact cardinal endpoints only");
        auto ready_at=5000+3*step;
        if(kind==Case::owner_change)f.owner.rig=6;
        if(kind==Case::unknown_gap){f.native.state=999;f.poll(ready_at,last_buttons,last_y);ready_at+=step;}
        if(kind==Case::observation_gap)ready_at=5000+2*step+151;
        f.native.state=100;f.native.words[1]=0xc000;f.prepare=false;
        const auto ready=f.poll(ready_at,last_buttons,last_y);
        const bool eligible=kind==Case::cardinal || pad;
        check(ready.manual.owns==eligible,
            "only a cardinal pair with uninterrupted native owner continuity starts after ground readiness");
        if(eligible){
            check(ready.right_y==(sign>0?-20000:20000),
                "actual ground readiness retains the prepared Manual or Nose direction without a new press");
            const auto* dispatched=f.input.input_diagnostic(0);
            check(dispatched->dispatched==1 && dispatched->dispatch[0].disposition==InputDisposition::started &&
                dispatched->dispatch[0].request.recognized_at==5000+2*step,
                "ground readiness dispatches the original queued request without renewing recognition");
        }
        else check(!f.poll(ready_at+step,last_buttons,last_y).manual.owns,
            "diagonal endpoints or retired native authority cannot restart from a held terminal");
    }
}
}
int main(){
    for(unsigned step:{17u,34u}){
        Fixture f;f.flip.id=2;f.poll(1000);check(!f.poll(1000+step,0x4000).authored_playback,"air flip waits while the real native selection is occupied");
        f.native.state=100;f.native.words[1]=0xc000;f.flip.id=-1;
        check(!f.poll(1000+2*step).authored_playback,"native landing retires the old queued airborne request despite a stale air caller pose");
        check(!f.poll(1000+3*step,0x4000).authored_playback,"fresh X on native ground cannot use a stale caller air pose to execute an air flip");
        f.native.state=200;f.native.words[1]=0;
        check(!f.poll(1000+4*step,0x4000).authored_playback,"landing then relaunch cannot replay the held ground press or old waiting request");
        f.poll(1000+5*step);
        check(f.poll(1000+6*step,0x4000).requested_id==2,"a genuinely fresh press in independently observed air still uses the established native mapping");
    }
    Fixture air;air.caller=SkatePose::riding;air.poll(2000);
    check(!air.poll(2017,0x1000).authored_playback,"actual air rejects stale grounded setup permission");
    // The flip fixture holds Left; this pair needs cardinal Up and Down.
    Fixture recovery;recovery.left_x=0;recovery.caller=SkatePose::riding;recovery.native.state=101;recovery.prepare=true;
    recovery.poll(3000);recovery.poll(3017,0,25000);
    check(!recovery.poll(3034,0,-25000).manual.owns,"verified native revert preparation recognizes without executing");
    recovery.native.state=100;recovery.native.words[1]=0xc000;recovery.prepare=false;
    check(recovery.poll(3051,0,-25000).manual.owns,"source-supported preparation still dispatches the recognized pair after actual ground readiness");
    Fixture unknown;unknown.native.state=999;const auto unavailable=unknown.poll(4000,0x4000);
    check(unavailable.buttons==0x4000&&unavailable.right_x==321&&!unavailable.authored_playback,"unknown native state restores physical channels rather than guessing air/ground eligibility");
    preparation_directions();
    std::cout<<checks<<" native pose reconciliation checks passed\n";
}

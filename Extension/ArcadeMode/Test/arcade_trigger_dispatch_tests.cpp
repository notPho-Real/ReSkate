#include "Extension/ArcadeMode/arcade_input_dispatch.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* message) {
    ++checks;if(!value){std::fprintf(stderr,"trigger dispatch: %s\n",message);std::exit(1);}
}
struct Fixture {
    UnifiedInput input;
    InputEnvironment environment{InputContext::ground,true};
    PhysicalInputSample pad{};
    std::vector<InputRequest> requests;
    TriggerOwner qualification{};
    unsigned qualifier_calls{};
    bool ready{true};
    Fixture() {
        environment.native_context={true,true,{1,1,1},1,0};
        environment.native_context.flip={true,-1,100,100,1,0,1,1,1};
        environment.triggers={this,[](void* context,const TriggerPress& press,const InputNativeContext&,std::uint64_t) noexcept {
            auto& f=*static_cast<Fixture*>(context);++f.qualifier_calls;
            return TriggerDecision{f.qualification,press.press.at+500,f.ready};
        }};
    }
    void sample(std::uint64_t now) {
        auto& native=environment.native_context;
        native.sampled_at=now;native.physics_step=now+1;
        input.poll(now,pad,environment,{this,[](void* context,const InputRequest& request) noexcept {
            auto& f=*static_cast<Fixture*>(context);f.requests.push_back(request);
            return request.action==InputAction::revert&&!f.ready?InputDisposition::waiting:InputDisposition::started;
        }});
    }
    void air() {
        environment.context=InputContext::air;
        environment.native_context.airborne=true;
        environment.native_context.airborne_episode=1;
        environment.native_context.flip.state=200;
    }
    void ground() {
        environment.context=InputContext::ground;
        environment.native_context.airborne=false;
        environment.native_context.flip.state=100;
    }
};
void ground_edges() {
    for(unsigned cadence:{8u,17u,34u})for(unsigned strength:{31u,80u,128u,255u}) {
        Fixture f;f.environment.native_context.airborne_episode=3;f.environment.native_context.air_departure=3;
        f.sample(0);f.pad.rt=static_cast<std::uint8_t>(strength);f.sample(cadence);
        check(f.requests.size()==1&&f.requests[0].action==InputAction::switch_stance,"fresh ground RT requests Switch, never Revert");
        const auto first=f.requests[0];
        check(first.trigger.owner==TriggerOwner::switch_stance&&first.physical_pressed_at==cadence&&
            first.trigger.native.generations.owner==1&&first.input_sequence==first.trigger.press.sequence,"Switch retains original edge and owner");
        for(unsigned i=2;i<20;++i)f.sample(cadence*i);
        check(f.requests.size()==1,"held RT does not repeatedly toggle stance");
        f.pad.rt=30;f.sample(cadence*20);f.pad.rt=static_cast<std::uint8_t>(strength);f.sample(cadence*21);
        check(f.requests.size()==2&&f.requests.back().input_sequence>first.input_sequence,"release then second RT requests return stance");
    }
}
void setup_survives_riding_push() {
    for(unsigned foot:{0u,0x20u})for(bool select_nollie:{false,true}) {
        Fixture f;f.environment.native_context.flip.stance=foot;f.sample(0);
        if(select_nollie){f.pad.lt=255;f.sample(10);f.pad.lt=0;f.sample(90);}
        const auto expected=select_nollie?SetupStance::nollie:SetupStance::normal;
        f.environment.native_context.flip.stance=foot|0x40u;++f.environment.native_context.generations.action;
        for(unsigned at=100;at<600;at+=17)f.sample(at);
        check(f.input.selected_setup()==expected && f.requests.empty(),
            "unified setup survives native push without LT or push creating a jump");
        f.environment.native_context.flip.stance=foot;++f.environment.native_context.generations.action;f.sample(600);
        f.pad.buttons=TrickControls::mask(TrickControls::ollie);f.sample(700);
        f.pad.buttons=0;f.sample(900);
        check(f.requests.size()==2 && f.requests[0].action==InputAction::crouch &&
            f.requests[1].action==InputAction::ollie_release && f.requests[0].setup==expected &&
            f.requests[1].setup==expected && f.requests[1].nollie==select_nollie &&
            f.requests[1].physical_pressed_at==700 && f.requests[1].physical_released_at==900,
            "A keeps its original timestamps and selected Ollie or Nollie identity after native push");
        f.environment.conflict=true;f.sample(901);f.environment.conflict=false;f.sample(902);
        check(f.input.selected_setup()==SetupStance::normal,"bail or conflicting native ownership still clears setup");
    }
}
void contextual_precedence() {
    {
        Fixture f;f.environment.context=InputContext::balance;f.environment.manual_active=true;
        f.sample(0);f.pad.rt=255;f.sample(10);f.environment.manual_pivot_admitted=true;f.sample(20);
        check(f.requests.empty(),"a pending Manual does not gain Pivot authority retroactively while RT stays held");
    }
    for(auto qualified:{TriggerOwner::landing_revert,TriggerOwner::transfer}) {
        Fixture f;f.environment.context=InputContext::balance;f.environment.manual_pivot_admitted=true;
        f.environment.native_context.airborne_episode=3;f.environment.native_context.air_departure=3;
        f.qualification=qualified;f.sample(0);f.pad.rt=255;f.sample(10);
        check(f.qualifier_calls==0&&f.requests.size()==1&&f.requests[0].action==InputAction::manual_pivot,
            "established Manual owns RT before vert or ground commands");
    }
    for(unsigned channel:{0u,1u}) {
        Fixture f;f.qualification=TriggerOwner::landing_revert;f.ready=false;f.sample(0);
        (channel?f.pad.rt:f.pad.lt)=255;f.sample(10);
        check(f.requests.size()==1&&f.requests[0].action==InputAction::revert&&
            f.requests[0].trigger.owner==TriggerOwner::landing_revert,"qualified touchdown reserves either trigger before setup or Switch");
        check(f.input.selected_setup()==SetupStance::normal,"landing reservation never selects Nollie");
        f.ready=true;f.sample(40);
        check(f.requests.size()==2&&f.requests[1].input_sequence==f.requests[0].input_sequence&&
            f.requests[1].physical_pressed_at==10&&f.requests[1].deadline==510,"reservation retries exact original edge without renewing deadline");
        f.sample(50);check(f.requests.size()==2,"completed reservation consumes held trigger exclusively");
    }
}
void no_landing_replay() {
    for(unsigned channel:{0u,1u})for(auto qualified:{TriggerOwner::none,TriggerOwner::transfer,TriggerOwner::landing_revert}) {
        Fixture f;f.air();f.qualification=qualified;f.sample(0);
        (channel?f.pad.rt:f.pad.lt)=255;f.sample(10);
        const auto calls=f.requests.size();f.ground();f.qualification=TriggerOwner::none;f.sample(20);
        (channel?f.pad.rt:f.pad.lt)=0;f.sample(30);
        check(f.requests.size()==calls&&f.input.selected_setup()==SetupStance::normal,
            "airborne press and release cannot replay as a ground setup or Switch");
        (channel?f.pad.rt:f.pad.lt)=255;f.sample(40);
        (channel?f.pad.rt:f.pad.lt)=0;f.sample(50);
        check(channel?f.requests.size()==calls+1:f.input.selected_setup()==SetupStance::nollie,
            "a new physical ground cycle remains usable after landing");
    }
    Fixture f;f.air();f.qualification=TriggerOwner::landing_revert;f.ready=false;f.sample(0);f.pad.rt=255;f.sample(10);
    f.ground();f.ready=true;f.sample(30);
    check(f.requests.size()==2&&f.requests.back().context==InputContext::air&&
        f.requests.back().trigger.native.airborne,"reserved landing action alone carries original AIR context to ground");
}
void interruption() {
    for(unsigned kind:{0u,1u,2u})for(unsigned channel:{0u,1u}) {
        Fixture f;f.sample(0);(channel?f.pad.rt:f.pad.lt)=255;f.sample(10);
        const auto calls=f.requests.size();
        if(kind==0)f.environment.gameplay=false;
        if(kind==1)++f.environment.native_context.generations.owner;
        if(kind==2)f.environment.conflict=true;
        f.sample(20);f.environment.gameplay=true;f.environment.conflict=false;f.sample(30);
        check(f.requests.size()==calls&&f.input.selected_setup()==SetupStance::normal,
            "OFF, owner replacement and conflict cancel selection without replaying held inputs");
        (channel?f.pad.rt:f.pad.lt)=0;f.sample(40);(channel?f.pad.rt:f.pad.lt)=255;f.sample(50);
        (channel?f.pad.rt:f.pad.lt)=0;f.sample(60);
        check(channel?f.requests.size()==calls+1:f.input.selected_setup()==SetupStance::nollie,
            "interruption permits only a new physical cycle");
    }
}
}
int main() {setup_survives_riding_push();ground_edges();contextual_precedence();no_landing_replay();interruption();
    std::printf("trigger dispatch: %u checks passed\n",checks);}

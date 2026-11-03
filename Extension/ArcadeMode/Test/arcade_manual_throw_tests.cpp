#include "Extension/ArcadeMode/arcade_input_dispatch.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{}, attempts{};
void check(bool ok,const char* why) { ++checks; if(!ok) {std::cerr<<why<<'\n';std::exit(1);} }
struct Sink {
    unsigned count{}; InputAction action{};
    static InputDisposition receive(void* context,const InputRequest& r) noexcept {
        auto& s=*static_cast<Sink*>(context);
        if(r.action==InputAction::manual || r.action==InputAction::nose_manual) {++s.count;s.action=r.action;}
        return InputDisposition::unsupported;
    }
    InputAdapter adapter() {return {this,receive};}
};
void trajectories() {
    for(unsigned hz:{30u,60u,120u}) for(int sign:{-1,1})
    for(auto context:{InputContext::ground,InputContext::air}) for(unsigned attempt=0;attempt<20;++attempt) {
        UnifiedInput input;Sink sink;InputEnvironment env{context,true};env.policy=InputPolicy::production();
        const unsigned first=100+attempt%7, second=first+80+(attempt%5)*65;
        for(unsigned step=0;step<=hz;++step) {
            const auto now=std::uint64_t{step}*1000/hz;
            PhysicalInputSample p{};
            if(now>=first) p={0,static_cast<short>(attempt%2?21000:0),static_cast<short>(-sign*15000)};
            if(attempt%3 && now>=second-30 && now<second) p={};
            if(now>=second) p={0,static_cast<short>(attempt%2?-28000:4000),static_cast<short>(sign*24000)};
            input.poll(now,p,env,sink.adapter());const auto count=sink.count;
            input.poll(now,p,env,sink.adapter());check(count==sink.count,"duplicate poll created a second request");
        }
        check(sink.count==1,"two deliberate physical strokes must recognize exactly once");
        check(sink.action==(sign<0?InputAction::manual:InputAction::nose_manual),"manual direction reversed");
        ++attempts;
    }
}
void negatives() {
    for(unsigned hz:{30u,60u,120u}) for(unsigned kind=0;kind<8;++kind) {
        UnifiedInput input;Sink sink;InputEnvironment env{InputContext::ground,true};env.policy=InputPolicy::production();
        for(unsigned step=0;step<=hz*10;++step) {
            const auto now=std::uint64_t{step}*1000/hz;
            PhysicalInputSample p{};
            if(now>=100 || kind==0) p.y=24000;
            if(now>=300) p.y=-24000;
            if(kind==1) p.x=32000,p.y=static_cast<short>(now<300?12000:-12000);
            if(kind==2) p.y=24000;
            if(kind==3 && now>=200 && now<300) p.buttons=0x4000;
            if(kind==4) env.manual_active=true;
            if(kind==5) p.y=static_cast<short>(now<100?0:now<700?24000:-24000);
            if(kind==6 && now>=200 && now<300) env.gameplay=false;else env.gameplay=true;
            if(kind==7) p.x=32000,p.y=static_cast<short>(now<300?18000:-18000);
            input.poll(now,p,env,sink.adapter());
        }
        check(sink.count==0,"startup hold, steering, held direction, conflict, active balance, timeout or OFF made a manual");
    }
}
}
int main() { trajectories();negatives();std::cout<<attempts<<" physical trajectories, "<<checks<<" checks passed; native gameplay unverified\n"; }

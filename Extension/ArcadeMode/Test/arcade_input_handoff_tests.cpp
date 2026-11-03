#include "Extension/ArcadeMode/arcade_input_dispatch.h"
#include <iostream>
#include <vector>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{}, failures{};
void check(bool value,const char* reason){++checks;if(!value){++failures;if(failures<12)std::cerr<<"FAIL: "<<reason<<'\n';}}
struct Sink {
    std::vector<InputRequest> requests;
    static InputDisposition receive(void* p,const InputRequest& request) noexcept {
        if(request.action==InputAction::manual||request.action==InputAction::nose_manual)
            static_cast<Sink*>(p)->requests.push_back(request);
        return InputDisposition::unsupported;
    }
    InputAdapter adapter(){return{this,receive};}
};
PhysicalInputSample sample(int sign,std::uint16_t pad=0,std::int16_t x=0){return{pad,x,static_cast<std::int16_t>(sign*25000)};}
void effective_direction_pairs(){
    for(std::uint64_t cadence:{17u,34u})for(std::uint64_t phase=0;phase<cadence;++phase)
    for(int sign:{-1,1})for(unsigned travel=0;travel<4;++travel){
        UnifiedInput input;Sink sink;InputEnvironment env{InputContext::ground,true};
        std::uint64_t first{},last{};
        for(std::uint64_t now=0;now<1100;now+=cadence){
            auto physical=now>=20+phase?sample(sign):PhysicalInputSample{};
            if(now>=600+phase&&now<600+phase+2*cadence){
                physical.buttons=static_cast<std::uint16_t>(sign>0?2:1);
                if(!first)first=now;
                // Stick travel hidden by the physical pad cannot add a command.
                if(travel==2)physical.x=20000;
                if(travel==3)physical.buttons=static_cast<std::uint16_t>(physical.buttons|0x0040u);
            }
            if(now>=600+phase+2*cadence&&!last){
                if(travel==1){physical.y=0;last=now+cadence;}
                else if(travel==2){physical.x=20000;last=now;}
                else last=now;
            }
            input.poll(now,physical,env,sink.adapter());
            const auto count=sink.requests.size();input.poll(now,physical,env,sink.adapter());
            check(sink.requests.size()==count,"duplicate poll cannot repeat or retimestamp a logical pair");
        }
        check(sink.requests.size()==1,"aged held stick plus opposite pad excursion recognizes one newly effective logical pair");
        if(sink.requests.size()==1){
            const auto& request=sink.requests[0];
            check(request.action==(sign>0?InputAction::nose_manual:InputAction::manual)&&request.first_input_at==first&&
                request.last_input_at==last&&request.recognized_at==last,"both signs retain actual sampled pair endpoints through neutral, diagonal and unrelated input");
        }
        unsigned used{};for(const auto& event:input.history().events()){
            check(event.token.source!=InputSource::pad&&event.token.source!=InputSource::stick&&event.token.source!=InputSource::vertical_pad&&
                event.token.source!=InputSource::vertical_stick,"handoff stays in one physical command journal");
            if(event.owners&InputConsumption::manual){++used;check(event.at==first||event.at==last,"only the two actual pair presses receive Manual ownership");}
            if(event.token.source==InputSource::buttons)check(!event.owners,"permitted intervening shoulder remains unconsumed");
        }
        check(used==2,"successful handoff consumes exactly two canonical direction presses");
    }
}
void no_new_logical_edge(){
    for(std::uint64_t cadence:{17u,34u})for(int sign:{-1,1})for(unsigned kind=0;kind<5;++kind){
        UnifiedInput input;Sink sink;InputEnvironment env{InputContext::ground,true};
        for(std::uint64_t now=0;now<1100;now+=cadence){
            auto physical=sample(sign);
            if(kind==1&&now>=400&&now<500)physical.buttons=static_cast<std::uint16_t>(sign>0?1:2);
            if(kind==2&&now>=400&&now<500)physical.x=20000; // only diagonal, never an opposite cardinal
            if(kind==3&&now>=400)physical.buttons=0x40;
            if(kind==4&&now>=400&&now<950)physical.buttons=static_cast<std::uint16_t>(sign>0?2:1); // expired opposite prefix
            input.poll(now,physical,env,sink.adapter());
        }
        check(sink.requests.empty(),"startup-held, no excursion, same logical pad direction, diagonal travel, unrelated input and expired prefix cannot manufacture a pair");
        if(kind==0||kind==1||kind==3){
            unsigned presses{};for(const auto& event:input.history().events())
                presses+=event.token.source==InputSource::direction&&event.token.edge==InputEdge::press;
            check(!presses,"unchanged effective direction stays baselined even when its physical source changes");
        }
    }
}
void cancelled_handoff(){
    for(std::uint64_t cadence:{17u,34u})for(int sign:{-1,1})for(unsigned loss=0;loss<4;++loss){
        UnifiedInput input;Sink sink;InputEnvironment env{InputContext::ground,true};std::uint64_t step{};
        const auto poll=[&](std::uint64_t now,PhysicalInputSample physical){
            env.native_context={true,true,{loss==0&&now>=500?2u:1u,1,1},++step,now,false,false,0,0};
            input.poll(now,physical,env,sink.adapter());
        };
        poll(0,sample(sign));poll(400,sample(sign,static_cast<std::uint16_t>(sign>0?2:1)));
        if(loss==1)env.gameplay=false;
        if(loss==2)env.context=InputContext::unknown;
        if(loss==3)env.conflict=true;
        poll(500,sample(sign,static_cast<std::uint16_t>(sign>0?2:1)));
        env.gameplay=true;env.context=InputContext::ground;env.conflict=false;
        poll(500+cadence,sample(sign));poll(500+2*cadence,sample(sign));
        check(sink.requests.empty(),"owner, OFF, unknown and intent conflict discard an earlier prefix while keeping held baselines");
        poll(600,sample(sign,static_cast<std::uint16_t>(sign>0?2:1)));poll(600+cadence,sample(sign));
        check(sink.requests.size()==1,"fresh logical reversal after cancellation rearms through the same physical selector");
    }
}
void used_prefix_and_diagonal(){
    for(int sign:{-1,1}){
        UnifiedInput input;Sink sink;InputEnvironment env{InputContext::ground,true};
        input.poll(0,sample(sign),env,sink.adapter());input.poll(400,sample(sign,static_cast<std::uint16_t>(sign>0?2:1)),env,sink.adapter());
        input.poll(420,sample(sign),env,sink.adapter());input.poll(440,sample(sign,static_cast<std::uint16_t>(sign>0?2:1)),env,sink.adapter());
        check(sink.requests.size()==1,"a successful handoff cannot donate its consumed terminal to a reverse Manual");
        input.poll(460,sample(sign),env,sink.adapter());
        check(sink.requests.size()==2,"a later complete fresh pair can rearm without clearing category ownership");
        UnifiedInput diagonal;Sink other;diagonal.poll(0,{},env,other.adapter());
        diagonal.poll(400,{0,31000,static_cast<short>(-sign*18000)},env,other.adapter());diagonal.poll(420,sample(sign),env,other.adapter());
        check(other.requests.empty(),"steering-dominant diagonal cannot supply a Manual prefix");
    }
}
void manual_cone_qualification(){
    for(unsigned hz:{30u,60u,120u})for(int sign:{-1,1}){
        UnifiedInput input;Sink sink;InputEnvironment env{InputContext::ground,true};
        env.policy=InputPolicy::production();
        std::uint64_t first{},last{},qualified{};
        for(unsigned step=0;step<hz;++step){
            const auto now=std::uint64_t{step}*1000/hz;
            PhysicalInputSample physical{};
            if(now>=100){physical={0,31000,static_cast<short>(-sign*19000)};if(!first)first=now;}
            if(now>=140)physical={0,22000,static_cast<short>(-sign*25000)};
            if(now>=220){physical={0,31000,static_cast<short>(sign*19000)};if(!last)last=now;}
            if(now>=260){physical={0,22000,static_cast<short>(sign*25000)};if(!qualified)qualified=now;}
            input.poll(now,physical,env,sink.adapter());
            const auto count=sink.requests.size();input.poll(now,physical,env,sink.adapter());
            check(sink.requests.size()==count,"duplicate cone sample cannot add a Manual request");
        }
        check(sink.requests.size()==1,"entering the Manual cone inside each unchanged diagonal recognizes one opposite pair");
        if(sink.requests.size()==1){const auto&r=sink.requests[0];
            check(r.first_input_at==first && r.last_input_at==last && r.recognized_at==qualified,
                "cone qualification preserves physical edge timestamps rather than backdating or renewing them");
            check(r.action==(sign>0?InputAction::nose_manual:InputAction::manual),"cone qualification chooses the correct manual");
        }
        check(input.history().events().size()==3,"cone entry creates no extra shared controller edges");
        for(const auto&e:input.history().events())
            check(e.at==first||e.at==last,"all shared event timestamps still describe actual eight-way changes");
    }
    for(unsigned invalid=0;invalid<5;++invalid){
        UnifiedInput input;Sink sink;InputEnvironment env{InputContext::ground,true};env.policy=InputPolicy::production();
        input.poll(0,invalid==0?PhysicalInputSample{0,31000,-19000}:PhysicalInputSample{},env,sink.adapter());
        input.poll(100,{0,31000,-19000},env,sink.adapter());
        input.poll(140,{0,22000,-25000},env,sink.adapter());
        if(invalid==1)input.reset_manual_continuity();
        const unsigned second=invalid==2?700:220;
        input.poll(second,{0,31000,19000},env,sink.adapter());
        if(invalid==3)env.policy=InputPolicy::strict();
        input.poll(second+40,{0,invalid==4?short{31000}:short{22000},invalid==4?short{19000}:short{25000}},env,sink.adapter());
        check(sink.requests.empty(),"startup held, cancelled, expired, strict and steering-only throws do not gain Manual authority");
    }
    InputEvents journal;const auto policy=InputPolicy::production();
    journal.sample(0,{},false,false,policy);journal.sample(10,{0,31000,19000},false,false,policy);
    const auto edge=journal.history().events()[0];
    journal.history().consume(InputMatch::single(edge),InputConsumption::regular);
    journal.sample(20,{0,22000,25000},false,false,policy);
    check(journal.history().events()[0].manual_vertical==0,"an edge consumed by another trick cannot gain manual qualification");
    InputEvents jitter;jitter.sample(0,{},false,false,policy);jitter.sample(10,{0,31000,19000},false,false,policy);
    jitter.sample(20,{0,22000,25000},false,false,policy);jitter.sample(30,{0,31000,19000},false,false,policy);
    jitter.sample(40,{0,22000,25000},false,false,policy);
    check(jitter.history().events().size()==1 && jitter.history().events()[0].at==10 &&
        jitter.history().events()[0].manual_qualified_at==20,"cone jitter cannot renew a throw or its qualification clock");
}
}
int main(){effective_direction_pairs();no_new_logical_edge();cancelled_handoff();used_prefix_and_diagonal();manual_cone_qualification();
    std::cout<<checks<<" logical handoff checks, "<<failures<<" failed\n";return failures?1:0;}

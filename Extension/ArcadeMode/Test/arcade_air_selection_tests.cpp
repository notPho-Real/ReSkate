#include "Extension/ArcadeMode/arcade_input_session.h"
#include <iostream>
#include <vector>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{}, failures{};
void check(bool value,const char* reason) {
    ++checks; if (!value) { ++failures; if (failures < 15) std::cerr << "FAIL: " << reason << '\n'; }
}
struct Sink {
    std::vector<InputRequest> requests;
    InputDisposition result{InputDisposition::unsupported};
    static InputDisposition receive(void* p,const InputRequest& r) noexcept {
        auto& self = *static_cast<Sink*>(p); self.requests.push_back(r); return self.result;
    }
    InputAdapter adapter() noexcept { return {this,receive}; }
    const InputRequest* last(InputAction action) const {
        for (auto i=requests.rbegin();i!=requests.rend();++i) if (i->action==action) return &*i;
        return nullptr;
    }
};
PhysicalInputSample direction(int code,std::uint16_t buttons=0,bool pad=false) {
    const int x=code==4||code==7||code==1?-1:code==6||code==9||code==3?1:0;
    const int y=code>=7?1:code>=1&&code<=3?-1:0;
    if (pad) return {static_cast<std::uint16_t>(buttons|(x<0?4:x>0?8:0)|(y>0?1:y<0?2:0))};
    return {buttons,static_cast<std::int16_t>(x*25000),static_cast<std::int16_t>(y*25000)};
}
void buffered_rows() {
    for (int code : {4,8,6,2,7,9,1,3}) for (bool nollie : {false,true}) for (bool pad : {false,true})
    for (std::uint64_t age : {499u,500u,501u}) {
        UnifiedInput input; Sink sink; InputEnvironment env{InputContext::air,true}; env.nollie=nollie;
        input.poll(0,{},env,sink.adapter()); input.poll(10,{0x4000},env,sink.adapter());
        input.poll(20,{0x2000},env,sink.adapter()); input.poll(30,{},env,sink.adapter());
        input.poll(10+age,direction(code,0,pad),env,sink.adapter());
        const auto* request=sink.last(InputAction::flip);
        check((request!=nullptr)==(age<500),"every installed direction preserves an unconsumed face across an independent face action with strict500ms age");
        check(age>=500 ? !request : request && request->direction==code && request->presses==1 && request->nollie==nollie &&
            request->first_input_at==10 && request->last_input_at==10+age &&
            request->physical_pressed_at==10 && request->physical_released_at==20,
            "buffered selection retains exact direction, stance and physical footprint times");
    }
    for (std::uint64_t cadence : {17u,34u}) for (std::uint64_t phase=0;phase<cadence;++phase) {
        UnifiedInput input; Sink sink; InputEnvironment env{InputContext::air,true};
        for (std::uint64_t now=phase;now<700;now+=cadence) {
            const auto buttons=static_cast<std::uint16_t>(now>=100&&now<150?0x4000:now>=160&&now<210?0x2000:0);
            input.poll(now,direction(now>=300?7:0,buttons),env,sink.adapter());
            input.poll(now,direction(now>=300?7:0,buttons),env,sink.adapter());
        }
        unsigned flips{};for (const auto& request:sink.requests) if (request.action==InputAction::flip) ++flips;
        check(flips==1 && sink.last(InputAction::flip)->direction==7,
            "all30/60Hz phases and duplicate polls select the buffered supported diagonal once");
    }
    for (int code : {4,8,6,2,7,9,1,3}) {
        UnifiedInput input; Sink sink; InputEnvironment env{InputContext::air,true}; env.named_grabs=true;
        input.poll(0,{},env,sink.adapter()); input.poll(10,{0x2000},env,sink.adapter());
        check(!sink.last(InputAction::grab),"named grab does not consume an unmapped neutral face before its directional row can match");
        input.poll(100,direction(code),env,sink.adapter());
        check(sink.last(InputAction::grab) && sink.last(InputAction::grab)->direction==code &&
            sink.last(InputAction::grab)->first_input_at==10,"named directional grab is recognized later without claiming native support");
    }
}
void cancellation_boundaries() {
    for (unsigned boundary=0;boundary<6;++boundary) {
        UnifiedInput input; Sink sink; InputEnvironment env{InputContext::air,true};
        if (boundary==4) env.context=InputContext::ground;
        if (boundary==5) env.native_context={true,true,{1,1,1},1,0,false};
        input.poll(0,{},env,sink.adapter());
        if (boundary==5) env.native_context={true,true,{1,1,1},2,10,false};
        input.poll(10,{0x4000},env,sink.adapter());
        if (boundary==0) { env.gameplay=false; input.poll(20,{},env,sink.adapter()); env.gameplay=true; }
        if (boundary==1) input.poll(20,{0x10},env,sink.adapter());
        if (boundary==2) { env.conflict=true; input.poll(20,{},env,sink.adapter()); env.conflict=false; }
        if (boundary==3) { env.context=InputContext::ground; input.poll(20,{},env,sink.adapter()); }
        env.context=InputContext::air;
        if (boundary==5) env.native_context={true,true,{1,1,2},3,100,false};
        input.poll(100,direction(4),env,sink.adapter());
        check(!sink.last(InputAction::flip),"hard/intent/native-action/air-entry boundaries cannot borrow a discarded face from the old context");
    }
}
template<class Request>
bool selected(const Request& request,ArcadeTrick trick,float speed,unsigned slack) {
    if constexpr (requires { request.air.trick; })
        return request.air.trick==trick && request.air.authored_speed==speed && request.air.trick_slack==slack &&
            request.air.extra && request.air.use_current && request.air.query_ms()==300;
    return false;
}
template<class Request>
bool grab_selection(const Request& request,ArcadeTrick trick,float speed,unsigned percent,bool force) {
    if constexpr (requires { request.air.trick; })
        return request.air.trick==trick && request.air.authored_speed==speed &&
            request.air.minimum_percent==percent && request.air.force_init==force &&
            request.air.extra && !request.air.use_current && request.air.release_mask()==0x2000;
    return false;
}
void extra_row_predicates() {
    const int codes[]{4,8,6,2,7,9,1,3};
    const ArcadeTrick destinations[]{ArcadeTrick::double_kickflip,ArcadeTrick::double_impossible,
        ArcadeTrick::double_heelflip,ArcadeTrick::shove_it_360,ArcadeTrick::hardflip_360,
        ArcadeTrick::inward_heelflip_360,ArcadeTrick::flip_360,ArcadeTrick::heelflip_360};
    const float speeds[]{1,1.2f,1,1,1.1f,1,.9f,1.3f}; const unsigned slacks[]{15,15,15,10,20,15,10,20};
    for (unsigned i=0;i<8;++i) for (bool nollie : {false,true}) {
        UnifiedInput input; Sink sink; sink.result=InputDisposition::started;
        InputEnvironment env{InputContext::air,true};env.nollie=nollie;
        input.poll(0,{},env,sink.adapter());input.poll(10,direction(codes[i]),env,sink.adapter());
        input.poll(20,direction(codes[i],0x4000),env,sink.adapter());
        input.poll(30,{},env,sink.adapter());input.poll(100,direction(codes[i]==6?4:6),env,sink.adapter());
        input.poll(110,direction(codes[i]==6?4:6,0x4000),env,sink.adapter());
        const auto* extra=sink.last(InputAction::flip);
        check(extra && extra->presses==2 && extra->direction==codes[i] && extra->nollie==nollie,
            "installed core extra uses plain face Press and retains parent direction despite a fresh different direction");
        if (extra) check(selected(*extra,destinations[i],speeds[i],slacks[i]),
            "active extra exports the existing semantic destination and per-row speed/slack/continuation declaration");
        bool exact_face{};for(const auto& event:input.history().events())
            if(event.at==110 && event.token.source==InputSource::buttons) exact_face=event.owners==InputConsumption::extra;
        check(exact_face,"plain extra Press consumes exactly its face category rather than a direction-tweak footprint");
        bool independent_direction{};for(const auto& event:input.history().events())
            if(event.at==100 && event.token.source==InputSource::direction && event.token.edge==InputEdge::press)
                independent_direction=event.owners==0;
        check(independent_direction,"a plain face extra leaves the independent new direction available to other table rows");
    }
    const ArcadeTrick grabs[]{ArcadeTrick::method,ArcadeTrick::rocket_air,ArcadeTrick::stiffy,
        ArcadeTrick::one_foot_tailgrab,ArcadeTrick::one_foot_japan,ArcadeTrick::judo,ArcadeTrick::sacktap,ArcadeTrick::christ_air};
    const float grab_speeds[]{1.2f,1,1.25f,1,1.3f,1,1.5f,1};
    for (unsigned i=0;i<8;++i) {
        UnifiedInput input;Sink sink;sink.result=InputDisposition::started;
        InputEnvironment env{InputContext::air,true};env.named_grabs=true;
        input.poll(0,{},env,sink.adapter());input.poll(10,direction(codes[i]),env,sink.adapter());
        input.poll(20,direction(codes[i],0x2000),env,sink.adapter());input.poll(30,{},env,sink.adapter());
        input.poll(100,direction(codes[i]==6?4:6),env,sink.adapter());input.poll(110,direction(codes[i]==6?4:6,0x2000),env,sink.adapter());
        const auto* extra=sink.last(InputAction::grab);
        check(extra && extra->presses==2 && extra->direction==codes[i],
            "every plain core grab extra is distinct from the separately supported direction-tweak grammar");
        if (extra) check(grab_selection(*extra,grabs[i],grab_speeds[i],i==4?70u:60u,i==2||i==6||i==7),
            "per-row named grab selection preserves speed, release and initialization declarations without inventing execution");
    }
}
void actual_session() {
    InputSession session;InputAdmission admission{};
    admission.gameplay=admission.authored=admission.action_fresh=admission.inspected_stance=true;
    admission.pose=SkatePose::air;admission.action_mode=ActionExperiment::classic;
    const std::array catalog{PreparedGesture{2,false,GestureContext::ground,
        std::vector<AuthoredStickStep>{{0,0,-32767},{18,28000,28000},{21,0,0},{24,0,0}}}};
    admission.catalog=catalog;
    session.poll(0,{},0,0,admission);session.poll(10,{0x4000},0,0,admission);
    session.poll(20,{0x2000},0,0,admission);session.poll(30,{},0,0,admission);
    const auto out=session.poll(100,direction(4),0,0,admission);
    check(out.authored_playback && out.requested_id==2,"the production Session reaches the existing exact native recipe for the recovered buffered input");
    InputSession named;admission.named_grab_bindings=true;
    named.poll(0,{},0,0,admission);named.poll(10,{0x2000},0,0,admission);
    const auto unavailable=named.poll(100,direction(4),0,0,admission);
    check(!unavailable.action.owned && !unavailable.authored_playback &&
        unavailable.action.result==DispatchResult::unavailable && unavailable.action.trick==ArcadeTrick::melon,
        "buffered named selection remains explicitly unavailable and cannot substitute the basic native grab");
}
}
int main() {
    buffered_rows();cancellation_boundaries();extra_row_predicates();actual_session();
    std::cout<<checks<<" air selection checks, "<<failures<<" failed\n";return failures?1:0;
}

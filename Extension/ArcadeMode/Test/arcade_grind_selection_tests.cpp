#include "Extension/ArcadeMode/arcade_input_session.h"
#include <iostream>
#include <vector>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{},failures{};
void check(bool value,const char* reason){++checks;if(!value){++failures;if(failures<16)std::cerr<<"FAIL: "<<reason<<'\n';}}
struct Sink {
    std::vector<InputRequest> requests;
    bool wait{};
    static InputDisposition receive(void* p,const InputRequest& r) noexcept {
        auto& self=*static_cast<Sink*>(p);self.requests.push_back(r);
        return self.wait && (r.action==InputAction::rail_select||r.action==InputAction::rail_change||r.action==InputAction::lip_change)
            ? InputDisposition::waiting:InputDisposition::unsupported;
    }
    InputAdapter adapter(){return{this,receive};}
    unsigned count(InputAction action)const{unsigned n{};for(const auto&r:requests)n+=r.action==action;return n;}
};
PhysicalInputSample direction(int code,std::uint16_t buttons=0,bool pad=false){
    const int x=code==4||code==7||code==1?-1:code==6||code==9||code==3?1:0;
    const int y=code>=7?1:code>=1&&code<=3?-1:0;
    return pad?PhysicalInputSample{static_cast<std::uint16_t>(buttons|(x<0?4:x>0?8:0)|(y>0?1:y<0?2:0))}:
        PhysicalInputSample{buttons,static_cast<std::int16_t>(x*25000),static_cast<std::int16_t>(y*25000)};
}
InputEnvironment rail_environment(InputRailPhase phase){
    InputEnvironment env{phase==InputRailPhase::lip_established||phase==InputRailPhase::lip_selection?InputContext::lip:InputContext::rail,true};
    env.rail.phase=phase;env.rail.generation=7;env.rail.fresh=env.rail.ready=env.rail.geometry_known=true;return env;
}
void pair_boundaries(){
    constexpr std::array<std::string_view,10> names{"","50-50","Noseslide","Nosegrind","Crooked",
        "Bluntslide","Nosebluntslide","Smith","5-0","Tailslide"};
    struct Pair{unsigned table,first,second,target,window;};
    const Pair pairs[]{{0,15,15,1,300},{0,15,14,2,300},{0,15,13,3,300},{0,13,13,4,300},{0,13,14,5,300},
        {0,13,15,6,300},{0,14,14,7,300},{0,14,13,8,300},{0,14,15,9,300},
        {1,13,13,40,300},{1,13,14,41,300},{1,14,14,42,300},{1,14,13,43,300},{1,15,15,44,300},{1,15,14,45,300},{1,15,13,46,300},
        {2,14,14,50,300},{2,15,15,51,300},{2,13,13,52,300},{2,13,14,53,300},{2,14,13,54,300},{2,15,13,55,300},{3,15,15,56,500}};
    for(const auto& row:pairs)for(unsigned age:{row.window-1,row.window,row.window+1})for(bool front:{false,true}){
        UnifiedInput input;Sink sink;auto env=rail_environment(row.table?InputRailPhase::lip_established:InputRailPhase::established);
        env.rail.frontside=front;env.rail.lip_table=row.table?row.table-1:0;env.rail.ready=false;
        input.poll(0,{},env,sink.adapter());input.poll(10,{static_cast<std::uint16_t>(1u<<row.first)},env,sink.adapter());
        input.poll(20,{},env,sink.adapter());input.poll(30,{static_cast<std::uint16_t>(1u<<row.second)},env,sink.adapter());
        check(sink.requests.empty(),"actor unreadiness cannot dispatch established changes");
        env.rail.ready=true;input.poll(10+age,{},env,sink.adapter());
        check(age<row.window?sink.requests.size()==1&&sink.requests[0].variant==row.target:sink.requests.empty(),
            "every installed established pair keeps strict299/300/301 or special499/500/501 first-edge age");
        if (age<row.window && !sink.requests.empty()) {
            const auto target=sink.requests[0].grind_target();
            check(row.table ? target==GrindChangeTarget::none : grind_change_name(target)==names[row.target],
                "established rail targets have explicit names; Lip variants cannot alias them");
        }
        const auto count=sink.requests.size();input.poll(11+age,{},env,sink.adapter());
        check(sink.requests.size()==count,"consumption prevents held or neutral polls from repeating a change");
    }
}
void early_boundaries(){
    for(int code:{9,3,1,7,8,6,4,2})for(unsigned age:{999u,1000u,1001u})for(bool pad:{false,true})for(bool front:{false,true}){
        UnifiedInput input;Sink sink;auto env=rail_environment(InputRailPhase::early);env.rail.frontside=front;env.rail.ready=false;
        input.poll(0,{},env,sink.adapter());input.poll(10,direction(code,0,pad),env,sink.adapter());
        input.poll(20,{0x8000},env,sink.adapter());input.poll(30,{},env,sink.adapter());input.poll(40,{0x8000},env,sink.adapter());
        env.rail.ready=true;input.poll(10+age,{0x8000},env,sink.adapter());
        const auto target=code==9?10u:code==3?11u:code==1?12u:code==7?13u:code==8?14u:code==6?15u:code==4?16u:17u;
        check(age<1000?sink.requests.size()==1&&sink.requests[0].variant==target:sink.requests.empty(),
            "early direction/Y/Y uses the observed phase and strict1000ms first-direction age");
        const auto count=sink.requests.size();input.poll(11+age,{0x8000},env,sink.adapter());
        check(sink.requests.size()==count,"held Y cannot repeat an early change");
    }
}
void table_cancellation(){
    for(unsigned change=0;change<5;++change){
        UnifiedInput input;Sink sink;sink.wait=true;
        auto env=rail_environment(change==2?InputRailPhase::lip_established:InputRailPhase::established);
        input.poll(0,{},env,sink.adapter());input.poll(10,{0x2000},env,sink.adapter());input.poll(20,{},env,sink.adapter());
        input.poll(30,{0x2000},env,sink.adapter());check(!sink.requests.empty(),"pending source-table change was actually recognized");
        sink.requests.clear();
        if(change==0)env.rail.phase=InputRailPhase::early;
        if(change==1)env.rail.excluded_variant=4;
        if(change==2)env.rail.lip_table=1;
        if(change==3)++env.rail.generation;
        if(change==4)env.rail.fresh=false;
        input.poll(40,{0x2000},env,sink.adapter());
        check(sink.requests.empty(),"observed table/parent/episode or qualification change cancels old waiting named work before its adapter");
    }
}
void use_rail_events(){
    for(bool combined:{false,true}){
        UnifiedInput input;Sink sink;InputEnvironment env{InputContext::air,true};env.manual_safe=false;env.manual_prepare=true;
        input.poll(0,{},env,sink.adapter());input.poll(combined?5:10,direction(combined?2:8),env,sink.adapter());
        input.poll(combined?10:20,direction(combined?8:2),env,sink.adapter());
        if(combined){input.poll(15,{},env,sink.adapter());input.poll(20,direction(8),env,sink.adapter());}
        input.poll(30,direction(combined?8:0,0x8000),env,sink.adapter());
        if(combined)input.poll(40,direction(8,0x4000),env,sink.adapter());
        sink.requests.clear();env=rail_environment(InputRailPhase::lip_selection);env.rail.ready=false;env.rail.gameframe=env.rail.selection_frame=100;
        input.poll(100,{},env,sink.adapter());
        bool up{},down{};for(const auto& r:sink.requests)if(r.action==InputAction::lip_select){up=up||r.variant==8;down=down||r.variant==2;}
        check(up&&down,"Lip ordinary matching reuses Manual-only and combined Manual/ExtraGrind categories without clearing their ownership");
        bool retained{};for(const auto& e:input.history().events())
            if(e.at==10&&e.token.source==InputSource::direction&&e.token.edge==InputEdge::press)
                retained=e.owners==(combined?11u:3u);
        check(retained,"ordinary reuse adds Regular while retaining original Manual and ExtraGrind flags");
    }
    for(bool extra:{false,true}){
        UnifiedInput input;Sink sink;auto env=extra?rail_environment(InputRailPhase::early):InputEnvironment{InputContext::ground,true};
        if(!extra){env.jump_binding=InputJumpBinding::single_release;env.jump_generation=1;}
        input.poll(0,{},env,sink.adapter());input.poll(10,direction(8),env,sink.adapter());
        input.poll(20,{static_cast<std::uint16_t>(extra?0x8000:0x1000)},env,sink.adapter());input.poll(30,{},env,sink.adapter());
        if(extra)input.poll(40,{0x8000},env,sink.adapter());
        bool consumed{};for(const auto& event:input.history().events())
            if(event.at==10&&event.token.edge==InputEdge::press)consumed=(event.owners&(extra?InputConsumption::extra:InputConsumption::regular))!=0;
        check(consumed,"negative reuse fixture actually consumes its direction through an ordinary release or contextual extra request");
        sink.requests.clear();env=rail_environment(InputRailPhase::lip_selection);env.rail.gameframe=env.rail.selection_frame=100;
        input.poll(100,{},env,sink.adapter());
        check(sink.count(InputAction::lip_select)==0,"Lip reuse cannot borrow Regular or Extra-owned history when only Manual and ExtraGrind are allowed");
    }
}
void contact_binding(){
    for(unsigned index=0;index<16;++index){
        UnifiedInput input;Sink sink;sink.wait=true;InputEnvironment env{InputContext::air,true};
        env.rail.phase=InputRailPhase::approach;env.rail.fresh=env.rail.ready=env.rail.geometry_known=true;env.rail.generation=5;
        input.poll(0,{},env,sink.adapter());input.poll(10,direction(8),env,sink.adapter());input.poll(20,{},env,sink.adapter());
        input.poll(30,direction(8),env,sink.adapter());input.poll(40,direction(8,0x8000),env,sink.adapter());
        const auto original=input.pending_rail();
        check(sink.count(InputAction::rail_select)==0&&!original.geometry_known&&!original.rail_generation,
            "an approach selection is unbound and cannot reach a named adapter before actual rail context");
        env=rail_environment(InputRailPhase::early);env.rail.right=(index&1u)!=0;env.rail.parallel=(index&2u)!=0;
        env.rail.backwards=(index&4u)!=0;env.rail.regular=(index&8u)!=0;sink.requests.clear();
        input.poll(100,{0x8000},env,sink.adapter());const auto bound=input.pending_rail();
        check(sink.count(InputAction::rail_select)==1&&bound.geometry_known&&bound.geometry==index&&bound.rail_generation==7,
            "dedicated selection binds to the16-way geometry from its first qualified actual rail observation");
        check(bound.recognized_at==original.recognized_at&&bound.deadline==original.deadline&&bound.first_input_at==original.first_input_at&&
            bound.last_input_at==original.last_input_at&&bound.session==original.session&&bound.variant==original.variant,
            "late binding cannot renew the semantic input identity, recognition time or original lifetime");
        ++env.rail.generation;sink.requests.clear();input.poll(110,{0x8000},env,sink.adapter());
        check(input.pending_rail().action==InputAction::none&&sink.requests.empty(),"bound extra-grind work cannot retarget another observed rail episode");
    }
}
void pending_contact_lifecycle(){
    for(unsigned missing=0;missing<4;++missing){
        UnifiedInput input;Sink sink;sink.wait=true;InputEnvironment env{InputContext::air,true};env.rail.phase=InputRailPhase::approach;
        input.poll(0,{},env,sink.adapter());input.poll(10,direction(8),env,sink.adapter());input.poll(20,{},env,sink.adapter());
        input.poll(30,direction(8),env,sink.adapter());input.poll(40,direction(8,0x8000),env,sink.adapter());
        env=rail_environment(InputRailPhase::early);
        if(missing==0)env.rail.ready=false;
        if(missing==1)env.rail.geometry_known=false;
        if(missing==2)env.rail.fresh=false;
        if(missing==3)env.rail.generation=0;
        input.poll(100,{0x8000},env,sink.adapter());
        check(sink.requests.empty()&&input.pending_rail().action==InputAction::rail_select&&!input.pending_rail().geometry_known,
            "partial actual contact cannot dispatch or erase an unbound selection");
        env=rail_environment(InputRailPhase::early);input.poll(110,{0x8000},env,sink.adapter());
        check(sink.count(InputAction::rail_select)==1&&input.pending_rail().rail_generation==7&&input.pending_rail().deadline==1040,
            "first complete observation binds a preserved request after partial contact without renewing its deadline");
        sink.requests.clear();env.rail.ready=false;input.poll(120,{0x8000},env,sink.adapter());
        check(sink.requests.empty()&&input.pending_rail().geometry_known,"readiness loss suspends the bound request without rebinding");
        env.rail.ready=true;env.rail.right=true;input.poll(130,{0x8000},env,sink.adapter());
        check(sink.count(InputAction::rail_select)==1&&sink.requests[0].geometry==0,"same-episode readiness recovery preserves the first admitted geometry");
        sink.requests.clear();env.rail.ready=false;input.poll(140,{0x8000},env,sink.adapter());
        env.rail.fresh=false;input.poll(150,{0x8000},env,sink.adapter());
        check(sink.requests.empty()&&input.pending_rail().action==InputAction::none,"qualification loss cancels bound work even after a preceding unreadiness sample");
    }
    for(unsigned age:{1000u,1001u})for(bool ready:{false,true}){
        UnifiedInput input;Sink sink;sink.wait=true;InputEnvironment env{InputContext::ground,true};env.rail.phase=InputRailPhase::approach;
        input.poll(0,{},env,sink.adapter());input.poll(10,direction(8),env,sink.adapter());input.poll(20,{},env,sink.adapter());
        input.poll(30,direction(8),env,sink.adapter());input.poll(40,direction(8,0x8000),env,sink.adapter());
        env=rail_environment(InputRailPhase::early);env.rail.ready=ready;input.poll(40+age,{0x8000},env,sink.adapter());
        check((input.pending_rail().action==InputAction::rail_select)==(age==1000),"extra-grind lifetime is inclusive1000ms from recognition even while contact is not ready");
        check(sink.count(InputAction::rail_select)==static_cast<unsigned>(ready&&age==1000),"only qualified unexpired contact reaches the adapter");
    }
    for(unsigned cancellation=0;cancellation<3;++cancellation){
        UnifiedInput input;Sink sink;sink.wait=true;InputEnvironment env{InputContext::ground,true};env.rail.phase=InputRailPhase::approach;
        input.poll(0,{},env,sink.adapter());input.poll(10,direction(8),env,sink.adapter());input.poll(20,{},env,sink.adapter());
        input.poll(30,direction(8),env,sink.adapter());input.poll(40,direction(8,0x8000),env,sink.adapter());
        if(cancellation==0)env.context=InputContext::lip;
        if(cancellation==1)env.gameplay=false;
        if(cancellation==2)env.context=InputContext::unknown;
        input.poll(50,{0x8000},env,sink.adapter());
        check(input.pending_rail().action==InputAction::none&&sink.requests.empty(),"Lip entry, gameplay loss and unknown ownership cannot retain approach work");
    }
    UnifiedInput input;Sink sink;sink.wait=true;InputEnvironment env{InputContext::ground,true};env.rail.phase=InputRailPhase::approach;
    input.poll(0,{},env,sink.adapter());input.poll(10,direction(8),env,sink.adapter());input.poll(20,{},env,sink.adapter());
    input.poll(30,direction(8),env,sink.adapter());input.poll(40,direction(8,0x8000),env,sink.adapter());
    const auto first=input.pending_rail();input.poll(50,{0x8000},env,sink.adapter());
    check(input.pending_rail().recognized_at==first.recognized_at,"held Y cannot replace or renew a pending selection");
    input.poll(60,{},env,sink.adapter());input.poll(70,direction(2),env,sink.adapter());input.poll(80,{},env,sink.adapter());
    input.poll(90,direction(2),env,sink.adapter());input.poll(100,direction(2,0x8000),env,sink.adapter());
    const auto replacement=input.pending_rail();
    check(replacement.variant==21&&replacement.recognized_at==100&&replacement.deadline==1100&&replacement.first_input_at==70,
        "fresh opposite pair replaces the one shared extra-grind slot using only its own physical match");
}
void platform_and_native_negatives(){
    for(unsigned age:{99u,100u,101u}){
        UnifiedInput input;Sink sink;auto env=rail_environment(InputRailPhase::established);
        input.poll(0,{},env,sink.adapter());input.poll(10,{0,0,0,0,255},env,sink.adapter());input.poll(10+age,{},env,sink.adapter());
        check(sink.requests.empty(),"platform-specific shoulder release is not silently installed as a universal rail exit");
    }
    InputSession session;InputAdmission admission{};admission.gameplay=admission.action_fresh=true;admission.pose=SkatePose::grind;
    admission.action_mode=ActionExperiment::classic;admission.rail=rail_environment(InputRailPhase::established).rail;
    session.poll(0,{},0,0,admission);session.poll(10,{0x2000},0,0,admission);session.poll(20,{},0,0,admission);
    const auto out=session.poll(30,{0x2000},0,0,admission);
    check(out.unsupported&&!out.action.owned&&!out.authored_playback&&!out.requested_id&&!out.phase,
        "actual Session keeps a named rail change unavailable without a native capability; generic pressure is not a substitute");
}
}
int main(){pair_boundaries();early_boundaries();table_cancellation();use_rail_events();contact_binding();pending_contact_lifecycle();platform_and_native_negatives();
    std::cout<<checks<<" grind/Lip selector checks, "<<failures<<" failed\n";return failures?1:0;}

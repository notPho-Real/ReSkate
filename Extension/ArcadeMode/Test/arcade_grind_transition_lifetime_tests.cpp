#include "Extension/ArcadeMode/arcade_grind_transition_lifetime.h"
#include "Extension/ArcadeMode/arcade_grind_contact.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>

using namespace dingosdk::arcade_mode;
namespace c=dingosdk::game::build::v20260929::arcade_grind_transition;
namespace {
unsigned checks{};
void check(bool pass,const char* message) {
    ++checks;if(!pass){std::cerr<<message<<'\n';std::exit(1);}
}
struct Memory {
    std::map<std::uintptr_t,std::uint8_t> bytes;
    template<class T> void put(std::uintptr_t address,T value) {
        const auto* p=reinterpret_cast<const unsigned char*>(&value);
        for(unsigned i=0;i<sizeof(T);++i)bytes[address+i]=p[i];
    }
    template<class T> bool operator()(std::uintptr_t address,T& value) {
        auto* p=reinterpret_cast<unsigned char*>(&value);
        for(unsigned i=0;i<sizeof(T);++i){auto it=bytes.find(address+i);if(it==bytes.end())return false;p[i]=it->second;}
        return true;
    }
};
struct Fixture {
    GrindOrientationScope scope{0x140000000,0x200000,0x300000,0x400000,0x140000000+c::orientation_return};
    Memory memory;
    bool valid{true},invalidate_during{},change_actor{},native_value{true};
    unsigned calls{},writes{};
    Fixture() {
        memory.put(scope.core,scope.base+c::core_vtable);
        memory.put(scope.core+c::core_context,scope.context);
        memory.put(scope.core+c::core_actor,scope.actor);
        memory.put(scope.core+c::active_actor,scope.actor);
        memory.put(scope.actor,scope.base+c::transition_vtable);
        memory.put(scope.actor+c::actor_context,scope.context);
        memory.put(scope.context+c::selected_state,c::transition_state);
        active(true);
    }
    void active(bool value){memory.put(scope.actor+c::actor_active,std::uint8_t(value));}
    bool active(){std::uint8_t value{};check(memory(scope.actor+c::actor_active,value),"active field exists");return value!=0;}
    void step() {
        const auto result=preserve_grind_transition_lifetime(scope,memory,
            [&](std::uintptr_t address,std::uint8_t value){
                check(address==scope.actor+c::actor_active,"only native lifetime byte may be restored");
                ++writes;memory.put(address,value);return true;
            },[&]{return valid;},[&]{
                ++calls;active(native_value);
                if(invalidate_during)valid=false;
                if(change_actor)memory.put(scope.core+c::active_actor,std::uintptr_t{0x500000});
                return std::uintptr_t{0x600000};
            });
        check(result==0x600000 && calls==1,"original runs once and its output pointer is preserved");
    }
};
struct ContactFixture {
    static constexpr std::uintptr_t base=0x140000000,core=0x200000,context=0x300000,
        actor=0x400000,selector=0x500000,pages=0x600000,output=0x700000;
    Memory memory;
    ContactFixture(std::uint32_t state=401) {
        memory.put(core,base+c::core_vtable);
        memory.put(core+c::core_context,context);
        memory.put(core+c::active_actor,actor);
        memory.put(actor,base+c::contact_vtables[state-400]);
        memory.put(actor+c::actor_context,context);
        memory.put(context+c::selected_state,state);
        memory.put(core+0x440,selector);memory.put(selector,pages);memory.put(pages+32,output);
        memory.put(context+4016,std::uint8_t{1});memory.put(output+400,std::uint8_t{1});
        memory.put(output+188,std::uint32_t{1});
        memory.put(output+192,std::uint32_t{20});memory.put(actor+c::missing_segment,std::uint8_t{0});
        // Original c69 completion receipt's four-metre rail. This reader
        // establishes a current segment, not whether enough travel remains.
        memory.put(actor+c::segment_begin,std::array<float,3>{633.076416f,199.243103f,1080.58496f});
        memory.put(actor+c::segment_end,std::array<float,3>{633.076416f,199.243103f,1084.58496f});
        // Synthetic opaque words exercise native equality, not pointer validity.
        memory.put(actor+c::segment_identity,std::array<std::uint64_t,2>{17,29});
    }
    NativeGrindContact sample(){return read_grind_contact(base,core,context,memory,[]{return true;});}
};
}
int main() {
    for(std::uint32_t state=400;state<=405;++state) {
        ContactFixture f(state);
        const auto sample=f.sample();
        check(sample.valid && sample.actor==f.actor && sample.output==f.output && sample.state==state &&
            sample.identity==20 && sample.geometry==1 && sample.end[2]-sample.begin[2]==4.f &&
            sample.segment_identity==std::array<std::uint64_t,2>{17,29},
            "original contact actors expose the current native segment without a minimum-length policy");
    }
    for(unsigned reject=0;reject<16;++reject) {
        ContactFixture f;
        switch(reject) {
        case 0:f.memory.put(f.context+c::selected_state,std::uint32_t{406});break;
        case 1:f.memory.put(f.context+c::selected_state,std::uint32_t{701});break;
        case 2:f.memory.put(f.context+4016,std::uint8_t{0});break;
        case 3:f.memory.put(f.output+400,std::uint8_t{0});break;
        case 4:f.memory.put(f.actor+c::missing_segment,std::uint8_t{1});break;
        case 5:f.memory.put(f.actor+c::missing_segment,std::uint8_t{2});break;
        case 6:f.memory.put(f.actor+c::actor_context,std::uintptr_t{0x800000});break;
        case 7:f.memory.put(f.actor,f.base+c::contact_vtables[0]);break;
        case 8:f.memory.bytes.erase(f.actor+c::segment_end);break;
        case 9:f.memory.put(f.actor+c::segment_end,std::array<float,3>{633.076416f,199.243103f,1080.58496f});break;
        case 10:f.memory.put(f.actor+c::segment_begin,std::array<float,3>{0,std::numeric_limits<float>::infinity(),0});break;
        case 11:f.memory.put(f.output+192,std::uint32_t{77});break;
        case 12:f.memory.put(f.pages+32,std::uintptr_t{0});break;
        case 13:f.memory.put(f.core+c::core_context,std::uintptr_t{0x800000});break;
        case 14:f.memory.bytes.erase(f.actor+c::segment_identity+8);break;
        case 15:f.memory.bytes.erase(f.output+188);break;
        }
        check(!f.sample().valid,"missing contact/segment, invalid owner or rail exit cannot admit a new request");
    }
    for(unsigned changed=0;changed<11;++changed) {
        ContactFixture f;unsigned reads{},resolves{};
        const auto sample=read_grind_contact(f.base,f.core,f.context,[&](auto address,auto& value) {
            const bool ok=f.memory(address,value);
            // Change between reading the candidate fields and validating them.
            if(address==f.actor+c::segment_identity && ++reads==1) {
                switch(changed) {
                case 0:f.memory.put(f.core+c::active_actor,std::uintptr_t{0x800000});break;
                case 1:f.memory.put(f.pages+32,std::uintptr_t{0x800000});break;
                case 2:f.memory.put(f.output+192,std::uint32_t{23});break;
                case 3:f.memory.put(f.context+4016,std::uint8_t{0});break;
                case 4:f.memory.put(f.actor+c::segment_end,std::array<float,3>{633.076416f,199.243103f,1088.58496f});break;
                case 5:f.memory.put(f.context+c::selected_state,std::uint32_t{403});break;
                case 6:f.memory.put(f.selector,std::uintptr_t{0x800000});break;
                case 8:f.memory.put(f.actor+c::segment_identity,std::array<std::uint64_t,2>{18,29});break;
                case 9:f.memory.put(f.actor+c::segment_identity,std::array<std::uint64_t,2>{17,30});break;
                case 10:f.memory.put(f.output+188,std::uint32_t{2});break;
                default:break;
                }
            }
            return ok;
        },[&]{return ++resolves==1 || changed!=7;});
        check(!sample.valid,"changed native ownership, identity or geometry invalidates the entire contact sample");
    }
    {
        ContactFixture f;
        f.memory.put(f.actor+c::segment_identity,std::array<std::uint64_t,2>{});
        check(f.sample().valid,"opaque native segment words have no invented nonzero requirement");
    }
    // Captured native entry: initialized active=1; shared helper proposes 0.
    // The repair applies across the native actor's own update cadence, including
    // its temporary lift. Native completion executes outside this helper.
    for(unsigned hz:{30u,60u,120u}) {
        Fixture f;f.native_value=false;
        for(unsigned i=0;i<hz/2;++i) {
            f.calls=0;f.step();check(f.active(),"shared admission clear must not terminate initialized 406");
        }
        f.active(false);f.calls=0;f.native_value=true;f.step();
        check(!f.active(),"native completion cannot be resurrected by a later admission result");
    }
    for(unsigned reject=0;reject<10;++reject) {
        Fixture f;f.native_value=false;
        switch(reject) {
        case 0:f.valid=false;break; // OFF/menu/deadline/no request.
        case 1:f.invalidate_during=true;break;
        case 2:f.change_actor=true;break;
        case 3:f.scope.caller+=1;break;
        case 4:f.memory.put(f.scope.context+c::selected_state,std::uint32_t{401});break;
        case 5:f.memory.put(f.scope.actor+c::actor_context,std::uintptr_t{0x700000});break;
        case 6:f.memory.put(f.scope.core+c::core_actor,std::uintptr_t{0x700000});break;
        case 7:f.memory.put(f.scope.actor,std::uintptr_t{0x700000});break;
        case 8:f.memory.bytes.erase(f.scope.actor+c::actor_active);break;
        case 9:f.memory.put(f.scope.actor+c::actor_active,std::uint8_t{2});break;
        }
        f.step();check(!f.writes && !f.active(),"ineligible or changed owner must retain original behavior");
    }
    Fixture unchanged;unchanged.step();check(!unchanged.writes,"no redundant native write");

    // Native c69 receipt: 401 -> 406 -> 403, phase=1, original completion
    // clears active before cleanup. Neither an admission clear nor cleanup
    // at a different state/caller can impersonate that update boundary.
    for(unsigned hz:{30u,60u,120u}) {
        Fixture f;f.scope.caller=f.scope.base+c::update_return;
        f.memory.put(f.scope.actor+1040,std::uint32_t{401});
        f.memory.put(f.scope.actor+1044,std::uint32_t{403});
        f.memory.put(f.scope.actor+1424,1.f);
        f.memory.put(f.scope.actor+72,std::array<float,3>{0,.35557747f,0});
        for(unsigned i=0;i<hz/4;++i) {
            unsigned calls{};
            const auto result=observe_grind_transition_completion(f.scope,f.memory,[&]{return f.valid;},[&]{++calls;});
            check(calls==1 && !result.valid && f.active(),"ongoing native update is not completion");
        }
        unsigned calls{};
        const auto done=observe_grind_transition_completion(f.scope,f.memory,[&]{return f.valid;},[&]{++calls;f.active(false);});
        check(calls==1 && done.valid && done.source==401 && done.target==403 &&
            done.phase==1 && done.orientation[1]==.35557747f && !f.active(),"capture-matched original completion is observed without restoring active");
        const auto repeat=observe_grind_transition_completion(f.scope,f.memory,[&]{return f.valid;},[]{});
        check(!repeat.valid,"completed level cannot produce another completion edge");
    }
    for(unsigned reject=0;reject<10;++reject) {
        Fixture f;f.scope.caller=f.scope.base+c::update_return;
        f.memory.put(f.scope.actor+1040,std::uint32_t{401});
        f.memory.put(f.scope.actor+1044,std::uint32_t{403});
        f.memory.put(f.scope.actor+1424,1.f);
        f.memory.put(f.scope.actor+72,std::array<float,3>{0,.35557747f,0});
        if(reject==0)f.scope.caller=f.scope.base+c::orientation_return;
        if(reject==1)f.valid=false;
        if(reject==2)f.active(false);
        if(reject==3)f.memory.put(f.scope.actor+1424,.5f);
        if(reject==4)f.memory.put(f.scope.actor+1044,std::uint32_t{200});
        if(reject==5)f.memory.put(f.scope.actor+72,std::array<float,3>{0,std::numeric_limits<float>::quiet_NaN(),0});
        if(reject==6)f.memory.bytes.erase(f.scope.actor+1424);
        unsigned calls{};
        const auto result=observe_grind_transition_completion(f.scope,f.memory,[&]{return f.valid;},[&]{
            ++calls;f.active(false);
            if(reject==7)f.valid=false;
            if(reject==8)f.memory.put(f.scope.context+c::selected_state,std::uint32_t{200});
            if(reject==9)f.memory.put(f.scope.core+c::active_actor,std::uintptr_t{0x700000});
        });
        check(calls==1 && !result.valid && !f.active(),"invalid scope/field/owner or cleanup supplies no completion and original remains authoritative");
    }
    // Native update 0x48268e6 compares its clamped phase against exactly 1
    // before entering the sole completion-clear branch. A near-complete
    // sample cannot establish that branch, even if the active byte changed.
    for(const float phase:{.99f,std::nextafter(1.f,0.f),std::nextafter(1.f,2.f),
            std::numeric_limits<float>::quiet_NaN()}) {
        Fixture f;f.scope.caller=f.scope.base+c::update_return;
        f.memory.put(f.scope.actor+1040,std::uint32_t{401});
        f.memory.put(f.scope.actor+1044,std::uint32_t{403});
        f.memory.put(f.scope.actor+1424,phase);
        f.memory.put(f.scope.actor+72,std::array<float,3>{0,.35557747f,0});
        unsigned calls{};
        const auto result=observe_grind_transition_completion(f.scope,f.memory,[&]{return f.valid;},
            [&]{++calls;f.active(false);});
        check(calls==1 && !result.valid && !f.active(),"only the native clamped terminal phase can prove completion");
    }
    std::cout<<checks<<" native grind lifetime checks passed\n";
}

#include "Extension/ArcadeMode/arcade_grind_transition_lifetime.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
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
}
int main() {
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
    std::cout<<checks<<" native grind lifetime checks passed\n";
}

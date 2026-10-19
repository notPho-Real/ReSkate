#include "Extension/ArcadeMode/arcade_native_input_state.h"
#include "Extension/ArcadeMode/arcade_native_mapping.h"
#include <cstdlib>
#include <iostream>
#include <type_traits>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why){++checks;if(!value){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
}
int main(){
    namespace layout=dingosdk::game::build::v20260929::arcade_tricks;
    constexpr std::uintptr_t core=0x100000;std::uintptr_t object=0x200000;std::int32_t id=2;
    auto read=[&](std::uintptr_t at,auto& value){using T=std::remove_reference_t<decltype(value)>;
        if(at==core+layout::core_selection){value=static_cast<T>(object);return true;}
        if(at==object+layout::selection_flip){value=static_cast<T>(id);return true;}return false;};
    const auto before=read_flip_selection(read,core);object=0x300000;const auto after=read_flip_selection(read,core);
    check(before!=after,"same-id replacement of the native selection object must invalidate retained action identity");
    NativeInputState observer;NativeRequestSample native{};native.valid=true;native.state=200;native.crouch_mode=0;
    const auto first=observer.observe(1000,{1,2,3,4,5},native,before);
    const auto stable=observer.observe(1017,{1,2,3,4,5},native,before);
    check(stable.action_generation==first.action_generation,"repeated reads preserve selection lifetime rather than inventing occurrences");
    const auto replaced=observer.observe(1034,{1,2,3,4,5},native,after);
    check(replaced.action_generation>stable.action_generation&&replaced.contact_generation==stable.contact_generation&&
          replaced.owner_generation==stable.owner_generation,"same-id native object replacement changes only action lifetime");
    check(replaced.selection_object==object,"native frame carries the validated internal selection object");
    id=-1;const auto none=read_flip_selection(read,core);
    check(none.valid&&none.id==-1&&none.object==object,"known no-selection and unavailable are distinct states of a known object");
    const auto ended=observer.observe(1051,{1,2,3,4,5},native,none);
    check(ended.action_generation>replaced.action_generation,"native selection end retires the active action lifetime");
    const auto unavailable=observer.observe(1068,{1,2,3,4,5},native,{});
    check(!unavailable.flip_known&&!unavailable.selection_object&&unavailable.action_generation>ended.action_generation,
          "unreadable native selection relinquishes identity instead of retaining a previous action");
    for(bool nollie:{false,true})for(const auto trick:{ArcadeTrick::kickflip,ArcadeTrick::impossible,ArcadeTrick::heelflip,
        ArcadeTrick::pop_shove_it,ArcadeTrick::hardflip,ArcadeTrick::inward_heelflip,ArcadeTrick::varial_kickflip,
        ArcadeTrick::varial_heelflip,ArcadeTrick::shove_it_360,ArcadeTrick::hardflip_360,ArcadeTrick::inward_heelflip_360,
        ArcadeTrick::flip_360,ArcadeTrick::heelflip_360}) {
        id=native_flip_id(trick,nollie);const auto selection=read_flip_selection(read,core);
        check(id>0&&selection.valid&&selection.id==id&&selection.object==object,"every established regular/nollie catalog identity retains its native object lifetime");
    }
    for(const auto trick:{ArcadeTrick::double_kickflip,ArcadeTrick::double_impossible,ArcadeTrick::double_heelflip,
        ArcadeTrick::triple_kickflip,ArcadeTrick::triple_impossible,ArcadeTrick::triple_heelflip,ArcadeTrick::shove_it_540})
        for(bool nollie:{false,true})check(native_flip_id(trick,nollie)==0,"missing exact repeat identity remains unsupported without a new guessed mapping");
    std::cout<<checks<<" native air selection lifecycle checks passed\n";
}

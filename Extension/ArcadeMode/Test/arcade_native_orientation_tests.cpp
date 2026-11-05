#include "Engine/Game/Skater/native_orientation.h"
#include "Extension/ArcadeMode/arcade_input_policy.h"
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>
using namespace dingosdk;
namespace layout=game::build::v20260929::animation_values;
unsigned checks{};
void check(bool value){++checks;if(!value)std::exit(1);}
struct Memory {
    std::map<std::uintptr_t,std::vector<unsigned char>> cells;
    template<class T> void put(std::uintptr_t address,const T& value){
        auto& bytes=cells[address];bytes.resize(sizeof(T));std::memcpy(bytes.data(),&value,sizeof(T));
    }
    template<class T> bool read(std::uintptr_t address,T& value)const{
        const auto it=cells.find(address);
        if(it==cells.end() || it->second.size()!=sizeof(T))return false;
        std::memcpy(&value,it->second.data(),sizeof(T));return true;
    }
};
int main(){
    constexpr std::uintptr_t core=0x10000,context=0x20000,source=0x30000,root=0x40000,exported=0x50000;
    for(bool fakie:{false,true})for(bool basis:{false,true})for(bool foot:{false,true})for(bool adjacent:{false,true}){
        Memory memory;
        memory.put(core+layout::frame_source,source);memory.put(core+layout::state_root,root);
        memory.put(root+layout::export_state,exported);
        std::array<std::uint8_t,5> flags{static_cast<std::uint8_t>(basis),0,static_cast<std::uint8_t>(foot),static_cast<std::uint8_t>(adjacent),static_cast<std::uint8_t>(fakie)};
        std::array<std::uint8_t,3> exports{flags[4],flags[2],flags[3]};
        memory.put(source+layout::frame_flags,flags);memory.put(exported+layout::exported_flags,exports);
        memory.put(context+layout::frame_context,fakie?layout::native_mask:0u);
        memory.put(context+layout::basis_context,basis?layout::basis_mask:0u);
        const auto read=[&](auto address,auto& value){return memory.read(address,value);};
        check(read_native_orientation(read,core,context)==NativeOrientation{true,fakie});
        const auto original=memory;
        // Any missing source invalidates the observation, without guessing from
        // leading foot, last frame or the other corroborating sources.
        for(const auto& [address,bytes]:original.cells){
            (void)bytes;memory=original;memory.cells.erase(address);
            check(!read_native_orientation(read,core,context).valid);
        }
        memory=original;memory.put(context+layout::frame_context,fakie?0u:layout::native_mask);
        check(!read_native_orientation(read,core,context).valid);
        memory=original;exports[0]=!exports[0];memory.put(exported+layout::exported_flags,exports);
        check(!read_native_orientation(read,core,context).valid);
        memory=original;flags[2]=2;memory.put(source+layout::frame_flags,flags);
        check(!read_native_orientation(read,core,context).valid);
        memory=original;exports={static_cast<std::uint8_t>(fakie),static_cast<std::uint8_t>(!foot),static_cast<std::uint8_t>(adjacent)};
        memory.put(exported+layout::exported_flags,exports);
        check(!read_native_orientation(read,core,context).valid);
        memory=original;memory.put(core+layout::frame_source,std::uintptr_t{3});
        check(!read_native_orientation(read,core,context).valid);
    }
    std::cout<<checks<<" native orientation checks passed\n";
}

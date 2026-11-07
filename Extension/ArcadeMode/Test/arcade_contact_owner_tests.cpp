#include "Extension/ArcadeMode/arcade_contact_owner.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <vector>
namespace {
unsigned checks{};
void check(bool value,const char* message) {
    ++checks;if(!value){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}
}
struct Memory {
    static constexpr std::uintptr_t provider=0x1000,begin=0x2000,owner=0x900000;
    static constexpr auto stride=dingosdk::game::build::v20260929::arcade_wall_contact::provider_owner_size;
    std::map<std::uintptr_t,std::vector<unsigned char>> bytes;
    unsigned range_reads{};bool replace_range{},replace_owner{};
    template<class T> void put(std::uintptr_t at,const T& value) {
        auto& data=bytes[at];data.resize(sizeof value);std::memcpy(data.data(),&value,sizeof value);
    }
    explicit Memory(std::size_t count=3) {
        put(provider+0x28,std::array<std::uintptr_t,2>{begin,begin+count*stride});
        for(std::size_t i=0;i<count;++i)put(begin+i*stride,std::uintptr_t{owner+i});
    }
    template<class T> bool read(std::uintptr_t at,T& out) {
        const auto it=bytes.find(at);if(it==bytes.end()||it->second.size()!=sizeof out)return false;
        if(at==provider+0x28&&++range_reads==2&&replace_range)return false;
        if(at==begin&&range_reads==2&&replace_owner)return false;
        std::memcpy(&out,it->second.data(),sizeof out);return true;
    }
    std::uintptr_t query(std::uintptr_t key=owner) {
        return dingosdk::arcade_mode::contact_owner::vector(provider,key,
            [&](auto at,auto& value){return read(at,value);});
    }
};
}
int main() {
    {Memory m;check(m.query()==Memory::begin+0x40,"first native owner");}
    {Memory m;check(m.query(Memory::owner+2)==Memory::begin+2*Memory::stride+0x40,"later native owner");}
    {Memory m;check(!m.query(Memory::owner+99),"missing owner cannot borrow first native record");}
    {Memory m;check(!m.query(0),"null owner rejected");}
    {Memory m(0);check(!m.query(),"empty owner table");}
    {Memory m;m.put(Memory::begin+Memory::stride,Memory::owner);check(!m.query(),"duplicate owner ambiguous");}
    {Memory m;m.bytes.erase(Memory::begin+Memory::stride);check(!m.query(),"partial table read fails closed");}
    {Memory m;m.put(Memory::provider+0x28,std::array<std::uintptr_t,2>{Memory::begin,Memory::begin-1});check(!m.query(),"reversed range rejected");}
    {Memory m;m.put(Memory::provider+0x28,std::array<std::uintptr_t,2>{Memory::begin,Memory::begin+1});check(!m.query(),"partial native owner record rejected");}
    {Memory m(129);check(!m.query(),"unbounded owner traversal rejected");}
    {Memory m;m.replace_range=true;check(!m.query(),"owner table replacement during observation rejected");}
    {Memory m;m.replace_owner=true;check(!m.query(),"matched native owner replacement rejected");}
    std::cout<<checks<<" native contact owner checks passed\n";
}

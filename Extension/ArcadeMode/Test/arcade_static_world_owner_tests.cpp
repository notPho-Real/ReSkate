#include "Extension/ArcadeMode/arcade_static_world_owner.h"
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <vector>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* message) {
    ++checks;if(!value){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}
}
struct Fixture {
    static constexpr std::uintptr_t world=0x1000,store=0x3000,owners=0x10000,kinds=0x11000,values=0x12000,generations=0x13000;
    SurfaceGeometryBody shape{world,42,48},body{world,43,50331648};
    std::map<std::uintptr_t,std::vector<unsigned char>> bytes;
    unsigned current_calls{};bool allocated_static=true;
    std::function<void()> between;
    template<class T> void put(std::uintptr_t at,const T& value) {
        auto& data=bytes[at];data.resize(sizeof value);std::memcpy(data.data(),&value,sizeof value);
    }
    Fixture() {
        put(store+0x1008,owners);put(store+0x8b0,kinds);put(store+0x8b8,values);
        put(world+0x810,generations);put(world+0x818,std::uintptr_t{generations+100*4});
        put(owners+42*4,std::uint32_t{69});put(generations+69*4,std::uint32_t{50331648});
        put(kinds+(69>>3),std::uint8_t{1u<<(69&7)});put(values+69*8,std::uint64_t{0x12345678});
    }
    template<class T> bool read(std::uintptr_t at,T& out) {
        const auto it=bytes.find(at);if(it==bytes.end()||it->second.size()!=sizeof out)return false;
        std::memcpy(&out,it->second.data(),sizeof out);return true;
    }
    bool query(SurfaceGeometryWorldOwner& out) {
        return static_world_owner_detail::observe(shape,body,store,
            [&](auto at,auto& value){return read(at,value);},[&] {
                if(++current_calls==2&&between)between();return allocated_static;
            },out);
    }
};
}
int main() {
    {Fixture f;SurfaceGeometryWorldOwner out;
     check(f.query(out),"valid static non-ECS geometry needs no ECS entity ID");
     check(out==SurfaceGeometryWorldOwner{f.body,f.store,69,50331648,0,0x12345678},"complete native owner incarnation retained");}
    {Fixture f;f.put(Fixture::generations+69*4,std::uint32_t{0});SurfaceGeometryWorldOwner out;
     check(f.query(out)&&out.generation==0,"zero native generation is not an invented invalid sentinel");}
    for(unsigned scenario=0;scenario<12;++scenario) {
        Fixture f;SurfaceGeometryWorldOwner out{f.body,f.store,1,2,3,4};
        switch(scenario) {
        case 0:f.put(Fixture::owners+42*4,UINT32_MAX);break;
        case 1:f.put(Fixture::owners+42*4,std::uint32_t{100});break;
        case 2:f.put(Fixture::kinds+(69>>3),std::uint8_t{0});break;
        case 3:f.put(Fixture::values+69*8,std::uint64_t{0});break;
        case 4:f.put(Fixture::world+0x818,std::uintptr_t{Fixture::generations-4});break;
        case 5:f.put(Fixture::world+0x818,std::uintptr_t{Fixture::generations+3});break;
        case 6:f.put(Fixture::world+0x818,std::uintptr_t{Fixture::generations+1048577*4});break;
        case 7:f.bytes.erase(Fixture::generations+69*4);break;
        case 8:f.allocated_static=false;break;
        case 9:f.shape.generation=256;break;
        case 10:++f.body.world;break;
        case 11:f.shape.index=1048576;break;
        }
        check(!f.query(out),"unassigned, ECS, malformed, moving or invalid association rejected");
        check(out==SurfaceGeometryWorldOwner{},"rejection clears any previous owner proof");
    }
    for(unsigned scenario=0;scenario<6;++scenario) {
        Fixture f;SurfaceGeometryWorldOwner out;
        f.between=[&] {
            switch(scenario) {
            case 0:f.put(Fixture::generations+69*4,std::uint32_t{50331649});break;
            case 1:f.put(Fixture::values+69*8,std::uint64_t{0x87654321});break;
            case 2:f.put(Fixture::owners+42*4,std::uint32_t{68});break;
            case 3:f.put(Fixture::kinds+(69>>3),std::uint8_t{0});break;
            case 4:f.put(Fixture::store+0x1008,std::uintptr_t{0x20000});break;
            case 5:f.allocated_static=false;break;
            }
        };
        check(!f.query(out),"generation/value/owner/kind/table/motion replacement revokes read");
    }
    std::cout<<checks<<" static world owner checks passed\n";
}

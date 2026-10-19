#include "Extension/ArcadeMode/arcade_rail_adapter_probe.h"
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool ok,const char* why) { ++checks;if(!ok) {std::cerr<<why<<'\n';std::exit(1);} }
struct Memory {
    static constexpr std::uintptr_t base=0x20000;
    std::array<unsigned char,32768> data{};
    std::size_t next{},reads{};
    std::uintptr_t blocked{},mutation_at{};
    unsigned matches{},mutate_on{};
    std::uint32_t mutation{};
    GrindObserverOwner owner{};
    RailAdapterLipBindings bindings{};
    std::uintptr_t block{},page{};
    std::uintptr_t alloc(std::size_t bytes) {auto at=base+next;next+=(bytes+7)&~std::size_t{7};return at;}
    template<class T> void put(std::uintptr_t at,T value) {std::memcpy(data.data()+at-base,&value,sizeof(value));}
    template<class T> bool operator()(std::uintptr_t at,T& out) {
        ++reads;if(at==blocked || at<base || at-base>data.size()-sizeof(T))return false;
        if(at==mutation_at && ++matches==mutate_on)put(at,mutation);
        std::memcpy(&out,data.data()+at-base,sizeof(out));return true;
    }
    Memory() {
        owner={0x140000000,alloc(0x100),alloc(16),alloc(0x1600),alloc(16),7,100,true};
        auto collection=alloc(0x1040),component=alloc(0x80),anim=alloc(0x200),holder=alloc(0x260);
        block=alloc(0x160);page=alloc(0x840);
        put(owner.entity+0x70,collection);put(collection,owner.entity);put(collection+8,std::uint8_t{1});
        put(collection+0x20,component);put(component,owner.base+0x63aaee8);put(component+0x68,anim);
        put(anim+0xe8,holder);put(holder+8,block);put(block,owner.base+0x61a8bd8);put(block+0x60,page);
        put(owner.context+0x1414,std::uint32_t{200});bindings.generation=owner.generation;
        for(std::size_t i=0;i<bindings.objects.size();++i) {
            const auto at=alloc(0x100),object=at+0x10,name=object+0x40;
            bindings.objects[i]=object;const auto& asset=rail_adapter_probe_detail::assets[i];
            put(at,asset.id);put(object+0x18,name);
            std::memcpy(data.data()+name-base,asset.name.data(),asset.name.size());
            const auto offset=static_cast<std::uint32_t>(0x80+i*0x20);
            put(object+0x28,0x01000000u|(offset<<12));
            put(page+0x71c+(offset>>5)*4,std::uint32_t{1});
            if(i<3)put(page+offset,static_cast<std::uint8_t>(i==0));
            else put(page+offset,std::array<float,3>{1.f,2.f,3.f});
        }
    }
    RailAdapterLipSnapshot sample(std::uint64_t now=100,bool supported=true) {
        return read_rail_adapter_lip(*this,[&]{return owner;},bindings,now,supported);
    }
};
}
int main() {
    Memory memory;auto out=memory.sample();
    check(out.valid && out.fields==31 && out.found && !out.lip && !out.handplant,"candidate flag is not performed Lip");
    check(out.direction==std::array<float,3>{1,2,3} && out.nearest==out.direction,"independent finite physical geometry copied");
    check(out.sampled_at==100 && out.physics_state==200 && out.owner.generation==7,"original producer proof retained");
    check(memory.sample(250).valid && !memory.sample(251).valid && !memory.sample(99).valid,"exact existing owner lease and rollback");
    auto unsupported=memory;unsupported.reads=0;check(!unsupported.sample(100,false).valid && unsupported.reads==0,"unsupported build performs no reads");
    for(std::size_t index=0;index<5;++index) {
        auto missing=memory;missing.bindings.objects[index]=0;const auto partial=missing.sample();
        check(partial.valid && !(partial.fields&(1u<<index)) && partial.fields==(31u&~(1u<<index)),"missing binding keeps independent validity explicit");
        auto wrong=memory;wrong.put(wrong.bindings.objects[index]-0x10,std::uint8_t{0});
        check(!(wrong.sample().fields&(1u<<index)),"exact GUID required");
        auto stale=memory;stale.put(stale.bindings.objects[index]+0x28,std::uint32_t{0});
        check(!(stale.sample().fields&(1u<<index)),"dead handle never valid");
        auto bitmap=memory;bitmap.put(bitmap.page+0x71c+(4+index)*4,std::uint32_t{0});
        check(!(bitmap.sample().fields&(1u<<index)),"uninitialized field is not false or zero geometry");
    }
    for(std::size_t i=3;i<5;++i) {
        auto nonfinite=memory;nonfinite.put(nonfinite.page+0x80+i*0x20,std::numeric_limits<float>::infinity());
        check(!(nonfinite.sample().fields&(1u<<i)),"nonfinite geometry unavailable");
        auto crossing=memory;crossing.put(crossing.bindings.objects[i]+0x28,0x01000000u|(0x718u<<12));
        check(!(crossing.sample().fields&(1u<<i)),"vector cannot cross validity bitmap");
    }
    for(unsigned changed=0;changed<3;++changed) {
        auto unstable=memory;
        if(changed==0) {unstable.mutation_at=unstable.bindings.objects[0]+0x28;unstable.mutation=0x010a0000u;}
        if(changed==1) {unstable.mutation_at=unstable.page+0x71c+4*4;unstable.mutation=0;}
        if(changed==2) {unstable.mutation_at=unstable.block+0x60;unstable.mutation=0;}
        unstable.mutate_on=2;
        check(!(unstable.sample().fields&1u),"changed page/handle/validity cannot publish the old flag");
    }
    auto no_name=memory;no_name.put(no_name.bindings.objects[0]+0x40,'X');
    check(!(no_name.sample().fields&1u),"exact asset name required as well as GUID");
    auto no_zero=memory;no_zero.put(no_zero.owner.context+0x1414,std::uint32_t{0});
    check(!no_zero.sample().valid,"empty physics state does not establish an owner context");
    auto mismatch=memory;mismatch.bindings.generation++;
    check(!mismatch.sample().valid,"bindings belong to actual observer owner generation");
    auto changing=memory;unsigned resolve_count{};
    out=read_rail_adapter_lip(changing,[&]{auto owner=changing.owner;if(++resolve_count>1)owner.entity+=8;return owner;},changing.bindings,100,true);
    check(!out.valid,"owner replacement invalidates all partial output");
    changing=memory;changing.mutation_at=changing.owner.context+0x1414;changing.mutate_on=2;changing.mutation=401;
    check(!changing.sample().valid,"context change invalidates all output");
    for(std::uint32_t state : {100u,200u,401u,403u,504u,UINT32_MAX}) {
        auto phase=memory;phase.put(phase.owner.context+0x1414,state);const auto value=phase.sample();
        check(value.valid && value.physics_state==state && !value.lip,"state numbers do not invent Lip phase or identity");
    }
    auto active=memory;active.put(active.page+0xa0,std::uint8_t{1});out=active.sample();
    check(out.valid && out.lip && !out.handplant,"independent physical Lip flag observed");
    active.put(active.page+0xc0,std::uint8_t{1});out=active.sample();
    check(out.lip && out.handplant,"handplant remains separate from Lip and named mapping");
    std::cout<<checks<<" rail lip probe checks passed\n";
}

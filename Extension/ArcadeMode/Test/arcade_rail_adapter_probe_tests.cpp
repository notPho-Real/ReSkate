#include "Extension/ArcadeMode/arcade_rail_adapter_probe.h"
#include "Extension/ArcadeMode/arcade_gesture.h"
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
    std::uintptr_t blocked{},mutation_at{},mutation_target{};
    unsigned matches{},mutate_on{};
    std::uint32_t mutation{};
    GrindObserverOwner owner{};
    RailAdapterLipBindings bindings{};
    std::uintptr_t block{},page{};
    std::uintptr_t alloc(std::size_t bytes) {auto at=base+next;next+=(bytes+7)&~std::size_t{7};return at;}
    template<class T> void put(std::uintptr_t at,T value) {std::memcpy(data.data()+at-base,&value,sizeof(value));}
    template<class T> bool operator()(std::uintptr_t at,T& out) {
        ++reads;if(at==blocked || at<base || at-base>data.size()-sizeof(T))return false;
        if(at==mutation_at && ++matches==mutate_on)put(mutation_target?mutation_target:at,mutation);
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
    // The existing page walker supplies these bounded fixture spans to BOTH
    // allowlists; there is no second scanner or all-fields readiness barrier.
    RailAdapterLipBindings discovered{};
    for(std::size_t at=0;at<memory.data.size();at+=4096)
        inspect_rail_lip_candidates(memory,Memory::base+at,4096,7,discovered);
    check(discovered.objects==memory.bindings.objects && discovered.generation==7,"existing resident-span discovery resolves exact five asset objects");
    const auto published=discovered;discovered.objects[0]=0;
    check(published.objects==memory.bindings.objects,"published binding value is independent of later worker candidate changes");
    check(refresh_rail_lip_candidates(memory,published,8).objects==published.objects,"generation change revalidates immutable asset hints");
    auto retired=memory;retired.put(published.objects[2]+0x28,std::uint32_t{0});
    const auto refreshed=refresh_rail_lip_candidates(retired,published,8);
    check(!refreshed.objects[2]&&refreshed.objects[1]&&refreshed.generation==8,"unloaded hint stays absent while independent fields rebind");
    const auto reads=memory.reads;RailAdapterLipBindings invalid{};
    check(!inspect_rail_lip_candidates(memory,Memory::base,4097,7,invalid)&&memory.reads==reads,"existing page-span bound preserved");
    for(std::size_t index=0;index<5;++index){
        const auto mask=std::uint8_t(1u<<index);const auto object=memory.bindings.objects[index];
        for(unsigned fault=0;fault<5;++fault){
            auto field=memory;
            if(fault==0)field.put(object+0x28,0x01000010u|(0x80u<<12));
            if(fault==1)field.put(object+0x28,0x01000000u|(0x71cu<<12));
            if(fault==2)field.put(object+0x40+rail_adapter_probe_detail::assets[index].name.size(),'x');
            if(fault==3)field.put(object+0x18,object+0x800);
            if(fault==4)field.blocked=field.page+0x80+index*0x20;
            check(field.sample().fields==std::uint8_t(31u&~mask),"page/index/value/terminated-name/locality/read failure invalidates ONLY affected field");
        }
        for(bool name:{false,true}){
            auto replaced=memory;replaced.mutation_at=replaced.page+0x80+index*0x20;replaced.mutate_on=1;
            replaced.mutation_target=name?object+0x40:object-0x10;replaced.mutation=0;
            check(!(replaced.sample().fields&mask),"GUID/name replacement after first validation cannot publish an old value");
        }
        auto bad_name=memory;bad_name.put(object+0x40,'x');RailAdapterLipBindings candidates{};
        inspect_rail_lip_candidates(bad_name,object-0x10,4,7,candidates);
        check(!candidates.objects[index],"discovery requires exact name in addition to exact GUID");
    }
    for(std::size_t index=3;index<5;++index)for(unsigned axis=0;axis<3;++axis){
        auto nan=memory;nan.put(nan.page+0x80+index*0x20+axis*4,std::numeric_limits<float>::quiet_NaN());
        check(!(nan.sample().fields&(1u<<index)),"NaN in every vector axis remains unavailable rather than zero geometry");
    }
    // Existing two-enum reading is independent of absent optional bindings.
    auto separate=memory;GrindStateBindings enums{{},7};
    for(unsigned index=0;index<2;++index){
        const auto id=separate.alloc(0x100),object=id+0x10,name=object+0x40;enums.objects[index]=object;
        separate.put(id,grind_observer_detail::assets[index].id);separate.put(object+0x18,name);
        std::memcpy(separate.data.data()+name-Memory::base,grind_observer_detail::assets[index].name.data(),grind_observer_detail::assets[index].name.size());
        const auto offset=0x200u+index*0x20u;separate.put(object+0x28,0x01000000u|(offset<<12));
        separate.put(separate.page+offset,index+5);separate.put(separate.page+0x71c+(offset>>5)*4,std::uint32_t{1});
    }
    separate.put(separate.owner.context+0x1414,std::uint32_t{401});
    for(unsigned missing=0;missing<=5;++missing){
        separate.owner.sampled_at=100;
        auto optional=separate.bindings;
        for(unsigned i=0;i<missing;++i)optional.objects[i]=0;
        const auto raw=read_native_grind(separate,[&]{return separate.owner;},enums,100,true);
        const auto partial=read_rail_adapter_lip(separate,[&]{return separate.owner;},optional,100,true);
        check(raw.valid&&raw.grind_type_valid&&raw.skate_grind_type_valid,"missing optional Lip fields never starve two-enum readiness");
        check(partial.valid&&partial.fields==std::uint8_t(31u&~((1u<<missing)-1)),"optional field validity is preserved independently");
        NativeInputState producer;NativeInputOwner owner{separate.owner.base,separate.owner.entity,separate.owner.core,separate.owner.context,separate.owner.rig};
        NativeRequestSample sample{};sample.valid=true;sample.state=401;sample.crouch_mode=1;
        auto frame=producer.observe(100,owner,sample,{true,-1});ArcadeInput facade(true);facade.enable(true);
        facade.publish_native_context(frame);facade.publish_authority(100,frame.owner_generation,1,true);facade.publish_native_lip(partial,frame);
        check(facade.native_lip_observation(100).fields==partial.fields,"actual producer/facade joins read-only partial observations without actor readiness");
        check(facade.request_native_rail(RailAdapterMethod::align,100,200,true)==RailAdapterResult::started,"generic rail remains independently admitted with missing optional fields");
        check(facade.sample_native_rail(100,true,true).owns_right_stick,"existing generic RS adapter remains available");
        frame=producer.observe(250,owner,sample,{true,-1});facade.publish_native_context(frame);facade.publish_authority(250,frame.owner_generation,1,true);
        check(facade.native_lip_observation(250).valid,"original read timestamp includes150ms endpoint");
        frame=producer.observe(251,owner,sample,{true,-1});facade.publish_native_context(frame);facade.publish_authority(251,frame.owner_generation,1,true);
        check(!facade.native_lip_observation(251).valid,"new client/physics context and repeated getters cannot renew old field evidence");
        facade.publish_native_lip(partial,frame);check(!facade.native_lip_observation(251).valid,"republishing original expired fields cannot borrow fresh native frame time");
        separate.owner.sampled_at=251;
        const auto fresh=read_rail_adapter_lip(separate,[&]{return separate.owner;},optional,251,true);
        facade.publish_native_lip(fresh,frame);
        check(facade.native_lip_observation(251).valid,"a new actual physical read can establish a new original observation");
        ++owner.rig;frame=producer.observe(260,owner,sample,{true,-1});facade.publish_native_context(frame);facade.publish_authority(260,frame.owner_generation,1,true);
        check(!facade.native_lip_observation(260).valid,"full owner/generation replacement cannot retain old optional native fields");
    }
    std::cout<<checks<<" rail lip probe checks passed\n";
}

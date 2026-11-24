#include "Extension/ArcadeMode/arcade_stance_input.h"
#include "Extension/ArcadeMode/arcade_native_asset_cache.h"
#include "Extension/ArcadeMode/arcade_stance_transition_scope.h"
#include <map>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* message) {
    ++checks;if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
InputNativeContext frame(std::uint64_t at=10,std::uint64_t step=1,unsigned stance=0) {
    InputNativeContext value{true,true,{1,1,1},step,at};
    value.flip={true,-1,0x10000,100,1,stance,step,1,1};return value;
}
TriggerPress press(InputNativeContext native=frame()) {
    return {{{InputSource::triggers,1,InputEdge::press},10,1},{},InputContext::ground,
        native,TriggerOwner::switch_stance,160};
}
struct ScopeMemory {
    std::map<std::uintptr_t,std::uint8_t> bytes;
    template<class T> void put(std::uintptr_t at,const T& value) {
        const auto* data=reinterpret_cast<const std::uint8_t*>(&value);
        for(std::size_t i=0;i<sizeof(T);++i)bytes[at+i]=data[i];
    }
    template<class T> bool read(std::uintptr_t at,T& value) const {
        auto* data=reinterpret_cast<std::uint8_t*>(&value);
        for(std::size_t i=0;i<sizeof(T);++i) {
            const auto found=bytes.find(at+i);if(found==bytes.end())return false;
            data[i]=found->second;
        }
        return true;
    }
};
void native_transition_scope() {
    namespace c=dingosdk::game::build::v20260929::arcade_stance;
    namespace e=dingosdk::game::build::v20260929::arcade_fingerflip;
    constexpr std::uintptr_t image=0x10000000,vm=0x2000,instance=0x3000,resource=0x4000,
        arguments=0x5000,reference=0x6000,key=0x12340001,registry=0x7000,entries=0x8000,
        entity=0x9000,context=0xa000,predicates=0xb000,constants=0xc000,
        asset=0x20000,graph=0x30000,wrapper=0x40000;
    for(unsigned fault=0;fault<17;++fault) {
        ScopeMemory m;
        m.put(vm+e::vm_instance,instance);m.put(vm+e::vm_resource,resource);m.put(instance,resource);
        m.put(resource+e::resource_key,c::regular_transition_key);m.put(resource+e::resource_layout,c::regular_transition_layout);
        m.put(asset-c::asset_guid_prefix,c::transition_asset.guid);m.put(asset+c::expression_graph,graph);
        m.put(graph+c::graph_resource_wrapper,wrapper);m.put(wrapper+c::wrapper_resource,resource);
        const auto pages=instance+64;
        m.put(pages+c::transition_context_page,context);m.put(pages+c::transition_predicate_page,predicates);
        m.put(pages+c::transition_flags_page,constants);m.put(pages+e::argument_page,arguments);
        m.put(arguments+e::context_argument,reference);m.put(reference,key);
        m.put(image+e::context_registry,registry);m.put(registry+e::registry_entries,entries);
        m.put(registry+e::registry_mask,std::uint32_t{15});
        const auto entry=entries+(key&15)*e::registry_entry_size;
        m.put(entry,key);m.put(entry+e::registry_entity,entity);
        auto call_context=context,call_predicate=predicates+c::transition_predicate_offset,
            call_flags=constants+c::transition_flags_offset;
        if(fault==1)m.put(resource+e::resource_key,c::regular_transition_key+1);
        if(fault==2){auto layout=c::regular_transition_layout;++layout[1];m.put(resource+e::resource_layout,layout);}
        if(fault==3)m.put(instance,resource+8);
        if(fault==4)++call_context;
        if(fault==5)++call_predicate;
        if(fault==6)++call_flags;
        if(fault==7)m.put(entry+e::registry_entity,entity+8);
        if(fault==8)m.put(entry,key+16);
        if(fault==9)m.put(registry+e::registry_mask,std::uint32_t{0xffffffff});
        if(fault==10)m.bytes.erase(reference);
        if(fault==11)m.put(arguments+e::context_argument,std::uintptr_t{});
        if(fault==12)m.put(pages+c::transition_context_page,std::uintptr_t{});
        if(fault==13)m.put(vm+e::vm_instance,std::uintptr_t{});
        if(fault==14){auto guid=c::transition_asset.guid;guid[0]^=1;m.put(asset-c::asset_guid_prefix,guid);}
        if(fault==15)m.put(wrapper+c::wrapper_resource,resource+8);
        if(fault==16)m.put(graph+c::graph_resource_wrapper,std::uintptr_t{});
        check(native_stance::transition_scope([&](auto at,auto& value){return m.read(at,value);},
            image,vm,call_context,call_predicate,call_flags,entity,asset)==(fault==0),
            "native transition accepts only exact Regular expression, call arguments and local context generation");
    }
    for(unsigned cadence:{8u,17u,33u}) {
        StanceInputLease lease;
        check(lease.start(press(),frame(),10),"fresh RT starts native transition lease");
        const auto issued=frame(10+cadence,2);
        check(lease.issued(issued,issued.sampled_at),"actual consumer invocation marks delivery");
        check(!lease.issued(issued,issued.sampled_at),"one physical RT cannot invoke a second transition");
        check(lease.observe(frame(10+2*cadence,3),10+2*cadence,true)&&!lease.observed_change(),
            "native callback delivery does not prove foot change");
        check(!lease.observe(frame(10+3*cadence,4,0x20),10+3*cadence,true)&&lease.observed_change(),
            "later native foot observation completes delivery without synthetic B input");
    }
}
void exported_asset_identity() {
    namespace contract=dingosdk::game::build::v20260929::arcade_stance;
    // Independently inspected exported instance IDs in supported-build records
    // 1014, 818, 407 and 22455. InputScheme imports corroborate the PIA and
    // processor instance IDs. The file GUIDs identify containers, not objects.
    constexpr std::array<contract::Guid,4> instances{{
        {0xd2,0x4a,0x46,0x5d,0x17,0x48,0x19,0x4b,0xb5,0xa1,0x37,0x24,0x2f,0xca,0xa8,0x9b},
        {0xee,0xbc,0xfd,0x55,0x7e,0x89,0xf3,0x4f,0x87,0xf2,0xe6,0x84,0x1f,0x26,0x4e,0x04},
        {0x73,0x6d,0x5b,0xa6,0x5d,0xb8,0x8d,0x40,0x95,0xbd,0xbf,0x0e,0x2b,0xc8,0x85,0xa4},
        {0x79,0x77,0x05,0xfe,0x62,0x12,0x5f,0x42,0x8e,0x4c,0xb9,0x83,0xcf,0x08,0xed,0xf1}
    }};
    constexpr std::array<contract::Guid,4> files{{
        {0x1a,0xae,0xa6,0x40,0x29,0xbd,0xb2,0x4a,0x97,0xfb,0x22,0x68,0xc9,0x8b,0x56,0x0e},
        {0xd2,0x48,0xbb,0x3d,0x7b,0xbe,0x32,0x4a,0xa8,0xee,0x92,0x3e,0x5d,0x61,0x14,0xfa},
        {0xa4,0xa7,0xc8,0x25,0x03,0xd7,0x27,0x4e,0x9b,0x97,0x9e,0x66,0xd1,0x61,0x1a,0x32},
        {0x2f,0xed,0xa2,0x9e,0x6d,0xbf,0xdd,0x4a,0x80,0x0f,0x3b,0x7c,0x1b,0x9b,0xcb,0x46}
    }};
    std::array<std::array<std::uint8_t,32>,instances.size()> storage{};
    struct Asset {std::uintptr_t object{};contract::Guid guid{};};
    std::array<Asset,instances.size()> loaded{};
    static_assert(instances.size()==contract::assets.size());
    for(std::size_t i=0;i<loaded.size();++i) {
        std::memcpy(storage[i].data(),instances[i].data(),instances[i].size());
        loaded[i]={reinterpret_cast<std::uintptr_t>(storage[i].data()+16),contract::assets[i].guid};
    }
    const auto matches=[](std::uintptr_t object,const contract::Guid& expected) noexcept {
        contract::Guid actual{};
        std::memcpy(actual.data(),reinterpret_cast<const void*>(object-contract::asset_guid_prefix),actual.size());
        return actual==expected;
    };
    const auto available=[&]{return native_assets_current(std::span{loaded},matches);};
    check(available(),"stance readiness recognizes actual exported asset instances");
    for(std::size_t i=0;i<loaded.size();++i) {
        loaded[i].guid=files[i];
        check(!available(),"a file GUID cannot authorize its exported stance object");
        loaded[i].guid=contract::assets[i].guid;
        const auto object=loaded[i].object;loaded[i].object=0;
        check(!available(),"a missing stance asset still withholds readiness");
        loaded[i].object=object;storage[i][0]^=1u;
        check(!available(),"a recycled stance asset address still invalidates readiness");
        storage[i][0]^=1u;
    }
    StanceInput input;
    check(available()&&input.start(press(),frame(),10),"verified asset identity admits the original RT producer");
    auto output=input.controls(frame(),10,1);
    check(output.owned&&!output.buttons&&input.delivered(frame(),output,0,10,1),
        "admitted Switch first publishes its owned neutral packet");
    output=input.controls(frame(20,2),20,2);
    check(output.owned&&output.buttons==0x2000,"a subsequent delivered native frame can begin the B gesture");
    check(!input.observed_change(),"asset readiness and B output never prove a physical stance change");
}
void original_edge_and_delivery() {
    for(unsigned stance:{0u,0x20u}) {
        StanceInputLease lease;auto native=frame(10,1,stance);auto input=press(native);
        input.released=true;input.release={{InputSource::triggers,1,InputEdge::release},10,2};
        check(lease.start(input,native,10),"original RT can initiate a native gesture even after its tap release");
        check(!lease.observed_change()&&!lease.was_issued(),"queued input proves neither delivery nor native stance");
        check(lease.issued(native,10),"adapter acknowledges the original gesture once");
        check(!lease.issued(native,10),"duplicate controller polls cannot repeat native issuance");
        check(lease.observe(frame(20,2,stance),20,true),"RT release does not cancel the already initiated native actor");
        auto changed=frame(30,3,stance^0x20u);++changed.generations.action;
        check(!lease.observe(changed,30,true)&&lease.observed_change(),"fresh native leading-foot change completes either stance direction");
        check(!lease.start(input,changed,30),"completed RT sequence cannot replay");
    }
    for(unsigned fault=0;fault<3;++fault) {
        StanceInputLease lease;auto input=press();check(lease.start(input,frame(),10),"negative starts owned input");
        if(fault)check(lease.issued(frame(),10),"negative supplies gesture before impossible observation");
        auto changed=frame(fault==2?10u:20u,fault==1?1u:2u,0x20);
        check(!lease.observe(changed,20,true)&&!lease.observed_change(),
            "pre-existing, duplicate-step or pre-delivery-time bit changes are not action proof");
    }
}
void interruptions() {
    for(unsigned fault=0;fault<11;++fault) {
        StanceInputLease lease;auto input=press();check(lease.start(input,frame(),10),"interruption starts lease");
        check(lease.issued(frame(),10),"interruption begins after delivery");
        auto next=frame(20,2);auto now=20u;
        if(fault==0)next.required=false;
        if(fault==1)++next.generations.owner;
        if(fault==2)++next.generations.contact;
        if(fault==3)++next.flip.object;
        if(fault==4)next.airborne=true;
        if(fault==5)next.rail_family=true;
        if(fault==6)next.flip.stance=0x40;
        if(fault==7)next.flip.state=602;
        if(fault==8)now=9;
        if(fault==9)now=171;
        check(!lease.observe(next,now,fault!=10)&&!lease.observed_change(),"owner, contact, stale clock, OFF and incompatible native contexts cancel");
        check(!lease.start(input,frame(),10),"cancelled physical sequence cannot revive");
    }
    StanceInputLease lease;auto input=press();check(lease.start(input,frame(),10),"waiting request starts");
    check(!lease.observe(frame(161,2),161,true),"waiting request cannot renew its physical edge deadline");
    input.press.sequence=3;input.press.at=170;input.deadline=320;input.native=frame(170,3);
    check(lease.start(input,input.native,170),"fresh physical edge remains available after retirement");
    check(lease.issued(input.native,170),"completion ceiling has an issued native gesture");
    for(unsigned at=180;at<=1170;at+=10)
        check(lease.observe(frame(at,at),at,true),"fresh samples may observe completion without another controller press");
    check(!lease.observe(frame(1171,1171),1171,true)&&!lease.observed_change(),
        "unacknowledged native request expires without a false stance success or another press");
}
void denied_sources() {
    for(unsigned fault=0;fault<10;++fault) {
        StanceInputLease lease;auto input=press();auto native=frame();
        if(fault==0)input.owner=TriggerOwner::landing_revert;
        if(fault==1)input.context=InputContext::air;
        if(fault==2)input.press.token.code=0;
        if(fault==3)input.press.token.source=InputSource::buttons;
        if(fault==4)input.press.token.edge=InputEdge::release;
        if(fault==5)input.deadline=9;
        if(fault==6)input.deadline=511;
        if(fault==7)++native.generations.action;
        if(fault==8)native.flip.stance=0x20;
        if(fault==9)input.native.valid=false;
        check(!lease.start(input,native,10),"unrelated, expired, relabeled or unowned input cannot request Switch");
    }
}
void finite_double_tap() {
    for(unsigned cadence:{1u,8u,17u,33u,60u,100u})for(unsigned stance:{0u,0x20u}) {
        StanceInput input;check(input.start(press(frame(10,1,stance)),frame(10,1,stance),10),"finite producer accepts original RT");
        for(unsigned phase=0;phase<5;++phase) {
            const auto at=10+phase*cadence;auto native=frame(at,phase+1,stance);
            const auto output=input.controls(native,at,phase+1);
            check(output.owned&&output.phase==phase&&output.buttons==((phase==1||phase==3)?0x2000u:0u),
                "original double tap is exactly neutral/down/up/down/up");
            check(input.controls(native,at,phase+1)==output,"duplicate poll cannot advance an undelivered packet");
            check(input.delivered(native,output,static_cast<std::uint16_t>(output.buttons|0x80u),at,phase+1),
                "only owned B is compared; unrelated final packet bits remain untouched");
            check(input.delivered(native,output,output.buttons,at,phase+1),"duplicate delivery cannot generate another press");
            check(input.controls(native,at,phase+1)==output,"duplicate delivered tick remains the same phase");
            check(!input.observed_change(),"completed output phases do not prove native foot change");
        }
        auto changed=frame(10+5*cadence,6,stance^0x20u);++changed.generations.action;
        check(!input.controls(changed,changed.sampled_at,6).owned&&input.observed_change()&&!input.active(),
            "new native leading-foot observation after final release completes the owned transaction");
        check(!input.start(press(frame(10,1,stance)),changed,changed.sampled_at),"completed edge never repeats doubletap");
    }
}
void packet_and_phase_guards() {
    for(unsigned fault=0;fault<5;++fault) {
        StanceInput input;check(input.start(press(),frame(),10),"packet negative starts");
        auto control=input.controls(frame(),10,1);auto native=frame();auto final=control.buttons;
        if(fault==0)++control.serial;
        if(fault==1)++control.phase;
        if(fault==2)final=0x2000;
        if(fault==3)++native.generations.owner;
        if(fault==4)control.owned=false;
        check(!input.delivered(native,control,final,10,1)&&!input.active(),"foreign, superseded or overwritten packet cannot count as delivery");
    }
    for(unsigned missing=0;missing<3;++missing) {
        StanceInput input;check(input.start(press(),frame(),10),"phase receipt negative starts");
        const auto c=input.controls(frame(),10,1);
        if(missing)check(input.delivered(frame(),c,0,10,1),"negative has initial neutral receipt");
        auto next=frame(20,missing==1?1u:2u);
        const auto after=input.controls(next,20,missing==2?1u:2u);
        check(after.owned&&after.phase==0&&after.buttons==0,
            "missing delivery, physics progression or controller tick cannot synthesize first press");
    }
    for(unsigned boundary:{299u,300u,301u}) {
        StanceInput input;input.start(press(),frame(),10);
        auto a=input.controls(frame(),10,1);input.delivered(frame(),a,0,10,1);
        a=input.controls(frame(20,2),20,2);input.delivered(frame(20,2),a,0x2000,20,2);
        a=input.controls(frame(120,3),120,3);input.delivered(frame(120,3),a,0,120,3);
        input.controls(frame(220,3),220,3); // No new phase or receipt while physics repeats.
        a=input.controls(frame(20+boundary,4),20+boundary,4);
        check(boundary<300 ? a.owned&&a.phase==3&&a.buttons==0x2000 : !a.owned&&!input.active(),
            "second press obeys native300ms boundary without renewing from duplicate polls");
    }
}
void completion_requires_release() {
    for(unsigned interruption=0;interruption<3;++interruption) {
        StanceInput input;input.start(press(),frame(),10);
        for(unsigned phase=0;phase<4;++phase) {
            auto native=frame(10+phase*10,phase+1);auto out=input.controls(native,native.sampled_at,phase+1);
            check(input.delivered(native,out,out.buttons,native.sampled_at,phase+1),"completion negative delivers complete second press");
        }
        auto changed=frame(50,5,0x20);++changed.generations.action;
        auto repeated=input.controls(changed,50,4);
        check(repeated.owned&&repeated.phase==3&&repeated.buttons==0x2000,
            "early native acknowledgment cannot release on the same controller tick as second press");
        auto release=input.controls(changed,50,5);
        check(release.owned&&release.buttons==0&&!input.observed_change(),"early native acknowledgment still requires final released packet");
        if(interruption==1) {input.cancel();check(!input.active()&&!input.observed_change(),"OFF cancels pending release without false completion");continue;}
        if(interruption==2)++changed.generations.owner;
        const bool delivered=input.delivered(changed,release,0,50,5);
        if(interruption==2) {check(!delivered&&!input.observed_change(),"owner loss between sample and final packet cancels");continue;}
        check(delivered,"final release has exact delivery receipt");
        changed.physics_step=6;changed.sampled_at=60;
        check(!input.controls(changed,60,6).owned&&input.observed_change(),"completion publishes only after final B release");
    }
    StanceInput input;input.start(press(),frame(),10);auto c=input.controls(frame(),10,1);
    input.delivered(frame(),c,0,10,1);c=input.controls(frame(20,2),20,2);input.delivered(frame(20,2),c,0x2000,20,2);
    check(!input.controls(frame(30,3,0x20),30,3).owned&&!input.observed_change(),
        "foot change before second press is unrelated and cannot acknowledge Switch");
}
void lifecycle_and_journal() {
    for(unsigned phase=0;phase<5;++phase)for(unsigned fault=0;fault<8;++fault) {
        StanceInput input;input.start(press(),frame(),10);
        for(unsigned i=0;i<=phase;++i) {
            const auto native=frame(10+i*10,i+1);const auto out=input.controls(native,native.sampled_at,i+1);
            check(input.delivered(native,out,out.buttons,native.sampled_at,i+1),"each cancellation phase has exact published history");
        }
        const auto at=20+phase*10;auto native=frame(at,phase+2);
        if(fault==0)++native.generations.owner;
        if(fault==1)++native.generations.contact;
        if(fault==2)++native.flip.object;
        if(fault==3)native.airborne=true;
        if(fault==4)native.flip.stance=0x40;
        if(fault==5)native.flip.state=602;
        if(fault==6)native.required=false;
        if(fault==7)input.cancel();
        check(!input.controls(native,at,phase+2).owned&&!input.observed_change()&&!input.active(),
            "OFF, owner loss, airborne, bail and stale evidence cancel every finite output phase");
    }
    InputEvents events;TriggerJournal journal;StanceInput input;PhysicalInputSample pad{};
    auto native=frame();
    const auto sample=[&](std::uint64_t now) {
        native.sampled_at=now;native.physics_step=now;
        events.sample(now,pad,false,false,InputPolicy::production());
        journal.observe(events.history(),InputContext::ground,native);
    };
    sample(0);pad.rt=255;sample(10);
    check(journal.claim(1,{TriggerOwner::switch_stance,160},events.history(),10),"production journal claims first Switch edge");
    const auto first=journal.get(1);
    check(input.start(first,native,10),"production original edge starts finite producer");
    input.cancel();events.reset();journal.reset();++native.generations.owner;sample(20);
    check(!journal.get(1),"OFF and owner replacement do not replay held RT");
    check(!input.start(first,native,20),"retired old-owner trigger cannot restart output");
    pad.rt=0;sample(30);pad.rt=255;sample(40);
    check(journal.claim(1,{TriggerOwner::switch_stance,190},events.history(),40),"fresh new-owner press is claimable");
    check(journal.get(1).press.sequence>first.press.sequence,"journal reset preserves physical sequence watermark");
    check(input.start(journal.get(1),native,40),"fresh press after OFF starts despite retired prior lease");
    auto foreign=native;++foreign.generations.owner;
    check(input.same_owner(native)&&!input.same_owner(foreign),"adapter can retire old-owner lease before dispatching replacement's first press");
}
void transient_acknowledgement() {
    for(unsigned stance:{0u,0x20u})for(unsigned revert_at=0;revert_at<3;++revert_at) {
        StanceInput input;input.start(press(frame(10,1,stance)),frame(10,1,stance),10);
        for(unsigned phase=0;phase<4;++phase) {
            auto native=frame(10+phase*10,phase+1,stance);
            const auto output=input.controls(native,native.sampled_at,phase+1);
            check(input.delivered(native,output,output.buttons,native.sampled_at,phase+1),
                "transient acknowledgment starts with exact second press delivery");
        }
        auto changed=frame(50,5,stance^0x20u);
        auto release=input.controls(changed,50,revert_at==2?4u:5u);
        check(!input.observed_change(),"transient early foot change is not completion");
        if(revert_at==2) {
            check(release.owned&&release.buttons==0x2000,"duplicate tick preserves delivered second press");
            release=input.controls(frame(60,6,stance),60,6);
        }
        check(release.owned&&release.buttons==0,"finite producer still releases B");
        const auto delivery=revert_at==0?frame(55,6,stance):revert_at==2?frame(60,6,stance):changed;
        check(input.delivered(delivery,release,0,delivery.sampled_at,revert_at==2?6u:5u),
            "final released packet is published even when the leading foot returns");
        check(input.controls(frame(70,7,stance),70,7).owned&&!input.observed_change(),
            "latched transient leading-foot change cannot acknowledge original stance after final release");
        check(input.controls(frame(80,8,stance),80,8).owned&&!input.observed_change(),
            "phase5 repeated observations cannot revive a transient acknowledgment");
        check(!input.controls(frame(90,9,stance^0x20u),90,9).owned&&input.observed_change(),
            "current actual changed leading foot can complete after the finalized release");
    }
}

void awaiting_acknowledgement_reserves_neutral() {
    for(unsigned cadence:{8u,17u,33u})for(unsigned stance:{0u,0x20u})for(bool acknowledge:{false,true}) {
        StanceInput input;check(input.start(press(frame(10,1,stance)),frame(10,1,stance),10),
            "observation reservation starts from original physical RT");
        for(unsigned phase=0;phase<5;++phase) {
            const auto native=frame(10+phase*cadence,phase+1,stance);
            const auto output=input.controls(native,native.sampled_at,phase+1);
            check(input.delivered(native,output,output.buttons,native.sampled_at,phase+1),
                "observation reservation follows exact finite double tap");
        }
        const auto first_down=10+cadence;
        unsigned step=6;
        for(auto at=10+5*cadence;at<=first_down+1000;at+=cadence,++step) {
            const auto native=frame(at,step,stance);
            const auto output=input.controls(native,at,step);
            // Same final-controls rule as arcade_mode.cpp. Returning an unowned
            // phase5 lets auto-push immediately cancel this native observation.
            const bool automatic_push=true;
            const auto native_faces=automatic_push&&!output.owned?0x4000u:0u;
            check(!native_faces&&output.owned&&output.phase==5&&output.buttons==0,
                "pending stance acknowledgment suppresses automatic X with owned neutral B");
            check(input.delivered(native,output,static_cast<std::uint16_t>(native_faces),at,step),
                "phase5 accepts only its final neutral receipt");
            check(input.controls(native,at,step)==output&&
                input.delivered(native,output,0,at,step)&&!input.observed_change(),
                "duplicate neutral polls neither advance phase nor invent native success");
            if(acknowledge) {
                const auto changed=frame(at+cadence,step+1,stance^0x20u);
                check(!input.controls(changed,changed.sampled_at,step+1).owned&&
                    input.observed_change()&&!input.active(),"actual delayed foot change releases the reservation immediately");
                break;
            }
        }
        if(!acknowledge) {
            auto native=frame(first_down+1000,++step,stance);
            const auto boundary=input.controls(native,native.sampled_at,step);
            check(boundary.owned&&input.delivered(native,boundary,0,native.sampled_at,step),
                "existing inclusive observation ceiling remains valid");
            native=frame(first_down+1001,++step,stance);
            check(!input.controls(native,native.sampled_at,step).owned&&!input.active()&&!input.observed_change(),
                "repeated phase5 delivery cannot renew original one-second observation ceiling");
        }
    }
    for(unsigned cadence:{8u,17u,33u})for(unsigned fault=0;fault<8;++fault) {
        StanceInput input;input.start(press(),frame(),10);
        for(unsigned phase=0;phase<5;++phase) {
            const auto native=frame(10+phase*cadence,phase+1);
            const auto output=input.controls(native,native.sampled_at,phase+1);
            check(input.delivered(native,output,output.buttons,native.sampled_at,phase+1),"negative reaches final B release");
        }
        auto native=frame(10+5*cadence,6);
        const auto neutral=input.controls(native,native.sampled_at,6);
        check(neutral.owned&&neutral.phase==5,"negative starts during acknowledgment observation");
        if(fault==0)++native.generations.owner;
        if(fault==1)++native.generations.contact;
        if(fault==2)++native.flip.object;
        if(fault==3)native.airborne=true;
        if(fault==4)native.flip.stance=0x40;
        if(fault==5)native.flip.state=602;
        if(fault==6)input.cancel(); // Existing callsite cancels on OFF or conflicting physical input.
        if(fault==7)check(!input.delivered(native,neutral,0x2000,native.sampled_at,6),
            "foreign final B cannot be mistaken for the observation neutral");
        check(!input.controls(native,native.sampled_at,6).owned&&!input.active()&&!input.observed_change(),
            "owner loss, push, AIR, bail, explicit cancellation and foreign final output release observation");
    }
}

}
int main(){native_transition_scope();awaiting_acknowledgement_reserves_neutral();exported_asset_identity();original_edge_and_delivery();interruptions();denied_sources();finite_double_tap();packet_and_phase_guards();completion_requires_release();lifecycle_and_journal();transient_acknowledgement();std::printf("stance input: %u checks passed\n",checks);}

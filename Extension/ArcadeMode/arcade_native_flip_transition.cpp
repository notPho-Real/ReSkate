#include "arcade_native_flip_transition.h"
#include "arcade_native_flip_entry.h"
#include "Engine/Core/Hooks/hooks.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Core/Log/logging.h"
#include "Engine/Game/Abi/native_data.h"
#include "Engine/Game/Build/image_identity.h"
#include "Engine/Game/Build/20260929/arcade_flip_transition.h"
#include <atomic>
#include <mutex>
#include <Windows.h>

namespace dingosdk::arcade_mode::native_flip {
namespace {
namespace contract=game::build::v20260929::arcade_flip_transition;
using Candidate=std::uint32_t(*)(std::uintptr_t,std::uint32_t,std::uintptr_t,float);
using Evaluate=std::uintptr_t(*)(std::uintptr_t,std::uintptr_t,float,std::uintptr_t);
using Commit=bool(*)(std::uintptr_t,std::uintptr_t,std::uintptr_t,bool);
Candidate original_candidate{}; Evaluate original_evaluate{}; Commit original_commit{};
std::uintptr_t image{};
std::atomic<bool> hooks_ready{}, assets_ready{}, gameplay_allowed{};
std::mutex mutex;
struct Lease {
    NoBailSkater owner{};
    std::uintptr_t block{};
    std::uint64_t serial{}, expires{};
    std::int32_t requested{};
    bool airborne{}, delivered{};
    std::uintptr_t initialized_ptm{};
};
Lease lease;
std::uint64_t next_serial{}, retry_at{};
struct StateAsset { const char* name; Guid guid; std::uintptr_t object{}; };
std::array<StateAsset,4> assets{{
    {"Animation/Dingo/EnumGS.Anim.CurrentFlipTrick",contract::current},
    {"Animation/Dingo/EnumGS.Intent.FlipTrick",contract::intent},
    {"Animation/Dingo/Bool.Anim.Stateflow.IsFlipTrickChanged",contract::changed},
    {"Animation/Dingo/Bool.Intent.AllowFlipTrickReselection",contract::allow}
}};
template<class T> bool read(std::uintptr_t at,T& value) noexcept { return memory::peek(at,value); }
std::uintptr_t pointer(std::uintptr_t at) noexcept { std::uintptr_t p{};return read(at,p)?p:0; }
bool guid_is(std::uintptr_t object,const Guid& expected) noexcept {
    Guid value{};return object>=0x10010 && read(object-16,value) && value==expected;
}
std::uintptr_t state_block(const NoBailSkater& owner) noexcept {
    const auto collection=pointer(owner.entity+0x70);
    std::uint8_t count{};
    if (!collection || pointer(collection)!=owner.entity || !read(collection+8,count) || count>128) return 0;
    for (unsigned i=0;i<count;++i) {
        const auto component=pointer(collection+0x20+i*0x20);
        if (component && pointer(component)==image+0x63aaee8) {
            const auto animatable=pointer(component+0x68);
            const auto holder=pointer(animatable+0xe8);
            const auto block=pointer(holder+8);
            return pointer(block)==image+0x61a8bd8?block:0;
        }
    }
    return 0;
}
std::uintptr_t state_address(const Lease& l,unsigned index) noexcept {
    const auto& asset=assets[index]; std::uint32_t handle{},valid{};
    if (!guid_is(asset.object,asset.guid) || !read(asset.object+0x28,handle)) return 0;
    const auto page=pointer(l.block+0x60+8*(handle&4095));
    const auto offset=(handle>>12)&2047;
    if (!page || !read(page+0x71c+4*(offset>>5),valid) || !(valid&(1u<<(offset&31)))) return 0;
    return page+offset;
}
template<class T> bool state(const Lease& l,unsigned index,T& value) noexcept {
    const auto at=state_address(l,index); return at && read(at,value);
}
bool write_intent(const Lease& l,std::int32_t value) noexcept {
    const auto at=state_address(l,1); if (!at || at%4) return false;
    __try { *reinterpret_cast<std::int32_t*>(at)=value;return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
bool valid(const Lease& l) noexcept {
    if (!l.serial || !gameplay_allowed.load() || !assets_ready.load() || GetTickCount64()>l.expires) return false;
    NoBailSkater owner{};
    return no_bail_skater(owner) && owner==l.owner && state_block(owner)==l.block;
}
bool in_air(const Lease& l) noexcept {
    std::uint32_t value{};return read(l.owner.context+0x1414,value) && (value==200 || value==201);
}
bool rising(const Lease& l) noexcept {
    const auto board=pointer(pointer(l.owner.core+0x430)+0x18);
    const auto bodies=pointer(board+0x20); std::uint32_t count{};float vy{};
    return pointer(board)==image+0x65e67a0 && pointer(bodies+0x10)==board && read(bodies,count) && count==9 &&
        read(bodies+0x74,vy) && std::isfinite(vy) && vy>0.25f;
}
Lease snapshot() noexcept { std::lock_guard lock(mutex);return lease; }
std::uint32_t candidate(std::uintptr_t context,std::uint32_t index,std::uintptr_t view,float delta) {
    auto l=snapshot();
    if (valid(l) && view==l.block+0x48 && in_air(l)) {
        std::int32_t current{}, intent{}; bool changed{},allowed{};
        if (state(l,0,current) && state(l,1,intent) && state(l,2,changed) && state(l,3,allowed)) {
            const bool ollie=current==1 || current==16;
            if (ollie && l.requested && !l.delivered && allowed && rising(l) && (intent==0 || intent==current)) {
                std::lock_guard lock(mutex);
                if (lease.serial==l.serial && write_intent(l,l.requested)) lease.delivered=l.delivered=true;
            }
            // Keep the existing launch node available for its native IsChanged
            // self-transition. Pending selection must survive its evaluation;
            // ID changes alone are not animation acknowledgments.
            if ((ollie && rising(l)) || (l.delivered && current==l.requested && changed)) {
                const auto node=pointer(context+8), entries=pointer(node+0x98);
                std::uint32_t count{};
                if (entries && read(entries-4,count) && index<(count&0x7fffffff) && index<255) {
                    const auto transition=pointer(entries+48*index+8)&~std::uintptr_t{4};
                    if (guid_is(transition,contract::exit_early) || guid_is(transition,contract::exit_done)) return 255;
                }
            }
        }
    }
    return original_candidate(context,index,view,delta);
}
struct Evaluation { Lease lease{}; std::uintptr_t ptm{}; bool initialized{}; float entry{}; };
thread_local Evaluation* evaluating{};
bool commit(std::uintptr_t path,std::uintptr_t result,std::uintptr_t parameters,bool update) {
    auto* e=evaluating;
    if (e && path==e->ptm+0x70 && valid(e->lease) && in_air(e->lease)) {
        float duration{};std::int32_t selected{};
        if (read(e->ptm+0x20,duration) && duration>1e30f && state(e->lease,0,selected) && selected==e->lease.requested) {
            if (const auto entry=post_pop_entry([](auto at,auto& out){return read(at,out);},image,result)) {
                // Stack-owned selection result; native commit below installs
                // its pose/path and native ef2f70 owns actor init and cleanup.
                *reinterpret_cast<std::uint32_t*>(result)=entry->key;
                e->initialized=true;e->entry=entry->tick;
            }
        }
    }
    const bool committed=original_commit(path,result,parameters,update);
    if (committed && e && e->initialized) {
        std::lock_guard lock(mutex);
        if (lease.serial==e->lease.serial) lease.initialized_ptm=e->ptm;
    }
    return committed;
}
std::uintptr_t evaluate(std::uintptr_t ptm,std::uintptr_t output,float delta,std::uintptr_t context) {
    const auto l=snapshot();
    Evaluation e{l,ptm};
    auto* previous=evaluating;
    const bool eligible=valid(l) && l.delivered && ordinary_flip(l.requested) &&
        pointer(context+8)==l.block+0x48 && pointer(ptm)==image+0x617e0d0 &&
        guid_is(pointer(ptm+0x8960),contract::launch_ptm);
    evaluating=eligible?&e:nullptr;
    const auto result=original_evaluate(ptm,output,delta,context);
    evaluating=previous;
    // Zero-delta prediction creates disposable controllers. Only the real
    // advancing evaluation can complete delivery; do not retire on prediction.
    if (eligible && delta>0 && valid(l)) {
        float duration{},ticks{};std::int32_t selected{};
        if (state(l,0,selected) && selected==l.requested && read(ptm+0x20,duration) &&
            duration>0 && duration<10000 && read(ptm+0x1c,ticks) && ticks>0 && pointer(ptm+0x8970)) {
            std::lock_guard lock(mutex);
            if (lease.serial==l.serial && lease.initialized_ptm==ptm) {
                std::int32_t intent{};
                if (state(l,1,intent) && intent==l.requested) write_intent(l,0);
                logging::log(logging::Level::info,logging::Channel::skater,
                    "ARCADE NATIVE FLIP: serial={} selected={} ticks={} duration={} native_evaluation=true.",l.serial,selected,ticks,duration);
                lease={};
            }
        }
    }
    return result;
}
}
void cancel() noexcept {
    std::lock_guard lock(mutex);
    NoBailSkater owner{};std::int32_t intent{};
    if (lease.delivered && no_bail_skater(owner) && owner==lease.owner && state_block(owner)==lease.block &&
        state(lease,1,intent) && intent==lease.requested) write_intent(lease,0);
    lease={};
}
bool available() noexcept { return hooks_ready.load() && assets_ready.load() && gameplay_allowed.load(); }
void tick(bool gameplay) noexcept {
    gameplay_allowed.store(gameplay);
    if (!gameplay) { cancel();return; }
    if (!hooks_ready.load()) return;
    if (!assets_ready.load() && GetTickCount64()>=retry_at) {
        retry_at=GetTickCount64()+1000;
        const auto find=game::native_data().find_asset;
        if (find) {
            bool complete=true;
            for (auto& asset:assets) {
                if (guid_is(asset.object,asset.guid)) continue;
                asset.object=0;
                for (std::uint16_t domain=0;domain<0xbbf;++domain) {
                    if (!pointer(image+contract::domain_owners+8ULL*domain)) continue;
                    const auto object=find(domain,asset.name);
                    if (guid_is(object,asset.guid)) { asset.object=object;break; }
                }
                complete=complete && asset.object;
            }
            assets_ready.store(complete);
        }
    }
    const auto l=snapshot();
    if (!l.serial) return;
    const bool air=in_air(l);
    if (!valid(l) || (l.airborne && !air)) { cancel();return; }
    if (air) { std::lock_guard lock(mutex);if (lease.serial==l.serial) lease.airborne=true; }
}
void arm(const NoBailSkater& owner) noexcept {
    if (!available()) return;
    const auto block=state_block(owner);
    if (!block) return;
    std::lock_guard lock(mutex);
    lease={owner,block,++next_serial,GetTickCount64()+2500};
}
bool request(const NoBailSkater& owner,std::int32_t id) noexcept {
    if (!available() || !ordinary_flip(id)) return false;
    auto l=snapshot();
    if (!valid(l) || owner!=l.owner || !in_air(l) || !rising(l)) return false;
    std::lock_guard lock(mutex);
    if (lease.serial!=l.serial || lease.requested) return false;
    lease.requested=id;return true;
}
bool start(std::uintptr_t base) noexcept {
    if (hooks_ready.load()) return true;
    if (!supported_build::running_image_matches(base)) return false;
    for (const auto& fp:contract::functions) {
        std::array<unsigned char,32> bytes{};
        if (!read(base+fp.rva,bytes) || bytes!=fp.bytes) return false;
    }
    image=base;
    std::array<void*,3> targets{};
    const std::array<void*,3> replacements{reinterpret_cast<void*>(&candidate),reinterpret_cast<void*>(&evaluate),reinterpret_cast<void*>(&commit)};
    std::array<void*,3> originals{};
    auto rollback=[&] { for (auto target:targets) if (target) { hook_disable(target);hook_remove(target); } };
    for (unsigned i=0;i<3;++i) {
        targets[i]=reinterpret_cast<void*>(base+contract::functions[i].rva);
        if (hook_prepare(targets[i],replacements[i],&originals[i])!=HookOk) { rollback();return false; }
    }
    original_candidate=reinterpret_cast<Candidate>(originals[0]);
    original_evaluate=reinterpret_cast<Evaluate>(originals[1]);
    original_commit=reinterpret_cast<Commit>(originals[2]);
    for (auto target:targets) if (hook_queue_enable(target)!=HookOk) { rollback();return false; }
    if (hook_apply_queued()!=HookOk) {
        rollback();
        return false;
    }
    hooks_ready.store(true);return true;
}
}

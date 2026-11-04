#include "arcade_native_footplant.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Core/Log/logging.h"
#include "Engine/Game/Abi/native_data.h"
#include "Engine/Game/Input/gameplay_options.h"
#include "Engine/Game/Build/20260929/arcade_footplant.h"
#include "Engine/Game/Build/20260929/arcade_flip_transition.h"
#include <atomic>
#include <mutex>
#include <Windows.h>

namespace dingosdk::arcade_mode::native_footplant {
namespace {
namespace contract=game::build::v20260929::arcade_footplant;
struct Asset {const char* name;contract::Guid guid;std::uintptr_t object{};};
std::array<Asset,6> assets{{
    {"Animation/Dingo/Bool.GP.CanNoComply",contract::can_no_comply},
    {"Animation/Dingo/Bool.Anim.Is.NoComply",contract::no_comply},
    {"Animation/Dingo/Bool.Intent.Boneless",contract::boneless},
    {"Animation/Dingo/Bool.Intent.LeftHandGrab",contract::left},
    {"Animation/Dingo/Bool.Intent.RightHandGrab",contract::right},
    {"Animation/Dingo/EnumGS.GrabType",contract::family}
}};
constexpr auto option_owner="arcade-native-footplant";
constexpr std::array option_keys{"egf_invertxaonboard","egf_frontbackfoot","nocomply"};
std::atomic<bool> permitted{},ready{};
std::uintptr_t image{};
std::uint64_t retry_at{},settings_since{},last_poll{},last_step{},serial{};
std::mutex mutex;
FootplantInput lease;
NoBailSkater owner;
struct Evidence {std::uint64_t serial{};Footplant kind{};bool entered{};unsigned state{};} evidence;
template<class T> bool read(std::uintptr_t at,T& value) noexcept {return memory::peek(at,value);}
std::uintptr_t pointer(std::uintptr_t at) noexcept {std::uintptr_t value{};return read(at,value)?value:0;}
bool guid_is(std::uintptr_t object,const contract::Guid& expected) noexcept {
    contract::Guid actual{};return object>=0x10000+contract::asset_guid_prefix &&
        read(object-contract::asset_guid_prefix,actual) && actual==expected;
}
std::uintptr_t block_for(const NoBailSkater& skater) noexcept {
    const auto collection=pointer(skater.entity+contract::collection);std::uint8_t count{};
    if (!collection || pointer(collection+contract::collection_owner)!=skater.entity ||
        !read(collection+contract::collection_count,count) || count>128) return 0;
    for (unsigned i=0;i<count;++i) {
        const auto component=pointer(collection+contract::components+i*contract::component_stride);
        if (component && pointer(component)==image+contract::component_type) {
            const auto block=pointer(pointer(pointer(component+contract::animatable)+contract::holder)+contract::state_block);
            return pointer(block)==image+contract::block_type?block:0;
        }
    }
    return 0;
}
template<class T> bool state(std::uintptr_t block,unsigned index,T& value) noexcept {
    const auto& asset=assets[index];std::uint32_t handle{},valid{};
    if (!guid_is(asset.object,asset.guid) || !read(asset.object+contract::handle,handle) ||
        !(handle>>24) || (handle&4095)>=16) return false;
    const auto page=pointer(block+contract::pages+8*(handle&4095));const auto offset=(handle>>12)&2047;
    return page && read(page+contract::valid_words+4*(offset>>5),valid) &&
        (valid&(1u<<(offset&31))) && read(page+offset,value);
}
bool flag(std::uintptr_t block,unsigned index) noexcept {
    std::uint8_t value{};return state(block,index,value) && value==1;
}
FootplantObservation observe(const NoBailSkater& skater,std::uint64_t now,std::uint64_t step) noexcept {
    FootplantObservation out{};NoBailSkater current{};
    if (!permitted.load() || !ready.load() || !no_bail_skater(current) || current!=skater) return out;
    const auto block=block_for(skater);
    if (!block || !read(skater.context+contract::physics_state,out.state)) return out;
    out.valid=true;out.sampled_at=now;out.step=step;
    out.can_no_comply=flag(block,0);out.no_comply=flag(block,1);out.boneless=flag(block,2);
    out.left_hand=flag(block,3);out.right_hand=flag(block,4);
    if (!state(block,5,out.grab)) out.grab=0;
    return out;
}
void retire() noexcept {
    lease.cancel();owner={};settings_since=last_poll=last_step=0;
    try {input::gameplay_options().release(option_owner);} catch (...) { }
}
bool settings(std::uint64_t now,bool require_sample) noexcept {
    try {
        auto& options=input::gameplay_options();
        const bool no_comply=lease.kind()==Footplant::no_comply;
        const std::array values{false,no_comply,no_comply};
        for (unsigned i=0;i<option_keys.size();++i)
            if (!options.hold(option_owner,option_keys[i],values[i],now).empty()) return false;
        if (!settings_since) settings_since=now+1;
        if (require_sample) for (unsigned i=0;i<option_keys.size();++i) {
            const auto sample=options.sampled(option_keys[i],settings_since,now);
            if (!sample || *sample!=values[i]) return false;
        }
        return true;
    } catch (...) {return false;}
}
}
bool available() noexcept {return permitted.load() && ready.load();}
void cancel() noexcept {
    std::lock_guard lock(mutex);
    if (lease.active()) {
        const auto kind=lease.kind();const auto now=GetTickCount64();
        const auto native=observe(owner,now,last_step);
        // The controller facade intentionally excludes the PTM state from
        // ordinary gameplay admission. Record its original entry before
        // retiring controls; never broaden admission to keep this lease alive.
        lease.poll(now,native,false);
        evidence={serial,kind,lease.entered(),native.state};
    }
    retire();
}
void tick(std::uintptr_t base,bool gameplay) noexcept {
    permitted.store(gameplay);
    Evidence pending{};
    {
        std::lock_guard lock(mutex);pending=evidence;evidence={};
        const auto now=GetTickCount64();
        if (!gameplay || (lease.active() && (now<last_poll || now-last_poll>150))) retire();
    }
    if (pending.serial) logging::log(logging::Level::info,logging::Channel::skater,
        "ARCADE NATIVE FOOTPLANT END: serial={} kind={} native_entry={} state={} animation_acceptance=unverified launch_scoring_acceptance=unverified.",
        pending.serial,static_cast<unsigned>(pending.kind),pending.entered,pending.state);
    if (!gameplay || !base || (image && image!=base)) return;
    if (!image) image=base;
    if (!ready.load() && GetTickCount64()>=retry_at) {
        retry_at=GetTickCount64()+1000;const auto find=game::native_data().find_asset;if(!find)return;
        bool complete=true;
        for (auto& asset:assets) {
            if (guid_is(asset.object,asset.guid)) continue;
            asset.object=0;
            for (std::uint16_t domain=0;domain<0xbbf;++domain) {
                if (!pointer(image+game::build::v20260929::arcade_flip_transition::domain_owners+8ULL*domain)) continue;
                const auto object=find(domain,asset.name);
                if (guid_is(object,asset.guid)) {asset.object=object;break;}
            }
            complete=complete && asset.object;
        }
        ready.store(complete);
    }
}
bool request(const NoBailSkater& skater,std::uint64_t step,Footplant kind) noexcept {
    std::lock_guard lock(mutex);const auto now=GetTickCount64();
    if (!lease.start(kind,now,observe(skater,now,step))) return false;
    owner=skater;last_poll=now;last_step=step;++serial;
    if (!settings(now,false)) {retire();return false;}
    return true;
}
FootplantControl controls(const NoBailSkater& skater,std::uint64_t step) noexcept {
    std::lock_guard lock(mutex);const auto now=GetTickCount64();
    if (!lease.active() || skater!=owner) {retire();return {};}
    last_poll=now;last_step=step;const auto kind=lease.kind();const auto native=observe(skater,now,step);
    const auto out=lease.poll(now,native,settings(now,true));
    if (!out.owned) {evidence={serial,kind,lease.entered(),native.state};retire();}
    return out;
}
}

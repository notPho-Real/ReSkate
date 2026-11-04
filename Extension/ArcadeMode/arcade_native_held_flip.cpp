#include "arcade_native_held_flip.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Core/Log/logging.h"
#include "Engine/Game/Abi/native_data.h"
#include "Engine/Game/Build/20260929/arcade_held_flip.h"
#include "Engine/Game/Build/20260929/arcade_flip_transition.h"
#include <atomic>
#include <mutex>
#include <Windows.h>

namespace dingosdk::arcade_mode::native_held_flip {
namespace {
namespace contract=game::build::v20260929::arcade_held_flip;
namespace flip=game::build::v20260929::arcade_flip_transition;
struct Asset {const char* name;contract::Guid guid;std::uintptr_t object{};};
std::array<Asset,5> assets{{
    {"Animation/Dingo/EnumGS.Anim.CurrentFlipTrick",flip::current},
    {"Animation/Dingo/Bool.Anim.PrepareHeldFlip",contract::prepare},
    {"Animation/Dingo/Bool.Intent.HeldFlip",contract::intent},
    {"Animation/Dingo/Bool.State.Is.HeldFlip",contract::active},
    {"Animation/Dingo/Float.Anim.TrickHeldFlipCount",contract::extra_rotations}
}};
std::uintptr_t image{};
std::uint64_t retry_at{},next_serial{};
std::atomic<bool> permitted{},ready{},tracking{};
std::mutex mutex;
struct Observation {
    NoBailSkater owner{};
    std::uintptr_t block{},actor{};
    std::uint64_t serial{},sampled_at{},advanced_at{};
    std::int32_t id{};
    bool initialized{},advanced{},reported{};
} observation;
template<class T> bool read(std::uintptr_t at,T& value) noexcept {return memory::peek(at,value);}
std::uintptr_t pointer(std::uintptr_t at) noexcept {std::uintptr_t p{};return read(at,p)?p:0;}
bool guid_is(std::uintptr_t object,const contract::Guid& expected) noexcept {
    contract::Guid actual{};
    return object>=0x10010 && read(object-16,actual) && actual==expected;
}
std::uintptr_t block_for(const NoBailSkater& owner) noexcept {
    const auto collection=pointer(owner.entity+contract::collection);std::uint8_t count{};
    if (!collection || pointer(collection+contract::collection_owner)!=owner.entity ||
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
bool state_bool(std::uintptr_t block,unsigned index,bool& value) noexcept {
    std::uint8_t raw{};
    if (!state(block,index,raw) || raw>1) return false;
    value=raw!=0;return true;
}
bool current() noexcept {
    const auto now=GetTickCount64();NoBailSkater owner{};std::uint32_t state{};
    return permitted.load() && observation.serial && now>=observation.sampled_at &&
        now-observation.sampled_at<=150 && no_bail_skater(owner) && owner==observation.owner &&
        block_for(owner)==observation.block && read(owner.context+contract::physics_state,state) &&
        (state==200 || state==201);
}
bool exact_actor(std::uintptr_t actor) noexcept {
    const auto asset=pointer(actor+contract::actor_asset);
    return pointer(asset)==image+contract::actor_type &&
        guid_is(asset,observation.id==2?contract::kickflip_loop:contract::heelflip_loop);
}
}
void tick(std::uintptr_t base,bool gameplay) noexcept {
    permitted.store(gameplay);
    if (!gameplay) {tracking.store(false);std::lock_guard lock(mutex);observation={};return;}
    if (!base || (image && image!=base)) return;
    if (!image) image=base;
    if (!ready.load() && GetTickCount64()>=retry_at) {
        retry_at=GetTickCount64()+1000;const auto find=game::native_data().find_asset;
        if (!find) return;
        bool complete=true;
        for (auto& asset:assets) {
            if (guid_is(asset.object,asset.guid)) continue;
            asset.object=0;
            for (std::uint16_t domain=0;domain<0xbbf;++domain) {
                if (!pointer(image+flip::domain_owners+8ULL*domain)) continue;
                const auto object=find(domain,asset.name);
                if (guid_is(object,asset.guid)) {asset.object=object;break;}
            }
            complete=complete && asset.object;
        }
        ready.store(complete);
    }
}
HeldFlipObservation observe(const NoBailSkater& owner,std::uint64_t now) noexcept {
    HeldFlipObservation out{};
    if (!permitted.load() || !ready.load()) return out;
    std::lock_guard lock(mutex);
    out.available=true;out.sampled_at=now;
    const auto block=block_for(owner);std::uint32_t phase{};
    if (!block || !read(owner.context+contract::physics_state,phase) || (phase!=200 && phase!=201) ||
        !state(block,0,out.id) || (out.id!=2 && out.id!=3) ||
        !state_bool(block,1,out.prepare) || !state(block,4,out.extra_rotations) ||
        !std::isfinite(out.extra_rotations) || out.extra_rotations<0 || out.extra_rotations>32) {
        tracking.store(false);observation={};return out;
    }
    // Held state pages are legitimately unallocated before the first native
    // EAT entry. Unknown is never positive acknowledgment of held ownership.
    bool intent{},active{};
    out.holding=state_bool(block,2,intent) && state_bool(block,3,active) && intent && active;
    out.valid=true;
    if (!observation.serial || owner!=observation.owner || block!=observation.block || out.id!=observation.id ||
        now<observation.sampled_at || now-observation.sampled_at>150) {
        observation={owner,block,0,++next_serial,now,0,out.id};
    }
    observation.sampled_at=now;
    out.actor_advanced=observation.initialized && observation.advanced &&
        now>=observation.advanced_at && now-observation.advanced_at<=150;
    tracking.store(true);
    return out;
}
void initialized(std::uintptr_t actor,std::uintptr_t view) noexcept {
    if (!tracking.load()) return;
    std::lock_guard lock(mutex);float duration{};
    if (!current() || view!=observation.block+contract::state_view || !exact_actor(actor) ||
        !read(actor+contract::actor_duration,duration) || !std::isfinite(duration) || duration<=0 || duration>10000) return;
    observation.actor=actor;observation.initialized=true;observation.advanced=false;observation.reported=false;
}
Evaluation before_evaluate(std::uintptr_t actor,std::uintptr_t context,float delta) noexcept {
    if (!tracking.load() || !std::isfinite(delta) || delta<=0) return {};
    std::lock_guard lock(mutex);float before{};
    if (!current() || !observation.initialized || actor!=observation.actor || !exact_actor(actor) ||
        pointer(context+contract::evaluation_view)!=observation.block+contract::state_view ||
        !read(actor+contract::actor_time,before) || !std::isfinite(before)) return {};
    return {actor,before,observation.serial};
}
void after_evaluate(Evaluation token,std::uintptr_t context,float delta) noexcept {
    if (!token.actor) return;
    std::lock_guard lock(mutex);float after{},count{};
    if (!current() || token.serial!=observation.serial || token.actor!=observation.actor ||
        pointer(context+contract::evaluation_view)!=observation.block+contract::state_view ||
        !exact_actor(token.actor) || !read(token.actor+contract::actor_time,after) ||
        !std::isfinite(after) || after<=token.before) return;
    observation.advanced=true;observation.advanced_at=GetTickCount64();
    if (!observation.reported && state(observation.block,4,count)) {
        observation.reported=true;
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE NATIVE HELD FLIP: serial={} native_id={} loop_initialized=true before={} after={} delta={} extra_rotations={} scoring_acceptance=unverified.",
            observation.serial,observation.id,token.before,after,delta,count);
    }
}
}

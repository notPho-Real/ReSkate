#include "arcade_native_footplant.h"
#include "arcade_native_ground_grab.h"
#include "arcade_native_asset_cache.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Core/Log/logging.h"
#include "Engine/Game/Abi/native_data.h"
#include "Engine/Game/Input/gameplay_options.h"
#include "Engine/Game/Build/20260929/arcade_footplant.h"
#include "Engine/Game/Build/20260929/arcade_ground_grab.h"
#include "Engine/Game/Build/20260929/arcade_flip_transition.h"
#include <atomic>
#include <cmath>
#include <mutex>
#include <Windows.h>

namespace dingosdk::arcade_mode {
namespace {
namespace contract=game::build::v20260929::arcade_footplant;
namespace grab_contract=game::build::v20260929::arcade_ground_grab;
struct Asset {const char* name;contract::Guid guid;std::uintptr_t object{};};
std::array<Asset,10> assets{{
    {"Animation/Dingo/Bool.GP.CanNoComply",contract::can_no_comply},
    {"Animation/Dingo/Bool.Anim.Is.NoComply",contract::no_comply},
    {"Animation/Dingo/Bool.Intent.Boneless",contract::boneless},
    {"Animation/Dingo/Bool.Intent.LeftHandGrab",contract::left},
    {"Animation/Dingo/Bool.Intent.RightHandGrab",contract::right},
    {"Animation/Dingo/EnumGS.GrabType",contract::family},
    {"Animation/Dingo/Bool.GP.CanGroundGrab",grab_contract::can_grab},
    {"Animation/Dingo/Bool.GP.Grab.LeftHand",grab_contract::left},
    {"Animation/Dingo/Bool.GP.Grab.RightHand",grab_contract::right},
    {"Animation/Dingo/Bool.State.Is.Coffin",grab_contract::coffin}
}};
constexpr auto option_owner="arcade-native-footplant";
constexpr std::array option_keys{"egf_invertxaonboard","egf_frontbackfoot","nocomply"};
std::atomic<bool> permitted{},ready{},grab_ready{},grab_active{};
std::uintptr_t image{};
std::uint64_t retry_at{},settings_since{},last_poll{},last_step{},serial{};
std::mutex mutex;
FootplantInput lease;
NoBailSkater owner;
struct Evidence {std::uint64_t serial{};Footplant kind{};bool entered{};unsigned state{};} evidence;
GroundGrabInput grab_lease;
NoBailSkater grab_owner;
std::uint64_t grab_poll{},grab_serial{};
struct GrabEvidence {std::uint64_t serial{};GroundGrab kind{};bool acknowledged{};} grab_evidence;
struct GrabActor {
    std::uintptr_t object{};
    std::uint64_t advanced_at{};
    float before{},after{},delta{};
    bool initialized{},reported{};
} grab_actor;
struct GrabAnimationEvidence {std::uint64_t serial{};GroundGrab kind{};GrabActor actor{};} grab_animation;
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
GroundGrabObservation observe_grab(const NoBailSkater& skater,std::uint64_t now,std::uint64_t step) noexcept {
    GroundGrabObservation out{};NoBailSkater current{};
    if (!permitted.load() || !grab_ready.load() || !no_bail_skater(current) || current!=skater) return out;
    const auto block=block_for(skater);
    std::uint8_t can{},left{},right{},coffin{};std::uint32_t coffin_handle{};
    // Before the first Coffin, the original asset has no allocated state
    // handle. That is not a corrupt read and must not prevent every ordinary
    // ground grab from starting. Once allocated, require the normal valid slot;
    // missing pages or failed reads never become a false Coffin observation.
    const bool coffin_unallocated=guid_is(assets[9].object,assets[9].guid) &&
        read(assets[9].object+contract::handle,coffin_handle) && !coffin_handle;
    if (!block || !read(skater.context+contract::physics_state,out.state) ||
        !state(block,6,can) || !state(block,7,left) || !state(block,8,right) ||
        (!coffin_unallocated && !state(block,9,coffin)) ||
        can>1 || left>1 || right>1 || coffin>1) return out;
    out.valid=true;out.sampled_at=now;out.step=step;
    out.can_grab=can!=0;out.left=left!=0;out.right=right!=0;out.coffin=coffin!=0;
    out.actor_advanced=grab_actor.initialized && grab_actor.advanced_at && now>=grab_actor.advanced_at &&
        now-grab_actor.advanced_at<=150 && grab_actor.delta>0 && grab_actor.after>grab_actor.before;
    return out;
}
void retire_grab() noexcept {
    if(grab_lease.active())grab_evidence={grab_serial,grab_lease.kind(),grab_lease.acknowledged()};
    grab_active.store(false);grab_lease.cancel();grab_owner={};grab_poll=0;grab_actor={};
}
bool ground_actor_is(std::uintptr_t actor) noexcept {
    const auto kind=static_cast<unsigned>(grab_lease.kind());
    const auto asset=pointer(actor+contract::actor_asset);
    return grab_lease.active() && kind>=1 && kind<=grab_contract::actors.size() &&
        pointer(asset)==image+contract::actor_type &&
        guid_is(asset,grab_contract::actors[kind-1]);
}
bool ground_actor_owner(std::uintptr_t view) noexcept {
    NoBailSkater current{};unsigned state{};
    if(!permitted.load() || !grab_ready.load() || !grab_lease.active() ||
        !no_bail_skater(current) || current!=grab_owner ||
        !read(current.context+contract::physics_state,state) || state!=100)return false;
    const auto block=block_for(current);return block && view==block+contract::state_view;
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
namespace native_footplant {
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
    Evidence pending{};GrabEvidence pending_grab{};GrabAnimationEvidence animation{};
    {
        std::lock_guard lock(mutex);pending=evidence;evidence={};
        const auto now=GetTickCount64();
        if (!gameplay || (lease.active() && (now<last_poll || now-last_poll>150))) retire();
        if (!gameplay || (grab_lease.active() && (now<grab_poll || now-grab_poll>150))) retire_grab();
        pending_grab=grab_evidence;grab_evidence={};
        animation=grab_animation;grab_animation={};
    }
    if (pending.serial) logging::log(logging::Level::info,logging::Channel::skater,
        "ARCADE NATIVE FOOTPLANT END: serial={} kind={} native_entry={} state={} animation_acceptance=unverified launch_scoring_acceptance=unverified.",
        pending.serial,static_cast<unsigned>(pending.kind),pending.entered,pending.state);
    if(pending_grab.serial)logging::log(logging::Level::info,logging::Channel::skater,
        "ARCADE NATIVE GROUND GRAB END: serial={} kind={} native_animation_acknowledged={} scoring_exit_acceptance=unverified.",
        pending_grab.serial,static_cast<unsigned>(pending_grab.kind),pending_grab.acknowledged);
    if(animation.serial)logging::log(logging::Level::info,logging::Channel::skater,
        "ARCADE NATIVE GROUND GRAB: serial={} kind={} actor={} initialized=true before={} after={} delta={} native_evaluation=true scoring_exit_acceptance=unverified.",
        animation.serial,static_cast<unsigned>(animation.kind),animation.actor.object,
        animation.actor.before,animation.actor.after,animation.actor.delta);
    if (!gameplay || !base || (image && image!=base)) return;
    if (!image) image=base;
    if (ready.load() && !native_assets_current(std::span{assets}.first<6>(),guid_is)) {
        ready.store(false);std::lock_guard lock(mutex);retire();
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE NATIVE ASSET CACHE: family=footplant invalidated=true.");
    }
    if (grab_ready.load() && !native_assets_current(std::span{assets}.subspan<6>(),guid_is)) {
        grab_ready.store(false);std::lock_guard lock(mutex);retire_grab();
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE NATIVE ASSET CACHE: family=ground_grab invalidated=true.");
    }
    if ((!ready.load() || !grab_ready.load()) && GetTickCount64()>=retry_at) {
        retry_at=GetTickCount64()+1000;const auto find=game::native_data().find_asset;if(!find)return;
        bool complete=true,ground_complete=true;
        for (unsigned i=0;i<assets.size();++i) {
            auto& asset=assets[i];
            if (guid_is(asset.object,asset.guid)) continue;
            asset.object=0;
            for (std::uint16_t domain=0;domain<0xbbf;++domain) {
                if (!pointer(image+game::build::v20260929::arcade_flip_transition::domain_owners+8ULL*domain)) continue;
                const auto object=find(domain,asset.name);
                if (guid_is(object,asset.guid)) {asset.object=object;break;}
            }
            if(i<6)complete=complete && asset.object;
            else ground_complete=ground_complete && asset.object;
        }
        ready.store(complete);
        grab_ready.store(ground_complete);
    }
}
bool request(const NoBailSkater& skater,std::uint64_t step,Footplant kind) noexcept {
    std::lock_guard lock(mutex);const auto now=GetTickCount64();
    if(grab_lease.active())return false;
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
namespace native_ground_grab {
bool available() noexcept {return permitted.load() && grab_ready.load();}
bool request(const NoBailSkater& skater,std::uint64_t step,GroundGrab kind) noexcept {
    std::lock_guard lock(mutex);const auto now=GetTickCount64();
    if(lease.active() || !grab_lease.start(kind,now,observe_grab(skater,now,step)))return false;
    grab_owner=skater;grab_poll=now;++grab_serial;grab_active.store(true);return true;
}
GroundGrabControl controls(const NoBailSkater& skater,std::uint64_t step,bool held) noexcept {
    std::lock_guard lock(mutex);const auto now=GetTickCount64();
    if(!grab_lease.active() || skater!=grab_owner){retire_grab();return {};}
    grab_poll=now;
    const GrabEvidence ending{grab_serial,grab_lease.kind(),grab_lease.acknowledged()};
    const auto out=grab_lease.poll(now,observe_grab(skater,now,step),held);
    if(!out.owned){grab_evidence=ending;retire_grab();}
    return out;
}
void cancel() noexcept {std::lock_guard lock(mutex);retire_grab();}
void initialized(std::uintptr_t actor,std::uintptr_t context) noexcept {
    if(!grab_active.load())return;
    std::lock_guard lock(mutex);float duration{};
    if(ground_actor_owner(context) && ground_actor_is(actor) &&
        read(actor+contract::actor_duration,duration) && std::isfinite(duration) && duration>0 && duration<10000)
        grab_actor={actor,0,0,0,0,true,false};
}
Evaluation before_evaluate(std::uintptr_t actor,std::uintptr_t context,float delta) noexcept {
    if(!grab_active.load())return {};
    std::lock_guard lock(mutex);float before{};
    if(!std::isfinite(delta) || delta<=0 || !ground_actor_owner(pointer(context+contract::evaluation_view)) ||
        !ground_actor_is(actor) || !grab_actor.initialized || grab_actor.object!=actor ||
        !read(actor+contract::actor_time,before) || !std::isfinite(before))return {};
    return {grab_serial,actor,before};
}
void after_evaluate(Evaluation token,std::uintptr_t context,float delta) noexcept {
    if(!token.serial)return;
    std::lock_guard lock(mutex);float after{};
    if(token.serial!=grab_serial || !ground_actor_owner(pointer(context+contract::evaluation_view)) ||
        !ground_actor_is(token.actor) || !grab_actor.initialized || grab_actor.object!=token.actor ||
        !read(token.actor+contract::actor_time,after) || !std::isfinite(after) || after<=token.before ||
        !std::isfinite(delta) || delta<=0)return;
    grab_actor.before=token.before;grab_actor.after=after;grab_actor.delta=delta;grab_actor.advanced_at=GetTickCount64();
    if(!grab_actor.reported) {grab_actor.reported=true;grab_animation={grab_serial,grab_lease.kind(),grab_actor};}
}
}
}

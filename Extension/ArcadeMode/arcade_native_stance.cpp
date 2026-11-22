#include "arcade_native_stance.h"
#include "arcade_input_dispatch.h"
#include "arcade_native_asset_cache.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Core/Log/logging.h"
#include "Engine/Game/Abi/native_data.h"
#include "Engine/Game/Build/20260929/arcade_stance.h"
#include <mutex>
#include <Windows.h>

namespace dingosdk::arcade_mode::native_stance {
namespace {
namespace contract=game::build::v20260929::arcade_stance;
struct Asset {const char* name{};contract::Guid guid{};std::uintptr_t object{};};
std::array<Asset,contract::assets.size()> assets{};
std::mutex mutex;
std::uintptr_t image{};
std::uint64_t retry_at{},last_poll{};
bool permitted{},ready{};
StanceInput input;
NoBailSkater owner;
#if defined(DINGOSDK_DEV_TOOLS)
struct Evidence {std::uint64_t sequence{};bool double_tap{},changed{};};
Evidence pending;
#endif
std::uintptr_t pointer(std::uintptr_t at) noexcept {
    std::uintptr_t value{};return memory::peek(at,value)?value:0;
}
bool guid_is(std::uintptr_t object,const contract::Guid& expected) noexcept {
    contract::Guid actual{};return object>=0x10000+contract::asset_guid_prefix &&
        memory::peek(object-contract::asset_guid_prefix,actual) && actual==expected;
}
bool assets_current() noexcept {return native_assets_current(std::span{assets},guid_is);}
// Admission and tap delivery require exact live/publication agreement. Only a
// final neutral reservation may await publication of a live leading-foot bit;
// that observation still uses the original frame lease and completion clock.
bool current(const NoBailSkater& skater,const InputNativeContext& native,std::uint64_t now,
             bool neutral_observation=false) noexcept {
    NoBailSkater before{},after{};std::uint32_t state{},stance{};
    return permitted && ready && assets_current() && native.required && native.fresh(now) &&
        no_bail_skater(before) && before==skater && before.base==image &&
        memory::peek(skater.context+contract::physics_state,state) && state==native.flip.state &&
        memory::peek(skater.context+contract::stance_flags,stance) &&
        ((stance&0x60u)==native.flip.stance ||
         (neutral_observation && input.awaiting_observation(native,now) &&
          ((stance&0x60u)^native.flip.stance)==game::build::v20260929::launches::stance_switch_mask)) &&
        no_bail_skater(after) && after==before;
}
void retire() noexcept {
#if defined(DINGOSDK_DEV_TOOLS)
    if(owner.context && input.sequence())pending={input.sequence(),input.double_tap_delivered(),input.observed_change()};
#endif
    input.cancel();owner={};last_poll=0;
}
}
void tick(std::uintptr_t base,bool gameplay) noexcept {
#if defined(DINGOSDK_DEV_TOOLS)
    Evidence evidence{};
#endif
    {
        std::lock_guard lock(mutex);const auto now=GetTickCount64();
        permitted=gameplay && base && (!image || image==base);
        if(!permitted || (input.active() && (now<last_poll || now-last_poll>InputTiming::native_sample_lease_ms)))retire();
        if(permitted) {
            if(!image) {
                image=base;
                for(std::size_t i=0;i<assets.size();++i)assets[i]={contract::assets[i].name,contract::assets[i].guid,0};
            }
            if(ready && !assets_current()){ready=false;retire();}
            if(!ready && now>=retry_at) {
                retry_at=now+1000;
                const auto find=game::native_data().find_asset;
                if(find) {
                    for(auto& asset:assets) {
                        if(guid_is(asset.object,asset.guid))continue;
                        asset.object=0;
                        for(std::uint16_t domain=0;domain<0xbbf;++domain) {
                            if(!pointer(image+contract::domain_owners+8ULL*domain))continue;
                            const auto object=find(domain,asset.name);
                            if(guid_is(object,asset.guid)){asset.object=object;break;}
                        }
                    }
                    ready=assets_current();
                }
            }
        }
#if defined(DINGOSDK_DEV_TOOLS)
        evidence=pending;pending={};
#endif
    }
#if defined(DINGOSDK_DEV_TOOLS)
    if(evidence.sequence)logging::log(logging::Level::info,logging::Channel::skater,
        "ARCADE NATIVE STANCE END: sequence={} double_tap_delivered={} leading_foot_changed={} physical_heading_momentum_acceptance=unverified.",
        evidence.sequence,evidence.double_tap,evidence.changed);
#endif
}
bool available() noexcept {
    std::lock_guard lock(mutex);return permitted && ready && assets_current();
}
bool request(const NoBailSkater& skater,const InputRequest& request,const InputNativeContext& native,std::uint64_t now) noexcept {
    std::lock_guard lock(mutex);
    // A replacement local skater can dispatch its first physical RT before
    // final_controls retires the old owner. Retire only that stale lease here.
    if(input.active() && (skater!=owner || !input.same_owner(native) || !current(skater,native,now) ||
       now<last_poll || now-last_poll>InputTiming::native_sample_lease_ms))retire();
    if(request.action!=InputAction::switch_stance || request.input_sequence!=request.trigger.press.sequence ||
       !current(skater,native,now) || !input.start(request.trigger,native,now))return false;
    owner=skater;last_poll=now;return true;
}
StanceControl controls(const NoBailSkater& skater,const InputNativeContext& native,std::uint64_t now,std::uint64_t tick) noexcept {
    std::lock_guard lock(mutex);
    if(!input.active())return {};
    if(skater!=owner || !current(skater,native,now,true)){retire();return {};}
    last_poll=now;
    const auto control=input.controls(native,now,tick);
    if(!input.active())retire();
    return control;
}
void delivered(const NoBailSkater& skater,const InputNativeContext& native,StanceControl control,
               std::uint16_t final_buttons,std::uint64_t now,std::uint64_t tick) noexcept {
    std::lock_guard lock(mutex);
    const bool neutral_observation=control.owned && control.phase==5 && !control.buttons && !final_buttons;
    if(skater!=owner || !current(skater,native,now,neutral_observation) ||
       !input.delivered(native,control,final_buttons,now,tick))retire();
}
void cancel() noexcept {std::lock_guard lock(mutex);retire();}
}

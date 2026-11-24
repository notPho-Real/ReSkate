#include "arcade_native_stance.h"
#include "arcade_input_dispatch.h"
#include "arcade_native_asset_cache.h"
#include "arcade_stance_transition_scope.h"
#include "Extension/Progression/local_entitlement_trigger_runtime.h"
#include "Engine/Core/Hooks/hooks.h"
#include "Engine/Game/Build/image_identity.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Core/Log/logging.h"
#include "Engine/Game/Abi/native_data.h"
#include "Engine/Game/Build/20260929/arcade_stance.h"
#include <atomic>
#include <mutex>
#include <Windows.h>

namespace dingosdk::arcade_mode::native_stance {
namespace {
namespace contract=game::build::v20260929::arcade_stance;
struct Asset {const char* name{};contract::Guid guid{};std::uintptr_t object{};};
std::array<Asset,1> assets{};
std::mutex mutex;
std::uintptr_t image{};
std::uint64_t retry_at{},last_poll{};
bool permitted{},ready{},hook_attempted{},hook_ready{};
using TransitionConsumer=void(*)(std::uintptr_t,const bool*,const bool*);
TransitionConsumer original_transition{};
std::atomic<bool> awaiting_consumer{};
StanceInputLease input;
InputNativeContext latest_native;
std::uint64_t serial{};
NoBailSkater owner;
#if defined(DINGOSDK_DEV_TOOLS)
struct Evidence {std::uint64_t sequence{};bool submitted{},changed{};};
Evidence pending;
struct ArbitrationObservation {
    bool valid{};
    std::uintptr_t scheduler{},state{},action{};
    std::int32_t state_priority{},action_priority{},requested_priority{};
};
ArbitrationObservation arbitration(std::uintptr_t action) noexcept {
    ArbitrationObservation result;std::uintptr_t data{};
    if(!action || !memory::peek(action+contract::action_data,data))return result;
    data&=~std::uintptr_t{4};
    result.valid=data && memory::peek(action+contract::action_scheduler,result.scheduler) && result.scheduler &&
        memory::peek(data+contract::data_priority,result.requested_priority) &&
        memory::peek(result.scheduler+contract::scheduler_pending_state,result.state) &&
        memory::peek(result.scheduler+contract::scheduler_state_priority,result.state_priority) &&
        memory::peek(result.scheduler+contract::scheduler_pending_action,result.action) &&
        memory::peek(result.scheduler+contract::scheduler_action_priority,result.action_priority);
    return result;
}
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
// final neutral reservation may await publication of live stance preparation;
// that observation still uses the original frame lease and completion clock.
bool current(const NoBailSkater& skater,const InputNativeContext& native,std::uint64_t now,
             bool neutral_observation=false) noexcept {
    NoBailSkater before{},after{};std::uint32_t state{},stance{};
    return permitted && ready && assets_current() && native.required && native.fresh(now) &&
        no_bail_skater(before) && before==skater && before.base==image &&
        memory::peek(skater.context+contract::physics_state,state) && state==native.flip.state &&
        memory::peek(skater.context+contract::stance_flags,stance) &&
        ((stance&0x60u)==native.flip.stance ||
         (neutral_observation && input.active() && input.was_issued() &&
          ((native.contacts_known && (native.contacts&0xc000u) && !(native.flip.stance&~0x60u)) ||
           ((stance&0x60u)^native.flip.stance)==game::build::v20260929::launches::stance_switch_mask))) &&
        no_bail_skater(after) && after==before;
}
void retire() noexcept {
    awaiting_consumer.store(false,std::memory_order_release);
#if defined(DINGOSDK_DEV_TOOLS)
    if(owner.context && input.sequence())pending={input.sequence(),input.was_issued(),input.observed_change()};
#endif
    input.cancel();owner={};latest_native={};last_poll=0;
}
void transition(std::uintptr_t context,const bool* predicate,const bool* flags) {
    if(!awaiting_consumer.load(std::memory_order_acquire)) {
        original_transition(context,predicate,flags);return;
    }
    bool requested=false;
    std::uint64_t sequence{};
    {
        std::unique_lock lock(mutex,std::try_to_lock);
        const auto now=GetTickCount64();
        if(lock.owns_lock() && input.active() && !input.was_issued() &&
           current(owner,latest_native,now) && now>=last_poll &&
           now-last_poll<=InputTiming::native_sample_lease_ms &&
           transition_scope([](auto at,auto& value){return memory::peek(at,value);},image,
               profile_runtime::executing_expression,context,reinterpret_cast<std::uintptr_t>(predicate),
               reinterpret_cast<std::uintptr_t>(flags),owner.entity,assets[0].object) && input.issued(latest_native,now)) {
            requested=true;sequence=input.sequence();
            awaiting_consumer.store(false,std::memory_order_release);
        }
    }
    // Only replace this invocation's input predicate. The original callback
    // retains its branch eligibility, timers and actor lifecycle. Never write a
    // stance bit, animation state, heading or velocity, or call it a second time.
    const bool selected=true;
#if defined(DINGOSDK_DEV_TOOLS)
    const auto action=sequence?pointer(context+contract::consumer_action):0;
    const auto prior_branch=action?pointer(action+contract::action_branch):0;
    const auto prior_arbitration=sequence?arbitration(action):ArbitrationObservation{};
#endif
    original_transition(context,requested?&selected:predicate,flags);
#if defined(DINGOSDK_DEV_TOOLS)
    if(sequence) {
        // Observe the original consumer's result once. Zero authored dwell
        // rules out arbitrary longer input pulses; these fields distinguish
        // branch resolution from arbitration and later actor/foot execution.
        std::uintptr_t branch{};
        std::uint8_t pending_action{},native_flags{};
        float elapsed{},truth_elapsed{},truth_duration{},transition_time{};
        const auto data=action?pointer(action+contract::action_data)&~std::uintptr_t{4}:0;
        const bool observed=action && data && pointer(context+contract::consumer_action)==action &&
            memory::peek(action+contract::action_branch,branch) &&
            memory::peek(action+contract::action_pending,pending_action) &&
            memory::peek(action+contract::action_elapsed,elapsed) &&
            memory::peek(action+contract::action_truth_elapsed,truth_elapsed) &&
            memory::peek(data+contract::data_truth_duration,truth_duration) &&
            memory::peek(data+contract::data_transition_time,transition_time) &&
            memory::peek(reinterpret_cast<std::uintptr_t>(flags),native_flags);
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE NATIVE STANCE REQUEST: sequence={} native_transition_consumer=true observed={} branch_before={} branch_after={} pending={} flags={} elapsed={} truth_elapsed={} truth_duration={} transition_time={} physical_completion=unverified.",
            sequence,observed,prior_branch,branch,pending_action,native_flags,elapsed,truth_elapsed,truth_duration,transition_time);
        const auto after=observed?arbitration(action):ArbitrationObservation{};
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE NATIVE STANCE ARBITRATION: sequence={} observed={} same_scheduler={} requested_priority={} scheduler={} state_before={} state_priority_before={} state_after={} state_priority_after={} action_before={} action_priority_before={} action_after={} action_priority_after={} physical_completion=unverified.",
            sequence,prior_arbitration.valid && after.valid,
            prior_arbitration.valid && after.valid && prior_arbitration.scheduler==after.scheduler,
            after.requested_priority,after.scheduler,prior_arbitration.state,prior_arbitration.state_priority,
            after.state,after.state_priority,prior_arbitration.action,prior_arbitration.action_priority,after.action,after.action_priority);
    }
#else
    (void)sequence;
#endif
}
bool install_transition(std::uintptr_t base) noexcept {
    if(!supported_build::running_image_matches(base))return false;
    std::array<unsigned char,32> bytes{};
    if(!memory::peek(base+contract::transition_consumer,bytes) || bytes!=contract::transition_consumer_prefix)return false;
    auto* target=reinterpret_cast<void*>(base+contract::transition_consumer);
    void* original{};
    if(hook_prepare(target,reinterpret_cast<void*>(&transition),&original)!=HookOk)return false;
    original_transition=reinterpret_cast<TransitionConsumer>(original);
    HookBatch batch;
    return hook_enable(target)==HookOk;
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
            if(!hook_attempted){hook_attempted=true;hook_ready=install_transition(base);}
            if(!image) {
                image=base;
                assets[0]={contract::transition_asset.name,contract::transition_asset.guid,0};
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
        "ARCADE NATIVE STANCE END: sequence={} transition_submitted={} leading_foot_changed={} physical_heading_momentum_acceptance=unverified.",
        evidence.sequence,evidence.submitted,evidence.changed);
#endif
}
bool available() noexcept {
    std::lock_guard lock(mutex);return permitted && hook_ready && ready && assets_current();
}
bool request(const NoBailSkater& skater,const InputRequest& request,const InputNativeContext& native,std::uint64_t now) noexcept {
    std::lock_guard lock(mutex);
    // A replacement local skater can dispatch its first physical RT before
    // final_controls retires the old owner. Retire only that stale lease here.
    if(input.active() && (skater!=owner || !current(skater,native,now,true) ||
       now<last_poll || now-last_poll>InputTiming::native_sample_lease_ms ||
       !input.observe(native,now,true)))retire();
    if(!hook_ready || request.action!=InputAction::switch_stance || request.input_sequence!=request.trigger.press.sequence ||
       !current(skater,native,now) || !input.start(request.trigger,native,now))return false;
    owner=skater;latest_native=native;last_poll=now;++serial;
    awaiting_consumer.store(true,std::memory_order_release);return true;
}
StanceControl controls(const NoBailSkater& skater,const InputNativeContext& native,std::uint64_t now,std::uint64_t tick) noexcept {
    std::lock_guard lock(mutex);
    if(!input.active())return {};
    if(skater!=owner || !current(skater,native,now,true)){retire();return {};}
    (void)tick;
    last_poll=now;latest_native=native;
    if(!input.observe(native,now,true)){retire();return {};}
    // Reserve neutral face output while native EAT consumes the request and
    // changes feet. No synthetic B gesture competes with that animation.
    return {true,0,serial,5};
}
void delivered(const NoBailSkater& skater,const InputNativeContext& native,StanceControl control,
               std::uint16_t final_buttons,std::uint64_t now,std::uint64_t tick) noexcept {
    std::lock_guard lock(mutex);
    (void)tick;
    const bool neutral_observation=control.owned && control.phase==5 && !control.buttons && !final_buttons;
    if(skater!=owner || !current(skater,native,now,neutral_observation) ||
       !input.active() || control.serial!=serial || !neutral_observation)retire();
}
void cancel() noexcept {std::lock_guard lock(mutex);retire();}
}

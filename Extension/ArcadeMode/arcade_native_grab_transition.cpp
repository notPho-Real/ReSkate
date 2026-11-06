#include "arcade_native_grab_transition.h"
#include "arcade_native_asset_cache.h"
#include "arcade_native_grab_lease.h"
#include "arcade_native_held_flip.h"
#include "arcade_native_manual_pose.h"
#include "arcade_native_ground_grab.h"
#include "arcade_fingerflip.h"
#include "Engine/Core/Hooks/hooks.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Core/Log/logging.h"
#include "Engine/Game/Abi/native_data.h"
#include "Engine/Game/Input/gameplay_options.h"
#include "Engine/Game/Build/image_identity.h"
#include "Engine/Game/Build/20260929/arcade_grab_transition.h"
#include "Engine/Game/Build/20260929/arcade_flip_transition.h"
#include "Engine/Game/Build/20260929/arcade_fingerflip.h"
#include "Engine/Game/Build/20260929/arcade_held_flip.h"
#include <atomic>
#include <bit>
#include <mutex>
#include <Windows.h>

namespace dingosdk::arcade_mode::native_grab {
namespace {
namespace contract=game::build::v20260929::arcade_grab_transition;
namespace actor_contract=game::build::v20260929::arcade_held_flip;
namespace finger_contract=game::build::v20260929::arcade_fingerflip;
using Guid=std::array<std::uint8_t,16>;
std::uintptr_t image{};
contract::Initialize original_initialize{};
contract::Evaluate original_evaluate{};
std::atomic<bool> hooks_ready{},assets_ready{},gameplay_allowed{},active{};
std::mutex mutex;
struct Asset {const char* name; Guid guid; std::uintptr_t object{};};
std::array<Asset,15> assets{{
    {"Animation/Dingo/Bool.Intent.LeftHandGrab",contract::left},
    {"Animation/Dingo/Bool.Intent.RightHandGrab",contract::right},
    {"Animation/Dingo/Float.Intent.TweakLeftRight",contract::tweak_x},
    {"Animation/Dingo/Float.Intent.TweakFrontBack",contract::tweak_y},
    {"Animation/Dingo/Bool.Anim.IsGrabbing",contract::grabbing},
    {"Animation/Dingo/Enum.GS.DetailedGrabType",contract::detail},
    {"Animation/Dingo/EnumGS.GrabType",contract::family},
    {"Animation/Dingo/Bool.GP.CanGrab",contract::can_grab},
    {"Animation/Dingo/EnumGS.OneFooterType",contract::one_footer},
    {"Animation/Dingo/Bool.State.Is.FingerFlip",finger_contract::state},
    {"Animation/Dingo/Enum.GS.FingerFlipType",finger_contract::identity},
    {"Animation/Dingo/Bool.Intent.GrabNose",contract::nose},
    {"Animation/Dingo/Bool.Intent.GrabTail",contract::tail},
    {"Animation/Dingo/Bool.Intent.GrabStale",contract::stale},
    {"Animation/Dingo/Bool.Intent.GrabWeddle",contract::weddle}
}};
static_assert(std::size(pilots)==contract::actor_assets.size());
static_assert(std::size(fingerflips)==finger_contract::actors.size());
constexpr auto footer_actor_end=contract::actor_assets.size()+contract::footer_entry_actors.size();
constexpr auto grab_actor_count=footer_actor_end+contract::grab_entry_actors.size();
const Pilot* actor_profile(unsigned index) noexcept {
    if(index<contract::actor_assets.size())return &pilots[index];
    if(index>=grab_actor_count)return nullptr;
    if(index>=footer_actor_end) {
        const auto detail=contract::grab_entry_actors[index-footer_actor_end].detail;
        for(const auto& profile:pilots)if(!profile.one_footer && profile.detail==detail)return &profile;
        return nullptr;
    }
    const auto foot=contract::footer_entry_actors[index-contract::actor_assets.size()].one_footer;
    for(const auto& profile:pilots)if(profile.one_footer==foot)return &profile;
    return nullptr;
}
const contract::GrabActorAsset& grab_actor_asset(unsigned index) noexcept {
    if(index>=footer_actor_end)return contract::grab_entry_actors[index-footer_actor_end].actor;
    return index<contract::actor_assets.size()?contract::actor_assets[index]:
        contract::footer_entry_actors[index-contract::actor_assets.size()].actor;
}
struct Written {std::uintptr_t at{};std::uint32_t previous{},value{};unsigned size{};};
struct Actor {std::uintptr_t object{};float before{},after{},delta{};std::uint64_t advanced_at{};bool initialized{},missing_initialization_reported{};};
// Observation only: distinguish a failed owner proof from a native landing.
// Missing reads remain unknown; these fields never grant permission.
struct OwnershipObservation {
    std::uint64_t ms{};
    unsigned owner_failure{}; // 1=no request, 2=resolver, 3=identity, 4=state block
    unsigned validity_failure{}; // 1=gameplay, 2=assets, 3=owner, 4=air state
    bool owner_verified{},state_read{};
    std::uint32_t state{};
};
struct Session {
    Lease lease{};NoBailSkater owner{};std::uintptr_t block{};
    std::uint64_t episode{},serial{};
    InputGenerations lineage{};InputAirEpisode air{};
    std::array<Written,4> writes{};
    std::array<Actor,grab_actor_count+finger_contract::actors.size()> actors{};
    FingerflipInput finger{};
    std::uint64_t finger_settings_since{};
    bool reported{};
    std::uint64_t foot_settings_since{};
    OwnershipObservation last_invalid{};
    unsigned other_actor_reports{};
} session;
std::uint64_t serial{},retry_at{};
unsigned other_actor_reports{};
constexpr auto foot_settings_owner="arcade-native-foot-input";
constexpr auto finger_settings_owner="arcade-native-finger-input";
constexpr auto finger_setting="egf_enablefingerflips";
void refresh_finger_settings(std::uint64_t now) noexcept {
    if(!session.finger.active())return;
    try {
        if(input::gameplay_options().hold(finger_settings_owner,finger_setting,true,now).empty() && !session.finger_settings_since)
            session.finger_settings_since=now+1;
    } catch(...) { }
}
bool finger_settings_applied(std::uint64_t now) noexcept {
    if(!session.finger_settings_since)return false;
    try {
        const auto value=input::gameplay_options().sampled(finger_setting,session.finger_settings_since,now);
        return value && *value;
    } catch(...) {return false;}
}
constexpr std::array foot_settings{"egf_invertxaonboard","egf_frontbackfoot"};
bool foot_settings_applied(std::uint64_t now) noexcept {
    if (!session.foot_settings_since) return false;
    try {
        for (const auto key:foot_settings) {
            const auto value=input::gameplay_options().sampled(key,session.foot_settings_since,now);
            if (!value || *value) return false;
        }
        return true;
    } catch (...) {return false;}
}
void refresh_foot_settings(std::uint64_t now) noexcept {
    const auto* p=session.lease.selected();
    if (!p || !p->one_footer) return;
    try {
        for (const auto key:foot_settings)
            if (!input::gameplay_options().hold(foot_settings_owner,key,false,now).empty()) return;
        if (!session.foot_settings_since) session.foot_settings_since=now+1;
    } catch (...) { }
}
// Input producers only copy bounded evidence; the client tick formats logs.
struct Event {
    bool ending{},held{},acknowledged{},airborne{};
    std::uint64_t serial{},episode{};
    ArcadeTrick trick{};std::int32_t detail{};InputRequest physical{};
    bool input_trace{},extra{},native_adapter{},retired{},native_owner{};
    std::uint64_t ms{},origin_action{},current_action{};
    unsigned failed_proofs{},gate{},disposition{},result{};
    const char* ending_reason{};
    OwnershipObservation ending_owner{},last_invalid{};
};
std::array<Event,32> events{};
unsigned event_count{},lost_events{};
void enqueue(Event event) noexcept {
    if (event_count<events.size()) events[event_count++]=event;
    else ++lost_events;
}
void flush_events() noexcept {
    std::array<Event,32> pending{};unsigned count{},lost{};
    {std::lock_guard lock(mutex);pending=events;count=event_count;lost=lost_events;event_count=lost_events=0;}
    for (unsigned i=0;i<count;++i) {
        const auto& e=pending[i];
        if (e.input_trace) logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE GRAB INPUT: ms={} recognized_at={} pressed_at={} direction={} presses={} form={} semantic={} request_action={} origin_action={} current_action={} extra={} native_adapter={} retired={} failed_proofs={} native_owner={} gate={} disposition={} result={} animation_acknowledged=false.",
            e.ms,e.physical.recognized_at,e.physical.physical_pressed_at,e.physical.direction,e.physical.presses,
            static_cast<unsigned>(e.physical.form),static_cast<unsigned>(e.physical.air.trick),e.physical.generations.action,
            e.origin_action,e.current_action,e.extra,e.native_adapter,e.retired,e.failed_proofs,e.native_owner,e.gate,e.disposition,e.result);
        else if (e.ending) logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE NATIVE GRAB END: serial={} episode={} acknowledged={} airborne={} intent_withdrawn=true landing_scoring_acceptance=unverified reason={} observed_at={} owner_verified={} owner_failure={} state_read={} state={} last_invalid_at={} validity_failure={} invalid_owner_failure={} invalid_state_read={} invalid_state={}.",
            e.serial,e.episode,e.acknowledged,e.airborne,e.ending_reason,e.ending_owner.ms,
            e.ending_owner.owner_verified,e.ending_owner.owner_failure,e.ending_owner.state_read,e.ending_owner.state,
            e.last_invalid.ms,e.last_invalid.validity_failure,e.last_invalid.owner_failure,
            e.last_invalid.state_read,e.last_invalid.state);
        else logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE NATIVE GRAB REQUEST: serial={} episode={} semantic={} expected_detail={} held={} pressed_at={} released_at={} recognized_at={} direction={} presses={} acknowledged=false.",
            e.serial,e.episode,static_cast<unsigned>(e.trick),e.detail,e.held,e.physical.physical_pressed_at,
            e.physical.physical_released_at,e.physical.recognized_at,e.physical.direction,e.physical.presses);
    }
    if (lost) logging::log(logging::Level::warning,logging::Channel::skater,"ARCADE NATIVE GRAB EVIDENCE LOST: count={}.",lost);
}
template<class T> bool read(std::uintptr_t at,T& value) noexcept {return memory::peek(at,value);}
std::uintptr_t pointer(std::uintptr_t at) noexcept {std::uintptr_t p{};return read(at,p)?p:0;}
bool guid_is(std::uintptr_t object,const Guid& expected) noexcept {
    Guid value{};return object>=0x10010 && read(object-16,value) && value==expected;
}
// Same qualified local component path as the verified flip bridge. No shared
// flip lease or flip state is used by this separate grab owner.
std::uintptr_t state_block(const NoBailSkater& owner) noexcept {
    const auto collection=pointer(owner.entity+0x70);std::uint8_t count{};
    if (!collection || pointer(collection)!=owner.entity || !read(collection+8,count) || count>128) return 0;
    for (unsigned i=0;i<count;++i) {
        const auto component=pointer(collection+0x20+i*0x20);
        if (component && pointer(component)==image+0x63aaee8) {
            const auto block=pointer(pointer(pointer(component+0x68)+0xe8)+8);
            return pointer(block)==image+0x61a8bd8?block:0;
        }
    }
    return 0;
}
bool same_owner(OwnershipObservation* observation=nullptr) noexcept {
    NoBailSkater owner{};
    unsigned failure{};
    if (!session.serial) failure=1;
    else if (!no_bail_skater(owner)) failure=2;
    else if (owner!=session.owner) failure=3;
    else if (state_block(owner)!=session.block) failure=4;
    if (observation) {observation->owner_failure=failure;observation->owner_verified=!failure;}
    return !failure;
}
bool airborne(OwnershipObservation* observation=nullptr) noexcept {
    std::uint32_t state{};
    const bool known=read(session.owner.context+0x1414,state);
    if (observation) {observation->state_read=known;observation->state=state;}
    return known && (state==200 || state==201);
}
std::uintptr_t address_at(std::uintptr_t block,unsigned index) noexcept {
    const auto& a=assets[index];std::uint32_t handle{},valid{};
    if (!guid_is(a.object,a.guid) || !read(a.object+0x28,handle)) return 0;
    const auto page=pointer(block+0x60+8*(handle&4095));const auto offset=(handle>>12)&2047;
    if (!page || !read(page+0x71c+4*(offset>>5),valid) || !(valid&(1u<<(offset&31)))) return 0;
    return page+offset;
}
std::uintptr_t address(unsigned index) noexcept {return address_at(session.block,index);}
bool write(std::uintptr_t at,std::uint32_t value,unsigned size) noexcept {
    if (!at || (size==4 && at%4)) return false;
    __try {
        if (size==1) *reinterpret_cast<std::uint8_t*>(at)=static_cast<std::uint8_t>(value);
        else *reinterpret_cast<std::uint32_t*>(at)=value;
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
bool bits(std::uintptr_t at,unsigned size,std::uint32_t& value) noexcept {
    if (size==4) return read(at,value);
    std::uint8_t b{};if (!read(at,b)) return false;value=b;return true;
}
void restore() noexcept {
    if (same_owner()) for (unsigned i=0;i<session.writes.size();++i) {
        auto& w=session.writes[i];std::uint32_t value{};
        if (w.at && address(i)==w.at && bits(w.at,w.size,value) && value==w.value) write(w.at,w.previous,w.size);
    }
    session.writes={};
}
void retire(const char* reason) noexcept {
    if (session.serial) {
        Event event{true,false,session.reported,false,session.serial,session.episode};
        event.ending_reason=reason;event.ending_owner.ms=GetTickCount64();
        event.airborne=same_owner(&event.ending_owner) && airborne(&event.ending_owner);
        event.last_invalid=session.last_invalid;enqueue(event);
    }
    input::gameplay_options().release(foot_settings_owner);
    input::gameplay_options().release(finger_settings_owner);
    restore();session={};active.store(false);
}
bool valid() noexcept {
    OwnershipObservation observation{};
    if (!gameplay_allowed.load()) observation.validity_failure=1;
    else if (!assets_ready.load()) observation.validity_failure=2;
    else if (!same_owner(&observation)) observation.validity_failure=3;
    else if (!airborne(&observation)) observation.validity_failure=4;
    if (!observation.validity_failure) return true;
    observation.ms=GetTickCount64();session.last_invalid=observation;return false;
}
RetargetObservation retarget_observation() noexcept {
    RetargetObservation out{};
    const auto left=address(0),right=address(1),grabbing=address(4),family=address(6),can_grab=address(7);
    out.valid=left && right && grabbing && family && can_grab &&
        read(left,out.left) && read(right,out.right) && read(grabbing,out.grabbing) &&
        read(family,out.family) && read(can_grab,out.can_grab);
    if (const auto* selected=session.lease.selected()) {
        const auto index=selected->selector_y<0?11u:selected->selector_y>0?12u:
            selected->selector_x!=0?(selected->hands==1?13u:14u):0u;
        if (index) {
            const auto selector=address(index);
            out.selector_ready=selector && read(selector,out.selector_ready) && out.selector_ready;
        }
    }
    return out;
}
FingerflipObservation finger_observation() noexcept {
    FingerflipObservation out{};
    out.valid=address(9) && address(10) && address(4) && address(6) && address(7) &&
        read(address(9),out.in_fingerflip) && read(address(10),out.identity) &&
        read(address(4),out.grabbing) && read(address(6),out.family) && read(address(7),out.can_grab);
    return out;
}
int actor_index(std::uintptr_t actor) noexcept {
    const auto asset=pointer(actor+0x60);
    if (pointer(asset)!=image+0x61ba838) return -1;
    for (unsigned i=0;i<grab_actor_count;++i) if (guid_is(asset,grab_actor_asset(i).guid)) return static_cast<int>(i);
    for(unsigned i=0;i<finger_contract::actors.size();++i)
        if(guid_is(asset,finger_contract::actors[i]))return static_cast<int>(grab_actor_count+i);
    return -1;
}
void acknowledge(std::uint64_t now) noexcept {
    if(session.finger.active()) {
        const auto native=finger_observation();
        if(!native.valid || session.finger.acknowledged())return;
        for(unsigned i=0;i<finger_contract::actors.size();++i) {
            const auto index=grab_actor_count+i;const auto& a=session.actors[index];
            if(&fingerflips[i]!=session.finger.row() || now<a.advanced_at || now-a.advanced_at>Lease::heartbeat_ms)continue;
            if(session.finger.acknowledge(native.identity,a.initialized,a.object && actor_index(a.object)==static_cast<int>(index),a.delta,a.before,a.after))
                logging::log(logging::Level::info,logging::Channel::skater,
                    "ARCADE NATIVE FINGERFLIP: serial={} episode={} identity={} actor={} before={} after={} delta={} initialized=true destination={} scoring_acceptance=unverified.",
                    session.serial,session.episode,native.identity,a.object,a.before,a.after,a.delta,session.finger.row()->destination);
        }
        return;
    }
    const auto* selected=session.lease.selected();if (!selected || session.lease.acknowledged()) return;
    bool grabbing{};
    const auto grabbing_at=address(4);
    if (!grabbing_at || !read(grabbing_at,grabbing)) return;
    const auto identity=observe_identity(*selected,[](bool footer,std::int32_t& value) noexcept {
        const auto at=address(footer?8:5);
        return at && read(at,value);
    });
    if (!identity.valid) return;
    const auto detail=identity.detail,one_footer=identity.one_footer;
    for (unsigned i=0;i<grab_actor_count;++i) {
        const auto& a=session.actors[i];
        const auto* profile=actor_profile(i);
        if (!profile || profile->trick!=selected->trick || now<a.advanced_at || now-a.advanced_at>Lease::heartbeat_ms) continue;
        if (session.lease.acknowledge(detail,a.delta,a.before,a.after,a.initialized,
                                    a.object && actor_index(a.object)==static_cast<int>(i),grabbing,one_footer) && !session.reported) {
            session.reported=true;
            const auto board=pointer(pointer(session.owner.core+0x430)+0x18);
            const auto bodies=pointer(board+0x20);std::uint32_t count{};float vy{};
            const bool velocity_known=pointer(board)==image+0x65e67a0 && pointer(bodies+0x10)==board &&
                read(bodies,count) && count==9 && read(bodies+0x74,vy) && std::isfinite(vy);
            logging::log(logging::Level::info,logging::Channel::skater,
                "ARCADE NATIVE GRAB: serial={} episode={} detail={} one_footer={} actor={} actor_asset={} before={} after={} delta={} initialized=true native_evaluation=true velocity_known={} vertical_velocity={} phase={}.",
                session.serial,session.episode,detail,one_footer,a.object,grab_actor_asset(i).name,a.before,a.after,a.delta,velocity_known,vy,
                i<contract::actor_assets.size()?"loop":"into");
        }
    }
}
void initialize(std::uintptr_t actor,std::uintptr_t context,std::uintptr_t parameters) {
    std::uint64_t occurrence{};int index=-1;
    if (active.load()) {
        std::lock_guard lock(mutex);
        if (valid() && context==session.block+0x48) {occurrence=session.serial;index=actor_index(actor);}
    }
    original_initialize(actor,context,parameters);
    native_held_flip::initialized(actor,context);
    native_manual_pose::initialized(actor,context);
    native_ground_grab::initialized(actor,context);
    if (occurrence && index<0) {
        std::lock_guard lock(mutex);
        const auto* selected=session.lease.selected();
        // Rocket can reach its native identity through Into reselection while
        // neither expected actor is observed. Record the selected original
        // asset under the same bounded owner checks; never acknowledge it here.
        if(session.serial==occurrence && selected &&
            (selected->one_footer || selected->trick==ArcadeTrick::rocket_air) && valid() &&
            context==session.block+actor_contract::state_view &&
            session.other_actor_reports<8 && other_actor_reports<32) {
            const auto asset=pointer(actor+actor_contract::actor_asset);
            std::array<std::uint32_t,4> words{};
            if(asset>=0x10000+actor_contract::asset_guid_prefix &&
                pointer(asset)==image+actor_contract::actor_type && read(asset-actor_contract::asset_guid_prefix,words)) {
                ++session.other_actor_reports;++other_actor_reports;
                logging::log(logging::Level::info,logging::Channel::skater,
                    "ARCADE NATIVE GRAB OTHER ACTOR: serial={} semantic={} expected_detail={} expected_footer={} guid_words_le={:08x},{:08x},{:08x},{:08x} acceptance=false.",
                    occurrence,static_cast<unsigned>(selected->trick),selected->detail,
                    selected->one_footer,words[0],words[1],words[2],words[3]);
            }
        }
    }
    if (index>=0) {
        std::lock_guard lock(mutex);float duration{};
        if (session.serial==occurrence && valid() && actor_index(actor)==index &&
            read(actor+0x20,duration) && std::isfinite(duration) && duration>0 && duration<10000) {
            session.actors[index]={actor,0,0,0,0,true};
            if(const auto* profile=actor_profile(static_cast<unsigned>(index)))logging::log(logging::Level::info,logging::Channel::skater,
                "ARCADE NATIVE GRAB INIT: serial={} episode={} detail={} actor={} actor_asset={} duration={}.",
                occurrence,session.episode,profile->detail,actor,grab_actor_asset(static_cast<unsigned>(index)).name,duration);
        }
    }
}
std::uintptr_t evaluate(std::uintptr_t actor,std::uintptr_t output,float delta,std::uintptr_t context) {
    const auto held=native_held_flip::before_evaluate(actor,context,delta);
    const auto manual=native_manual_pose::before_evaluate(actor,context,delta);
    const auto ground=native_ground_grab::before_evaluate(actor,context,delta);
    std::uint64_t occurrence{};int index=-1;float before{};
    if (active.load() && std::isfinite(delta) && delta>0) {
        std::lock_guard lock(mutex);
        if (valid() && pointer(context+8)==session.block+0x48 && read(actor+0x1c,before)) {
            occurrence=session.serial;index=actor_index(actor);
        }
    }
    const auto result=original_evaluate(actor,output,delta,context);
    native_held_flip::after_evaluate(held,context,delta);
    native_manual_pose::after_evaluate(manual,context,delta);
    native_ground_grab::after_evaluate(ground,context,delta);
    if (index>=0) {
        std::lock_guard lock(mutex);float after{};
        if (session.serial==occurrence && valid() && read(actor+0x1c,after)) {
            auto& a=session.actors[index];
            if (a.initialized && a.object==actor) {
                a.before=before;a.after=after;a.delta=delta;a.advanced_at=GetTickCount64();
                acknowledge(a.advanced_at);
            } else if (!a.missing_initialization_reported) {
                // A reused native actor may advance without an initialization
                // observed in this lease. Record that gap, never acknowledge it.
                a.missing_initialization_reported=true;
                logging::log(logging::Level::info,logging::Channel::skater,
                    "ARCADE NATIVE GRAB UNTRACKED ADVANCE: serial={} actor_index={} before={} after={} delta={} initialized={} same_actor={} acceptance=false.",
                    occurrence,index,before,after,delta,a.initialized,a.object==actor);
            }
        }
    }
    return result;
}
}
void cancel() noexcept {std::lock_guard lock(mutex);retire("cancel");}
bool available() noexcept {return hooks_ready.load() && assets_ready.load() && gameplay_allowed.load();}
bool blocks_flip(const NoBailSkater& owner) noexcept {
    if (!available()) return false;
    std::lock_guard lock(mutex);
    if (session.serial && session.owner==owner) return true;
    const auto block=state_block(owner);if (!block) return false;
    const auto at=address_at(block,4);bool grabbing{};
    return at && read(at,grabbing) && grabbing;
}
void tick(bool gameplay) noexcept {
    flush_events();
    gameplay_allowed.store(gameplay);
    if (!gameplay) {cancel();return;}
    if (!hooks_ready.load()) return;
    if (assets_ready.load() && !native_assets_current(std::span{assets}.first<8>(),guid_is)) {
        assets_ready.store(false);
        cancel();
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE NATIVE ASSET CACHE: family=grab invalidated=true.");
    }
    const bool missing_assets=!assets_ready.load() ||
        !native_assets_current(std::span{assets}.subspan<8>(),guid_is);
    if (missing_assets && GetTickCount64()>=retry_at) {
        retry_at=GetTickCount64()+1000;const auto find=game::native_data().find_asset;
        if (find) {
            bool complete=true;
            for (auto& a:assets) {
                if (guid_is(a.object,a.guid)) continue;
                a.object=0;
                for (std::uint16_t domain=0;domain<0xbbf;++domain) {
                    if (!pointer(image+game::build::v20260929::arcade_flip_transition::domain_owners+8ULL*domain)) continue;
                    const auto object=find(domain,a.name);
                    if (guid_is(object,a.guid)) {a.object=object;break;}
                }
                if (&a<&assets[8]) complete=complete && a.object;
            }
            assets_ready.store(complete);
        }
    }
    if (active.load()) {
        std::lock_guard lock(mutex);
        if (!valid() || !session.lease.intent(GetTickCount64())) retire("tick");
    }
}
void refresh(const NoBailSkater& owner,std::uint64_t episode,bool held,bool permitted) noexcept {
    if (!active.load()) return;
    std::lock_guard lock(mutex);
    session.lease.refresh(GetTickCount64(),held,permitted && owner==session.owner && episode && episode==session.episode && valid());
    if (!session.lease.selected()) retire("refresh");
    else {refresh_foot_settings(GetTickCount64());refresh_finger_settings(GetTickCount64());}
}
bool request(const NoBailSkater& owner,std::uint64_t episode,const InputRequest& physical,ArcadeTrick trick,bool held) noexcept {
    if (!available() || !episode || !pilot(trick)) return false;
    const auto block=state_block(owner);if (!block) return false;
    if (pilot(trick)->one_footer && !address_at(block,8)) return false;
    std::lock_guard lock(mutex);
    if(session.finger.active())return false;
    if (session.serial && (owner!=session.owner || episode!=session.episode)) retire("request_owner_episode");
    // A changed native action lineage needs the same independent grab-owner
    // proof at dispatch too. Never borrow a retired flip selector as permission.
    if (session.serial && physical.generations!=session.lineage &&
        (!valid() || !session.lease.intent(GetTickCount64()) || !retarget_observation().eligible(session.lease.selected()))) return false;
    if (!session.serial) {session.owner=owner;session.block=block;session.episode=episode;}
    session.serial=++serial;session.reported=false;
    if (!valid() || !session.lease.request(trick,GetTickCount64(),held)) {retire("request");return false;}
    input::gameplay_options().release(foot_settings_owner);session.foot_settings_since=0;
    refresh_foot_settings(GetTickCount64());
    session.lineage=physical.generations;session.air=physical.air_episode;
    active.store(true);
    enqueue({false,held,false,false,session.serial,episode,trick,pilot(trick)->detail,physical});
    return true;
}
unsigned hand_inputs(const NoBailSkater& owner,std::uint64_t episode) noexcept {
    return controls(owner,episode).hands;
}
Control controls(const NoBailSkater& owner,std::uint64_t episode,std::uint64_t tick) noexcept {
    if (!active.load()) return {};
    std::lock_guard lock(mutex);
    const auto now=GetTickCount64();
    if(!valid() || owner!=session.owner || episode!=session.episode || !session.lease.intent(now))return {};
    if(session.finger.active()) {
        if(!tick)return session.finger.control(); // Legacy hand-only reads cannot advance a gesture.
        auto result=session.finger.poll(now,tick,finger_settings_applied(now),finger_observation());
        if(!session.finger.active()) {retire("fingerflip_timeout_or_owner");return {};}
        if(session.finger.completed()) {
            const auto destination=session.finger.row()->destination;
            logging::log(logging::Level::info,logging::Channel::skater,
                "ARCADE NATIVE FINGERFLIP RETURN: serial={} identity={} destination={} native_grabbing=true scoring_acceptance=unverified.",
                session.serial,session.finger.row()->identity,destination);
            session.lease.request(grab_family_trick(destination),now,true);session.reported=false;
            session.finger.cancel();session.finger_settings_since=0;
            input::gameplay_options().release(finger_settings_owner);
        }
        return result;
    }
    return session.lease.controls(retarget_observation(),foot_settings_applied(now));
}
bool fingerflip(const NoBailSkater& owner,std::uint64_t episode,std::uint8_t id,
               std::span<const AuthoredStickStep> steps,std::uint64_t tick,bool mirror) noexcept {
    if(!active.load())return false;
    std::lock_guard lock(mutex);const auto now=GetTickCount64();
    if(!valid() || owner!=session.owner || episode!=session.episode || !session.lease.intent(now) ||
        !session.lease.selected() || !address(9) || !address(10))return false;
    const auto observed=finger_observation();
    if(!observed.valid || observed.in_fingerflip || !session.finger.start(*session.lease.selected(),
        session.lease.acknowledged(),retarget_observation(),id,steps,now,tick,mirror))return false;
    session.serial=++serial;
    for(std::size_t i=grab_actor_count;i<session.actors.size();++i)session.actors[i]={};
    restore();refresh_finger_settings(now);
    logging::log(logging::Level::info,logging::Channel::skater,
        "ARCADE NATIVE FINGERFLIP REQUEST: serial={} episode={} gesture={} expected_identity={} source_family={} destination={} animation_acknowledged=false.",
        session.serial,episode,id,session.finger.row()->identity,session.finger.row()->family,session.finger.row()->destination);
    return true;
}
bool right_hand_input(const NoBailSkater& owner,std::uint64_t episode) noexcept {
    return (hand_inputs(owner,episode)&2u)!=0;
}
bool continues(const NoBailSkater& owner,std::uint64_t episode,InputGenerations origin,InputAirEpisode air) noexcept {
    if (!active.load()) return false;
    std::lock_guard lock(mutex);
    return valid() && owner==session.owner && episode && episode==session.episode &&
        air.number==episode && air==session.air && origin==session.lineage &&
        session.lease.intent(GetTickCount64()) && retarget_observation().eligible(session.lease.selected()) && valid();
}
void trace_input(std::uint64_t now,const UnifiedInput::Diagnostic& diagnostic,const ActionInputOutput& output) noexcept {
    const auto& proof=diagnostic.grab_repeat;
    if (!proof.observed) return;
    const auto& current=proof.current;const auto& before=proof.before.flip;
    const auto& flip=current.flip;
    unsigned failed{};
    if (!current.valid || !current.airborne || !current.airborne_episode) failed|=1u;
    if (current.air_episode()!=proof.before.air_episode()) failed|=2u;
    if (current.generations.owner!=proof.origin.owner) failed|=4u;
    if (current.generations.action<=proof.origin.action || current.generations.action-proof.origin.action!=1) failed|=8u;
    if (!before.valid || !flip.valid) failed|=16u;
    if (before.id!=1 || flip.id!=-1) failed|=32u;
    if (!before.object || before.object!=flip.object) failed|=64u;
    if (before.state!=201 || before.state!=flip.state) failed|=128u;
    if (before.mode!=2 || before.mode!=flip.mode) failed|=256u;
    if (before.stance!=flip.stance) failed|=512u;
    if (!before.step || flip.step<=before.step) failed|=1024u;
    if (!before.mode_generation || before.mode_generation!=flip.mode_generation) failed|=2048u;
    for (std::size_t i=0;i<diagnostic.recognition_count;++i) {
        const auto& request=diagnostic.recognition[i].request;
        if (request.action!=InputAction::grab) continue;
        Event e{};e.input_trace=true;e.ms=now;e.physical=request;
        e.extra=proof.extra;e.native_adapter=proof.native_adapter;e.retired=proof.retired;e.native_owner=proof.native_owner;
        e.origin_action=proof.origin.action;e.current_action=current.generations.action;
        e.failed_proofs=failed;e.gate=output.grab_gate;e.result=static_cast<unsigned>(output.result);
        for (std::size_t j=0;j<diagnostic.dispatched;++j) {
            const auto& dispatch=diagnostic.dispatch[j];
            if (dispatch.request.recognition==request.recognition)
                e.disposition=static_cast<unsigned>(dispatch.disposition);
        }
        std::lock_guard lock(mutex);enqueue(e);
    }
}
void before_transition(std::uintptr_t view,float delta) noexcept {
    if (!active.load() || !std::isfinite(delta) || delta<=0) return;
    std::lock_guard lock(mutex);
    if (!valid()) {retire("transition_owner");return;}
    if (view!=session.block+0x48) return;
    const auto now=GetTickCount64();acknowledge(now);
    // Native fingerflip entry temporarily leaves IsGrabbing. Its actor and
    // destination own this handoff; the old pose must not be reasserted here.
    if(session.finger.active()) {restore();return;}
    if (session.lease.acknowledged()) {
        bool grabbing{};const auto at=address(4);
        if (!at || !read(at,grabbing) || !grabbing) {retire("native_exit");return;}
    }
    if (!session.lease.intent(now)) {retire("transition_lease");return;}
    const auto& p=*session.lease.selected();
    const std::array<std::uint32_t,4> values{0,1,std::bit_cast<std::uint32_t>(p.x),std::bit_cast<std::uint32_t>(p.y)};
    // The original trigger-input path owns hand intent and CanGrab. HIL proved
    // hand-state writes alone bypass that eligibility producer. Only the two
    // inspected pose-request signals are supplied here; native eligibility,
    // identities, animation clocks, blending and physics remain game-owned.
    for (unsigned i=2;i<4;++i) {
        const auto at=address(i);const unsigned size=i<2?1:4;auto& w=session.writes[i];
        if (!at || (w.at && w.at!=at)) {retire("signal_address");return;}
        if (!w.at) {
            std::uint32_t previous{};if (!bits(at,size,previous)) {retire("signal_read");return;}
            w={at,previous,values[i],size};
        }
        w.value=values[i];if (!write(at,w.value,size)) {retire("signal_write");return;}
    }
}
bool start(std::uintptr_t base) noexcept {
    if (hooks_ready.load()) return true;
    if (!supported_build::running_image_matches(base)) return false;
    for (const auto& fp:contract::functions) {std::array<unsigned char,32> bytes{};if (!read(base+fp.rva,bytes) || bytes!=fp.bytes) return false;}
    image=base;std::array<void*,2> targets{},originals{};
    const std::array<void*,2> replacements{reinterpret_cast<void*>(&initialize),reinterpret_cast<void*>(&evaluate)};
    auto rollback=[&]{for (auto target:targets) if(target){hook_disable(target);hook_remove(target);}};
    for (unsigned i=0;i<2;++i) {
        targets[i]=reinterpret_cast<void*>(base+contract::functions[i].rva);
        if (hook_prepare(targets[i],replacements[i],&originals[i])!=HookOk) {rollback();return false;}
    }
    original_initialize=reinterpret_cast<contract::Initialize>(originals[0]);
    original_evaluate=reinterpret_cast<contract::Evaluate>(originals[1]);
    for (auto target:targets) if (hook_queue_enable(target)!=HookOk) {rollback();return false;}
    if (hook_apply_queued()!=HookOk) {rollback();return false;}
    hooks_ready.store(true);return true;
}
}

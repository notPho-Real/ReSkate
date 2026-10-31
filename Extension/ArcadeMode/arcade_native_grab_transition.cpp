#include "arcade_native_grab_transition.h"
#include "arcade_native_grab_lease.h"
#include "Engine/Core/Hooks/hooks.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Core/Log/logging.h"
#include "Engine/Game/Abi/native_data.h"
#include "Engine/Game/Build/image_identity.h"
#include "Engine/Game/Build/20260929/arcade_grab_transition.h"
#include "Engine/Game/Build/20260929/arcade_flip_transition.h"
#include <atomic>
#include <bit>
#include <mutex>
#include <Windows.h>

namespace dingosdk::arcade_mode::native_grab {
namespace {
namespace contract=game::build::v20260929::arcade_grab_transition;
using Guid=std::array<std::uint8_t,16>;
std::uintptr_t image{};
contract::Initialize original_initialize{};
contract::Evaluate original_evaluate{};
std::atomic<bool> hooks_ready{},assets_ready{},gameplay_allowed{},active{};
std::mutex mutex;
struct Asset {const char* name; Guid guid; std::uintptr_t object{};};
std::array<Asset,8> assets{{
    {"Animation/Dingo/Bool.Intent.LeftHandGrab",contract::left},
    {"Animation/Dingo/Bool.Intent.RightHandGrab",contract::right},
    {"Animation/Dingo/Float.Intent.TweakLeftRight",contract::tweak_x},
    {"Animation/Dingo/Float.Intent.TweakFrontBack",contract::tweak_y},
    {"Animation/Dingo/Bool.Anim.IsGrabbing",contract::grabbing},
    {"Animation/Dingo/Enum.GS.DetailedGrabType",contract::detail},
    {"Animation/Dingo/EnumGS.GrabType",contract::family},
    {"Animation/Dingo/Bool.GP.CanGrab",contract::can_grab}
}};
const std::array<Guid,3> actor_guids{contract::actor_bs,contract::actor_melon,contract::actor_method};
constexpr std::array<const char*,3> actor_names{"e7191c5c-9c2e-7abb-e719-1c5c9c2e7abb",
    "3fbc3fdd-701e-719d-3fbc-3fdd701e719d","12d1f15d-c040-6fd0-12d1-f15dc0406fd0"};
struct Written {std::uintptr_t at{};std::uint32_t previous{},value{};unsigned size{};};
struct Actor {std::uintptr_t object{};float before{},after{},delta{};std::uint64_t advanced_at{};bool initialized{};};
struct Session {
    Lease lease{};NoBailSkater owner{};std::uintptr_t block{};
    std::uint64_t episode{},serial{};
    InputGenerations lineage{};InputAirEpisode air{};
    std::array<Written,4> writes{};
    std::array<Actor,3> actors{};
    bool reported{};
} session;
std::uint64_t serial{},retry_at{};
// Input producers only copy bounded evidence; the client tick formats logs.
struct Event {
    bool ending{},held{},acknowledged{},airborne{};
    std::uint64_t serial{},episode{};
    ArcadeTrick trick{};std::int32_t detail{};InputRequest physical{};
    bool input_trace{},extra{},native_adapter{},retired{},native_owner{};
    std::uint64_t ms{},origin_action{},current_action{};
    unsigned failed_proofs{},gate{},disposition{},result{};
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
            "ARCADE NATIVE GRAB END: serial={} episode={} acknowledged={} airborne={} intent_withdrawn=true landing_scoring_acceptance=unverified.",
            e.serial,e.episode,e.acknowledged,e.airborne);
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
bool same_owner() noexcept {
    NoBailSkater owner{};
    return session.serial && no_bail_skater(owner) && owner==session.owner && state_block(owner)==session.block;
}
bool airborne() noexcept {
    std::uint32_t state{};
    return read(session.owner.context+0x1414,state) && (state==200 || state==201);
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
void retire() noexcept {
    if (session.serial) enqueue({true,false,session.reported,same_owner() && airborne(),session.serial,session.episode});
    restore();session={};active.store(false);
}
bool valid() noexcept {return gameplay_allowed.load() && assets_ready.load() && same_owner() && airborne();}
RetargetObservation retarget_observation() noexcept {
    RetargetObservation out{};
    const auto left=address(0),right=address(1),grabbing=address(4),family=address(6),can_grab=address(7);
    out.valid=left && right && grabbing && family && can_grab &&
        read(left,out.left) && read(right,out.right) && read(grabbing,out.grabbing) &&
        read(family,out.family) && read(can_grab,out.can_grab);
    return out;
}
int actor_index(std::uintptr_t actor) noexcept {
    const auto asset=pointer(actor+0x60);
    if (pointer(asset)!=image+0x61ba838) return -1;
    for (unsigned i=0;i<actor_guids.size();++i) if (guid_is(asset,actor_guids[i])) return static_cast<int>(i);
    return -1;
}
void acknowledge(std::uint64_t now) noexcept {
    const auto* selected=session.lease.selected();if (!selected || session.lease.acknowledged()) return;
    std::int32_t detail{};bool grabbing{};
    const auto detail_at=address(5),grabbing_at=address(4);
    if (!detail_at || !grabbing_at || !read(detail_at,detail) || !read(grabbing_at,grabbing)) return;
    for (unsigned i=0;i<3;++i) {
        const auto& a=session.actors[i];
        if (pilots[i].trick!=selected->trick || now<a.advanced_at || now-a.advanced_at>Lease::heartbeat_ms) continue;
        if (session.lease.acknowledge(detail,a.delta,a.before,a.after,a.initialized,
                                    a.object && actor_index(a.object)==static_cast<int>(i),grabbing) && !session.reported) {
            session.reported=true;
            const auto board=pointer(pointer(session.owner.core+0x430)+0x18);
            const auto bodies=pointer(board+0x20);std::uint32_t count{};float vy{};
            const bool velocity_known=pointer(board)==image+0x65e67a0 && pointer(bodies+0x10)==board &&
                read(bodies,count) && count==9 && read(bodies+0x74,vy) && std::isfinite(vy);
            logging::log(logging::Level::info,logging::Channel::skater,
                "ARCADE NATIVE GRAB: serial={} episode={} detail={} actor={} actor_asset={} before={} after={} delta={} initialized=true native_evaluation=true velocity_known={} vertical_velocity={}.",
                session.serial,session.episode,detail,a.object,actor_names[i],a.before,a.after,a.delta,velocity_known,vy);
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
    if (index>=0) {
        std::lock_guard lock(mutex);float duration{};
        if (session.serial==occurrence && valid() && actor_index(actor)==index &&
            read(actor+0x20,duration) && std::isfinite(duration) && duration>0 && duration<10000) {
            session.actors[index]={actor,0,0,0,0,true};
            logging::log(logging::Level::info,logging::Channel::skater,
                "ARCADE NATIVE GRAB INIT: serial={} episode={} detail={} actor={} actor_asset={} duration={}.",
                occurrence,session.episode,pilots[index].detail,actor,actor_names[index],duration);
        }
    }
}
std::uintptr_t evaluate(std::uintptr_t actor,std::uintptr_t output,float delta,std::uintptr_t context) {
    std::uint64_t occurrence{};int index=-1;float before{};
    if (active.load() && std::isfinite(delta) && delta>0) {
        std::lock_guard lock(mutex);
        if (valid() && pointer(context+8)==session.block+0x48 && read(actor+0x1c,before)) {
            occurrence=session.serial;index=actor_index(actor);
        }
    }
    const auto result=original_evaluate(actor,output,delta,context);
    if (index>=0) {
        std::lock_guard lock(mutex);float after{};
        if (session.serial==occurrence && valid() && read(actor+0x1c,after)) {
            auto& a=session.actors[index];
            if (a.initialized && a.object==actor) {
                a.before=before;a.after=after;a.delta=delta;a.advanced_at=GetTickCount64();
                acknowledge(a.advanced_at);
            }
        }
    }
    return result;
}
}
void cancel() noexcept {std::lock_guard lock(mutex);retire();}
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
    if (!assets_ready.load() && GetTickCount64()>=retry_at) {
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
                complete=complete && a.object;
            }
            assets_ready.store(complete);
        }
    }
    if (active.load()) {
        std::lock_guard lock(mutex);
        if (!valid() || !session.lease.intent(GetTickCount64())) retire();
    }
}
void refresh(const NoBailSkater& owner,std::uint64_t episode,bool held,bool permitted) noexcept {
    if (!active.load()) return;
    std::lock_guard lock(mutex);
    session.lease.refresh(GetTickCount64(),held,permitted && owner==session.owner && episode && episode==session.episode && valid());
    if (!session.lease.selected()) retire();
}
bool request(const NoBailSkater& owner,std::uint64_t episode,const InputRequest& physical,ArcadeTrick trick,bool held) noexcept {
    if (!available() || !episode || !pilot(trick)) return false;
    const auto block=state_block(owner);if (!block) return false;
    std::lock_guard lock(mutex);
    if (session.serial && (owner!=session.owner || episode!=session.episode)) retire();
    // A changed native action lineage needs the same independent grab-owner
    // proof at dispatch too. Never borrow a retired flip selector as permission.
    if (session.serial && physical.generations!=session.lineage &&
        (!valid() || !session.lease.intent(GetTickCount64()) || !retarget_observation().eligible())) return false;
    if (!session.serial) {session.owner=owner;session.block=block;session.episode=episode;}
    session.serial=++serial;session.reported=false;
    if (!valid() || !session.lease.request(trick,GetTickCount64(),held)) {retire();return false;}
    session.lineage=physical.generations;session.air=physical.air_episode;
    active.store(true);
    enqueue({false,held,false,false,session.serial,episode,trick,pilot(trick)->detail,physical});
    return true;
}
bool right_hand_input(const NoBailSkater& owner,std::uint64_t episode) noexcept {
    if (!active.load()) return false;
    std::lock_guard lock(mutex);
    return valid() && owner==session.owner && episode==session.episode &&
        session.lease.intent(GetTickCount64());
}
bool continues(const NoBailSkater& owner,std::uint64_t episode,InputGenerations origin,InputAirEpisode air) noexcept {
    if (!active.load()) return false;
    std::lock_guard lock(mutex);
    return valid() && owner==session.owner && episode && episode==session.episode &&
        air.number==episode && air==session.air && origin==session.lineage &&
        session.lease.intent(GetTickCount64()) && retarget_observation().eligible() && valid();
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
    if (!valid()) {retire();return;}
    if (view!=session.block+0x48) return;
    const auto now=GetTickCount64();acknowledge(now);
    if (session.lease.acknowledged()) {
        bool grabbing{};const auto at=address(4);
        if (!at || !read(at,grabbing) || !grabbing) {retire();return;}
    }
    if (!session.lease.intent(now)) {retire();return;}
    const auto& p=*session.lease.selected();
    const std::array<std::uint32_t,4> values{0,1,std::bit_cast<std::uint32_t>(p.x),std::bit_cast<std::uint32_t>(p.y)};
    // The original trigger-input path owns hand intent and CanGrab. HIL proved
    // hand-state writes alone bypass that eligibility producer. Only the two
    // inspected pose-request signals are supplied here; native eligibility,
    // identities, animation clocks, blending and physics remain game-owned.
    for (unsigned i=2;i<4;++i) {
        const auto at=address(i);const unsigned size=i<2?1:4;auto& w=session.writes[i];
        if (!at || (w.at && w.at!=at)) {retire();return;}
        if (!w.at) {
            std::uint32_t previous{};if (!bits(at,size,previous)) {retire();return;}
            w={at,previous,values[i],size};
        }
        w.value=values[i];if (!write(at,w.value,size)) {retire();return;}
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

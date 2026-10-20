#pragma once
#include "arcade_native_probe.h"
#include "arcade_controller_capture_types.h"
#include "arcade_controller_route_observation.h"
#include "arcade_native_trick.h"
#include "arcade_native_input_state.h"
#include "arcade_observation.h"
#include "arcade_manual_history.h"
#include "arcade_manual_input.h"
#include "arcade_fixture_trajectory.h"
#include <atomic>
#include <compare>
#include <cmath>
#include <span>
#include <string_view>
#include <type_traits>
#include <variant>

namespace dingosdk::arcade_mode {
// Fixed sanitized snapshots only. Formatting and sinks belong to the worker.
struct PadEvidence {
    std::uint64_t sequence{}, ms{};
    unsigned user{}, buttons{}, lt{}, rt{}, output{}, phase{};
    bool source{};
    std::uint64_t tick{};
    short right_x{}, right_y{};
    bool playback_sample{};
    unsigned playback_mode{}, input_packet{}, output_packet{};
    std::uint64_t poll{};
    bool action_sample{}, action_requested{}, action_owned{};
    unsigned action_mode{}, action_kind{}, action_trick{}, action_result{};
    short left_x{}, left_y{};
    unsigned output_lt{}, output_rt{};
    ManualInputDiagnostic manual{};
    short manual_x{}, manual_y{};
    unsigned manual_pose{};
    bool manual_sample{};
};
struct RemapEvidence {
    bool ollie{}, flip{}, unsupported{};
    unsigned started{}, requested_id{};
};
struct PadTelemetry {
    PadEvidence pad{};
    RemapEvidence request{};
    bool diagnostic{};
};
struct ComboSample { NativeRequestSample sample; std::uint64_t ms{}; NativeFlipSelection flip; };
struct RevertLinkEvidence { std::uint64_t ms{}, sequence{}, labels{}; bool accepted{}; unsigned remaining_ms{}; };
struct BalanceFailureEvidence { std::uint64_t ms{}; unsigned kind{}; std::uint64_t request{}; };
struct BalanceTerminationEvidence { std::uint64_t ms{}; unsigned pose{}, outcome{}; };
struct ManualPhysicsEvidence {
    std::uint64_t ms{};
    float step{}, elapsed{}, repeat_pressure{}, position{}, velocity{};
    unsigned profile{};
};
struct BalanceSampleEvidence { std::uint64_t ms{}; unsigned kind{}; float x{}, v{}, input{}, difficulty{}; short raw_x{}, raw_y{}; };
struct ComboEventEvidence { std::uint64_t ms{}; unsigned pose{}; bool active{}, failed{}; float fatigue{}, grind{}, manual{}; };
struct BalanceBailEvidence { std::uint64_t ms{}, request{}; unsigned result{}; };
struct ScoreEdgeEvidence { std::uint64_t sequence{}, observed{}, sampled{}, labels{}; unsigned uncertainty{}; };
struct HeartbeatEvidence {
    std::uint64_t sequence{}, ms{}; bool valid{}; unsigned state{}, mode{};
    bool speed_valid{}, push_valid{}; float speed{}; unsigned contacts{};
    std::uint64_t sampled{}, score{}, multiplier{}; bool score_valid{}, multiplier_valid{};
    unsigned authored{}, stage{}, field{}, error{}, rejected{};
};
struct NativeCaptureEvidence { std::uint64_t id{}; bool begin{}; unsigned samples{}; };
struct NativeSampleEvidence { std::uint64_t id{}, ms{}; NativeRequestSample sample{}; };
struct InputEdgeEvidence { char button{}; std::uint64_t count{}; Observation sample{}; };
// Explicit capture only, on the existing bounded client queue. Owner is an
// anonymous capture-local token for the full owner tuple, never an address.
enum class CaptureCorrelationKind : std::uint8_t { begin, event, end };
enum class CaptureCorrelationSource : std::uint8_t { boundary, input, recognition, native, manual_admission, manual_termination, native_admission, pad_poll };
struct CaptureCorrelationEvidence {
    std::uint64_t capture{}, ordinal{}, source_sequence{}, ms{}, native_ms{}, owner{};
    std::uint64_t owner_generation{}, contact_generation{}, action_generation{}, physics_step{};
    std::uint64_t first_event{}, last_event{}, first_ms{}, last_ms{}, recognized_ms{};
    std::uint64_t expected{}, drained{}, dropped{}, drop_begin{}, drop_end{}, first_ordinal{}, last_ordinal{};
    // V2: one recognition summary plus ordered member rows on this SAME queue.
    std::uint64_t recognition_id{}, recognition_session{}, action{}, context{}, footprint_kind{};
    std::uint64_t member_index{}, member_count{}, member_event{}, held_reuse{};
    ControllerCaptureDetails controller{};
    unsigned schema{1}, required_sources{31};
    CaptureCorrelationKind kind{};
    CaptureCorrelationSource source{};
    bool native_valid{}, events_known{}, complete{}, overflow{}, queue_admitted{};
};
// Overflow/reset is evidence loss. Do not use modular arithmetic to make a
// wrapped capture/cumulative-drop counter appear complete.
inline bool capture_correlation_add(std::uint64_t& value,std::uint64_t amount=1) noexcept {
    if (amount>UINT64_MAX-value) { value=UINT64_MAX; return false; }
    value+=amount; return true;
}
inline bool capture_correlation_delta(std::uint64_t before,std::uint64_t after,std::uint64_t& delta) noexcept {
    delta=0; if (after<before) return false; delta=after-before; return true;
}
inline bool capture_correlation_valid(const CaptureCorrelationEvidence& r) noexcept {
    if ((r.schema!=1&&r.schema!=2&&r.schema!=3) || !r.capture || !r.required_sources || r.required_sources>(r.schema==3?127u:31u) || r.owner>UINT32_MAX ||
        r.kind>CaptureCorrelationKind::end || static_cast<unsigned>(r.source)>(r.schema==3?7u:5u)) return false;
    if(r.schema==3){
        const auto& d=r.controller;
        if(r.owner>16)return false;
        if(d.slot>4||d.selected_slot>4||d.bitmap>15||d.bitmap_as_read>15||(!d.bitmap_readable&&d.bitmap_as_read)||d.binding_state>3||(!d.bitmap_known&&d.bitmap)||
            d.readiness>8191||d.ownership>1023||d.readiness_owner>16||d.binding_owner>16||d.route_reason>static_cast<unsigned>(ControllerRouteReason::binding_revoked))return false;
        if(r.source==CaptureCorrelationSource::pad_poll &&
            (d.slot>=4||d.poll_started>d.raw_ms||d.raw_ms>d.returned_ms||d.returned_ms!=r.ms||
             (d.context_known&&(d.remap_ms<d.raw_ms||d.remap_ms>d.returned_ms))))return false;
        if(r.native_valid&&static_cast<unsigned>(r.source)>=6&&(!d.native_observed||!std::isfinite(d.physics_dt)||d.physics_dt<.001f||d.physics_dt>.1f))return false;
    }
    if (r.native_valid && (!r.owner || !r.owner_generation || !r.contact_generation || !r.action_generation || !r.physics_step ||
        r.native_ms>r.ms || r.ms-r.native_ms>InputTiming::native_sample_lease_ms)) return false;
    if (r.events_known && ((r.first_event==0)!=(r.last_event==0) || r.first_event>r.last_event ||
        (r.first_event && (r.first_ms>r.last_ms || r.last_ms>r.ms)) ||
        (!r.first_event && (r.first_ms || r.last_ms)))) return false;
    if (!r.events_known && (r.first_event || r.last_event || r.first_ms || r.last_ms)) return false;
    if (r.recognized_ms>r.ms || (r.recognized_ms && r.first_event && r.recognized_ms<r.last_ms)) return false;
    const bool recognition=r.schema>=2&&r.kind==CaptureCorrelationKind::event&&r.source==CaptureCorrelationSource::recognition;
    if(recognition){
        if(!r.recognition_id||!r.action||r.action>21||r.context>6||r.footprint_kind>2||r.member_count>100||r.member_index>r.member_count||r.recognized_ms!=r.ms)return false;
        if(r.footprint_kind==1){
            if(!r.events_known||!r.member_count||!r.first_event||
               (!r.member_index&&r.member_event)||(r.member_index&&(!r.member_event||r.member_event<r.first_event||r.member_event>r.last_event)))return false;
        }else if(r.first_event||r.last_event||r.first_ms||r.last_ms||r.member_count||r.member_index||r.member_event||r.held_reuse||
                 (r.footprint_kind==2&&!r.events_known)||(r.footprint_kind==0&&r.events_known))return false;
    }else if(r.recognition_id||r.recognition_session||r.action||r.context||r.footprint_kind||r.member_index||r.member_count||r.member_event||r.held_reuse||r.queue_admitted)return false;
    if (r.kind==CaptureCorrelationKind::event)
        return r.ordinal && r.source!=CaptureCorrelationSource::boundary && r.source_sequence && !r.complete &&
            !r.expected && !r.drained && !r.dropped && !r.drop_begin && !r.drop_end && !r.first_ordinal && !r.last_ordinal &&
            (r.source!=CaptureCorrelationSource::native || !r.native_valid || r.ms==r.native_ms);
    if (r.source!=CaptureCorrelationSource::boundary || r.source_sequence || r.first_event || r.last_event ||
        r.first_ms || r.last_ms || r.recognized_ms) return false;
    if (r.kind==CaptureCorrelationKind::begin)
        return !r.ordinal && !r.complete && !r.expected && !r.drained && !r.dropped &&
            r.drop_begin==r.drop_end && !r.first_ordinal && !r.last_ordinal;
    return r.expected<UINT64_MAX && r.ordinal==r.expected+1 && r.drained<=r.expected &&
        r.dropped==r.expected-r.drained && ((!r.drained && !r.first_ordinal && !r.last_ordinal) ||
        (r.drained && r.first_ordinal && r.first_ordinal<=r.last_ordinal && r.last_ordinal<=r.expected));
}
inline bool capture_correlation_complete(const CaptureCorrelationEvidence& r) noexcept {
    return capture_correlation_valid(r) && r.kind==CaptureCorrelationKind::end && r.complete &&
        (r.schema==3 || r.native_valid) && r.events_known && !r.overflow && r.expected && r.drained==r.expected && !r.dropped &&
        r.drop_begin==r.drop_end && r.first_ordinal==1 && r.last_ordinal==r.expected;
}
// Probe paths contain at most four schema hashes/array indexes, never native
// text or pointers. Fixed keys also avoid formatting paths under ModelWriteLock.
struct ScoreProbePath {
    std::array<std::uint32_t, 4> parts{};
    unsigned count{};
    bool valid() const noexcept { return count > 0 && count <= parts.size(); }
    bool append(std::uint32_t part) noexcept {
        if (!valid() || count == parts.size()) return false;
        parts[count++] = part;
        return true;
    }
    auto operator<=>(const ScoreProbePath&) const = default;
};
struct ScoreProbePathText {
    std::array<char, 36> text{};
    std::size_t size{};
    std::string_view view() const noexcept { return {text.data(), size}; }
};
// Called by the background sink only; all emitted bytes are numeric hex/dots.
inline ScoreProbePathText score_probe_path_text(const ScoreProbePath& path) noexcept {
    ScoreProbePathText out{};
    if (!path.valid()) return out;
    constexpr std::string_view digits = "0123456789abcdef";
    for (unsigned i = 0; i < path.count; ++i) {
        if (i) out.text[out.size++] = '.';
        for (unsigned nibble = 0; nibble < 8; ++nibble)
            out.text[out.size++] = digits[(path.parts[i] >> ((7 - nibble) * 4)) & 15];
    }
    return out;
}
struct ScorePresenceEvidence { std::uint64_t ms{}; unsigned source{}, rows{}, stage{}, parts{}, labels{}; };
struct ManualHistoryEvidence { std::uint64_t sequence{}, ms{}; ManualHistoryDiagnostic diagnostic{}; };
// Read-only native offboard flags/pelvis motion, paired with Trainer's retained
// position and its own timestamp. No inferred jump or requested action here.
struct FootMotionEvidence {
    std::uint64_t sequence{}, ms{}, position_ms{}, owner_epoch{};
    bool valid{}, airborne{}, ragdoll{}, wipeout{};
    float y{}, vy{};
};
// Direct board velocity at each native simulation step while Trainer logging
// is enabled. Client-position CSV remains separate evidence.
struct BoardMotionEvidence {
    std::uint64_t sequence{}, ms{};
    float seconds{}, vx{}, vy{}, vz{};
    unsigned state{}, mode{}, requests{}, contacts{};
    bool valid{}, wipeout{};
};
inline bool board_motion_valid(const BoardMotionEvidence& r) noexcept {
    return r.sequence && std::isfinite(r.seconds) && r.seconds > 0 && r.seconds <= .1f &&
        std::isfinite(r.vx) && std::isfinite(r.vy) && std::isfinite(r.vz) &&
        std::abs(r.vx) < 1000 && std::abs(r.vy) < 1000 && std::abs(r.vz) < 1000;
}
inline bool foot_motion_valid(const FootMotionEvidence& r) noexcept {
    return r.sequence && r.owner_epoch && std::isfinite(r.y) && std::isfinite(r.vy) &&
        std::abs(r.y) < 1000000 && std::abs(r.vy) < 1000 &&
        (!r.valid || (r.position_ms && r.ms >= r.position_ms && r.ms - r.position_ms <= 150));
}
struct ScoreNodeEvidence { ScoreProbePath path{}; unsigned kind{}, size{}; };
struct ScoreValueEvidence { std::uint64_t ms{}; ScoreProbePath path{}; std::uint32_t value{}; bool label{}, limit{}; };
using ClientTelemetry = std::variant<Observation, RevertLinkEvidence, BalanceFailureEvidence, BalanceTerminationEvidence, ManualPhysicsEvidence,
    BalanceSampleEvidence, ComboEventEvidence, BalanceBailEvidence, ScoreEdgeEvidence,
    HeartbeatEvidence, NativeCaptureEvidence, NativeSampleEvidence, InputEdgeEvidence,
    ScorePresenceEvidence, ScoreNodeEvidence, ScoreValueEvidence, ManualHistoryEvidence, FixtureTrajectoryEvidence,
    FootMotionEvidence, BoardMotionEvidence, CaptureCorrelationEvidence, ControllerRouteEvidence>;
// Native UI observers use this bridge; the sink never receives native objects.
bool enqueue_client_telemetry(const ClientTelemetry& row) noexcept;
inline bool client_telemetry_valid(const ClientTelemetry& row) noexcept {
    if (const auto* manual = std::get_if<ManualPhysicsEvidence>(&row))
        return std::isfinite(manual->step) && manual->step > 0 && manual->step <= .1f &&
            std::isfinite(manual->elapsed) && manual->elapsed >= 0 &&
            std::isfinite(manual->repeat_pressure) && manual->repeat_pressure >= 0 &&
            std::isfinite(manual->position) && std::abs(manual->position) <= 1 && std::isfinite(manual->velocity) &&
            (manual->profile == 5 || manual->profile == 10);
    if (const auto* capture = std::get_if<CaptureCorrelationEvidence>(&row)) return capture_correlation_valid(*capture);
    if (const auto* route = std::get_if<ControllerRouteEvidence>(&row)) return controller_route_evidence_valid(*route);
    if (const auto* board = std::get_if<BoardMotionEvidence>(&row)) return board_motion_valid(*board);
    if (const auto* node = std::get_if<ScoreNodeEvidence>(&row)) return node->path.valid();
    if (const auto* value = std::get_if<ScoreValueEvidence>(&row)) return value->path.valid();
    if (const auto* foot = std::get_if<FootMotionEvidence>(&row)) return foot_motion_valid(*foot);
    return true;
}
inline bool client_telemetry_fresh(const ClientTelemetry& row, std::uint64_t now) noexcept {
    const auto* heartbeat = std::get_if<HeartbeatEvidence>(&row);
    return !heartbeat || (now >= heartbeat->ms && now - heartbeat->ms <= 100);
}

// Same bounded reservation/publication scheme as GrindQueue. A stalled sink
// cannot hold a producer lock. A reserved but unpublished slot is never read.
template<class Row, std::size_t N> class InputTelemetryQueue {
    static_assert(N >= 2 && std::is_trivially_copyable_v<Row>);
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
    struct Slot { std::atomic<std::uint64_t> turn{}; Row row{}; };
    std::array<Slot, N> rows_{};
    std::atomic<std::uint64_t> write_{}, dropped_{};
    std::atomic<bool> drop_overflow_{};
    std::uint64_t read_{}; // Single background consumer, including drop reports.
    std::uint64_t reported_{};
public:
    InputTelemetryQueue() noexcept {
        for (std::size_t i = 0; i < N; ++i) rows_[i].turn.store(i, std::memory_order_relaxed);
    }
    bool push(const Row& row) noexcept {
        auto at = write_.load(std::memory_order_relaxed);
        for (unsigned attempt = 0; attempt < 64; ++attempt) {
            auto& slot = rows_[at % N];
            const auto turn = slot.turn.load(std::memory_order_acquire);
            if (turn < at) break;
            if (turn == at && write_.compare_exchange_weak(at, at + 1, std::memory_order_relaxed)) {
                slot.row = row;
                slot.turn.store(at + 1, std::memory_order_release);
                return true;
            }
            at = write_.load(std::memory_order_relaxed);
        }
        lose();
        return false;
    }
    std::size_t take(std::span<Row> out) noexcept {
        std::size_t n{};
        while (n < out.size()) {
            auto& slot = rows_[read_ % N];
            if (slot.turn.load(std::memory_order_acquire) != read_ + 1) break;
            out[n++] = slot.row;
            slot.turn.store(read_ + N, std::memory_order_release);
            ++read_;
        }
        return n;
    }
    void lose() noexcept { if(dropped_.fetch_add(1,std::memory_order_relaxed)==UINT64_MAX)drop_overflow_.store(true,std::memory_order_release); }
    std::uint64_t dropped() const noexcept { return drop_overflow_.load(std::memory_order_acquire)?UINT64_MAX:dropped_.load(std::memory_order_relaxed); }
    std::uint64_t take_dropped() noexcept {
        const auto total = dropped();
        const auto delta = total - reported_;
        reported_ = total;
        return delta;
    }
};

template<std::size_t N> class ComboTelemetry {
    std::atomic_flag producer_ = ATOMIC_FLAG_INIT;
    NativeRequestSample previous_{};
    NativeFlipSelection flip_{};
public:
    InputTelemetryQueue<ComboSample, N> queue;
    void observe(const ComboSample& row) noexcept {
        // Concurrent physics observations may lose evidence, never stall physics.
        if (producer_.test_and_set(std::memory_order_acquire)) { queue.lose(); return; }
        if (row.sample != previous_ || row.flip != flip_) {
            previous_ = row.sample;
            flip_ = row.flip;
            queue.push(row);
        }
        producer_.clear(std::memory_order_release);
    }
};
struct InputTelemetry {
    InputTelemetryQueue<PadTelemetry, 64> pad;
    ComboTelemetry<64> combo;
    InputTelemetryQueue<ClientTelemetry, 64> client;
};
// Serialized by the existing input mutex. Actual producer snapshots only;
// the full owner tuple stays private and becomes capture-local token1 on wire.
class CaptureCorrelationProducer {
    NativeInputFrame proof_{};
    std::uint64_t next_{},id_{},started_{},until_{},expected_{},drops_{};
    std::array<std::uint64_t,8> sequences_{};
    std::array<NativeInputOwner,16> owners_{};
    bool observational_{};
    std::array<std::uint64_t,64> polls_{};std::uint64_t next_poll_{};
    unsigned scope_{};bool active_{},known_{true},overflow_{};
    static bool fresh(const NativeInputFrame& f,std::uint64_t now) noexcept {
        return f.valid&&f.owner.valid()&&f.owner_generation&&f.contact_generation&&f.action_generation&&f.physics_step&&
            f.sampled_at<=now&&now-f.sampled_at<=InputTiming::native_sample_lease_ms;
    }
    unsigned owner_token(const NativeInputFrame& f) noexcept {
        if(!f.owner.valid())return 0;
        for(unsigned i=0;i<owners_.size();++i){
            if(owners_[i]==f.owner)return i+1;
            if(!owners_[i].valid()){owners_[i]=f.owner;return i+1;}
        }
        overflow_=true;return 0;
    }
    CaptureCorrelationEvidence snapshot(std::uint64_t now,const NativeInputFrame& f) noexcept {
        CaptureCorrelationEvidence r;r.schema=observational_?3:2;r.capture=id_;r.required_sources=scope_;r.ms=now;r.native_ms=f.sampled_at;r.owner=observational_?owner_token(f):1;
        r.owner_generation=f.owner_generation;r.contact_generation=f.contact_generation;r.action_generation=f.action_generation;r.physics_step=f.physics_step;
        r.native_valid=fresh(f,now)&&r.owner;return r;
    }
    bool current(std::uint64_t now,const NativeInputFrame& f) const noexcept {
        return fresh(f,now)&&f.owner==proof_.owner&&f.owner_generation==proof_.owner_generation&&now>=started_;
    }
public:
    bool active()const noexcept{return active_;}
    bool observational()const noexcept{return observational_;}
    std::uint64_t capture_id()const noexcept{return active_?id_:0;}
    std::uint64_t reserve_poll()noexcept{
        if(!captures(CaptureCorrelationSource::pad_poll))return 0;
        for(auto& slot:polls_)if(!slot){
            if(next_poll_==UINT64_MAX){omitted(1);lost();return 0;}
            slot=++next_poll_;return slot;
        }
        omitted(1);lost();return 0;
    }
    bool release_poll(std::uint64_t capture,std::uint64_t poll)noexcept{
        if(!active_||capture!=id_||!poll)return false;
        for(auto& slot:polls_)if(slot==poll){slot=0;return true;}
        lost();return false;
    }
    bool captures(CaptureCorrelationSource source)const noexcept{
        const auto value=static_cast<unsigned>(source);return active_&&value&&value<=7&&(scope_&(1u<<(value-1)));
    }
    void omitted(std::uint64_t attempts)noexcept{
        if(!active_||!attempts)return;
        if(!capture_correlation_add(expected_,attempts))overflow_=true;
        known_=false;
    }
    void lost()noexcept{overflow_=true;}
    template<class Sink> bool begin(std::uint64_t now,std::uint64_t duration,unsigned scope,const NativeInputFrame& f,std::uint64_t drops,Sink&& sink) noexcept {
        if(active_||!duration||duration>5000||!scope||scope>127||(!(scope&96)&&!fresh(f,now))||next_==UINT64_MAX||duration>UINT64_MAX-now)return false;
        observational_=(scope&96)!=0;owners_={};polls_={};next_poll_=0;id_=++next_;started_=now;until_=now+duration;scope_=scope;proof_=f;expected_=0;drops_=drops;sequences_={};known_=true;overflow_=drops==UINT64_MAX;active_=true;
        auto r=snapshot(now,f);r.kind=CaptureCorrelationKind::begin;r.events_known=true;r.drop_begin=r.drop_end=drops;r.overflow=overflow_;
        // Lost begin remains an incomplete capture, never silently restarted.
        sink(r);return true;
    }
    template<class Sink> void end(std::uint64_t now,const NativeInputFrame& f,std::uint64_t drops,Sink&& sink,bool forced=false) noexcept {
        if(!active_)return;
        const bool same=current(now,f);const auto& original=same&&f.physics_step>=proof_.physics_step?f:proof_;
        auto r=snapshot(now,original);r.native_valid=r.native_valid&&same&&!forced;r.kind=CaptureCorrelationKind::end;
        for(auto poll:polls_)if(poll){omitted(1);}
        r.expected=expected_;r.ordinal=expected_;if(!capture_correlation_add(r.ordinal))overflow_=true;
        r.dropped=expected_;r.events_known=known_;r.drop_begin=drops_;r.drop_end=drops;r.overflow=overflow_||drops<drops_||drops==UINT64_MAX;
        r.complete=!forced&&(observational_||r.native_valid)&&known_&&!r.overflow;active_=false;sink(r);
    }
    template<class Sink> void maintain(std::uint64_t now,const NativeInputFrame& f,std::uint64_t drops,Sink&& sink) noexcept {
        if(active_&&(now<started_||now>until_||(!observational_&&!current(now,f))))
            end(now,f,drops,sink,now<started_||(!observational_&&!current(now,f)));
    }
private:
    template<class Sink> void submit(CaptureCorrelationEvidence row,Sink&& sink)noexcept{
        const auto source=static_cast<unsigned>(row.source);
        if(!source||source>7||!(scope_&(1u<<(source-1))))return;
        if(row.source_sequence<=sequences_[source])overflow_=true;
        sequences_[source]=row.source_sequence;
        row.schema=observational_?3:2;row.capture=id_;row.required_sources=scope_;if(!observational_)row.owner=1;row.kind=CaptureCorrelationKind::event;
        known_=known_&&row.events_known&&(observational_||row.native_valid);
        if(!capture_correlation_add(expected_))overflow_=true;row.ordinal=expected_;row.overflow=overflow_;
        sink(row);
    }
public:
    template<class Sink> void event(CaptureCorrelationEvidence row,const NativeInputFrame& f,std::uint64_t drops,Sink&& sink) noexcept {
        maintain(row.ms,f,drops,sink);if(!active_||!captures(row.source))return;
        row.native_ms=f.sampled_at;row.owner_generation=f.owner_generation;row.contact_generation=f.contact_generation;row.action_generation=f.action_generation;row.physics_step=f.physics_step;row.native_valid=fresh(f,row.ms);
        if(observational_){row.owner=owner_token(f);row.native_valid=row.native_valid&&row.owner;}
        if(f.physics_step>=proof_.physics_step)proof_=f;
        submit(row,sink);
    }
    template<class Sink> void observation(CaptureCorrelationEvidence row,const NativeInputFrame& f,std::uint64_t drops,Sink&& sink,const NativeInputFrame& readiness={},const NativeInputFrame& binding={}) noexcept {
        if(!observational_)return;
        row.controller.readiness_owner=owner_token(readiness);row.controller.binding_owner=owner_token(binding);
        event(row,f,drops,sink);
    }
    template<class Sink> void recognition(CaptureCorrelationEvidence row,const InputNativeContext& original,bool known,
            std::uint64_t delivery_now,const NativeInputFrame& authority,std::uint64_t drops,Sink&& sink)noexcept{
        maintain(delivery_now,authority,drops,sink);if(!active_||!captures(row.source))return;
        if(row.ms<started_||row.ms>until_||row.ms>delivery_now){omitted(1);lost();return;}
        // The original observation supplies every serialized proof scalar. The
        // current frame only proves continuity with the capture's private owner;
        // it never substitutes newer generation/time/step values into this row.
        row.native_ms=original.sampled_at;row.owner_generation=original.generations.owner;
        row.contact_generation=original.generations.contact;row.action_generation=original.generations.action;row.physics_step=original.physics_step;
        row.native_valid=known&&(!observational_||current(delivery_now,authority))&&original.required&&original.fresh(row.ms)&&original.generations.owner==proof_.owner_generation&&
            original.physics_step<=authority.physics_step&&original.sampled_at<=authority.sampled_at&&
            original.generations.contact<=authority.contact_generation&&original.generations.action<=authority.action_generation&&
            (original.physics_step!=authority.physics_step||(original.sampled_at==authority.sampled_at&&
             original.generations.contact==authority.contact_generation&&original.generations.action==authority.action_generation));
        if(observational_){row.owner=owner_token(authority);row.native_valid=row.native_valid&&row.owner;}
        submit(row,sink);
    }
};
// Existing client worker only. Counts successful sink writes, not reservations.
// A footer is finalized after all earlier queue rows have been drained.
class CaptureCorrelationDrain {
    struct Capture {
        std::uint64_t id{},written{},first{},last{};unsigned sources{};bool began{},known{true},overflow{};
    };
    std::array<Capture,16> captures_{};
    Capture* find(std::uint64_t id) noexcept {
        for(auto& c:captures_)if(c.id==id)return &c;
        for(auto& c:captures_)if(!c.id){c.id=id;return &c;}
        return nullptr;
    }
public:
    void written(const CaptureCorrelationEvidence& row,bool accepted) noexcept {
        auto* c=find(row.capture);if(!c)return;
        if(row.kind==CaptureCorrelationKind::begin){if(c->began)c->overflow=true;c->began=accepted;return;}
        if(row.kind!=CaptureCorrelationKind::event||!accepted)return;
        if(!capture_correlation_add(c->written))c->overflow=true;
        if(!c->first)c->first=row.ordinal;c->last=row.ordinal;
        c->sources|=1u<<(static_cast<unsigned>(row.source)-1);
        c->known=c->known&&row.events_known&&(row.schema==3||row.native_valid);c->overflow=c->overflow||row.overflow;
    }
    CaptureCorrelationEvidence footer(CaptureCorrelationEvidence row,std::uint64_t drops) noexcept {
        auto* c=find(row.capture);row.drop_end=drops;
        if(!c){row.complete=false;row.overflow=true;row.drained=0;row.dropped=row.expected;return row;}
        row.drained=c->written;row.first_ordinal=c->first;row.last_ordinal=c->last;
        row.overflow=row.overflow||c->overflow||row.drained>row.expected||drops==UINT64_MAX||drops<row.drop_begin;
        row.dropped=row.expected>=row.drained?row.expected-row.drained:0;
        row.complete=row.complete&&c->began&&c->known&&!row.overflow&&
            (c->sources&row.required_sources)==row.required_sources&&row.drained==row.expected&&row.drop_begin==drops;
        // Releasing a worker slot does not reuse the producer's capture ID.
        *c={};return row;
    }
};
// Published only after a worker starts. Its storage, like the pinned runtime,
// lasts until process teardown; toggling gameplay never resets a live queue.
class InputTelemetryLink {
    std::atomic<InputTelemetry*> target_{};
public:
    void publish(InputTelemetry& target) noexcept { target_.store(&target, std::memory_order_release); }
    bool available() const noexcept { return target_.load(std::memory_order_acquire)!=nullptr; }
    std::uint64_t client_dropped() const noexcept {
        const auto* target=target_.load(std::memory_order_acquire);return target?target->client.dropped():UINT64_MAX;
    }
    bool pad(const PadTelemetry& row) noexcept {
        if (auto* target = target_.load(std::memory_order_acquire)) return target->pad.push(row);
        return false;
    }
    void combo(const ComboSample& row) noexcept {
        if (auto* target = target_.load(std::memory_order_acquire)) target->combo.observe(row);
    }
    bool client(const ClientTelemetry& row) noexcept {
        if (auto* target = target_.load(std::memory_order_acquire)) {
            if (!client_telemetry_valid(row)) { target->client.lose(); return false; }
            return target->client.push(row);
        }
        return false;
    }
};
}

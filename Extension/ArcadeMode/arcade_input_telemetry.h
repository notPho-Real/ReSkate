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
#include "arcade_capture_schema4.h"
#include "arcade_settings_snapshot.h"
#include <algorithm>
#include <atomic>
#include <chrono>
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
    RuntimeSettingsSnapshot settings{};
};
struct NativeCaptureEvidence { std::uint64_t id{}; bool begin{}; unsigned samples{}; };
struct NativeSampleEvidence { std::uint64_t id{}, ms{}; NativeRequestSample sample{}; };
struct InputEdgeEvidence { char button{}; std::uint64_t count{}; Observation sample{}; };
// Explicit capture only, on the existing bounded client queue. Owner is an
// anonymous capture-local token for the full owner tuple, never an address.
enum class CaptureCorrelationKind : std::uint8_t { begin, event, end };
enum class CaptureCorrelationSource : std::uint8_t { boundary, input, recognition, native, manual_admission, manual_termination, native_admission, pad_poll };
struct CaptureBudgetReceipt {
    std::uint64_t attempted{},emitted{},drained{},dropped{},backlog{},peak{};
    std::uint64_t file_id{},file_bytes{},file_records{},prefix_hash{};
    std::uint32_t volume{};
    bool queue_valid{},file_valid{};
};
struct CaptureCorrelationEvidence {
    std::uint64_t capture{}, ordinal{}, source_sequence{}, ms{}, native_ms{}, owner{};
    std::uint64_t owner_generation{}, contact_generation{}, action_generation{}, physics_step{};
    std::uint64_t first_event{}, last_event{}, first_ms{}, last_ms{}, recognized_ms{};
    std::uint64_t expected{}, drained{}, dropped{}, drop_begin{}, drop_end{}, first_ordinal{}, last_ordinal{};
    // V2: one recognition summary plus ordered member rows on this SAME queue.
    std::uint64_t recognition_id{}, recognition_session{}, action{}, context{}, footprint_kind{};
    std::uint64_t member_index{}, member_count{}, member_event{}, held_reuse{};
    ControllerCaptureDetails controller{};
    CaptureSchema4 semantic{};
    CaptureBudgetReceipt budget{}; // Queue-only carrier, excluded from the wire.
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
    if ((r.schema!=1&&r.schema!=2&&r.schema!=3&&r.schema!=4) || !r.capture || !r.required_sources || r.required_sources>(r.schema>=3?127u:31u) || r.owner>UINT32_MAX ||
        r.kind>CaptureCorrelationKind::end || static_cast<unsigned>(r.source)>(r.schema>=3?7u:5u)) return false;
    if(r.schema==4){
        const auto& m=r.semantic;
        if(!m.valid() || (r.kind!=CaptureCorrelationKind::event && m.m_kind) ||
           (m.m_kind==0&&m.m_poll!=0) ||
           (m.m_kind==1&&r.source!=CaptureCorrelationSource::recognition) ||
           (m.m_kind==2&&(r.source!=CaptureCorrelationSource::pad_poll||!m.m_poll||
               m.m_poll!=r.source_sequence)) ||
           ((m.m_kind==3||m.m_kind==7)&&r.source!=CaptureCorrelationSource::native) ||
           ((m.m_kind==4||m.m_kind==5||m.m_kind==8)&&r.source!=CaptureCorrelationSource::manual_admission) ||
           (m.m_kind==6&&r.source!=CaptureCorrelationSource::manual_termination))return false;
    }
    if(r.schema==3||r.schema==4){
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
    return r.schema!=4 && capture_correlation_valid(r) && r.kind==CaptureCorrelationKind::end && r.complete &&
        (r.schema>=3 || r.native_valid) && r.events_known && !r.overflow && r.expected && r.drained==r.expected && !r.dropped &&
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
class CaptureArrivalMeter {
    std::atomic<bool> enabled_{},violation_{};
    std::atomic_flag busy_=ATOMIC_FLAG_INIT;
    std::uint64_t last_ms_{},credit_{16000};
    static std::uint64_t clock_ms() noexcept {
        const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        return ms<0?0:static_cast<std::uint64_t>(ms);
    }
public:
    void begin() noexcept {
        if(busy_.test_and_set(std::memory_order_acquire)){
            violation_.store(true,std::memory_order_release);return;
        }
        violation_.store(false,std::memory_order_release);
        last_ms_=clock_ms();credit_=16000;
        enabled_.store(true,std::memory_order_release);
        busy_.clear(std::memory_order_release);
    }
    void end() noexcept {enabled_.store(false,std::memory_order_release);}
    bool violation() const noexcept{return violation_.load(std::memory_order_acquire);}
    void arrival() noexcept {
        if(!enabled_.load(std::memory_order_acquire))return;
        // Contention itself is an unaccounted arrival: fail closed without
        // adding a producer wait to the input or native physics path.
        if(busy_.test_and_set(std::memory_order_acquire)){
            violation_.store(true,std::memory_order_release);return;
        }
        const auto now=clock_ms();
        if(now<last_ms_)violation_.store(true,std::memory_order_release);
        else {
            const auto elapsed=now-last_ms_;
            credit_=elapsed>=80?16000:std::min<std::uint64_t>(16000,credit_+elapsed*200);
            last_ms_=now;
            if(credit_<1000)violation_.store(true,std::memory_order_release);
            else credit_-=1000;
        }
        busy_.clear(std::memory_order_release);
    }
};
struct CaptureQueueNoPause { void operator()() const noexcept {} };
template<class Row, std::size_t N, class BeforePublish=CaptureQueueNoPause> class InputTelemetryQueue {
    static_assert(N >= 2 && std::is_trivially_copyable_v<Row>);
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
    struct Slot { std::atomic<std::uint64_t> turn{}; Row row{}; };
    std::array<Slot, N> rows_{};
    std::atomic<std::uint64_t> write_{}, published_{}, dropped_{}, attempted_{}, drained_{}, read_published_{}, peak_backlog_{},
        dropped_baseline_{},preexisting_{};
    std::atomic<bool> drop_overflow_{};
    std::atomic<bool> accounting_{};
    CaptureArrivalMeter* arrival_meter_{};
    std::uint64_t read_{}; // Single background consumer, including drop reports.
    std::uint64_t reported_{};
public:
    InputTelemetryQueue() noexcept {
        for (std::size_t i = 0; i < N; ++i) rows_[i].turn.store(i, std::memory_order_relaxed);
    }
    void attach(CaptureArrivalMeter& capture_arrival_meter) noexcept {arrival_meter_=&capture_arrival_meter;}
    void start_accounting() noexcept {
        attempted_.store(0,std::memory_order_relaxed);
        published_.store(0,std::memory_order_relaxed);
        drained_.store(0,std::memory_order_relaxed);
        const auto at=write_.load(std::memory_order_acquire);
        const auto read=read_published_.load(std::memory_order_acquire);
        peak_backlog_.store(at>=read?at-read:UINT64_MAX,std::memory_order_release);
        preexisting_.store(at>=read?at-read:UINT64_MAX,std::memory_order_release);
        dropped_baseline_.store(dropped_.load(std::memory_order_acquire),std::memory_order_release);
        accounting_.store(true,std::memory_order_release);
    }
    void stop_accounting() noexcept {accounting_.store(false,std::memory_order_release);}
    bool push(const Row& row) noexcept {
        if(arrival_meter_)arrival_meter_->arrival();
        const bool accounting=accounting_.load(std::memory_order_acquire);
        if(accounting)attempted_.fetch_add(1, std::memory_order_relaxed);
        auto at = write_.load(std::memory_order_relaxed);
        for (unsigned attempt = 0; attempt < 64; ++attempt) {
            if(at==UINT64_MAX){drop_overflow_.store(true,std::memory_order_release);break;}
            auto& slot = rows_[at % N];
            const auto turn = slot.turn.load(std::memory_order_acquire);
            if (turn < at) break;
            if (turn == at && write_.compare_exchange_weak(at, at + 1, std::memory_order_relaxed)) {
                const auto read=read_published_.load(std::memory_order_acquire);
                if(at+1<read)drop_overflow_.store(true,std::memory_order_release);
                else if(accounting){
                    const auto backlog=at+1-read;
                    auto peak=peak_backlog_.load(std::memory_order_relaxed);
                    while(backlog>peak&&!peak_backlog_.compare_exchange_weak(peak,backlog,std::memory_order_release,std::memory_order_relaxed)){}
                }
                // Test-only policy can pause a reservation here. The default
                // is empty and adds no work to the production queue.
                if constexpr(!std::is_same_v<BeforePublish,CaptureQueueNoPause>)BeforePublish{}();
                slot.row = row;
                slot.turn.store(at + 1, std::memory_order_release);
                if(accounting)published_.fetch_add(1, std::memory_order_release);
                return true;
            }
            at = write_.load(std::memory_order_relaxed);
        }
        lose(true);
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
        if (n) {
            if(accounting_.load(std::memory_order_acquire)){
                const auto capture_preexisting_rows=preexisting_.load(std::memory_order_acquire);
                const auto old=std::min<std::uint64_t>(capture_preexisting_rows,n);
                if(old)preexisting_.fetch_sub(old,std::memory_order_acq_rel);
                drained_.fetch_add(n-old,std::memory_order_relaxed);
            }
            read_published_.store(read_, std::memory_order_release);
        }
        return n;
    }
    void lose(bool counted=false) noexcept {
        if(!counted&&arrival_meter_)arrival_meter_->arrival();
        if(!counted&&accounting_.load(std::memory_order_acquire))attempted_.fetch_add(1,std::memory_order_relaxed);
        if(dropped_.fetch_add(1,std::memory_order_relaxed)==UINT64_MAX)drop_overflow_.store(true,std::memory_order_release);
    }
    struct Counters { std::uint64_t attempted{}, emitted{}, drained{}, dropped{}, backlog{}, peak{}; bool overflow{}; };
    Counters counters() const noexcept {
        const auto attempted=attempted_.load(std::memory_order_acquire);
        const auto queue_emitted=published_.load(std::memory_order_acquire);
        const auto drained=drained_.load(std::memory_order_acquire);
        const auto all_dropped=this->dropped();
        const auto baseline=dropped_baseline_.load(std::memory_order_acquire);
        const auto dropped=all_dropped>=baseline?all_dropped-baseline:UINT64_MAX;
        const auto read=read_published_.load(std::memory_order_acquire);
        const auto reserved=write_.load(std::memory_order_acquire);
        return {attempted,queue_emitted,drained,dropped,reserved>=read?reserved-read:UINT64_MAX,peak_backlog_.load(std::memory_order_acquire),
            attempted<queue_emitted||attempted-queue_emitted<dropped||drained>queue_emitted||reserved<read||
                all_dropped<baseline||drop_overflow_.load(std::memory_order_acquire)};
    }
    std::uint64_t dropped() const noexcept { return drop_overflow_.load(std::memory_order_acquire)?UINT64_MAX:dropped_.load(std::memory_order_relaxed); }
    std::uint64_t retirement_barrier() const noexcept {return write_.load(std::memory_order_acquire);}
    std::uint64_t consumed_through() const noexcept {return read_published_.load(std::memory_order_acquire);}
    std::uint64_t take_dropped() noexcept {
        const auto total = dropped();
        const auto delta = total - reported_;
        reported_ = total;
        return delta;
    }
};
template<class Row,std::size_t N,class BeforePublish>
bool capture_queue_barrier_reached(const InputTelemetryQueue<Row,N,BeforePublish>& queue,
                                   std::uint64_t barrier) noexcept {
    return queue.consumed_through()>=barrier;
}

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
    CaptureArrivalMeter arrival;
    InputTelemetryQueue<PadTelemetry, 64> pad;
    ComboTelemetry<64> combo;
    InputTelemetryQueue<ClientTelemetry, 64> client;
    InputTelemetry() noexcept {
        pad.attach(arrival);combo.queue.attach(arrival);client.attach(arrival);
    }
    void begin_capture_accounting() noexcept {
        pad.start_accounting();combo.queue.start_accounting();client.start_accounting();arrival.begin();
    }
    void end_capture_accounting() noexcept {
        arrival.end();pad.stop_accounting();combo.queue.stop_accounting();client.stop_accounting();
    }
    struct Counters {std::uint64_t attempted{},emitted{},drained{},dropped{},backlog{},peak{};bool invalid{};};
    Counters counters() const noexcept {
        Counters out{};
        const auto add=[&](const auto& q){
            const auto row=q.counters();
            out.invalid=out.invalid||row.overflow;
            const auto sum=[&](std::uint64_t& target,std::uint64_t value){
                if(value>UINT64_MAX-target){out.invalid=true;target=UINT64_MAX;}
                else target+=value;
            };
            sum(out.attempted,row.attempted);sum(out.emitted,row.emitted);
            sum(out.drained,row.drained);sum(out.dropped,row.dropped);sum(out.backlog,row.backlog);sum(out.peak,row.peak);
        };
        add(pad);add(combo.queue);add(client);
        out.invalid=out.invalid||arrival.violation()||out.attempted<out.emitted||out.drained>out.emitted||
            out.dropped>out.attempted||out.backlog>192||out.peak>192;
        return out;
    }
};
// Serialized by the existing input mutex. Actual producer snapshots only;
// the full owner tuple stays private and becomes capture-local token1 on wire.
class CaptureCorrelationProducer {
    NativeInputFrame proof_{};
    std::uint64_t next_{},id_{},started_{},until_{},expected_{},drops_{},semantic_attempted_{},semantic_queue_lost_{};
    std::array<std::uint64_t,8> sequences_{};
    std::array<NativeInputOwner,16> owners_{};
    bool observational_{},schema4_{};
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
        CaptureCorrelationEvidence r;r.schema=schema4_?4:observational_?3:2;r.capture=id_;r.required_sources=scope_;r.ms=now;r.native_ms=f.sampled_at;r.owner=observational_?owner_token(f):1;
        r.owner_generation=f.owner_generation;r.contact_generation=f.contact_generation;r.action_generation=f.action_generation;r.physics_step=f.physics_step;
        r.native_valid=fresh(f,now)&&r.owner;return r;
    }
    bool current(std::uint64_t now,const NativeInputFrame& f) const noexcept {
        return fresh(f,now)&&f.owner==proof_.owner&&f.owner_generation==proof_.owner_generation&&now>=started_;
    }
public:
    bool active()const noexcept{return active_;}
    bool observational()const noexcept{return observational_;}
    bool schema4()const noexcept{return active_&&schema4_;}
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
        if(active_||!duration||duration>(scope==255?1000u:5000u)||!scope||(scope>127&&scope!=255)||(!(scope&96)&&!fresh(f,now))||next_==UINT64_MAX||duration>UINT64_MAX-now)return false;
        schema4_=scope==255;observational_=(scope&96)!=0;owners_={};polls_={};next_poll_=0;id_=++next_;started_=now;until_=now+duration;scope_=schema4_?127u:scope;proof_=f;expected_=0;drops_=drops;semantic_attempted_=semantic_queue_lost_=0;sequences_={};known_=true;overflow_=drops==UINT64_MAX;active_=true;
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
        if(schema4_){r.semantic.m_attempted=semantic_attempted_;r.semantic.m_emitted=semantic_attempted_-semantic_queue_lost_;}
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
        row.schema=schema4_?4:observational_?3:2;row.capture=id_;row.required_sources=scope_;if(!observational_)row.owner=1;row.kind=CaptureCorrelationKind::event;
        known_=known_&&row.events_known&&(observational_||row.native_valid);
        if(!capture_correlation_add(expected_))overflow_=true;row.ordinal=expected_;row.overflow=overflow_;
        if(schema4_&&row.semantic.m_kind){
            // These are transport row counters. The reader assembles the
            // summary and its ordered members into one recognition transition.
            if(!capture_correlation_add(semantic_attempted_))overflow_=true;
            if(!sink(row)){if(!capture_correlation_add(semantic_queue_lost_))overflow_=true;overflow_=true;}
        }else sink(row);
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
        std::uint64_t id{},written{},semantic_written{},first{},last{},started{},last_cycle{},worst_gap{},peak{};
        unsigned sources{},coverage{};bool began{},known{true},overflow{};
        CaptureBudgetReceipt start{};
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
        if(row.kind==CaptureCorrelationKind::begin){
            if(c->began)c->overflow=true;c->began=accepted;c->start=row.budget;
            c->peak=row.budget.backlog;c->started=c->last_cycle=row.ms;return;
        }
        if(row.kind!=CaptureCorrelationKind::event||!accepted)return;
        if(!c->began)c->overflow=true;
        if(!capture_correlation_add(c->written))c->overflow=true;
        if(!c->first)c->first=row.ordinal;c->last=row.ordinal;
        c->sources|=1u<<(static_cast<unsigned>(row.source)-1);
        if(row.schema==4&&row.semantic.m_kind>=1&&row.semantic.m_kind<=8){
            c->coverage|=1u<<(row.semantic.m_kind-1);
            if(!capture_correlation_add(c->semantic_written))c->overflow=true;
        }
        c->known=c->known&&row.events_known&&(row.schema>=3||row.native_valid);c->overflow=c->overflow||row.overflow;
    }
    void cycle(std::uint64_t now,std::uint64_t backlog) noexcept {
        for(auto& c:captures_)if(c.id&&c.began){
            if(now<c.last_cycle)c.overflow=true;
            else c.worst_gap=std::max(c.worst_gap,now-c.last_cycle);
            c.last_cycle=now;c.peak=std::max(c.peak,backlog);
        }
    }
    CaptureBudgetReceipt start(std::uint64_t id) const noexcept {
        for(const auto& c:captures_)if(c.id==id)return c.start;
        return {};
    }
    void abandon(std::uint64_t id) noexcept {
        for(auto& c:captures_)if(c.id==id){c={};return;}
    }
    CaptureCorrelationEvidence footer(CaptureCorrelationEvidence row,std::uint64_t drops,
            CaptureBudgetReceipt finish={},std::uint64_t now=0) noexcept {
        auto* c=find(row.capture);row.drop_end=drops;
        if(!c){row.complete=false;row.overflow=true;row.drained=0;row.dropped=row.expected;return row;}
        row.drained=c->written;row.first_ordinal=c->first;row.last_ordinal=c->last;
        if(row.schema==4){
            auto& m=row.semantic;const auto& start=c->start;
            m.m_coverage=static_cast<std::uint16_t>(c->coverage);
            m.m_drained=c->semantic_written;
            m.m_dropped=m.m_attempted>=m.m_drained?m.m_attempted-m.m_drained:UINT64_MAX;
            const bool order=finish.attempted>=start.attempted&&finish.emitted>=start.emitted&&
                finish.drained>=start.drained&&finish.dropped>=start.dropped&&
                finish.file_bytes>=start.file_bytes&&finish.file_records>=start.file_records;
            if(order){
                m.q_attempted=finish.attempted-start.attempted;
                m.q_emitted=finish.emitted-start.emitted;
                m.q_drained=finish.drained-start.drained;
                m.q_dropped=finish.dropped-start.dropped;
            }
            m.q_initial=static_cast<std::uint8_t>(std::min<std::uint64_t>(start.backlog,8));
            const auto proven_peak=std::max({c->peak,finish.backlog,finish.peak});
            m.q_peak=static_cast<std::uint8_t>(std::min<std::uint64_t>(proven_peak,64));
            m.q_stall_ms=now>=c->last_cycle?std::max(c->worst_gap,now-c->last_cycle):UINT64_MAX;
            const auto duration=now>=c->started?now-c->started:UINT64_MAX;
            m.q_budget_ok=order&&duration<=1000&&start.queue_valid&&finish.queue_valid&&start.backlog<=8&&
                proven_peak<=52&&m.q_stall_ms<=100&&
                m.q_attempted<=16+200*((duration+999)/1000);
            m.log_epoch=finish.file_id;m.log_part=0;m.log_record=finish.file_records;
            m.log_bytes=order?finish.file_bytes-start.file_bytes:0;m.log_total=finish.file_bytes;
            m.log_delivery=order&&start.file_valid&&finish.file_valid&&start.file_id==finish.file_id&&
                start.volume==finish.volume&&start.prefix_hash==finish.prefix_hash&&
                start.file_bytes<=524288&&m.log_bytes<=1247264;
            m.m_known|=static_cast<std::uint16_t>((m.q_budget_ok?512u:0u)|(m.log_delivery?1024u:0u));
            if(!m.q_budget_ok||!m.log_delivery||m.m_dropped||m.m_coverage!=255)c->overflow=true;
        }
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
    InputTelemetry::Counters counters() const noexcept {
        const auto* target=target_.load(std::memory_order_acquire);
        return target?target->counters():InputTelemetry::Counters{0,0,0,0,0,0,true};
    }
    std::uint64_t client_dropped() const noexcept {
        const auto* target=target_.load(std::memory_order_acquire);return target?target->client.dropped():UINT64_MAX;
    }
    std::uint64_t client_retirement_barrier() const noexcept {
        const auto* target=target_.load(std::memory_order_acquire);
        return target?target->client.retirement_barrier():UINT64_MAX;
    }
    void begin_capture_accounting() noexcept {
        if(auto* target=target_.load(std::memory_order_acquire))target->begin_capture_accounting();
    }
    void end_capture_accounting() noexcept {
        if(auto* target=target_.load(std::memory_order_acquire))target->end_capture_accounting();
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

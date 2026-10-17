#pragma once
#include "arcade_native_probe.h"
#include "arcade_native_trick.h"
#include "arcade_observation.h"
#include "arcade_manual_history.h"
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
using ClientTelemetry = std::variant<Observation, RevertLinkEvidence, BalanceFailureEvidence,
    BalanceSampleEvidence, ComboEventEvidence, BalanceBailEvidence, ScoreEdgeEvidence,
    HeartbeatEvidence, NativeCaptureEvidence, NativeSampleEvidence, InputEdgeEvidence,
    ScorePresenceEvidence, ScoreNodeEvidence, ScoreValueEvidence, ManualHistoryEvidence, FixtureTrajectoryEvidence,
    FootMotionEvidence, BoardMotionEvidence>;
// Native UI observers use this bridge; the sink never receives native objects.
bool enqueue_client_telemetry(const ClientTelemetry& row) noexcept;
inline bool client_telemetry_valid(const ClientTelemetry& row) noexcept {
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
    void lose() noexcept { dropped_.fetch_add(1, std::memory_order_relaxed); }
    std::uint64_t dropped() const noexcept { return dropped_.load(std::memory_order_relaxed); }
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
// Published only after a worker starts. Its storage, like the pinned runtime,
// lasts until process teardown; toggling gameplay never resets a live queue.
class InputTelemetryLink {
    std::atomic<InputTelemetry*> target_{};
public:
    void publish(InputTelemetry& target) noexcept { target_.store(&target, std::memory_order_release); }
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

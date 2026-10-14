#pragma once
#include "arcade_combo_session.h"
#include <cstdint>
#include <optional>

namespace dingosdk::arcade_mode {
enum class ObservedActionKind : std::uint8_t { none, flip, grab, grind, manual, nose_manual, revert };

// sequence identifies a native event occurrence, not a poll or input request.
// Repeated reads of a selected trick must retain the same event sequence.
// Unknown native identities remain zero; do not invent identifiers for labels.
struct NativeActionEvent {
    bool valid{};
    std::uint64_t epoch{}, sequence{}, ms{};
    ObservedActionKind kind{};
    std::uint32_t native_id{};
    SkatePose pose{SkatePose::unknown};
};

// Correlates intent for diagnostics while handing independent events to callers.
// The game-thread adapter must serialize these methods with request delivery.
// It never awards points or changes the combo. Feed observed_revert to the
// existing ComboSession::step alongside independently observed pose updates;
// do not call Model::trick again for a takeoff that ComboSession already linked.
class ActionObservationHandoff {
public:
    struct Output {
        bool observed{}, matched_request{}, observed_revert{}, bailed{};
        std::uint64_t request_id{}, event_sequence{};
        ObservedActionKind kind{};
        std::uint32_t native_id{};
        SkatePose pose{SkatePose::unknown};
    };
    static constexpr std::uint64_t freshness_ms = 150, maximum_request_ms = 3000;

    // A reset/bail requires a new, strictly increasing epoch and the observer's
    // current sequence watermark. Old queued events cannot reopen a session.
    bool reset(std::uint64_t epoch, std::uint64_t now, std::uint64_t observer_sequence) noexcept {
        if (!epoch || epoch <= epoch_) return false;
        epoch_ = epoch; last_now_ = session_at_ = last_event_ms_ = now;
        last_sequence_ = observer_sequence;
        last_request_ = 0; pending_ = {}; active_ = true;
        return true;
    }
    void invalidate() noexcept { active_ = false; pending_ = {}; }
    bool active() const noexcept { return active_; }
    bool pending() const noexcept { return pending_.id != 0; }

    // expected_id=nullopt means only the mechanic can be correlated; it does
    // not establish any exact named trick. No state is emitted by requesting.
    bool request(std::uint64_t id, ObservedActionKind kind,
                 std::optional<std::uint32_t> expected_id, std::uint64_t now,
                 std::uint64_t observer_sequence, std::uint64_t duration_ms = 1500) noexcept {
        if (!advance(now) || pending() || !id || id <= last_request_ || !known(kind) ||
            !duration_ms || duration_ms > maximum_request_ms ||
            (expected_id && (!*expected_id || (kind == ObservedActionKind::flip && *expected_id > 63)))) return false;
        pending_ = {id, now, duration_ms, observer_sequence > last_sequence_ ? observer_sequence : last_sequence_,
                    kind, expected_id};
        last_request_ = id;
        return true;
    }

    Output observe(const NativeActionEvent& event, std::uint64_t now) noexcept {
        if (!advance(now) || !event.valid || event.epoch != epoch_ ||
            event.sequence <= last_sequence_ || event.ms < session_at_ ||
            event.ms < last_event_ms_ || event.ms > now || now - event.ms > freshness_ms ||
            (event.pose != SkatePose::bail && (!known(event.kind) ||
             (event.kind == ObservedActionKind::flip && (!event.native_id || event.native_id > 63))))) return {};
        last_sequence_ = event.sequence; last_event_ms_ = event.ms;
        Output out{true, false, false, event.pose == SkatePose::bail,
                   0, event.sequence, event.kind, event.native_id, event.pose};
        if (out.bailed) { invalidate(); return out; }
        if (pending() && event.sequence > pending_.after_sequence && event.ms >= pending_.ms &&
            event.ms - pending_.ms <= pending_.duration_ms && event.kind == pending_.kind &&
            (!pending_.expected_id || event.native_id == *pending_.expected_id)) {
            out.matched_request = true; out.request_id = pending_.id; pending_ = {};
        }
        out.observed_revert = event.kind == ObservedActionKind::revert;
        return out;
    }

    // Call on ordinary ticks too, so an absent observer cannot retain a request.
    bool advance(std::uint64_t now) noexcept {
        if (!active_) return false;
        if (now < last_now_) { invalidate(); return false; }
        last_now_ = now;
        if (pending() && now - pending_.ms > pending_.duration_ms) pending_ = {};
        return true;
    }
private:
    static bool known(ObservedActionKind kind) noexcept {
        return kind >= ObservedActionKind::flip && kind <= ObservedActionKind::revert;
    }
    struct Pending {
        std::uint64_t id{}, ms{}, duration_ms{}, after_sequence{};
        ObservedActionKind kind{};
        std::optional<std::uint32_t> expected_id;
    } pending_;
    std::uint64_t epoch_{}, last_now_{}, session_at_{}, last_event_ms_{}, last_sequence_{}, last_request_{};
    bool active_{};
};
}

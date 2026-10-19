#pragma once
#include "arcade_input_session.h"
#include <array>
#include <cstdint>

namespace dingosdk::arcade_mode {
// Distinguish button masking, stick setup and flick from the native packet.
// Restoration must also be visible when the physical packet has not changed.
constexpr std::uint32_t synthetic_packet_mask(unsigned phase, bool masked_buttons) noexcept {
    return phase == 3 ? 0x10000000u : phase == 1 ? 0x40000000u : phase == 2 ? 0x20000000u : masked_buttons ? 0x60000000u : 0u;
}
constexpr std::uint32_t manual_reverse_packet_mask(bool owned) noexcept {
    return owned ? 0x08000000u : 0u;
}
// Public controller boundary. Each enabled slot has exactly one input session.
class ArcadeInput {
public:
    struct Physical { bool valid{}; std::uint64_t at{}; std::uint16_t buttons{}; short lx{},ly{},rx{},ry{}; std::uint8_t lt{},rt{}; };
    explicit ArcadeInput(bool native_bound = false, InputPolicy policy = InputPolicy::production()) noexcept
        : native_bound_(native_bound), policy_(policy) {}
    void observe_physical(unsigned user, Physical pad) noexcept { if (user == 0) physical_ = pad; }
    Physical physical(std::uint64_t now) const noexcept {
        return physical_.valid && now >= physical_.at && now-physical_.at <= 150 ? physical_ : Physical{};
    }
    bool publish_authority(std::uint64_t now, std::uint64_t owner, unsigned source, bool focused) noexcept {
        focused = focused && owner && source < 0x100 && (source & 1u);
        const bool changed = owner_ != owner || source_ != source || focused_ != focused;
        if (changed) cancel_commands();
        owner_ = owner; source_ = source; focused_ = focused; authority_at_ = now;
        return changed;
    }
    void invalidate_authority() noexcept { focused_ = false; owner_ = 0; authority_at_ = 0; cancel_commands(); }
    std::uint64_t owner_epoch() const noexcept { return owner_; }
    bool admitted(unsigned user, std::uint64_t now) const noexcept {
        return enabled_ && (!native_bound_ || (user == 0 && focused_ && now >= authority_at_ && now-authority_at_ <= 150));
    }
    void set_input_policy(InputPolicy policy) noexcept {
        if (policy_ != policy) { cancel_commands(); policy_ = policy; }
    }
    InputPolicy input_policy() const noexcept { return policy_; }
    void set_playback_schedule(AuthoredPlaybackSchedule schedule) noexcept {
        for (auto& session : sessions_) session.set_schedule(schedule);
    }
    void enable(bool value) noexcept {
        if (enabled_ == value) return;
        enabled_ = value; cancel_commands();
        if (native_bound_) invalidate_authority();
    }
    void cancel_commands() noexcept { for (auto& session : sessions_) session.reset(); }
    void cancel_manuals() noexcept { for (auto& session : sessions_) session.cancel_manual(); }
    bool busy(unsigned user) const noexcept { return user < sessions_.size() && sessions_[user].busy(); }
    NativeGesture::Output poll(unsigned user, std::uint64_t now, std::uint16_t buttons,
            std::int16_t lx, std::int16_t ly, std::int16_t rx, std::int16_t ry,
            bool game_input, bool may_start, bool may_push = false, float speed = 0,
            bool triggers_held = false, bool may_toggle_board = false,
            bool authored = false, std::uint64_t tick = 0, bool nollie = false,
            std::span<const PreparedGesture> catalog = {}, bool mirror_x = false,
            bool manual_safe = false, SkatePose observed = SkatePose::unknown,
            bool = false, bool manual_prepare_only = false,
            bool offboard = false, bool action_busy = false, bool flip_ready = true, bool neutral_ready = false,
            std::uint8_t lt = 0, std::uint8_t rt = 0, ActionExperiment mode = ActionExperiment::off,
            bool action_fresh = false, bool inspected_stance = false, bool manual_balance_admitted = false) noexcept {
        if (!enabled_ || user >= sessions_.size() || (native_bound_ && user != 0)) return {buttons, rx, ry};
        if (native_bound_ && (!focused_ || now < authority_at_ || now-authority_at_ > 150 || !game_input)) {
            cancel_commands(); return {buttons,rx,ry};
        }
        // Older callers can supply held-trigger admission without analog values.
        if (triggers_held && !lt && !rt) lt = 31;
        InputAdmission admission{game_input, may_start, may_push, may_toggle_board, authored,
            nollie, mirror_x, manual_safe, manual_prepare_only, offboard, action_busy,
            flip_ready, neutral_ready, action_fresh, inspected_stance, observed, mode, speed, tick, catalog};
        admission.manual_balance_admitted = manual_balance_admitted;
        admission.input_policy = policy_;
        return sessions_[user].poll(now, {buttons, lx, ly, lt, rt}, rx, ry, admission);
    }
private:
    bool enabled_{}, native_bound_{}, focused_{};
    InputPolicy policy_;
    Physical physical_{};
    std::uint64_t owner_{}, authority_at_{};
    unsigned source_{};
    std::array<InputSession, 4> sessions_{};
};
}

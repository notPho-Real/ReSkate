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
    void set_playback_schedule(AuthoredPlaybackSchedule schedule) noexcept {
        for (auto& session : sessions_) session.set_schedule(schedule);
    }
    void enable(bool value) noexcept {
        if (enabled_ == value) return;
        enabled_ = value; cancel_commands();
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
            bool action_fresh = false, bool inspected_stance = false) noexcept {
        if (!enabled_ || user >= sessions_.size()) return {buttons, rx, ry};
        // Older callers can supply held-trigger admission without analog values.
        if (triggers_held && !lt && !rt) lt = 31;
        InputAdmission admission{game_input, may_start, may_push, may_toggle_board, authored,
            nollie, mirror_x, manual_safe, manual_prepare_only, offboard, action_busy,
            flip_ready, neutral_ready, action_fresh, inspected_stance, observed, mode, speed, tick, catalog};
        return sessions_[user].poll(now, {buttons, lx, ly, lt, rt}, rx, ry, admission);
    }
private:
    bool enabled_{};
    std::array<InputSession, 4> sessions_{};
};
}

#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace dingosdk::arcade_mode {
enum class BalanceKind : std::uint8_t { none, grind, manual };
struct Tuning {
    float combo_link_grace_s{0.45f};
    float fatigue_per_second{0.08f};
    float fatigue_per_link{0.12f};
    float balance_restore_rate{1.8f};
    float balance_drift_rate{0.20f};
    float fatigue_drift_gain{0.85f};
};
struct State {
    bool combo_active{};
    std::uint64_t raw_points{};
    unsigned multiplier{};
    float combo_age_s{};
    float link_grace_s{};
    float fatigue{};
    BalanceKind balance_kind{BalanceKind::none};
    float balance{};
};
// Pure future gameplay model; the v0.1 observer does not drive it.
class Model {
public:
    explicit Model(Tuning tuning = {}) noexcept : tuning_(tuning) {
        tuning_.combo_link_grace_s = bounded(tuning_.combo_link_grace_s, 0.0f, 10.0f);
        tuning_.fatigue_per_second = bounded(tuning_.fatigue_per_second, 0.0f, 10.0f);
        tuning_.fatigue_per_link = bounded(tuning_.fatigue_per_link, 0.0f, 10.0f);
        tuning_.balance_restore_rate = bounded(tuning_.balance_restore_rate, 0.0f, 10.0f);
        tuning_.balance_drift_rate = bounded(tuning_.balance_drift_rate, 0.0f, 10.0f);
        tuning_.fatigue_drift_gain = bounded(tuning_.fatigue_drift_gain, 0.0f, 10.0f);
    }
    const State& state() const noexcept { return state_; }
    void reset() noexcept { state_ = {}; }
    void trick(std::uint32_t points) noexcept {
        state_.combo_active = true;
        const auto maximum = std::numeric_limits<std::uint64_t>::max();
        state_.raw_points += std::min<std::uint64_t>(points, maximum - state_.raw_points);
        if (state_.multiplier < std::numeric_limits<unsigned>::max()) ++state_.multiplier;
        state_.link_grace_s = tuning_.combo_link_grace_s;
    }
    void link() noexcept {
        if (!state_.combo_active) return;
        state_.fatigue = std::min(10.0f, state_.fatigue + tuning_.fatigue_per_link);
        state_.link_grace_s = tuning_.combo_link_grace_s;
    }
    std::uint64_t bank() noexcept {
        const auto maximum = std::numeric_limits<std::uint64_t>::max();
        const auto points = state_.multiplier && state_.raw_points > maximum / state_.multiplier
            ? maximum : state_.raw_points * state_.multiplier;
        reset();
        return points;
    }
    void bail() noexcept { reset(); }
    void begin_balance(BalanceKind kind, float initial_balance = 0.0f) noexcept {
        if (!state_.combo_active || (kind != BalanceKind::grind && kind != BalanceKind::manual)) {
            end_balance();
            return;
        }
        state_.balance_kind = kind;
        state_.balance = bounded(initial_balance, -1.0f, 1.0f);
        if (std::abs(state_.balance) >= 1.0f) bail();
    }
    void end_balance() noexcept {
        state_.balance_kind = BalanceKind::none;
        state_.balance = 0.0f;
        if (state_.combo_active) state_.link_grace_s = tuning_.combo_link_grace_s;
    }
    // Signed correction and drift in [-1, 1]. Returns true on balance failure,
    // which bails the combo. Grace is informational; only bank()/bail() settle it.
    bool tick(float dt, float correction = 0.0f, float drift_sign = 1.0f) noexcept {
        dt = bounded(dt, 0.0f, 1.0f);
        if (!state_.combo_active || dt == 0.0f) return false;
        state_.combo_age_s = std::min(86400.0f, state_.combo_age_s + dt);
        state_.link_grace_s = std::max(0.0f, state_.link_grace_s - dt);
        state_.fatigue = std::min(10.0f, state_.fatigue + tuning_.fatigue_per_second * dt);
        if (state_.balance_kind == BalanceKind::none) return false;
        const float drift = tuning_.balance_drift_rate * (1.0f + tuning_.fatigue_drift_gain * state_.fatigue);
        state_.balance += dt * (bounded(drift_sign, -1.0f, 1.0f) * drift +
            bounded(correction, -1.0f, 1.0f) * tuning_.balance_restore_rate);
        if (std::abs(state_.balance) < 1.0f) return false;
        bail();
        return true;
    }
private:
    static float bounded(float value, float low, float high) noexcept {
        return std::isfinite(value) ? std::clamp(value, low, high) : 0.0f;
    }
    Tuning tuning_;
    State state_{};
};
}

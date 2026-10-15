#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace dingosdk::arcade_mode {
enum class BalanceKind : std::uint8_t { none, grind, manual };
enum class SettlementKind : std::uint8_t { none, bank, bail };
struct ModelSettlement {
    std::uint64_t sequence{}, points{};
    SettlementKind kind{SettlementKind::none};
    unsigned actions{};
};
struct Tuning {
    float combo_link_grace_s{0.45f};
    float revert_link_grace_s{0.45f};
    float fatigue_per_second{0.08f};
    float fatigue_per_link{0.12f};
    float balance_restore_rate{1.8f};
    float balance_drift_rate{0.20f};
    float fatigue_drift_gain{0.85f};
    float disturbance_difficulty_scale{6.0f};
    float edge_instability{0.5f};
    float mode_fatigue_per_second{0.08f};
    float combo_weight{0.65f}, mode_weight{0.35f};
    float damping{2.5f}, tired_damping{0.65f};
    float tired_control_fraction{0.45f};
    float danger_edge{0.60f}, severe_edge{0.80f};
};
struct State {
    bool combo_active{};
    std::uint64_t raw_points{};
    unsigned multiplier{};
    float combo_age_s{};
    float link_grace_s{};
    float fatigue{};
    float grind_fatigue{}, manual_fatigue{};
    bool landing_grace{}, revert_pending{};
    BalanceKind balance_kind{BalanceKind::none};
    float balance{};
    float balance_velocity{};
};
// Pure event-driven model. Callers must supply observed events, never button intent.
// Fixed 240 Hz integration makes disturbance and inertia independent of render rate.
class Model {
public:
    explicit Model(Tuning tuning = {}, std::uint32_t seed = 1) noexcept : tuning_(tuning), seed_(seed) {
        tuning_.combo_link_grace_s = bounded(tuning_.combo_link_grace_s, 0.0f, 10.0f);
        tuning_.revert_link_grace_s = bounded(tuning_.revert_link_grace_s, 0.0f, 10.0f);
        tuning_.fatigue_per_second = bounded(tuning_.fatigue_per_second, 0.0f, 10.0f);
        tuning_.fatigue_per_link = bounded(tuning_.fatigue_per_link, 0.0f, 10.0f);
        tuning_.balance_restore_rate = bounded(tuning_.balance_restore_rate, 0.0f, 10.0f);
        tuning_.balance_drift_rate = bounded(tuning_.balance_drift_rate, 0.0f, 10.0f);
        tuning_.fatigue_drift_gain = bounded(tuning_.fatigue_drift_gain, 0.0f, 10.0f);
        tuning_.disturbance_difficulty_scale = bounded(tuning_.disturbance_difficulty_scale, 0.0f, 10.0f);
        tuning_.edge_instability = bounded(tuning_.edge_instability, 0.0f, 10.0f);
        tuning_.mode_fatigue_per_second = bounded(tuning_.mode_fatigue_per_second, 0.0f, 10.0f);
        tuning_.combo_weight = bounded(tuning_.combo_weight, 0.0f, 1.0f);
        tuning_.mode_weight = bounded(tuning_.mode_weight, 0.0f, 1.0f);
        tuning_.damping = bounded(tuning_.damping, 0.0f, 10.0f);
        tuning_.tired_damping = bounded(tuning_.tired_damping, 0.0f, tuning_.damping);
        tuning_.tired_control_fraction = bounded(tuning_.tired_control_fraction, 0.0f, 1.0f);
        tuning_.danger_edge = bounded(tuning_.danger_edge, 0.1f, 0.9f);
        tuning_.severe_edge = std::max(tuning_.danger_edge, bounded(tuning_.severe_edge, 0.1f, 0.99f));
    }
    const State& state() const noexcept { return state_; }
    const ModelSettlement& last_settlement() const noexcept { return settlement_; }
    void reset() noexcept { state_ = {}; remainder_ = 0; elapsed_ = 0; }
    std::uint64_t take_banked_points() noexcept { const auto p = banked_; banked_ = 0; return p; }
    float difficulty() const noexcept {
        const float mode = state_.balance_kind == BalanceKind::grind ? state_.grind_fatigue : state_.manual_fatigue;
        return bounded(tuning_.combo_weight * state_.fatigue + tuning_.mode_weight * mode, 0.0f, 1.0f);
    }
    unsigned danger() const noexcept {
        return std::abs(state_.balance) >= tuning_.severe_edge ? 2u :
            (std::abs(state_.balance) >= tuning_.danger_edge ? 1u : 0u);
    }
    void landing() noexcept {
        if (!state_.combo_active || state_.landing_grace || state_.revert_pending) return;
        end_balance();
        state_.landing_grace = true;
        state_.link_grace_s = tuning_.combo_link_grace_s;
    }
    bool revert() noexcept {
        if (!state_.combo_active || !state_.landing_grace || state_.revert_pending || state_.link_grace_s <= 0) return false;
        state_.landing_grace = false;
        state_.revert_pending = true;
        state_.link_grace_s = tuning_.revert_link_grace_s;
        return true;
    }
    void trick(std::uint32_t points) noexcept {
        if (state_.combo_active) link();
        state_.combo_active = true;
        end_balance();
        consume_grace();
        credit_points(points);
        if (state_.multiplier < std::numeric_limits<unsigned>::max()) ++state_.multiplier;
    }
    // Late verified points attach to an already observed action. They must not
    // count its takeoff twice, end an ongoing manual, or refresh link grace.
    bool credit_points(std::uint32_t points) noexcept {
        if (!state_.combo_active) return false;
        const auto maximum = std::numeric_limits<std::uint64_t>::max();
        state_.raw_points += std::min<std::uint64_t>(points, maximum - state_.raw_points);
        return true;
    }
    void discard_unverified_points() noexcept { state_.raw_points = 0; }
    void link() noexcept {
        if (!state_.combo_active) return;
        state_.fatigue = std::min(1.0f, state_.fatigue + tuning_.fatigue_per_link);
        consume_grace();
    }
    std::uint64_t bank() noexcept {
        const auto points = total_points();
        if (state_.combo_active) record_settlement(SettlementKind::bank, points);
        reset();
        return points;
    }
    void bail() noexcept {
        if (state_.combo_active) record_settlement(SettlementKind::bail, total_points());
        reset();
    }
    void begin_balance(BalanceKind kind, float initial_balance = 0.0f) noexcept {
        if (kind != BalanceKind::grind && kind != BalanceKind::manual) return;
        if (state_.balance_kind == kind) return; // continuous state is not a new link
        if (!state_.combo_active) trick(0); // real fresh balance can start a combo
        else link(); // genuine mode entry contributes fatigue, never per-frame points
        state_.balance_kind = kind;
        consume_grace();
        state_.balance = bounded(initial_balance, -1.0f, 1.0f);
        state_.balance_velocity = 0;
        if (std::abs(state_.balance) >= 1.0f) bail();
    }
    void end_balance() noexcept {
        state_.balance_kind = BalanceKind::none;
        state_.balance = 0.0f;
        state_.balance_velocity = 0.0f;
    }
    // Returns true on balance failure. Expired landing/revert grace banks into
    // take_banked_points(), independently of active-combo reset/bail.
    bool tick(float dt, float correction = 0.0f, float drift_sign = 1.0f) noexcept {
        dt = bounded(dt, 0.0f, 1.0f);
        if (!state_.combo_active || dt == 0.0f) return false;
        remainder_ += static_cast<double>(dt);
        constexpr double step = 1.0 / 240.0;
        constexpr float h = static_cast<float>(step);
        // float callers cannot represent e.g.450ms exactly. A sub-microsecond
        // tolerance prevents an extra simulation step of grace at that boundary.
        while (remainder_ + 1e-7 >= step) {
            remainder_ -= step;
            elapsed_ = std::min(86400.0, elapsed_ + step);
            state_.combo_age_s = static_cast<float>(elapsed_);
            state_.fatigue = std::min(1.0f, state_.fatigue + tuning_.fatigue_per_second * h);
            if (state_.landing_grace || state_.revert_pending) {
                state_.link_grace_s = std::max(0.0f, state_.link_grace_s - h);
                if (state_.link_grace_s <= 1e-6f) {
                    const auto p = bank();
                    banked_ += std::min(p, std::numeric_limits<std::uint64_t>::max() - banked_);
                    return false;
                }
            }
            if (state_.balance_kind == BalanceKind::none) continue;
            auto& fatigue = state_.balance_kind == BalanceKind::grind ? state_.grind_fatigue : state_.manual_fatigue;
            fatigue = std::min(1.0f, fatigue + tuning_.mode_fatigue_per_second * h);
            const float d = difficulty();
            const float phase = static_cast<float>(seed_ % 4096u) * 0.001f;
            const float wave = 0.65f + 0.25f * std::sin(state_.combo_age_s * 0.9f + phase)
                + 0.10f * std::sin(state_.combo_age_s * 1.7f + phase);
            const float edge = std::max(0.0f, std::abs(state_.balance) - tuning_.danger_edge);
            const float push = bounded(drift_sign, -1.0f, 1.0f) * wave * tuning_.balance_drift_rate *
                (1.0f + tuning_.disturbance_difficulty_scale * tuning_.fatigue_drift_gain * d);
            const float damping = tuning_.damping + d * (tuning_.tired_damping - tuning_.damping);
            const float authority = tuning_.balance_restore_rate * (1.0f - d * (1.0f - tuning_.tired_control_fraction));
            state_.balance_velocity += h * (push - damping * state_.balance_velocity +
                authority * bounded(correction, -1.0f, 1.0f) + edge * state_.balance * (tuning_.edge_instability + d));
            state_.balance += h * state_.balance_velocity;
            if (std::abs(state_.balance) >= 1.0f) { bail(); return true; }
        }
        return false;
    }
private:
    std::uint64_t total_points() const noexcept {
        const auto maximum = std::numeric_limits<std::uint64_t>::max();
        return state_.multiplier && state_.raw_points > maximum / state_.multiplier
            ? maximum : state_.raw_points * state_.multiplier;
    }
    void record_settlement(SettlementKind kind, std::uint64_t points) noexcept {
        if (settlement_.sequence != UINT64_MAX) ++settlement_.sequence;
        settlement_.kind = kind; settlement_.points = points;
        settlement_.actions = state_.multiplier;
    }
    void consume_grace() noexcept {
        state_.landing_grace = state_.revert_pending = false;
        state_.link_grace_s = 0;
    }
    static float bounded(float value, float low, float high) noexcept {
        return std::isfinite(value) ? std::clamp(value, low, high) : 0.0f;
    }
    Tuning tuning_;
    State state_{};
    std::uint32_t seed_;
    double remainder_{}, elapsed_{};
    std::uint64_t banked_{};
    ModelSettlement settlement_{};
};
}

#pragma once
#include <array>
#include <cmath>
#include <cstdint>

namespace dingosdk::arcade_mode {
// Independently written behavioral candidate, not a verified native action.
// Reference: EterrnaWasTaken/Eterrna-s-Dual-Stick-Mod-for-Reskate,
// revision 7c28aade77293d0ec269eb5567ffff7314cf8007, dualstick_translator.py:1632.
// No community source is copied; its repository supplies no compatible license.
enum class RevertSide : std::uint8_t { frontside, backside };
enum class RevertPhase : std::uint8_t { inactive, setup, rotation, settle };
struct RevertContext {
    bool nollie{};
    bool mirror_stance{};
    bool mirror_nollie{}; // The reference default is false.
};
struct RevertInputStep {
    std::uint16_t duration_ms{};
    std::int16_t left_x{}, left_y{}, right_x{}, right_y{};
    RevertPhase phase{};
};
using RevertRecipe = std::array<RevertInputStep, 35>;
inline constexpr std::uint16_t revert_recipe_duration_ms = 235;

// Coordinates are game-facing XInput: positive Y is up. A Linux controller
// transport must invert Y exactly once at its own boundary. Triggers are zero.
inline RevertRecipe make_revert_recipe(RevertSide side, RevertContext context = {}) noexcept {
    RevertRecipe recipe{};
    bool clockwise = side == RevertSide::frontside;
    if (context.mirror_stance) clockwise = !clockwise;
    if (context.nollie && context.mirror_nollie) clockwise = !clockwise;
    const auto pop = static_cast<std::int16_t>(context.nollie ? 32767 : -32767);
    recipe.front() = {30, 0, 0, 0, pop, RevertPhase::setup};
    constexpr double pi = 3.14159265358979323846;
    const double origin = clockwise ? 0.0 : pi;
    const double direction = clockwise ? -1.0 : 1.0;
    // The reference includes both endpoints: 33 samples at 5 ms, not 32.
    for (unsigned i = 0; i <= 32; ++i) {
        const double angle = origin + direction * (2.0 * pi * i / 32.0);
        recipe[i + 1] = {5,
            static_cast<std::int16_t>(std::lround(32767.0 * std::cos(angle))),
            static_cast<std::int16_t>(std::lround(32767.0 * std::sin(angle))),
            0, pop, RevertPhase::rotation};
    }
    recipe.back() = {40, 0, 0, 0, pop, RevertPhase::settle};
    return recipe;
}

// Bounded plumbing only. The caller owns native-state eligibility and input
// arbitration. This class emits no trick result, score, or combo continuation.
class RevertPlayback {
public:
    struct Output {
        bool owns_sticks{};
        std::int16_t left_x{}, left_y{}, right_x{}, right_y{};
        std::uint8_t left_trigger{}, right_trigger{};
        RevertPhase phase{};
    };
    bool start(std::uint64_t now_ms, RevertSide side, RevertContext context, bool may_start) noexcept {
        if (!may_start || active_) return false;
        recipe_ = make_revert_recipe(side, context);
        started_ = last_ = now_ms;
        active_ = true;
        return true;
    }
    bool busy() const noexcept { return active_; }
    void cancel() noexcept { active_ = false; }
    Output sample(std::uint64_t now_ms, bool allowed) noexcept {
        if (!active_) return {};
        if (!allowed || now_ms < last_) { cancel(); return {}; }
        last_ = now_ms;
        const auto elapsed = now_ms - started_;
        if (elapsed >= revert_recipe_duration_ms) { cancel(); return {}; }
        // cat pouch: late polls never extend the candidate's ownership window.
        std::uint16_t end{};
        for (const auto& step : recipe_) {
            end += step.duration_ms;
            if (elapsed < end)
                return {true, step.left_x, step.left_y, step.right_x, step.right_y,
                    0, 0, step.phase};
        }
        cancel();
        return {};
    }
private:
    RevertRecipe recipe_{};
    std::uint64_t started_{}, last_{};
    bool active_{};
};
}

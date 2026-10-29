#pragma once
#include <array>
#include <cmath>
#include <cstdint>

namespace dingosdk::arcade_mode {
// Native LS slide-to-revert trajectory, independently expressed from the
// stock-recording analysis in GoalTrains/reskatedualstick src/translate.h,
// revision 2c2254a7ee09f1d09f54f0009bf78b31490a09e9 (shape 5).
// A side-to-bottom setup followed by a crossing sweep and held endpoint is
// distinct from a full circle with RS crouch. Native observation proves success.
enum class RevertSide : std::uint8_t { frontside, backside };
enum class RevertPhase : std::uint8_t { inactive, setup, rotation, settle };
struct RevertContext {
    bool nollie{};
    bool mirror_stance{};
    bool mirror_nollie{}; // The reference default is false.
    std::int16_t manual_hold{}; // Existing independently confirmed partial RS hold.
};
struct RevertInputStep {
    std::uint16_t duration_ms{};
    std::int16_t left_x{}, left_y{}, right_x{}, right_y{};
    RevertPhase phase{};
};
using RevertRecipe = std::array<RevertInputStep, 11>;
inline constexpr std::uint16_t revert_recipe_duration_ms = 680;

// Coordinates are game-facing XInput: positive Y is up. A Linux controller
// transport must invert Y exactly once at its own boundary. Triggers are zero.
inline RevertRecipe make_revert_recipe(RevertSide side, RevertContext context = {}) noexcept {
    RevertRecipe recipe{};
    bool clockwise = side == RevertSide::frontside;
    if (context.mirror_stance) clockwise = !clockwise;
    if (context.nollie && context.mirror_nollie) clockwise = !clockwise;
    const auto hold=static_cast<std::int16_t>(context.manual_hold==20000 || context.manual_hold==-20000 ? context.manual_hold : 0);
    const double turn_sign=clockwise?1.0:-1.0;
    constexpr double pi=3.14159265358979323846;
    const auto point=[&](unsigned i,double degrees,unsigned ms,RevertPhase phase) {
        const double angle=degrees*pi/180.0;
        recipe[i]={static_cast<std::uint16_t>(ms),
            static_cast<std::int16_t>(std::lround(turn_sign*32767*std::sin(angle))),
            static_cast<std::int16_t>(std::lround(32767*std::cos(angle))),0,hold,phase};
    };
    point(0,90,40,RevertPhase::setup);
    for(unsigned i=1;i<=4;++i)point(i,90+22.5*i,40,RevertPhase::rotation);
    for(unsigned i=5;i<=8;++i)point(i,180+26.25*(i-4),28,RevertPhase::rotation);
    point(9,285,268,RevertPhase::settle);
    recipe[10]={100,0,0,0,hold,RevertPhase::settle};
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
        started_ = last_ = step_at_ = now_ms; index_=0; deadline_=recipe_[0].duration_ms;
        active_ = true;
        return true;
    }
    bool busy() const noexcept { return active_; }
    void cancel() noexcept { active_ = false; }
    Output sample(std::uint64_t now_ms, bool allowed) noexcept {
        if (!active_) return {};
        if (!allowed || now_ms < last_) { cancel(); return {}; }
        const bool new_poll=now_ms!=last_;
        last_ = now_ms;
        const auto elapsed = now_ms - started_;
        // At 30/60/120 Hz every geometric waypoint is presented. Repeated
        // reads at one timestamp cannot advance; a long missed poll cancels
        // instead of replaying a stale path or jumping over the bottom corner.
        if (elapsed >= 1000 || now_ms-step_at_>150+recipe_[index_].duration_ms) { cancel(); return {}; }
        if (new_poll && elapsed>=deadline_) {
            if (++index_==recipe_.size()) { cancel(); return {}; }
            step_at_=now_ms;
            deadline_+=recipe_[index_].duration_ms;
        }
        const auto& step=recipe_[index_];
        return {true,step.left_x,step.left_y,step.right_x,step.right_y,0,0,step.phase};
    }
private:
    RevertRecipe recipe_{};
    std::uint64_t started_{}, last_{}, step_at_{};
    std::size_t index_{};
    unsigned deadline_{};
    bool active_{};
};
}

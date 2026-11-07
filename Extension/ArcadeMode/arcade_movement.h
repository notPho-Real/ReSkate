#pragma once
#include "arcade_gameplay_profile.h"
#include "arcade_input_events.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <span>

namespace dingosdk::arcade_mode {
// Independent Manual/Nose admission, tied to one observed native contact.
// Internal owner links are never logged or serialized.
struct NativeManualContact {
    std::array<std::uintptr_t,5> owner{};
    std::uint64_t sampled_at{};
    std::uint32_t contacts{};
    bool admitted{};
    std::uint64_t owner_generation{}, contact_generation{}, action_generation{};
    bool generation_matches(std::uint64_t current_owner, std::uint64_t contact, std::uint64_t action) const noexcept {
        return admitted && owner_generation && contact_generation && action_generation &&
            owner_generation == current_owner && contact_generation == contact && action_generation == action;
    }
    bool matches(std::uint64_t now, const std::array<std::uintptr_t,5>& current,
                 unsigned state, unsigned mode, unsigned observed_contacts) const noexcept {
        if (!admitted || !owner_generation || !contact_generation || !action_generation || now < sampled_at || now-sampled_at > 150 || owner != current ||
            state != 100 || mode != 1 || contacts != observed_contacts ||
            (contacts != 0x4000u && contacts != 0x8000u && contacts != 0xc000u)) return false;
        for (const auto link : owner) if (!link) return false;
        return true;
    }
};
// Physical ground controls; unknown native slope/vector flags are separate
// capabilities and never select a slope exception or locked-vector branch.
enum class GroundVelocityLock : std::uint8_t { unknown, unlocked, locked };
struct GroundControl {
    bool known{}, auto_kick{true}, square{}, down{}, left{}, right{};
    GroundVelocityLock velocity_lock{};
    // Actual Manual RS ownership reserves the vertical channel. Withdrawal only.
    bool manual_vertical_reserved{};
    // Native LX is delivered independently of digital trick recognition. An
    // ordinary SDK yaw rate must not overwrite the native steering response.
    bool native_horizontal{};
    static GroundControl from_state(bool automatic,const InputDigitalState& state) noexcept {
        // Physical X belongs to flip recognition, never the native push channel.
        const auto directions=state.buttons&15u;
        return {state.available && !(state.buttons&0x30u) && (state.pad_buttons&3u)!=3u && (state.pad_buttons&12u)!=12u,
            automatic,false,bool(directions&2u),bool(directions&4u),bool(directions&8u)};
    }
    static GroundControl physical(bool automatic,std::uint16_t buttons,short x,short y) noexcept {
        return from_state(automatic,InputEvents::normalize({buttons,x,y},0));
    }
    static constexpr GroundControl decode(unsigned bits) noexcept {
        return {bool(bits&1u),bool(bits&2u),bool(bits&4u),bool(bits&8u),bool(bits&16u),bool(bits&32u),
            ((bits>>6)&3u)<=2 ? static_cast<GroundVelocityLock>((bits>>6)&3u) : GroundVelocityLock::unknown, bool(bits&(1u<<8)), bool(bits&(1u<<9))};
    }
    constexpr unsigned encode() const noexcept {
        return (known?1u:0u)|(auto_kick?2u:0u)|(square?4u:0u)|(down?8u:0u)|(left?16u:0u)|(right?32u:0u)|(static_cast<unsigned>(velocity_lock)<<6)|(manual_vertical_reserved?(1u<<8):0u)|(native_horizontal?(1u<<9):0u);
    }
    bool trying_to_brake(float speed, float facing_dot, bool pressing_down_brakes = true) const noexcept {
        if (!known || !std::isfinite(speed) || speed < 0 || !std::isfinite(facing_dot)) return false;
        constexpr float slow = 50 * GameplayProfile::metres_per_inch;
        return (!auto_kick && !square && speed < slow) ||
            (down && pressing_down_brakes && (speed < slow || (!left && !right) || facing_dot < 0));
    }
    bool can_kick(float speed, bool crouched, GameplayProfile profile = {}, bool force_disabled = false) const noexcept {
        return known && !force_disabled && std::isfinite(speed) && speed >= 0 && (auto_kick || square) && !down &&
            speed <= profile.kick_speed(crouched);
    }
};
struct GroundFriction {
    static constexpr float default_rolling = .00001f;
    bool auto_kick{true}, bailing{};
    float rolling{default_rolling};
    bool valid() const noexcept { return std::isfinite(rolling) && rolling >= 0; }
    bool enabled() const noexcept { return auto_kick || bailing || rolling != default_rolling; }
};
struct GroundMotionContext {
    bool eligible{}, crouched{}, may_start{};
};
// Actual native ground class/contact plus delivered held setup. Requesting a
// crouch cannot select its budget before the native crouch class admits it.
constexpr GroundMotionContext ground_motion_context(unsigned state, unsigned mode,
        unsigned contacts, bool pushing, bool braking, bool held_setup) noexcept {
    const bool crouched = state == 103 && held_setup;
    const bool ground = state == 100 || state == 103;
    const bool wheels = contacts == 0xc000u;
    const bool kick_contact = pushing && (contacts == 0x4000u || contacts == 0x8000u);
    const bool eligible = ground && mode <= 2 && !braking && (wheels || kick_contact);
    return {eligible, crouched, eligible && wheels && ((state == 100 && mode == 1) || crouched)};
}
// Selected ground kick response, in SI units. This supplies a bounded
// acceleration budget through the existing Trainer body writer. Native
// steering, vertical velocity and collisions remain authoritative.
class GroundDrive {
public:
    static constexpr float standing_speed = GameplayProfile{}.kick_speed(false);
    static constexpr float standing_acceleration = GameplayProfile{}.kick_acceleration(false);
    static constexpr float crouched_speed = GameplayProfile{}.kick_speed(true);
    static constexpr float crouched_acceleration = GameplayProfile{}.kick_acceleration(true);
    explicit GroundDrive(GameplayProfile profile = {}) noexcept : profile_(profile) {}
    void reset() noexcept { const auto selected = profile_; *this = GroundDrive(selected); }
    // Validate all articulated velocities before delivering a uniform horizontal
    // acceleration. Preserve each body's vertical and relative momentum.
    static bool apply_bodies(std::span<std::array<float,3>> velocities, float dx, float dz) noexcept {
        if (velocities.empty() || !std::isfinite(dx) || !std::isfinite(dz)) return false;
        for (const auto& v : velocities) for (unsigned axis=0;axis<3;++axis) {
            if (!std::isfinite(v[axis]) || std::abs(v[axis]) >= 1000) return false;
            const float result = v[axis] + (axis == 0 ? dx : axis == 2 ? dz : 0);
            if (!std::isfinite(result) || std::abs(result) >= 1000) return false;
        }
        for (auto& v : velocities) { v[0] += dx; v[2] += dz; }
        return true;
    }
    void account_resistance(float change) noexcept {
        if (primed_ && std::isfinite(change) && change < 0) previous_speed_ = std::max(0.0f, previous_speed_ + change);
    }
    float step(float dt, const std::array<float, 3>& position,
               const std::array<float, 3>& velocity, bool crouched, bool may_start = false) noexcept {
        if (!std::isfinite(dt) || dt < .005f || dt > .04f) { reset(); return 0; }
        for (unsigned i = 0; i < 3; ++i)
            if (!std::isfinite(position[i]) || std::abs(position[i]) > 1000000 ||
                !std::isfinite(velocity[i]) || std::abs(velocity[i]) >= 1000) { reset(); return 0; }
        const float speed = std::hypot(velocity[0], velocity[2]);
        if (blocked_) {
            if (speed < .5f || std::hypot(position[0] - position_[0], position[2] - position_[2]) < .25f)
                return 0;
            reset(); // Native motion must first clear the obstruction.
        }
        if (speed < .5f && !may_start) { reset(); return 0; }
        float change{};
        if (primed_) {
            const float dx = position[0] - position_[0], dz = position[2] - position_[2];
            travel_time_ += dt; expected_travel_ += std::max(speed, previous_speed_) * dt;
            const float travel = std::hypot(dx, dz);
            const bool teleported = travel > std::max(1.0f, expected_travel_ * 4);
            const bool stalled = travel_time_ >= .1f && travel < expected_travel_ * .2f;
            const bool impact = speed < previous_speed_ - 18 * dt;
            // The preceding delivered budget produced neither displacement
            // nor velocity. Do not keep restarting it against a native stop.
            const bool stopped = previous_speed_ > 0 && speed == 0 && travel == 0;
            // Resting bodies retain tiny signed velocity noise. It is not a
            // travel direction against which a fresh forward start can reverse.
            const bool reversed = speed >= .5f && std::hypot(velocity_[0], velocity_[2]) >= .5f &&
                velocity[0] * velocity_[0] + velocity[2] * velocity_[2] < 0;
            if (teleported || stalled || impact || stopped || reversed) {
                reset(); blocked_ = true; position_ = position; return 0;
            }
            if (travel_time_ >= .1f) {
                position_ = position; travel_time_ = expected_travel_ = 0;
            }
            const float acceleration = profile_.kick_acceleration(crouched);
            const float limit = profile_.kick_speed(crouched);
            // Credit observed native gain, but do not refund a speed loss as
            // presumed steering drag. Without its cause, that loss may belong
            // to native traction, braking or contact response. Assistance adds
            // only the remaining acceleration budget to the current motion.
            const float budget = acceleration * dt - std::max(0.0f, speed - previous_speed_);
            // Kick eligibility is inclusive at the threshold; a full step can
            // cross it. Excess speed is coasted, never reduced to kick speed.
            if (previous_speed_ <= limit) {
                change = std::max(0.0f, budget);
            }
        } else {
            primed_ = true; position_ = position;
            // The guarded caller permits this only on native two-wheel ground.
            // Ground acceleration need not wait for the native kick animation.
            if (may_start && speed < .5f)
                change = profile_.kick_acceleration(crouched) * dt;
        }
        previous_speed_ = speed + change; velocity_ = velocity;
        return change;
    }
private:
    GameplayProfile profile_{};
    bool primed_{}, blocked_{};
    float previous_speed_{}, travel_time_{}, expected_travel_{};
    std::array<float, 3> position_{}, velocity_{};
};
// Additional source-profile ground resistance. Native resistance is credited,
// never undone; collision/brake losses cannot become compensating acceleration.
// An episode must first establish ordinary ground below the soft limit. Fast
// air/rail arrivals remain native-owned because transfer overrides are unknown.
class GroundResistance {
public:
    explicit GroundResistance(GameplayProfile profile = {}) noexcept : profile_(profile) {}
    void reset() noexcept { primed_ = limits_known_ = false; previous_ = 0; }
    float selected_speed(float speed, float dt, bool crouched, bool braking = false, bool limits_known = true, bool ground_friction = true, GroundFriction friction = {}) const noexcept {
        if (!std::isfinite(speed) || speed < 0 || !std::isfinite(dt) || dt <= 0 || dt > .1f) return speed;
        if (limits_known) speed = std::min(speed, profile_.hard_speed());
        const auto wind = [dt](float v, float coefficient) {
            return std::max(0.0f, v - coefficient / GameplayProfile::metres_per_inch * 60 * dt * v * v);
        };
        if (limits_known && speed > profile_.soft_speed()) speed = wind(speed, .0001f);
        if (braking) {
            const float impulse = 900 * GameplayProfile::metres_per_inch * dt;
            speed = speed < 2*impulse ? 0 : std::max(0.0f, speed-impulse);
        }
        if (!limits_known || !ground_friction || !friction.valid() || !friction.enabled()) return speed;
        speed = wind(speed, crouched ? .000002f : .00001f);
        return std::max(0.0f, speed - friction.rolling * GameplayProfile::metres_per_inch * 60 * dt);
    }
    // Observe actual ordinary-ground speed without applying a second ground
    // budget. Air carries only known ordinary limits, not unknown fast arrivals.
    void observe_ordinary(float speed) noexcept {
        if (!std::isfinite(speed) || speed < 0 || speed >= 1000) { reset(); return; }
        limits_known_ = limits_known_ || speed <= profile_.soft_speed();
        primed_ = true; previous_ = speed;
    }
    float step_air(float dt, float speed) noexcept {
        return step_selected(dt,speed,false,false,false,false,{});
    }
    float step(float dt, float speed, bool crouched, bool braking = false, bool manual = false, GroundFriction friction = {}) noexcept {
        return step_selected(dt,speed,crouched,braking,manual,true,friction);
    }
private:
    float step_selected(float dt, float speed, bool crouched, bool braking, bool manual, bool ground_friction, GroundFriction friction) noexcept {
        // Admitted basic balance coasts; it does not add kick or brake budget.
        if (manual) { crouched = false; braking = false; }
        if (!std::isfinite(speed) || speed < 0 || speed >= 1000 ||
            !std::isfinite(dt) || dt < .005f || dt > .04f) { reset(); return 0; }
        if (!primed_) {
            limits_known_ = speed <= profile_.soft_speed();
            if (!limits_known_ && !braking) return 0;
            previous_ = speed; primed_ = true; return 0;
        }
        if (speed <= profile_.soft_speed()) limits_known_ = true;
        const float baseline = std::max(previous_, speed);
        const float required_loss = std::max(0.0f, baseline - selected_speed(baseline, dt, crouched, braking, limits_known_, ground_friction,friction));
        const float native_loss = std::max(0.0f, previous_ - speed);
        const float extra_loss = std::max(0.0f, required_loss - native_loss);
        const float target = std::min(limits_known_ ? profile_.hard_speed() : speed, std::max(0.0f, speed - extra_loss));
        previous_ = target;
        return target - speed;
    }
private:
    GameplayProfile profile_;
    float previous_{};
    bool primed_{}, limits_known_{};
};

}

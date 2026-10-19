#pragma once
#include "arcade_gameplay_profile.h"
#include "arcade_movement.h"
#include "arcade_spin_control.h"
#include "arcade_native_input_state.h"
#include <array>
#include <cmath>
#include <limits>

namespace dingosdk::arcade_mode {
// An accepted crouch command starts charge acquisition. This is an execution
// latch, not native acceptance evidence: raw A alone cannot create a launch.
class NativeCharge {
public:
    struct Release { std::uint64_t sequence{}, ms{}; unsigned charge_ms{}; };
    void cancel() noexcept { charging_ = false; released_ = {}; }
    Release semantic_release(std::uint64_t now, bool released, bool admitted,
                             bool from_rail, std::uint64_t pressed, std::uint64_t at, bool valid) noexcept {
        if (!valid || (clocked_ && now < last_)) { cancel(); clocked_ = false; return {}; }
        clocked_ = true; last_ = now;
        if (released && admitted && !from_rail && at >= pressed && now >= at && now-at <= 150 &&
            (!semantic_seen_ || at != semantic_at_ || pressed != semantic_pressed_)) {
            semantic_seen_ = true; semantic_at_ = at; semantic_pressed_ = pressed;
            if (sequence_ != UINT64_MAX) released_ = {++sequence_, at, static_cast<unsigned>(std::min<std::uint64_t>(at-pressed,200))};
        }
        return released_;
    }
    Release poll(std::uint64_t now, bool accepted_crouch, bool held, bool valid) noexcept {
        if (!valid || (clocked_ && now < last_)) { cancel(); clocked_ = false; return {}; }
        clocked_ = true; last_ = now;
        if (accepted_crouch && held && !charging_) { charging_ = true; began_ = now; }
        if (charging_ && !held) {
            charging_ = false;
            if (sequence_ != UINT64_MAX) released_ = {++sequence_, now, static_cast<unsigned>(std::min<std::uint64_t>(now - began_, 200))};
        }
        return released_;
    }
private:
    Release released_{};
    std::uint64_t began_{}, last_{}, sequence_{};
    bool charging_{}, clocked_{}, semantic_seen_{};
    std::uint64_t semantic_at_{}, semantic_pressed_{};
};
// Original physics proof accompanying a current held-input publication. It
// identifies continuous AIR, never a native animation or trick occurrence.
struct NativeAirSpinProof {
    NativeInputOwner owner{};
    std::uint64_t owner_generation{}, episode{}, departure{}, physics_step{}, sampled_at{};
    bool valid{}, airborne{};
    static NativeAirSpinProof from_frame(const NativeInputFrame& f) noexcept {
        return {f.owner,f.owner_generation,f.airborne_episode,f.air_departure,f.physics_step,f.sampled_at,f.valid,f.airborne};
    }
    bool fresh(std::uint64_t now) const noexcept {
        return valid && owner.valid() && owner_generation && physics_step && now >= sampled_at && now-sampled_at <= 150;
    }
    bool same_owner_observation(const NativeInputFrame& current, std::uint64_t now) const noexcept {
        return fresh(now) && current.valid && current.owner.valid() && current.owner_generation && current.physics_step &&
            now >= current.sampled_at && now-current.sampled_at <= 150 && owner == current.owner &&
            owner_generation == current.owner_generation && physics_step <= current.physics_step && sampled_at <= current.sampled_at;
    }
    bool matches(const NativeAirSpinProof& current, std::uint64_t now) const noexcept {
        return fresh(now) && current.fresh(now) && airborne && current.airborne && episode &&
            owner == current.owner && owner_generation == current.owner_generation &&
            episode == current.episode && departure == current.departure &&
            physics_step <= current.physics_step && sampled_at <= current.sampled_at;
    }
};
struct AirMotionIntent {
    bool enabled{}, angular_allowed{};
    std::uint64_t sampled_at{}, release_sequence{}, release_at{};
    unsigned charge_ms{};
    float yaw_rate{};
    std::uint64_t generation{};
    NativeAirSpinProof angular_proof{};
    GroundControl ground_controls{};
    SpinCommand spin{};
};
struct AirMotionFrame {
    std::uint64_t now{}, owner{};
    unsigned state{};
    float dt{}, root_y{}, velocity_y{}, spin_y{};
    bool ordinary_ground{};
    float velocity_x{}, velocity_z{};
    bool native_required{}; // Production always requires original native proof.
    NativeAirSpinProof native{};
};
struct AirMotionOutput {
    bool ready{}, vertical{}, angular{};
    float velocity_y{}, spin_y{}, yaw{};
    bool horizontal{};
    float delta_x{}, delta_z{};
    SpinCommand spin{};
    bool tap_step{};
};
// The host still chooses takeoff, air, collision, landing and bail. Ordinary
// ground entry is required before this controller can own an air episode.
class AirMotion {
public:
    static constexpr float ordinary_gravity = 1350 * GameplayProfile::metres_per_inch;
    // Existing native motion targets lead actual root motion by one simulation
    // step. Integrate that next interval from the selected current velocity.
    static float next_height(float root_y, const AirMotionOutput& output, float dt) noexcept {
        return root_y + output.velocity_y*dt - .5f*ordinary_gravity*dt*dt;
    }
    static bool same_velocity(float a,float b) noexcept {
        return std::isfinite(a) && std::isfinite(b) &&
            std::abs(a-b)<=8*std::numeric_limits<float>::epsilon()*std::max({1.0f,std::abs(a),std::abs(b)});
    }
    static bool vertical_target_current(const AirMotionOutput& output,const AirMotionIntent& intent,
            const NativeAirSpinProof& output_proof,const NativeAirSpinProof& current,
            std::uint64_t now,float velocity_y,float dt) noexcept {
        // A cached physical target must belong to this flight and still own
        // the delivered velocity. A collision or another native impulse wins.
        return output.ready && output.vertical && intent.enabled &&
            now>=intent.sampled_at && now-intent.sampled_at<=150 && output_proof.matches(current,now) &&
            std::isfinite(dt) && dt>0 && dt<=.1f &&
            (same_velocity(velocity_y,output.velocity_y) ||
             same_velocity(velocity_y,output.velocity_y-ordinary_gravity*dt));
    }
    static bool angular_target_current(const AirMotionOutput& output, const AirMotionIntent& intent,
            const NativeAirSpinProof& output_proof, const NativeAirSpinProof& current, std::uint64_t now) noexcept {
        return output.angular && intent.enabled && intent.angular_allowed && intent.spin.facts.no_spin!=SpinFact::set &&
            output.spin.mode==intent.spin.mode && output.spin.epoch==intent.spin.epoch && output.spin.serial==intent.spin.serial &&
            output.spin.balance==intent.spin.balance && (output.tap_step || output.spin_y == intent.yaw_rate) &&
            now >= intent.sampled_at && now-intent.sampled_at <= 150 &&
            intent.sampled_at >= intent.angular_proof.sampled_at &&
            output_proof.matches(current,now) && intent.angular_proof.matches(current,now);
    }
    explicit AirMotion(GameplayProfile profile = {}) noexcept : profile_(profile), air_limits_(profile) {}
    void reset() noexcept {
        air_limits_.reset();tap_spin_={};
        owner_ = seen_release_ = pending_at_ = 0;
        pending_ = grounded_ = ordinary_ = vertical_ = spun_ = limits_active_ = air_seen_ = spin_blocked_ = false; yaw_ = 0;
    }
    AirMotionOutput step(const AirMotionIntent& intent, const AirMotionFrame& frame) noexcept {
        if (!intent.enabled || !frame.owner || frame.now < intent.sampled_at || frame.now - intent.sampled_at > 150 ||
            !std::isfinite(frame.dt) || frame.dt <= 0 || frame.dt > .1f ||
            !std::isfinite(frame.root_y) || !std::isfinite(frame.velocity_y) || !std::isfinite(frame.spin_y) ||
            !std::isfinite(frame.velocity_x) || !std::isfinite(frame.velocity_z) ||
            std::abs(frame.velocity_x) >= 1000 || std::abs(frame.velocity_z) >= 1000 ||
            std::abs(frame.root_y) > 1000000 || std::abs(frame.velocity_y) >= 1000 || std::abs(frame.spin_y) >= 1000 ||
            !std::isfinite(intent.yaw_rate) || std::abs(intent.yaw_rate) > profile_.angular_speed() + .0001f ||
            (clocked_ && (frame.now < last_ || frame.now - last_ > 150))) {
            reset(); clocked_ = false; return {};
        }
        clocked_ = true; last_ = frame.now;
        if (generation_ != intent.generation) { reset(); generation_ = intent.generation; }
        if (owner_ != frame.owner) {
            reset(); owner_ = frame.owner; seen_release_ = intent.release_sequence;
        }
        const bool ground = frame.state == 100 || frame.state == 103;
        const bool air = frame.state == 200 || frame.state == 201;
        if (!ground && !air) { reset(); return {}; }
        if (intent.release_sequence > seen_release_) {
            seen_release_ = intent.release_sequence;
            pending_ = (ground || grounded_) && frame.now >= intent.release_at &&
                intent.release_at <= intent.sampled_at && frame.now - intent.release_at <= 150;
            pending_at_ = intent.release_at; charge_ = std::min(intent.charge_ms, 200u);
        }
        if (pending_ && (frame.now < pending_at_ || frame.now - pending_at_ > 150)) pending_ = false;
        if (intent.angular_allowed && intent.yaw_rate == 0) spin_blocked_ = false;
        if (ground) {
            // Physics can observe landing and relaunch between input polls.
            // A stale held rate cannot restart the completed spin episode.
            if (air_seen_ && intent.yaw_rate != 0) spin_blocked_ = true;
            air_seen_ = false;
            tap_spin_.baseline(intent.spin);
            grounded_ = true; ordinary_ = frame.ordinary_ground; limits_active_ = ordinary_;
            if (ordinary_) air_limits_.observe_ordinary(std::hypot(frame.velocity_x,frame.velocity_z));
            else air_limits_.reset();
            ground_vy_ = frame.velocity_y; vertical_ = spun_ = false; yaw_ = 0;
            return {ordinary_, false, false};
        }
        air_seen_ = true;
        const bool taking_off = grounded_;
        grounded_ = false;
        if (!ordinary_) return {};
        if (taking_off && pending_) {
            vy_ = std::max(ground_vy_, 0.0f) + profile_.jump_speed(charge_);
            vertical_ = true; pending_ = false;
        } else if (taking_off) {
            vy_ = frame.velocity_y; vertical_ = true; // A step off a ledge is not an extra jump.
        } else if (vertical_) {
            const float expected = vy_ - ordinary_gravity * frame.dt;
            if (same_velocity(frame.velocity_y, vy_)) vy_ = expected;
            else if (same_velocity(frame.velocity_y, expected)) vy_ = frame.velocity_y;
            else {
                // A native impulse/contact or an unmodelled trajectory wins.
                // Without independent force identity, never overwrite it with
                // a cached ballistic path or count it as a new charged launch.
                vy_ = frame.velocity_y; vertical_ = false;
            }
        }
        const bool native_admitted = !frame.native_required ||
            (frame.native.owner.core == frame.owner && intent.angular_proof.matches(frame.native,frame.now) &&
             intent.sampled_at >= intent.angular_proof.sampled_at);
        // A genuinely fresh current-flight publication of the SAME held input
        // replaces the old flight ticket. Retimestamping an old ticket cannot.
        if (frame.native_required && native_admitted) spin_blocked_ = false;
        const bool spin_context = native_admitted && !spin_blocked_ && intent.angular_allowed;
        const bool spin_paused = intent.spin.facts.no_spin==SpinFact::set;
        if (!spin_context) tap_spin_.baseline(intent.spin);
        const float yaw_rate = spin_context ? tap_spin_.step(intent.spin,intent.yaw_rate,frame.dt,profile_.angular_speed()) : 0;
        const bool angular = spin_context && !spin_paused && (yaw_rate != 0 || spun_);
        if (spin_context && !spin_paused) {
            yaw_ = std::remainder(yaw_ + static_cast<double>(yaw_rate) * frame.dt, 6.2831853071795864769);
            spun_ = yaw_rate != 0;
        } else if(!spin_context) spun_ = false;
        AirMotionOutput output{true, vertical_, angular, vy_, yaw_rate, static_cast<float>(yaw_)};
        output.spin=intent.spin;output.tap_step=tap_spin_.tap_step();
        // Default ordinary flight carries the same hard/soft limits as ground,
        // without rolling drag. A native trajectory takeover also withdraws
        // this ordinary-flight authority; transfer overrides remain native-owned.
        if (!frame.ordinary_ground) limits_active_ = false;
        if (vertical_ && limits_active_) {
            const float speed = std::hypot(frame.velocity_x,frame.velocity_z);
            const float change = air_limits_.step_air(frame.dt,speed);
            if (change < 0 && speed > 0) {
                output.horizontal = true;
                output.delta_x = frame.velocity_x/speed*change;
                output.delta_z = frame.velocity_z/speed*change;
            }
        } else air_limits_.reset();
        return output;
    }
    // Validate the whole batch before changing any body. Uniform deltas keep
    // articulated relative motion while selecting the root's vertical/spin rate.
    template<std::size_t N>
    static bool apply_bodies(const AirMotionOutput& output,
        std::array<std::array<float, 3>, N>& velocities,
        std::array<std::array<float, 3>, N>& spins,
        const std::array<float, 3>& up = {0, 1, 0}) noexcept {
        static_assert(N > 0);
        if (!output.ready || (!output.vertical && !output.angular && !output.horizontal)) return false;
        const float dy = output.vertical ? output.velocity_y - velocities[0][1] : 0;
        const float dx = output.horizontal ? output.delta_x : 0;
        const float dz = output.horizontal ? output.delta_z : 0;
        const float length = std::hypot(up[0], up[1], up[2]);
        if (!std::isfinite(length) || length < .5f || length > 1.5f) return false;
        const std::array<float, 3> axis{up[0]/length, up[1]/length, up[2]/length};
        const float current = spins[0][0]*axis[0] + spins[0][1]*axis[1] + spins[0][2]*axis[2];
        const float ds = output.angular ? output.spin_y-current : 0;
        for (std::size_t i = 0; i < N; ++i) {
            for (unsigned component = 0; component < 3; ++component) {
                const float v = velocities[i][component] + (component == 1 ? dy : component == 0 ? dx : dz);
                const float s = spins[i][component] + ds * up[component] / length;
                if (!std::isfinite(v) || !std::isfinite(s) || std::abs(v) >= 1000 || std::abs(s) >= 1000) return false;
            }
        }
        for (std::size_t i = 0; i < N; ++i) {
            velocities[i][0] += dx; velocities[i][1] += dy; velocities[i][2] += dz;
            for (unsigned component = 0; component < 3; ++component) spins[i][component] += ds*axis[component];
        }
        return true;
    }
    static void rotate_target(std::array<float, 16>& matrix, float yaw) noexcept {
        const float c = std::cos(yaw), s = std::sin(yaw);
        // Compose around the current local up basis, preserving that basis.
        for (unsigned axis = 0; axis < 3; ++axis) {
            const float x = matrix[axis], z = matrix[8+axis];
            matrix[axis] = c*x-s*z; matrix[8+axis] = s*x+c*z;
        }
    }
private:
    GameplayProfile profile_;
    GroundResistance air_limits_;
    TapSpin tap_spin_;
    std::uint64_t owner_{}, seen_release_{}, pending_at_{}, last_{}, generation_{};
    unsigned charge_{};
    float ground_vy_{}, vy_{};
    double yaw_{};
    bool pending_{}, grounded_{}, ordinary_{}, vertical_{}, spun_{}, clocked_{}, limits_active_{}, air_seen_{}, spin_blocked_{};
};
}

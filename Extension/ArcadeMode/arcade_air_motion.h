#pragma once
#include "arcade_gameplay_profile.h"
#include "arcade_movement.h"
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
struct AirMotionIntent {
    bool enabled{}, angular_allowed{};
    std::uint64_t sampled_at{}, release_sequence{}, release_at{};
    unsigned charge_ms{};
    float yaw_rate{};
    std::uint64_t generation{};
};
struct AirMotionFrame {
    std::uint64_t now{}, owner{};
    unsigned state{};
    float dt{}, root_y{}, velocity_y{}, spin_y{};
    bool ordinary_ground{};
    float velocity_x{}, velocity_z{};
};
struct AirMotionOutput {
    bool ready{}, vertical{}, angular{};
    float velocity_y{}, spin_y{}, yaw{};
    bool horizontal{};
    float delta_x{}, delta_z{};
};
// The host still chooses takeoff, air, collision, landing and bail. Ordinary
// ground entry is required before this controller can own an air episode.
class AirMotion {
public:
    explicit AirMotion(GameplayProfile profile = {}) noexcept : profile_(profile), air_limits_(profile) {}
    void reset() noexcept {
        air_limits_.reset();
        owner_ = seen_release_ = pending_at_ = 0;
        pending_ = grounded_ = ordinary_ = vertical_ = spun_ = limits_active_ = false; yaw_ = 0;
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
        if (ground) {
            grounded_ = true; ordinary_ = frame.ordinary_ground; limits_active_ = ordinary_;
            if (ordinary_) air_limits_.observe_ordinary(std::hypot(frame.velocity_x,frame.velocity_z));
            else air_limits_.reset();
            ground_vy_ = frame.velocity_y; vertical_ = spun_ = false; yaw_ = 0;
            return {ordinary_, false, false};
        }
        const bool taking_off = grounded_;
        grounded_ = false;
        if (!ordinary_) return {};
        if (taking_off && pending_) {
            vy_ = std::max(ground_vy_, 0.0f) + profile_.jump_speed(charge_);
            vertical_ = true; pending_ = false;
        } else if (taking_off) {
            vy_ = frame.velocity_y; vertical_ = true; // A step off a ledge is not an extra jump.
        } else if (vertical_) {
            const float expected = vy_ - 1350 * GameplayProfile::metres_per_inch * frame.dt;
            const auto same = [](float a, float b) {
                return std::abs(a-b) <= 8*std::numeric_limits<float>::epsilon()*std::max({1.0f,std::abs(a),std::abs(b)});
            };
            if (same(frame.velocity_y, vy_)) vy_ = expected;
            else if (same(frame.velocity_y, expected)) vy_ = frame.velocity_y;
            else {
                // A native impulse/contact or an unmodelled trajectory wins.
                // Without independent force identity, never overwrite it with
                // a cached ballistic path or count it as a new charged launch.
                vy_ = frame.velocity_y; vertical_ = false;
            }
        }
        const bool angular = intent.angular_allowed && (intent.yaw_rate != 0 || spun_);
        if (intent.angular_allowed) {
            yaw_ = std::remainder(yaw_ + static_cast<double>(intent.yaw_rate) * frame.dt, 6.2831853071795864769);
            spun_ = intent.yaw_rate != 0;
        } else spun_ = false;
        AirMotionOutput output{true, vertical_, angular, vy_, intent.yaw_rate, static_cast<float>(yaw_)};
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
            for (unsigned axis = 0; axis < 3; ++axis) {
                const float v = velocities[i][axis] + (axis == 1 ? dy : axis == 0 ? dx : dz);
                const float s = spins[i][axis] + ds * up[axis] / length;
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
    std::uint64_t owner_{}, seen_release_{}, pending_at_{}, last_{}, generation_{};
    unsigned charge_{};
    float ground_vy_{}, vy_{};
    double yaw_{};
    bool pending_{}, grounded_{}, ordinary_{}, vertical_{}, spun_{}, clocked_{}, limits_active_{};
};
}

#pragma once
#include "arcade_gameplay_profile.h"
#include <array>
#include <cmath>

namespace dingosdk::arcade_mode {
// An accepted crouch command starts charge acquisition. This is an execution
// latch, not native acceptance evidence: raw A alone cannot create a launch.
class NativeCharge {
public:
    struct Release { std::uint64_t sequence{}, ms{}; unsigned charge_ms{}; };
    void cancel() noexcept { charging_ = false; released_ = {}; }
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
    bool charging_{}, clocked_{};
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
};
struct AirMotionOutput {
    bool ready{}, vertical{}, angular{};
    float velocity_y{}, spin_y{}, yaw{};
};
// The host still chooses takeoff, air, collision, landing and bail. Ordinary
// ground entry is required before this controller can own an air episode.
class AirMotion {
public:
    explicit AirMotion(GameplayProfile profile = {}) noexcept : profile_(profile) {}
    void reset() noexcept {
        owner_ = seen_release_ = pending_at_ = 0;
        pending_ = grounded_ = ordinary_ = vertical_ = spun_ = false; yaw_ = 0;
    }
    AirMotionOutput step(const AirMotionIntent& intent, const AirMotionFrame& frame) noexcept {
        if (!intent.enabled || !frame.owner || frame.now < intent.sampled_at || frame.now - intent.sampled_at > 150 ||
            !std::isfinite(frame.dt) || frame.dt <= 0 || frame.dt > .1f ||
            !std::isfinite(frame.root_y) || !std::isfinite(frame.velocity_y) || !std::isfinite(frame.spin_y) ||
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
            grounded_ = true; ordinary_ = frame.ordinary_ground;
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
        } else if (vertical_) vy_ -= 1350 * GameplayProfile::metres_per_inch * frame.dt;
        const bool angular = intent.angular_allowed && (intent.yaw_rate != 0 || spun_);
        if (intent.angular_allowed) {
            yaw_ = std::remainder(yaw_ + static_cast<double>(intent.yaw_rate) * frame.dt, 6.2831853071795864769);
            spun_ = intent.yaw_rate != 0;
        } else spun_ = false;
        return {true, vertical_, angular, vy_, intent.yaw_rate, static_cast<float>(yaw_)};
    }
    // Validate the whole batch before changing any body. Uniform deltas keep
    // articulated relative motion while selecting the root's vertical/spin rate.
    template<std::size_t N>
    static bool apply_bodies(const AirMotionOutput& output,
        std::array<std::array<float, 3>, N>& velocities,
        std::array<std::array<float, 3>, N>& spins) noexcept {
        static_assert(N > 0);
        if (!output.ready || (!output.vertical && !output.angular)) return false;
        const float dy = output.vertical ? output.velocity_y - velocities[0][1] : 0;
        const float ds = output.angular ? output.spin_y - spins[0][1] : 0;
        for (std::size_t i = 0; i < N; ++i) {
            for (unsigned axis = 0; axis < 3; ++axis) {
                const float v = velocities[i][axis] + (axis == 1 ? dy : 0);
                const float s = spins[i][axis] + (axis == 1 ? ds : 0);
                if (!std::isfinite(v) || !std::isfinite(s) || std::abs(v) >= 1000 || std::abs(s) >= 1000) return false;
            }
        }
        for (std::size_t i = 0; i < N; ++i) {
            velocities[i][1] += dy; spins[i][1] += ds;
        }
        return true;
    }
    static void rotate_target(std::array<float, 16>& matrix, float yaw) noexcept {
        const float c = std::cos(yaw), s = std::sin(yaw);
        for (unsigned column : {0u, 4u, 8u}) {
            const float x = matrix[column], z = matrix[column + 2];
            matrix[column] = c * x + s * z; matrix[column + 2] = -s * x + c * z;
        }
    }
private:
    GameplayProfile profile_;
    std::uint64_t owner_{}, seen_release_{}, pending_at_{}, last_{}, generation_{};
    unsigned charge_{};
    float ground_vy_{}, vy_{};
    double yaw_{};
    bool pending_{}, grounded_{}, ordinary_{}, vertical_{}, spun_{}, clocked_{};
};
}

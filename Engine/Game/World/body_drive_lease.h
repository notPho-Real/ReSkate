#pragma once
#include "body_hit_identity.h"
#include <array>
#include <cstdint>

namespace dingosdk::body_drive {
using NativeHandle = body_hit_identity::NativeHandle;
using Pose = std::array<float,8>; // xyzw quaternion, xyz position, padding.
struct World {
    std::uintptr_t context{}, world{}, state{};
    bool operator==(const World&) const = default;
};
// Own only the anchor and joint. The driven body's motion type, shapes,
// collision filters and articulation remain the caller's native state.
// Backend operations execute synchronously in a qualified physics producer.
class Lease {
public:
    Lease() = default;
    Lease(const Lease&) = delete;
    Lease& operator=(const Lease&) = delete;
    bool active() const noexcept { return active_; }
    bool occupied() const noexcept { return anchor_.world || joint_.world || uncertain_; }
    bool uncertain() const noexcept { return uncertain_; }
    NativeHandle driven() const noexcept { return driven_; }
    const World& world() const noexcept { return world_; }

    template<class Backend>
    bool begin(Backend& backend, World world, NativeHandle driven, const Pose& pose) {
        if (occupied() || !world.world || !world.state || driven.world != world.world ||
            !backend.current(world) || !backend.valid_pose(pose) || !backend.body_valid(driven)) return false;
        world_ = world;
        driven_ = driven;
        // Failed calls may have acquired a handle. Keep it for ordered cleanup;
        // never start another allocation over an uncertain previous attempt.
        if (!backend.create_anchor(world, pose, anchor_)) return failed(backend);
        if (!backend.current(world) || !backend.body_valid(driven) ||
            !backend.create_joint(world, driven, anchor_, joint_)) return failed(backend);
        if (!backend.current(world) || !backend.body_valid(driven) ||
            !backend.enable(joint_)) return failed(backend);
        active_ = true;
        return true;
    }

    template<class Backend>
    bool target(Backend& backend, World current, NativeHandle driven, const Pose& pose) {
        if (!active_ || uncertain_ || current != world_ || driven != driven_ ||
            !backend.current(world_) || !backend.valid_pose(pose) ||
            !backend.body_valid(driven_) || !backend.body_valid(anchor_) ||
            !backend.joint_valid(joint_)) return false;
        if (!backend.target(anchor_, pose)) {
            // Retain the lease for removal. Do not retry a target whose native
            // call may have performed only part of its work.
            uncertain_ = true;
            return false;
        }
        return backend.current(world_);
    }

    template<class Backend>
    bool end(Backend& backend, World current) {
        if (!occupied()) { clear(); return true; }
        // No native access through a world which is no longer current. The
        // caller must keep servicing cleanup until it observes world teardown.
        if (current != world_ || !backend.current(world_)) return false;
        if (cleanup_attempted_ || !backend.cleanup_known()) return false;
        cleanup_attempted_ = true;
        active_ = false;
        if (joint_.world && !backend.remove_joint(world_, joint_)) { uncertain_ = true; return false; }
        if (anchor_.world && !backend.remove_anchor(world_, anchor_)) { uncertain_ = true; return false; }
        clear();
        return true;
    }

    // Only a separately observed world-lifetime boundary permits forgetting
    // resources without native cleanup. A failed owner lookup is not one.
    bool world_destroyed(World destroyed) noexcept {
        if (destroyed != world_) return false;
        clear();
        return true;
    }
private:
    template<class Backend> bool failed(Backend& backend) {
        uncertain_ = true;
        end(backend, world_);
        return false;
    }
    void clear() noexcept {
        world_ = {}; driven_ = {}; anchor_ = {}; joint_ = {};
        active_ = uncertain_ = cleanup_attempted_ = false;
    }
    World world_{};
    NativeHandle driven_{}, anchor_{}, joint_{};
    bool active_{}, uncertain_{}, cleanup_attempted_{};
};
} // namespace dingosdk::body_drive

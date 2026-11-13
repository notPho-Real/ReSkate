#pragma once
#include "Engine/Game/World/body_drive_lease.h"
#include <cstdint>

namespace dingosdk::body_drive {
// Internal SDK adapter. Call only from the qualified native client physics
// producer; the feature still has to establish local ownership and clearance.
// This class creates no collision shape and never directly writes the driven
// body's transform, velocity, motion type or collision settings.
class NativeDrive {
public:
    bool start(std::uintptr_t base, std::uintptr_t client, NativeHandle body, const Pose& pose) noexcept;
    bool update(NativeHandle body, const Pose& pose) noexcept;
    bool stop() noexcept;
    bool occupied() const noexcept { return lease_.occupied(); }
    bool active() const noexcept { return lease_.active(); }
    bool uncertain() const noexcept { return lease_.uncertain() || world_lost_; }
private:
    friend class Lease;
    bool resolve() noexcept;
    bool capture(World& world) const noexcept;
    bool current(World world) const noexcept;
    bool valid_pose(const Pose& pose) const noexcept;
    bool body_valid(NativeHandle body) const noexcept;
    bool joint_valid(NativeHandle joint) const noexcept;
    bool create_anchor(World world, const Pose& pose, NativeHandle& out) noexcept;
    bool create_joint(World world, NativeHandle body, NativeHandle anchor, NativeHandle& out) noexcept;
    bool enable(NativeHandle joint) noexcept;
    bool target(NativeHandle anchor, const Pose& pose) noexcept;
    bool remove_joint(World world, NativeHandle& joint) noexcept;
    bool remove_anchor(World world, NativeHandle& anchor) noexcept;
    bool cleanup_known() const noexcept { return !unknown_acquisition_; }
    Lease lease_;
    std::uintptr_t base_{}, client_{};
    std::uint32_t thread_{};
    bool resolved_{}, unknown_acquisition_{};
    mutable bool world_lost_{};
    std::array<float,7> profile_{};
};
} // namespace dingosdk::body_drive

#include "native_body_drive.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Game/Build/image_identity.h"
#include "Engine/Game/Build/20260929/body_drive.h"
#include "Engine/Game/Build/20260929/engine.h"
#include "Engine/Game/Build/20260929/remote_collision.h"
#include "Engine/Game/Build/20260929/rigid_bodies.h"
#include <Windows.h>
#include <intrin.h>
#include <algorithm>
#include <cmath>

namespace dingosdk::body_drive {
namespace {
namespace c = addr::remote_collision;
namespace d = addr::body_drive;
namespace r = addr::rigid_bodies;
using GetWorld = std::uintptr_t (*)(std::uintptr_t);
using Valid = bool (*)(const NativeHandle*);
using Touch = void (*)(NativeHandle*);
using SetInt = void (*)(NativeHandle*, int);
using SetFloat = void (*)(NativeHandle*, float);
using SetPose = void (*)(NativeHandle*, const float*);
using Allocate = NativeHandle* (*)(std::uintptr_t, NativeHandle*);
using Remove = void (*)(std::uintptr_t, NativeHandle*);
using Create = void (*)(std::uintptr_t, NativeHandle*, const NativeHandle*, const NativeHandle*,
    float,float,float,float,float,float,float);
bool pointer(std::uintptr_t value) noexcept {
    return value >= 0x10000 && value < memory::highest_user_address - 0x20000;
}
bool matches(std::uintptr_t base, const game::build::Fingerprint& pin) noexcept {
    std::array<unsigned char,32> bytes{};
    return memory::peek_bytes(base + pin.rva, bytes.data(), bytes.size()) && bytes == pin.bytes;
}
template<class Function, class... Args>
bool call(Function function, Args... args) noexcept {
    __try { function(args...); return true; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
bool native_world(GetWorld function, std::uintptr_t context, std::uintptr_t& world) noexcept {
    __try { world = function(context); return pointer(world); }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
bool valid(Valid function, NativeHandle handle) noexcept {
    __try { return pointer(handle.world) && handle.index != 0xffffffffU && function(&handle); }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
bool allocate(Allocate function, std::uintptr_t world, NativeHandle& out) noexcept {
    __try { return function(world, &out) == &out; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
}
bool NativeDrive::resolve() noexcept {
    if (!supported_build::running_image_matches(base_)) return false;
    for (const auto* pin : {&c::physics_world, &c::allocate_body, &c::set_motion_type,
        &c::set_body_980, &c::clear_body_aa0, &c::set_transform, &c::body_valid,
        &d::create_joint, &d::joint_valid, &d::enable_joint, &d::disable_joint,
        &d::release_joint, &d::disable_body, &d::release_body, &d::body_collision})
        if (!matches(base_, *pin)) return false;
    for (std::size_t i=0; i<profile_.size(); ++i)
        if (!memory::peek(base_ + d::profile_values[i], profile_[i]) || profile_[i] != d::profile_expected[i]) return false;
    return true;
}
bool NativeDrive::capture(World& out) const noexcept {
    out = {};
    if (!resolved_ || world_lost_ || GetCurrentThreadId() != thread_ || !pointer(client_)) return false;
    std::uintptr_t vtable{}, context{};
    std::uint32_t phase{}, tls_index{};
    std::uintptr_t tls{}, current_context{};
    std::uint8_t active{};
    if (!memory::peek(client_, vtable) || vtable != base_ + addr::engine::client_vtable ||
        !memory::peek(client_ + r::client_context, context) || !pointer(context) ||
        !memory::peek(client_ + r::client_phase, phase) || (phase != 13 && phase != 21)) return false;
    if (!memory::peek(base_ + addr::engine::tls_index, tls_index) || tls_index > 4095 ||
        !memory::peek(static_cast<std::uintptr_t>(__readgsqword(0x58)) +
            std::uintptr_t{tls_index} * sizeof(std::uintptr_t), tls) || !pointer(tls) ||
        !memory::peek(tls + r::tls_active, active) || !active ||
        !memory::peek(tls + r::tls_context, current_context) || current_context != context) return false;
    out.context = context;
    if (!native_world(reinterpret_cast<GetWorld>(base_ + c::physics_world.rva), context, out.world) ||
        !memory::peek(out.world + addr::physics_bodies::store_offset, out.state) || !pointer(out.state)) return false;
    if (lease_.occupied() && out != lease_.world()) {
        // Once a different world has been observed, never resolve this lease
        // against recycled addresses. A new game process starts a new adapter.
        world_lost_ = true;
        return false;
    }
    return true;
}
bool NativeDrive::current(World expected) const noexcept {
    World actual;
    return capture(actual) && actual == expected;
}
bool NativeDrive::valid_pose(const Pose& pose) const noexcept {
    if (!std::all_of(pose.begin(),pose.end(),[](float value){return std::isfinite(value);})) return false;
    const float norm = pose[0]*pose[0]+pose[1]*pose[1]+pose[2]*pose[2]+pose[3]*pose[3];
    return std::abs(norm-1.f) <= .001f &&
        std::all_of(pose.begin()+4,pose.begin()+7,[](float value){return std::abs(value)<1000000.f;});
}
bool NativeDrive::body_valid(NativeHandle body) const noexcept {
    World world;
    std::uint32_t capacity{};
    std::uintptr_t allocated{};
    std::uint8_t bits{};
    return capture(world) && body.world == world.world &&
        memory::peek(world.state+r::body_capacity,capacity) && capacity<=1048576 && body.index<capacity &&
        memory::peek(world.state+r::allocated_bits,allocated) && pointer(allocated) &&
        memory::peek(allocated+(body.index>>3),bits) && (bits & (1U<<(body.index&7U))) &&
        valid(reinterpret_cast<Valid>(base_ + c::body_valid.rva), body);
}
bool NativeDrive::joint_valid(NativeHandle joint) const noexcept {
    World world;
    std::uint32_t capacity{};
    return capture(world) && joint.world == world.world &&
        memory::peek(world.world+d::joint_capacity,capacity) && joint.index<capacity &&
        valid(reinterpret_cast<Valid>(base_ + d::joint_valid.rva), joint);
}
bool NativeDrive::create_anchor(World world, const Pose& pose, NativeHandle& out) noexcept {
    if (!current(world)) return false;
    const bool made = allocate(reinterpret_cast<Allocate>(base_ + c::allocate_body.rva), world.world, out);
    if (!made || out.world != world.world || !body_valid(out)) {
        unknown_acquisition_ = true;
        return false;
    }
    // Same shape-free keyframed anchor preparation as the native articulated
    // controller. Only this new helper body receives these settings.
    return current(world) &&
        call(reinterpret_cast<SetInt>(base_ + c::set_motion_type.rva), &out, 2) &&
        call(reinterpret_cast<SetFloat>(base_ + c::set_body_980.rva), &out, 1.f) &&
        call(reinterpret_cast<Touch>(base_ + c::clear_body_aa0.rva), &out) &&
        call(reinterpret_cast<SetInt>(base_ + d::body_collision.rva), &out, 0) &&
        target(out, pose) && current(world);
}
bool NativeDrive::create_joint(World world, NativeHandle body, NativeHandle anchor, NativeHandle& out) noexcept {
    if (!current(world) || !body_valid(body) || !body_valid(anchor)) return false;
    const bool made = call(reinterpret_cast<Create>(base_ + d::create_joint.rva), world.world, &out, &body, &anchor,
        profile_[0],profile_[1],profile_[2],profile_[3],profile_[4],profile_[5],profile_[6]);
    if (!made || out.world != world.world || !joint_valid(out)) {
        // If creation faulted before publishing its handle, freeing the anchor
        // could leave an unknown joint referring to a reused body slot.
        if (!out.world || !joint_valid(out)) unknown_acquisition_ = true;
        return false;
    }
    return current(world);
}
bool NativeDrive::enable(NativeHandle joint) noexcept {
    return joint_valid(joint) && call(reinterpret_cast<Touch>(base_ + d::enable_joint.rva), &joint);
}
bool NativeDrive::target(NativeHandle anchor, const Pose& pose) noexcept {
    alignas(16) const auto native_pose=pose;
    return valid_pose(pose) && body_valid(anchor) &&
        call(reinterpret_cast<SetPose>(base_ + c::set_transform.rva), &anchor, native_pose.data());
}
bool NativeDrive::remove_joint(World world, NativeHandle& joint) noexcept {
    if (!current(world) || joint.world != world.world) return false;
    if (!joint_valid(joint)) return false;
    // Native wrapper disables, releases, and clears this exact generated handle.
    return call(reinterpret_cast<Remove>(base_ + d::release_joint.rva), world.world, &joint) &&
        !joint.world && joint.index == 0xffffffffU && current(world);
}
bool NativeDrive::remove_anchor(World world, NativeHandle& anchor) noexcept {
    if (!current(world) || anchor.world != world.world || !body_valid(anchor)) return false;
    const auto released = anchor;
    if (!call(reinterpret_cast<Touch>(base_ + d::disable_body.rva), &anchor) ||
        !call(reinterpret_cast<Remove>(base_ + d::release_body.rva), world.world, &anchor)) return false;
    // Release is deferred: validity can remain true until the native retirement
    // queue drains. Verify the exact disabled, queued generation instead of
    // interpreting that expected validity as a failed release (or retrying it).
    std::uintptr_t active{}, queued{}, generations{};
    std::uint8_t active_bits{}, queued_bits{};
    std::uint32_t generation{};
    const auto byte = std::uintptr_t{released.index >> 3};
    const auto bit = 1U << (released.index & 7U);
    if (!current(world) || anchor != released ||
        !memory::peek(world.state + d::body_active_bits, active) || !pointer(active) ||
        !memory::peek(world.world + d::body_retirement_bits, queued) || !pointer(queued) ||
        !memory::peek(world.world + r::world_body_generations, generations) || !pointer(generations) ||
        !memory::peek(active + byte, active_bits) || (active_bits & bit) ||
        !memory::peek(queued + byte, queued_bits) || !(queued_bits & bit) ||
        !memory::peek(generations + std::uintptr_t{released.index} * sizeof(generation), generation) ||
        generation != released.generation || !current(world)) return false;
    anchor = {};
    return true;
}
bool NativeDrive::start(std::uintptr_t base, std::uintptr_t client, NativeHandle body, const Pose& pose) noexcept {
    if (lease_.occupied()) return false;
    base_=base; client_=client; thread_=GetCurrentThreadId(); unknown_acquisition_=false;
    resolved_=resolve();
    World world;
    return capture(world) && body.world==world.world && lease_.begin(*this,world,body,pose);
}
bool NativeDrive::update(NativeHandle body, const Pose& pose) noexcept {
    World world;
    return capture(world) && lease_.target(*this,world,body,pose);
}
bool NativeDrive::stop() noexcept {
    if (!lease_.occupied()) return true;
    World world;
    return capture(world) && lease_.end(*this,world);
}
} // namespace dingosdk::body_drive

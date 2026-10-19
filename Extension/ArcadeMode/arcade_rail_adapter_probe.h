#pragma once
#include "arcade_grind_observer.h"
#include <cmath>

namespace dingosdk::arcade_mode {
// Uses the existing supported-build state-page reader and local owner resolver.
// Binding hints are supplied by its background owner; this reader does no scan.
struct RailAdapterLipBindings {
    std::array<std::uintptr_t, 5> objects{};
    std::uint64_t generation{};
};
struct RailAdapterLipSnapshot {
    GrindObserverOwner owner{};
    std::uint64_t sampled_at{};
    std::uint32_t physics_state{};
    std::uint8_t fields{};
    bool valid{}, found{}, lip{}, handplant{};
    std::array<float, 3> direction{}, nearest{};
    // Found is candidate geometry, not execution; lip and handplant are
    // independent physical flags, never a named variant or table epoch.
};
namespace rail_adapter_probe_detail {
using grind_observer_detail::Asset;
inline constexpr std::array assets{
    Asset{{0x7d,0xbc,0xc5,0x80,0xd7,0xd9,0xb2,0xed,0x80,0xc5,0xbc,0x7d,0xd9,0xd7,0xed,0xb2}, "Animation/Dingo/Bool.Physics.FoundLipTrick"},
    Asset{{0xf7,0x40,0x6e,0xeb,0x30,0x84,0x8a,0x2f,0xeb,0x6e,0x40,0xf7,0x84,0x30,0x2f,0x8a}, "Animation/Dingo/Bool.Physics.IsLipTricking"},
    Asset{{0xf4,0xe2,0x75,0xd3,0x3a,0xb6,0x36,0x20,0xd3,0x75,0xe2,0xf4,0xb6,0x3a,0x20,0x36}, "Animation/Dingo/Bool.Physics.IsHandplanting"},
    Asset{{0x91,0x7e,0x62,0x00,0xa7,0xe0,0x4d,0x9c,0x00,0x62,0x7e,0x91,0xe0,0xa7,0x9c,0x4d}, "Animation/Dingo/Vec3.Physics.LipDirection"},
    Asset{{0x09,0xda,0x55,0x9c,0xe0,0x97,0x4a,0xa8,0x9c,0x55,0xda,0x09,0x97,0xe0,0xa8,0x4a}, "Animation/Dingo/Vec3.Physics.LipNearestPoint"}};
template<class Read> bool matches(Read& read, std::size_t index, std::uintptr_t object) noexcept {
    using namespace grind_observer_detail;
    if (index >= assets.size() || object <= 0x10010) return false;
    const auto& asset = assets[index];
    std::array<std::uint8_t,16> id{}; std::uintptr_t name{};
    if (!get(read, object - 0x10, 0, id) || id != asset.id || !get(read, object, 0x18, name) ||
        name <= object - 0x10 || name - (object - 0x10) > 0x400) return false;
    for (std::size_t i = 0; i <= asset.name.size(); ++i) {
        char ch{};
        if (!get(read,name,i,ch) || ch != (i == asset.name.size() ? '\0' : asset.name[i])) return false;
    }
    return true;
}
template<class Read, class Value> bool value(Read& read, std::size_t index,
        std::uintptr_t object, std::uintptr_t block, Value& out) noexcept {
    using namespace grind_observer_detail;
    std::uint32_t handle{}, after_handle{}, bits{}, after_bits{};
    std::uintptr_t page{}, after_page{};
    if (!matches(read,index,object) || !get(read,object,0x28,handle) || !(handle >> 24)) return false;
    const auto number = handle & 0xfffu, offset = (handle >> 12) & 0x7ffu;
    if (number >= 16 || offset > 0x71c - sizeof(Value)) return false;
    const auto mask = std::uint32_t{1} << (offset & 31u);
    Value sampled{};
    if (!get(read,block,0x60 + number * 8,page) || !pointer(page) ||
        !get(read,page,0x71c + (offset >> 5) * 4,bits) || !(bits & mask) ||
        !get(read,page,offset,sampled) || !get(read,object,0x28,after_handle) || after_handle != handle ||
        !get(read,block,0x60 + number * 8,after_page) || after_page != page ||
        !get(read,page,0x71c + (offset >> 5) * 4,after_bits) || !(after_bits & mask) ||
        !matches(read,index,object)) return false;
    out = sampled; return true;
}
}
// Exact asset declarations and state-page reads from andrewnakas/reskate-trainer
// (GPL-3.0), f9c6dbdda278ceee532199756740a52491aceaba,
// Extension/Trainer/trainer_gamestate.cpp transition_objects/value_address/
// read_transition_state. Discovery consumes only the existing worker's bounded
// resident spans; it does not create another process scanner.
template<class Read> unsigned inspect_rail_lip_candidates(Read&& read, std::uintptr_t begin,
        std::size_t bytes, std::uint64_t generation, RailAdapterLipBindings& bindings) noexcept {
    return inspect_state_candidates(read,begin,bytes,generation,bindings,rail_adapter_probe_detail::assets,
        [](auto& reader,unsigned index,std::uintptr_t object){return rail_adapter_probe_detail::matches(reader,index,object);});
}
template<class Read> RailAdapterLipBindings refresh_rail_lip_candidates(Read&& read,
        const RailAdapterLipBindings& previous, std::uint64_t generation) noexcept {
    RailAdapterLipBindings next{{},generation};
    if (!generation) return next;
    for (auto hint : previous.objects) if (hint > 0x10010)
        inspect_rail_lip_candidates(read,hint-0x10,4,generation,next);
    return next;
}
// Read-only, non-atomic diagnostic observation. Partial validity stays explicit.
// It must not independently authorize a native action or install a trick table.
template<class Read, class ResolveOwner> RailAdapterLipSnapshot read_rail_adapter_lip(
        Read&& read, ResolveOwner&& resolve, const RailAdapterLipBindings& bindings,
        std::uint64_t now, bool supported_build) noexcept {
    using namespace grind_observer_detail;
    if (!supported_build) return {};
    const auto owner = resolve();
    if (!fresh(owner,now) || owner.generation != bindings.generation) return {};
    std::uint32_t before{}, after{};
    if (!get(read,owner.context,0x1414,before) || !before) return {};
    const auto block = states_block(read,owner);
    if (!block) return {};
    RailAdapterLipSnapshot result{};
    bool* flags[]{&result.found,&result.lip,&result.handplant};
    for (std::size_t i = 0; i < 3; ++i) {
        std::uint8_t flag{};
        if (rail_adapter_probe_detail::value(read,i,bindings.objects[i],block,flag)) {
            *flags[i] = flag != 0; result.fields |= static_cast<std::uint8_t>(1u << i);
        }
    }
    for (std::size_t i = 3; i < 5; ++i) {
        std::array<float,3> vector{};
        if (rail_adapter_probe_detail::value(read,i,bindings.objects[i],block,vector) &&
            std::isfinite(vector[0]) && std::isfinite(vector[1]) && std::isfinite(vector[2])) {
            (i == 3 ? result.direction : result.nearest) = vector;
            result.fields |= static_cast<std::uint8_t>(1u << i);
        }
    }
    const auto final_owner = resolve();
    if (!get(read,owner.context,0x1414,after) || after != before || !fresh(final_owner,now) ||
        !same(owner,final_owner) || owner.sampled_at != final_owner.sampled_at ||
        states_block(read,final_owner) != block) return {};
    result.valid = true; result.owner = owner; result.sampled_at = owner.sampled_at;
    result.physics_state = before; return result;
}
}

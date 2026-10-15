#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

// Read-only subset of andrewnakas/reskate-trainer (GPL-3.0), revision
// 606ba5ab68f96d08f750dfbcc8d4ae5122f250aa,
// Extension/Trainer/trainer_gamestate.cpp: asset IDs/names at 84–85, page
// resolver at 135–176, numeric reads in read_grind_control at 268–294.
// Build 20260929. No state writers, tuning options or setting hooks are used.
namespace dingosdk::arcade_mode {
enum class GrindSemantic : std::uint8_t { unknown };
enum class GrindContext : std::uint8_t { unknown, grind_physics };
struct NativeGrindSnapshot {
    bool valid{}, grind_type_valid{}, skate_grind_type_valid{};
    std::uint32_t physics_state{}, grind_type{}, skate_grind_type{};
    std::uint64_t sampled_at{}, generation{};
    GrindContext context{GrindContext::unknown};
    GrindSemantic grind_semantic{GrindSemantic::unknown}, skate_grind_semantic{GrindSemantic::unknown};
    // Readability is not an enum mapping, a trick occurrence, or proof that a
    // commanded Y trick happened. All numeric values have UNKNOWN semantics.
};
// Internal integration context, never serialized. Resolve this from the existing
// validated local owner before AND after sampling. Increment generation on owner,
// world, menu/toggle or observation-gap reset; never reuse bindings across it.
struct GrindObserverOwner {
    std::uintptr_t base{}, entity{}, core{}, context{}, rig{};
    std::uint64_t generation{}, sampled_at{};
    bool valid{};
};
struct GrindStateBindings {
    std::array<std::uintptr_t, 2> objects{};
    std::uint64_t generation{};
};
namespace grind_observer_detail {
struct Asset { std::array<std::uint8_t, 16> id; std::string_view name; };
inline constexpr std::array assets{
    Asset{{0x85,0xc4,0x26,0x2b,0xd5,0x4e,0x37,0x04,0x2b,0x26,0xc4,0x85,0x4e,0xd5,0x04,0x37},
        "Animation/Dingo/Enum.GS.Anim.CurrentGrindType"},
    Asset{{0x7f,0xf7,0xde,0xa2,0x14,0xd2,0x6b,0x3b,0xa2,0xde,0xf7,0x7f,0xd2,0x14,0x3b,0x6b},
        "Animation/Dingo/Enum.GS.Anim.CurrentSkateGrindType"}};
inline constexpr std::uintptr_t highest = 0x00007fffffffffffULL;
inline constexpr bool pointer(std::uintptr_t at) noexcept { return at > 0x10000 && at < highest && !(at & 3); }
template<class Read, class T> bool get(Read& read, std::uintptr_t base, std::size_t offset, T& out) noexcept {
    return base > 0x10000 && base < highest && offset <= highest - base &&
        sizeof(T) <= highest - base - offset && read(base + offset, out);
}
inline constexpr bool fresh(const GrindObserverOwner& owner, std::uint64_t now) noexcept {
    return owner.valid && owner.generation && now >= owner.sampled_at && now - owner.sampled_at <= 150 &&
        pointer(owner.base) && pointer(owner.entity) && pointer(owner.core) && pointer(owner.context) && pointer(owner.rig);
}
inline constexpr bool same(const GrindObserverOwner& a, const GrindObserverOwner& b) noexcept {
    return a.base == b.base && a.entity == b.entity && a.core == b.core && a.context == b.context &&
        a.rig == b.rig && a.generation == b.generation;
}
template<class Read> bool asset_matches(Read& read, unsigned index, std::uintptr_t object) noexcept {
    if (index >= assets.size() || object <= 0x10010) return false;
    const auto id_at = object - 0x10;
    std::array<std::uint8_t, 16> id{};
    std::uintptr_t name{};
    const auto& asset = assets[index];
    if (!get(read, id_at, 0, id) || id != asset.id || !get(read, object, 0x18, name) ||
        name <= id_at || name - id_at > 0x400) return false;
    // Exact fixed allowlist, including terminator. Nothing from game strings is
    // retained or logged; each source name has fewer than 64 bytes.
    for (std::size_t i = 0; i <= asset.name.size(); ++i) {
        char value{};
        if (!get(read, name, i, value) || value != (i == asset.name.size() ? '\0' : asset.name[i])) return false;
    }
    return true;
}
template<class Read> std::uintptr_t states_block(Read& read, const GrindObserverOwner& owner) noexcept {
    std::uintptr_t collection{}, back{}; std::uint8_t count{};
    if (!get(read, owner.entity, 0x70, collection) || !pointer(collection) ||
        !get(read, collection, 0, back) || back != owner.entity ||
        !get(read, collection, 8, count) || count > 128) return 0;
    for (unsigned i = 0; i < count; ++i) {
        std::uintptr_t component{}, type{}, animatable{};
        if (!get(read, collection, 0x20 + i * 0x20, component) || !pointer(component) ||
            !get(read, component, 0, type) || owner.base > highest - 0x63aaee8 || type != owner.base + 0x63aaee8) continue;
        if (!get(read, component, 0x68, animatable) || !pointer(animatable)) return 0;
        constexpr std::uintptr_t ways[2][2]{{0xe8, 8}, {0x1d8, 0x248}};
        for (const auto& way : ways) {
            std::uintptr_t holder{}, block{};
            if (get(read, animatable, way[0], holder) && pointer(holder) && get(read, holder, way[1], block) &&
                pointer(block) && get(read, block, 0, type) && type == owner.base + 0x61a8bd8) return block;
        }
        return 0;
    }
    return 0;
}
template<class Read> bool value(Read& read, unsigned index, std::uintptr_t object,
    std::uintptr_t block, std::uint32_t& out) noexcept {
    std::uint32_t handle{}, map{}, after_handle{}, after_map{}, value{};
    std::uintptr_t page{}, after_page{};
    if (!asset_matches(read, index, object) || !get(read, object, 0x28, handle) || !(handle >> 24)) return false;
    const auto page_index = handle & 0xfff, place = (handle >> 12) & 0x7ff;
    // 0x71c is the source's validity bitmap, not part of scalar value storage.
    // Reject a four-byte value crossing it; do not invent an enum numeric range.
    if (page_index >= 16 || place > 0x71c - sizeof(value) ||
        !get(read, block, 0x60 + page_index * 8, page) || !pointer(page) ||
        !get(read, page, 0x71c + (place >> 5) * 4, map) || !(map & (1u << (place & 31))) ||
        !get(read, page, place, value) || !get(read, object, 0x28, after_handle) || after_handle != handle ||
        !get(read, block, 0x60 + page_index * 8, after_page) || after_page != page ||
        !get(read, page, 0x71c + (place >> 5) * 4, after_map) || !(after_map & (1u << (place & 31)))) return false;
    out = value; return true;
}
}

// Worker-only discovery of one already-readable candidate span, <=4 KiB. The
// caller supplies resident pages from its existing bounded scanner and owns
// cancellation/publication. This helper never enumerates process memory or
// exposes partial bindings to a game thread. A trailing candidate may be checked
// with bounded reads outside the span, guarded by Read, to handle page boundaries.
template<class Read> unsigned inspect_grind_candidates(Read&& read, std::uintptr_t begin,
    std::size_t bytes, std::uint64_t generation, GrindStateBindings& bindings) noexcept {
    using namespace grind_observer_detail;
    if (!generation || !pointer(begin) || bytes > 4096 || bytes < 4 || bytes > highest - begin) return 0;
    if (bindings.generation != generation) bindings = {{}, generation};
    unsigned found{};
    for (std::size_t offset = 0; offset + 4 <= bytes; offset += 4) {
        std::uint32_t word{};
        if (!get(read, begin, offset, word)) continue;
        for (unsigned index = 0; index < assets.size(); ++index) {
            const auto& id = assets[index].id;
            const auto first = std::uint32_t{id[0]} | (std::uint32_t{id[1]} << 8) |
                (std::uint32_t{id[2]} << 16) | (std::uint32_t{id[3]} << 24);
            if (word != first) continue;
            const auto object = begin + offset + 0x10;
            std::uint32_t handle{}, known_handle{};
            const auto known = bindings.objects[index];
            if (known && asset_matches(read, index, known) && get(read, known, 0x28, known_handle) && (known_handle >> 24)) continue;
            if (asset_matches(read, index, object) && get(read, object, 0x28, handle) && (handle >> 24)) {
                bindings.objects[index] = object; ++found;
            }
        }
    }
    return found;
}

// Global assets may survive a skater reset, but their previous bindings do not.
// Treat addresses only as hints and repeat exact GUID/name/live-handle checks.
template<class Read> GrindStateBindings refresh_grind_candidates(Read&& read,
    const GrindStateBindings& previous, std::uint64_t generation) noexcept {
    GrindStateBindings next{{}, generation};
    if (!generation) return next;
    for (const auto hint : previous.objects) if (hint > 0x10010)
        inspect_grind_candidates(read, hint - 0x10, 4, generation, next);
    return next;
}

// Read supplies guarded copies; ResolveOwner freshly resolves the existing local
// owner each call. This does no discovery/allocations/writes. Keep it diagnostic;
// independent bounded reads do not constitute an atomic physics snapshot.
template<class Read, class ResolveOwner> NativeGrindSnapshot read_native_grind(
    Read&& read, ResolveOwner&& resolve_owner, const GrindStateBindings& bindings,
    std::uint64_t now, bool supported_build) noexcept {
    using namespace grind_observer_detail;
    if (!supported_build) return {};
    const auto owner = resolve_owner();
    if (!fresh(owner, now) || owner.generation != bindings.generation) return {};
    std::uint32_t before{}, after{};
    if (!get(read, owner.context, 0x1414, before) || !before) return {};
    NativeGrindSnapshot result{};
    result.context = before >= 400 && before < 500 ? GrindContext::grind_physics : GrindContext::unknown;
    std::uintptr_t block{};
    if (result.context == GrindContext::grind_physics) {
        block = states_block(read, owner);
        if (!block) return {};
        result.grind_type_valid = value(read, 0, bindings.objects[0], block, result.grind_type);
        result.skate_grind_type_valid = value(read, 1, bindings.objects[1], block, result.skate_grind_type);
    }
    if (!get(read, owner.context, 0x1414, after) || after != before ||
        (block && states_block(read, owner) != block)) return {};
    const auto current = resolve_owner();
    if (!fresh(current, now) || !same(owner, current)) return {};
    result.valid = true; result.physics_state = before; result.sampled_at = now; result.generation = owner.generation;
    return result;
}
}

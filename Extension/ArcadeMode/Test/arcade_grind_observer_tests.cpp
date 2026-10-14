#include "Extension/ArcadeMode/arcade_grind_observer.h"
#include <cstring>
#include <cstdlib>
#include <iostream>

namespace {
using namespace dingosdk::arcade_mode;
namespace detail = grind_observer_detail;
unsigned checks{};
void check(bool ok, const char* message) {
    ++checks;
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Fixture {
    static constexpr std::uintptr_t start = 0x20000;
    std::array<unsigned char, 32768> data{};
    std::size_t next{}, reads{};
    std::uintptr_t blocked{}, mutate_at{};
    unsigned mutate_on{}, matches{};
    std::uint32_t mutation{};
    GrindObserverOwner owner{};
    std::uintptr_t collection{}, component{}, animatable{}, holder{}, block{}, page{};
    std::array<std::uintptr_t, 2> objects{};
    std::uintptr_t alloc(std::size_t size) { auto at = start + next; next += (size + 7) & ~std::size_t{7}; return at; }
    template<class T> void put(std::uintptr_t at, T value) { std::memcpy(data.data() + at - start, &value, sizeof(value)); }
    template<class T> bool operator()(std::uintptr_t at, T& out) noexcept {
        ++reads;
        if (at == blocked || at < start || at - start > data.size() - sizeof(T)) return false;
        if (at == mutate_at && ++matches == mutate_on) put(at, mutation);
        std::memcpy(&out, data.data() + at - start, sizeof(T)); return true;
    }
    Fixture() {
        owner = {0x140000000, alloc(0x100), alloc(16), alloc(0x1600), alloc(16), 1, 1000, true};
        collection = alloc(0x1040); component = alloc(0x80); animatable = alloc(0x200);
        holder = alloc(0x260); block = alloc(0x160); page = alloc(0x840);
        put(owner.entity + 0x70, collection); put(collection, owner.entity); put(collection + 8, std::uint8_t{1});
        put(collection + 0x20, component); put(component, owner.base + 0x63aaee8);
        put(component + 0x68, animatable); put(animatable + 0xe8, holder); put(holder + 8, block);
        put(block, owner.base + 0x61a8bd8); put(block + 0x60, page);
        put(owner.context + 0x1414, std::uint32_t{401});
        for (unsigned i = 0; i < objects.size(); ++i) {
            const auto at = alloc(0x100); objects[i] = at + 0x10;
            put(at, detail::assets[i].id); const auto name = objects[i] + 0x40; put(objects[i] + 0x18, name);
            std::memcpy(data.data() + name - start, detail::assets[i].name.data(), detail::assets[i].name.size());
            const auto place = 0x20 + i * 4;
            put(objects[i] + 0x28, std::uint32_t{0x01000000} | (place << 12));
            put(page + place, static_cast<std::uint32_t>(i + 5));
        }
        put(page + 0x71c + 4, std::uint32_t{0x11});
    }
    GrindStateBindings bindings() const { return {objects, owner.generation}; }
    NativeGrindSnapshot sample(std::uint64_t now = 1000, bool build = true) {
        return read_native_grind(*this, [this] { return owner; }, bindings(), now, build);
    }
};
}
int main() {
    Fixture f;
    auto sample = f.sample();
    check(sample.valid && sample.grind_type_valid && sample.skate_grind_type_valid &&
          sample.grind_type == 5 && sample.skate_grind_type == 6, "both independent native scalars copied");
    check(sample.context == GrindContext::grind_physics && sample.physics_state == 401 &&
          sample.sampled_at == 1000 && sample.generation == 1, "fresh native grind context accompanies numeric sample");
    check(sample.grind_semantic == GrindSemantic::unknown && sample.skate_grind_semantic == GrindSemantic::unknown,
          "no numeric value is assigned a named grind");
    f.put(f.page + 0x20, UINT32_MAX); f.put(f.page + 0x24, std::uint32_t{0});
    sample = f.sample();
    check(sample.grind_type_valid && sample.grind_type == UINT32_MAX && sample.skate_grind_type_valid && sample.skate_grind_type == 0,
          "full uint32 range retained as raw unknown identity, including zero");
    check(!f.sample(1000, false).valid, "unsupported build never reads state");
    check(f.sample(1150).valid && !f.sample(1151).valid && !f.sample(999).valid, "owner lease rejects stale or reversed time");
    for (const auto state : {100u, 103u, 200u, 504u, UINT32_MAX}) {
        auto bad = f; bad.put(bad.owner.context + 0x1414, state); const auto result = bad.sample();
        check(result.valid && result.context == GrindContext::unknown && !result.grind_type_valid && !result.skate_grind_type_valid,
              "outside grind context never republishes retained animation identities");
    }
    auto bad = f; bad.put(bad.owner.context + 0x1414, std::uint32_t{0});
    check(!bad.sample().valid, "empty native selector state is unavailable");
    for (unsigned field = 0; field < 6; ++field) {
        auto changed = f; unsigned calls{};
        const auto result = read_native_grind(changed, [&] {
            auto owner = changed.owner;
            if (++calls == 2) {
                if (field == 0) owner.entity += 8;
                if (field == 1) owner.core += 8;
                if (field == 2) owner.context += 8;
                if (field == 3) owner.rig += 8;
                if (field == 4) ++owner.generation;
                if (field == 5) owner.base += 8;
            }
            return owner;
        }, changed.bindings(), 1000, true);
        check(!result.valid && !result.grind_type_valid, "owner changes discard the entire detached sample");
    }
    bad = f; auto binding = bad.bindings(); ++binding.generation;
    check(!read_native_grind(bad, [&] { return bad.owner; }, binding, 1000, true).valid, "old generation bindings rejected");
    bad = f; bad.owner.valid = false;
    check(!bad.sample().valid, "invalid current owner rejected");
    bad = f; bad.put(bad.collection, bad.owner.entity + 8);
    check(!bad.sample().valid, "component collection must belong to current entity");
    bad = f; bad.put(bad.collection + 8, std::uint8_t{129});
    check(!bad.sample().valid, "component traversal count bounded");
    bad = f; bad.put(bad.component, std::uintptr_t{0});
    check(!bad.sample().valid, "AntDriven vtable required");
    bad = f; bad.put(bad.block, std::uintptr_t{0});
    check(!bad.sample().valid, "state block vtable required");
    bad = f; bad.put(bad.animatable + 0xe8, std::uintptr_t{0}); bad.put(bad.animatable + 0x1d8, bad.holder); bad.put(bad.holder + 0x248, bad.block);
    check(bad.sample().grind_type_valid, "second upstream block path supported");
    bad = f; bad.put(bad.objects[0] - 0x10, std::uint8_t{0}); sample = bad.sample();
    check(sample.valid && !sample.grind_type_valid && sample.skate_grind_type_valid, "retired asset invalidates only its own numeric field");
    bad = f; bad.put(bad.objects[0] + 0x40, char{'X'});
    check(!bad.sample().grind_type_valid, "exact allowlisted asset name required");
    bad = f; bad.put(bad.objects[0] + 0x40 + detail::assets[0].name.size(), char{'X'});
    check(!bad.sample().grind_type_valid, "asset-name prefix alone cannot match");
    bad = f; bad.put(bad.objects[0] + 0x18, bad.objects[0] + 0x500);
    check(!bad.sample().grind_type_valid, "name pointer locality checked before reading");
    for (const auto handle : {0u, 0x01020010u, 0x0171a000u}) {
        bad = f; bad.put(bad.objects[0] + 0x28, handle);
        check(!bad.sample().grind_type_valid, "invalid handle, page index or scalar crossing validity map rejected");
    }
    bad = f; bad.put(bad.page + 0x720, std::uint32_t{0x10}); sample = bad.sample();
    check(!sample.grind_type_valid && sample.skate_grind_type_valid, "page validity bit required independently");
    bad = f; bad.blocked = bad.page + 0x20;
    check(!bad.sample().grind_type_valid, "failed guarded numeric read remains invalid");
    bad = f; bad.mutate_at = bad.owner.context + 0x1414; bad.mutate_on = 2; bad.mutation = 200;
    check(!bad.sample().valid, "physics transition during sampling rejects mixed context");
    bad = f; bad.mutate_at = bad.objects[0] + 0x28; bad.mutate_on = 2; bad.mutation = 0;
    check(!bad.sample().grind_type_valid, "handle replacement during sampling rejected");
    bad = f; bad.mutate_at = bad.page + 0x720; bad.mutate_on = 2; bad.mutation = 0;
    check(!bad.sample().grind_type_valid, "validity removal during sampling rejected");
    GrindStateBindings found{}; f.reads = 0;
    const auto candidate_start = f.objects[0] - 0x10;
    check(inspect_grind_candidates(f, candidate_start, 512, 1, found) == 2 && found.objects == f.objects,
          "bounded discovery identifies exact two asset candidates");
    check(f.reads < 1000, "small discovery span stays bounded");
    check(inspect_grind_candidates(f, candidate_start, 512, 1, found) == 0, "live bindings are retained across duplicate candidates");
    found = f.bindings();
    check(inspect_grind_candidates(f, f.owner.entity, 4, 2, found) == 0 && found.generation == 2 && !found.objects[0],
          "generation change clears discovery candidates");
    f.reads = 0;
    check(!inspect_grind_candidates(f, candidate_start, 4097, 1, found) && !f.reads, "oversized discovery request rejected before reads");
    check(!inspect_grind_candidates(f, candidate_start, 512, 0, found), "discovery needs a current generation");
    found = {}; f.reads = 0;
    check(inspect_grind_candidates(f, candidate_start, 4096, 1, found) == 2 && f.reads < 3000,
          "maximum accepted discovery span remains bounded");
    check(!detail::get(f, detail::highest - 2, 8, sample.physics_state), "address arithmetic cannot wrap into another object");
    std::cout << checks << " grind observer checks passed\n";
}

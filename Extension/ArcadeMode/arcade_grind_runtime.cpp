#include "arcade_grind_runtime.h"
#include "arcade_grind_capture.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Core/Log/logging.h"
#include "Extension/Skater/no_bail.h"
#include "Extension/Throwdowns/native_type_scan.h"
#include <algorithm>
#include <cmath>
#include <thread>

namespace dingosdk::arcade_mode {
namespace {
struct Read { template<class T> bool operator()(std::uintptr_t p, T& v) const noexcept { return memory::peek(p, v); } };
struct Runtime {
    std::atomic<std::uintptr_t> image_base{};
    std::atomic<std::uint64_t> generation{1}, expires{}, rejected{};
    std::atomic_flag guard = ATOMIC_FLAG_INIT, step_guard = ATOMIC_FLAG_INIT;
    GrindObserverOwner owner{};
    GrindStateBindings bindings{};
    bool arcade{}, speed_valid{};
    int speed_mm_s{};
    GrindQueue<256> queue;
    GrindTransitions transitions;
    GrindRow previous{};
    std::uint64_t next_sample{};
    // Discovery and logging belong exclusively to the background worker.
    multiplayer::NativeTypeScanCursor cursor{};
    GrindStateBindings candidates{};
    std::uint64_t scanned_pages{}, completed_scans{};
};
std::atomic<Runtime*> runtime{};
GrindObserverOwner owner_of(const NoBailSkater& owner, std::uint64_t generation, std::uint64_t now) noexcept {
    return {owner.base, owner.entity, owner.core, owner.context, owner.rig, generation, now, true};
}
bool visit(void* context, std::uintptr_t begin, std::size_t bytes) noexcept {
    auto& r = *static_cast<Runtime*>(context);
    if (r.candidates.generation != r.generation.load() || GetTickCount64() > r.expires.load()) return false;
    ++r.scanned_pages;
    inspect_grind_candidates(Read{}, begin, bytes, r.candidates.generation, r.candidates);
    return !r.candidates.objects[0] || !r.candidates.objects[1];
}
void worker(Runtime& r) noexcept {
    std::uint64_t scan_generation{}, report_at{}, retry_at{};
    std::uintptr_t scan_base{};
    for (;;) {
        try {
            const auto now = GetTickCount64(), generation = r.generation.load();
            const auto base = r.image_base.load();
            if (base != scan_base) {
                scan_base = base; r.cursor = {}; r.candidates = {}; scan_generation = 0;
            }
            if (generation != scan_generation) {
                scan_generation = generation; retry_at = 0;
                r.candidates = refresh_grind_candidates(Read{}, r.candidates, generation);
                // Keep only the global page traversal position across fixture
                // resets. Restarting it every trial can starve late assets.
                // Bindings are independently revalidated for the new generation.
            }
            if (now <= r.expires.load() && now >= retry_at &&
                (!r.candidates.objects[0] || !r.candidates.objects[1])) {
                const bool lowered = SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN) != 0;
                multiplayer::visit_native_pages_step(r.cursor, 2 * 1024 * 1024, 64, visit, &r);
                if (lowered) SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_END);
                if (r.cursor.complete && (!r.candidates.objects[0] || !r.candidates.objects[1])) {
                    ++r.completed_scans;
                    r.cursor = {}; retry_at = now + 1000;
                }
            }
            if (r.candidates.objects[0] && r.candidates.objects[1] && !r.guard.test_and_set(std::memory_order_acquire)) {
                if (r.generation.load() == generation && now <= r.expires.load()) r.bindings = r.candidates;
                r.guard.clear(std::memory_order_release);
            }
            std::array<GrindRow, 64> rows{};
            const auto count = r.queue.take(rows);
            for (std::size_t i = 0; i < count; ++i) {
                const auto& s = rows[i];
                if (s.kind == 1) logging::log(logging::Level::info, logging::Channel::skater,
                    "ARCADE GRIND PAD: seq={} ms={} generation={} user={} y={} x={} ry={} arcade={} dropped={}.",
                    s.seq, s.ms, s.generation, s.user, s.requested_y, s.x, s.y, s.arcade, s.dropped);
                else logging::log(logging::Level::info, logging::Channel::skater,
                    "ARCADE GRIND STATE: seq={} ms={} generation={} valid={} state={} edge={} duration={} grind_valid={} grind={} skate_valid={} skate={} speed_valid={} speed_mm_s={} arcade={} dropped={}.",
                    s.seq, s.ms, s.generation, s.valid, s.state, static_cast<unsigned>(s.edge), s.duration_ms,
                    s.grind_valid, s.grind, s.skate_valid, s.skate, s.speed_valid, s.speed_mm_s, s.arcade, s.dropped);
            }
            if (now >= report_at) {
                report_at = now + 1000;
                logging::log(logging::Level::info, logging::Channel::skater,
                    "ARCADE GRIND STATUS: generation={} active={} dropped={} rejected={}.",
                    generation, now <= r.expires.load(), r.queue.dropped(), r.rejected.load());
                logging::log(logging::Level::info, logging::Channel::skater,
                    "ARCADE GRIND DISCOVERY: generation={} bound={} pages={} passes={}.",
                    generation, unsigned(r.candidates.objects[0] != 0) | (unsigned(r.candidates.objects[1] != 0) << 1),
                    r.scanned_pages, r.completed_scans);
            }
        } catch (...) { /* Logging never blocks or throws into a physics/input producer. */ }
        Sleep(25);
    }
}
}
void invalidate_grind_observer() noexcept {
    if (auto* r = runtime.load()) { r->expires.store(0); ++r->generation; }
}
void grind_observer_tick(std::uintptr_t base, std::uint64_t now, bool gameplay,
    bool arcade_enabled, bool speed_valid, float speed) noexcept {
    try {
        static auto* r = [] { auto* p = new Runtime; std::thread([p] { worker(*p); }).detach(); return p; }();
        runtime.store(r, std::memory_order_release);
        r->image_base.store(base);
        NoBailSkater current{};
        const bool valid = gameplay && no_bail_skater(current) && current.base == base;
        if (r->guard.test_and_set(std::memory_order_acquire)) { ++r->rejected; return; }
        auto owner = valid ? owner_of(current, r->generation.load(), now) : GrindObserverOwner{};
        const auto expiry = r->expires.load();
        if (owner.valid != r->owner.valid || (valid && (!grind_observer_detail::same(owner, r->owner) ||
            (expiry && now > expiry))) || arcade_enabled != r->arcade) {
            r->expires.store(0); owner.generation = ++r->generation; r->bindings = {{}, owner.generation};
        }
        r->owner = owner; r->arcade = arcade_enabled;
        r->speed_valid = speed_valid && std::isfinite(speed) && speed >= 0 && speed <= 1000;
        r->speed_mm_s = r->speed_valid ? static_cast<int>(std::lround(speed * 1000)) : 0;
        r->expires.store(valid ? now + 150 : 0);
        r->guard.clear(std::memory_order_release);
    } catch (...) { invalidate_grind_observer(); }
}
void observe_grind_step(const NoBailSkater& supplied) noexcept {
    auto* r = runtime.load(std::memory_order_acquire);
    if (!r) return;
    const auto now = GetTickCount64(), generation = r->generation.load();
    if (now > r->expires.load()) return;
    if (r->guard.test_and_set(std::memory_order_acquire)) { ++r->rejected; return; }
    const auto owner = r->owner;
    auto bindings = r->bindings;
    GrindRow row{};
    row.ms = now; row.generation = generation; row.arcade = r->arcade;
    row.speed_valid = r->speed_valid; row.speed_mm_s = r->speed_mm_s;
    r->guard.clear(std::memory_order_release);
    if (!grind_observer_detail::same(owner, owner_of(supplied, generation, now))) return;
    auto resolve = [&]() noexcept {
        NoBailSkater value{};
        if (r->generation.load() != generation || now > r->expires.load() || !no_bail_skater(value)) return GrindObserverOwner{};
        return owner_of(value, generation, now);
    };
    bindings.generation = generation;
    const auto sample = read_native_grind(Read{}, resolve, bindings, now, true);
    // Enum binding can be unavailable without hiding independent native physics.
    std::uint32_t after{};
    row.valid = memory::peek(supplied.context + 0x1414, row.state) && row.state &&
        memory::peek(supplied.context + 0x1414, after) && after == row.state &&
        grind_observer_detail::same(owner, resolve());
    if (sample.valid && sample.physics_state == row.state) {
        row.grind_valid = sample.grind_type_valid; row.grind = sample.grind_type;
        row.skate_valid = sample.skate_grind_type_valid; row.skate = sample.skate_grind_type;
    }
    if (r->step_guard.test_and_set(std::memory_order_acquire)) { ++r->rejected; return; }
    row = r->transitions.sample(row);
    const auto& old = r->previous;
    if (row.edge != GrindEdge::sample || now >= r->next_sample || row.generation != old.generation ||
        row.valid != old.valid || row.state != old.state || row.grind_valid != old.grind_valid ||
        row.skate_valid != old.skate_valid || row.grind != old.grind || row.skate != old.skate) {
        r->queue.push(row); r->previous = row; r->next_sample = now + 100;
    }
    r->step_guard.clear(std::memory_order_release);
}
void observe_grind_pad(unsigned user, std::uint64_t now, bool y, short x, short ry, bool arcade) noexcept {
    auto* r = runtime.load(std::memory_order_acquire);
    if (!r || user >= 4 || now > r->expires.load()) return;
    // Called under the existing XInput mutex; each controller is tracked separately.
    static std::array<GrindRow, 4> previous{};
    auto& old = previous[user];
    const auto generation = r->generation.load();
    if (old.generation == generation && old.requested_y == y && old.x == x && old.y == ry &&
        now >= old.ms && now - old.ms < 100) return;
    GrindRow row{};
    row.kind = 1; row.user = user; row.ms = now; row.generation = generation;
    row.requested_y = y; row.x = x; row.y = ry; row.arcade = arcade;
    r->queue.push(row); old = row;
}
}

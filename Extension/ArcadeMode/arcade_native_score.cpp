#include "arcade_native_score.h"
#include "arcade_native_score_buffer.h"
#include "arcade_score_boundary.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Core/Log/logging.h"
#include "Engine/Game/Build/20260929/engine.h"
#include "Extension/Progression/local_entitlement_trigger_runtime.h"
#include "Extension/Skater/no_bail.h"
#include "Extension/Throwdowns/native_type_scan.h"
#include <thread>

namespace dingosdk::arcade_mode {
namespace {
using namespace native_score_detail;
namespace engine = game::build::v20260929::engine;
struct Read {
    template<class T> bool operator()(std::uintptr_t at, T& value) const noexcept { return memory::peek(at, value); }
};
struct Runtime {
    std::atomic<std::uintptr_t> base{};
    std::atomic<std::uint64_t> epoch{}, expires{}, binding_epoch{};
    std::atomic_flag binding_busy = ATOMIC_FLAG_INIT;
    Binding binding{};
    Buffer<128> events;
    std::atomic<std::uint64_t> rejected{}, contended{};
    // Client-thread lifecycle. These addresses are never published or logged.
    NoBailSkater owner{};
    std::uint64_t last_tick{};
    bool worker_started{};
    // Worker-only counters and schedule.
    std::uint64_t next_scan{}, report_at{}, stale{};
    // One worker at a time owns the discovery cursor and partial candidates.
    std::uint64_t scan_epoch{};
    multiplayer::NativeTypeScanCursor cursor{};
    std::array<multiplayer::NativeTypeQuery, 4> queries{};
};
std::atomic<Runtime*> published{};
bool same_owner(const NoBailSkater& a, const NoBailSkater& b) noexcept {
    return a.base == b.base && a.entity == b.entity && a.core == b.core && a.context == b.context && a.rig == b.rig;
}
bool client_realm(std::uintptr_t base) noexcept {
    // Existing validated-build address; guard native TLS access against teardown.
    __try {
        std::uintptr_t context{};
        reinterpret_cast<std::uintptr_t (*)(std::uintptr_t*)>(base + engine::current_context)(&context);
        std::uint32_t offset{}, tag{};
        return context && memory::peek(base + engine::context_type_offset, offset) && offset < 0x1000000 &&
            memory::peek(context + offset, tag) && tag == 0xbf0f9789;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
void scan_slice(Runtime& r, std::uint64_t epoch) noexcept {
    const bool lowered = SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN) != 0;
    if (r.scan_epoch != epoch) {
        r.scan_epoch = epoch; r.cursor = {};
        for (unsigned i = 0; i < r.queries.size(); ++i) r.queries[i] = {type_hashes[i], 0, 2};
    }
    std::size_t missing = 4;
    const auto deadline = GetTickCount64() + 10;
    // Cancel/expiry/time checks every 2 MiB/64 regions, at most 16 MiB per
    // slice. The time bound is cooperative between bounded Windows queries.
    for (unsigned i = 0; i < 8 && r.epoch.load() == epoch &&
        GetTickCount64() <= r.expires.load() && GetTickCount64() <= deadline; ++i) {
        missing = multiplayer::find_native_types_step(r.queries, r.cursor, 2 * 1024 * 1024, 64);
        if (!missing || r.cursor.complete) break;
        SwitchToThread();
    }
    if (!missing && r.epoch.load() == epoch) {
        std::array<std::uintptr_t, 4> objects{};
        for (unsigned i = 0; i < objects.size(); ++i) objects[i] = r.queries[i].object;
        Read read; Binding binding{};
        if (bind_schema(read, objects, binding) && !r.binding_busy.test_and_set(std::memory_order_acquire)) {
            if (r.epoch.load() == epoch && GetTickCount64() <= r.expires.load()) {
                r.binding = binding;
                r.binding_epoch.store(epoch, std::memory_order_release);
            }
            r.binding_busy.clear(std::memory_order_release);
        }
    }
    // Restart a completed unsuccessful pass later; never expose a partial map.
    if (r.cursor.complete && r.binding_epoch.load() != epoch) r.scan_epoch = 0;
    if (lowered) SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_END);
}
void run_worker(Runtime& r) noexcept {
    std::uint64_t previous_epoch{};
    // Process lifetime matches the existing runtime workers. All formatting and
    // file logging happen here, after the nonblocking interpreter publication.
    for (;;) {
        try {
            const auto now = GetTickCount64(), epoch = r.epoch.load();
            if (epoch != previous_epoch) { previous_epoch = epoch; r.next_scan = 0; }
            const bool active = epoch && now <= r.expires.load();
            if (active && r.binding_epoch.load() != epoch && now >= r.next_scan) {
                scan_slice(r, epoch);
                r.next_scan = GetTickCount64() + 100;
            }
            std::array<Event, 16> rows{};
            const auto count = r.events.take(rows);
            for (std::size_t i = 0; i < count; ++i) {
                const auto& row = rows[i];
                const auto delivered_at = GetTickCount64();
                const auto disposition = native_score_capture_status(row, r.epoch.load(), delivered_at,
                    delivered_at <= r.expires.load());
                if (disposition == ScoreDecision::stale) ++r.stale;
                // Preserve rejected diagnostics, but explicitly mark their
                // freshness. The game thread never formats or waits on this.
                logging::log(logging::Level::info, logging::Channel::skater,
                    "ARCADE NATIVE SCORE GUARD: seq={} epoch={} delivered={} decision={} authoritative=false.",
                    row.serial, row.epoch, delivered_at, static_cast<unsigned>(disposition));
                logging::log(logging::Level::info, logging::Channel::skater,
                    "ARCADE NATIVE SCORE: seq={} ms={} epoch={} route={} score={} reason={} success={} owner_verified=false.",
                    row.serial, row.sampled_at, row.epoch, static_cast<unsigned>(row.route), row.payload.score,
                    row.payload.reason, row.payload.success);
            }
            if (epoch && now >= r.report_at) {
                r.report_at = now + 5000;
                logging::log(logging::Level::info, logging::Channel::skater,
                    "ARCADE NATIVE SCORE STATUS: epoch={} active={} bound={} dropped={} rejected={} contended={} stale={} owner_verified=false.",
                    epoch, active, active && r.binding_epoch.load() == epoch, r.events.dropped(),
                    r.rejected.load(), r.contended.load(), r.stale);
            }
        } catch (...) { /* diagnostic output failure cannot reach the game thread */ }
        Sleep(25);
    }
}
}

void native_score_tick(std::uintptr_t base, std::uint64_t now, bool gameplay) noexcept {
    // Allocate only on client thread, before any interpreter publication.
    try {
        static auto* r = new Runtime;
        published.store(r, std::memory_order_release);
        NoBailSkater owner{};
        if (!gameplay || !no_bail_skater(owner) || owner.base != base) owner = {};
        const auto prior_expiry = r->expires.load();
        if (!same_owner(owner, r->owner) || (prior_expiry && now > prior_expiry) ||
            (r->last_tick && now < r->last_tick)) {
            r->expires.store(0);
            r->epoch.fetch_add(1);
            r->binding_epoch.store(0);
            r->owner = owner;
        }
        r->last_tick = now;
        r->base.store(base);
        r->expires.store(owner.entity ? now + 150 : 0);
        if (!r->worker_started) {
            auto* state = r;
            std::thread([state] { run_worker(*state); }).detach();
            r->worker_started = true;
        }
    } catch (...) { if (auto* r = published.load()) r->expires.store(0); }
}

void observe_native_score_expression(std::uintptr_t vm, std::uint32_t pc) noexcept {
    auto* r = published.load(std::memory_order_acquire);
    if (pc || !vm || !r || !r->expires.load(std::memory_order_relaxed)) return;
    Read read; std::uintptr_t resource{}; std::uint32_t hash{};
    if (!read(vm + 0x38, resource) || !read(resource + 0x10, hash)) return;
    const auto* graph = graph_for(hash);
    if (!graph) return;
    const auto now = GetTickCount64(), epoch = r->epoch.load();
    if (now > r->expires.load() || !epoch || r->binding_epoch.load(std::memory_order_acquire) != epoch) return;
    // The profiled interpreter may enter the ordinary interpreter. Suppress
    // same-VM ancestor entries; exceeding the bounded nesting walk fails closed.
    auto* scope = profile_runtime::expression_scope;
    scope = scope ? scope->parent : nullptr;
    for (unsigned depth = 0; scope; scope = scope->parent) {
        if (depth++ == 32 || scope->vm == vm) return;
    }
    std::uint32_t frame{}, constants{}, words{};
    std::uintptr_t instance{}, back{};
    if (!read(resource + 0x20, frame) || !read(resource + 0x24, constants) || !read(resource + 0x2c, words) ||
        !contract(*graph, frame, constants, words) || !read(vm + 0x30, instance) ||
        !read(instance, back) || back != resource || !client_realm(r->base.load())) { ++r->rejected; return; }
    Binding binding{};
    if (r->binding_busy.test_and_set(std::memory_order_acquire)) { ++r->contended; return; }
    const bool current = r->binding_epoch.load() == epoch && r->epoch.load() == epoch;
    if (current) binding = r->binding;
    r->binding_busy.clear(std::memory_order_release);
    if (!current) return;
    for (unsigned i = 0; i < binding.types.size(); ++i) {
        if (!type_matches(read, binding.types[i], type_hashes[i])) {
            auto expected = epoch;
            (void)r->binding_epoch.compare_exchange_strong(expected, 0);
            ++r->rejected; return;
        }
    }
    Event row{}; row.sampled_at = now; row.epoch = epoch; row.route = graph->route;
    if (graph->route != Route::line_start) {
        std::uintptr_t page{}, payload{};
        if (!read(instance + graph->frame + 8, page) || !read(page + graph->input, payload) ||
            !decode(read, binding, graph->route, payload, row.payload)) { ++r->rejected; return; }
    }
    // No raw addresses, text, or identity leave the callback. Different listener
    // routes can report the same end; this stream deliberately makes no bank claim.
    if (r->epoch.load() == epoch && now <= r->expires.load()) (void)r->events.push(row);
}
}

#include "arcade_native_score.h"
#include "arcade_native_score_buffer.h"
#include "arcade_score_boundary.h"
#include "arcade_score_capture_health.h"
#include "arcade_score_schema_snapshot.h"
#include "arcade_score_binder_diagnostic.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Core/Log/logging.h"
#include "Engine/Game/Build/20260929/engine.h"
#include "Extension/Progression/local_entitlement_trigger_runtime.h"
#include "Extension/Progression/neighborhood_unlock_override.h"
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
    CaptureHealth capture_health;
    std::atomic<std::uint64_t> rejected{}, contended{};
    // Client-thread lifecycle. These addresses are never published or logged.
    NoBailSkater owner{};
    std::uint64_t last_tick{};
    bool worker_started{};
    // Worker-only counters and schedule.
    std::uint64_t next_scan{}, report_at{}, stale{};
    // One worker at a time owns the discovery cursor and partial candidates.
    std::uint64_t scan_epoch{}, hint_epoch{};
    multiplayer::NativeTypeScanCursor cursor{};
    std::array<multiplayer::NativeTypeQuery, 4> queries{};
    SchemaSnapshotGate schema_gate;
    std::uint64_t schema_report_at{}, schema_serial{}, schema_incomplete{};
    std::array<std::array<std::atomic<std::uint64_t>, static_cast<unsigned>(BinderOutcome::count)>, 4> binder_counts{};
    std::atomic<bool> binder_overflow{};
    bool binder_attempted{}, binder_prepared{}, binder_attached{};
    std::uint64_t binder_report_at{};
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
void observe_score_binder(std::uintptr_t serialized, std::uintptr_t, void**) noexcept {
    auto* runtime = published.load(std::memory_order_acquire);
    if (!runtime) return;
    const auto epoch = runtime->epoch.load();
    const auto current = [&] {
        const auto expiry = runtime->expires.load();
        return epoch && expiry && GetTickCount64() <= expiry && runtime->epoch.load() == epoch;
    };
    Read read;
    auto diagnostic = inspect_score_binder(read, serialized, current);
    if (diagnostic.route == Route::none || !client_realm(runtime->base.load())) return;
    if (!current()) diagnostic.outcome = BinderOutcome::stale;
    const auto route = static_cast<unsigned>(diagnostic.route) - 1;
    const auto outcome = static_cast<unsigned>(diagnostic.outcome);
    count_capture_skip(runtime->binder_counts[route][outcome], runtime->binder_overflow);
}
void report_binder(Runtime& runtime, std::uint64_t now, bool active) {
    if (active && !runtime.binder_attempted) {
        runtime.binder_attempted = true;
        runtime.binder_attached = try_attach_expression_binder_observer(&observe_score_binder);
        if (runtime.binder_attached) {
            // Reuse the hook owner's unchanged exact image/data/callsite gates.
            // This diagnostic path never enables the progression override.
            runtime.binder_prepared = prepare_neighborhood_unlock_override(runtime.base.load());
            if (!runtime.binder_prepared) {
                detach_expression_binder_observer(&observe_score_binder);
                runtime.binder_attached = false;
            }
        }
    }
    if (!runtime.binder_attempted || now < runtime.binder_report_at) return;
    const auto sampled_ms = GetTickCount64(), sampled_epoch = runtime.epoch.load();
    const auto sampled_expiry = runtime.expires.load();
    const bool sampled_active = sampled_epoch && sampled_expiry && sampled_ms <= sampled_expiry;
    runtime.binder_report_at = sampled_ms + 5000;
    for (unsigned route = 0; route < runtime.binder_counts.size(); ++route) {
        std::array<std::uint64_t, static_cast<unsigned>(BinderOutcome::count)> counts{};
        for (unsigned i = 0; i < counts.size(); ++i) counts[i] = runtime.binder_counts[route][i].load();
        logging::log(logging::Level::info, logging::Channel::skater,
            "ARCADE NATIVE SCORE BINDER: ms={} epoch={} active={} prepared={} attached={} route={} contract_rejected={} mapping_unreadable={} count_rejected={} layout_rejected={} value_mismatch={} relationship_matches={} unverified={} stale={} overflow={} owner_verified=false.",
            sampled_ms, sampled_epoch, sampled_active, runtime.binder_prepared, runtime.binder_attached, route + 1,
            counts[0], counts[1], counts[2], counts[3], counts[4], counts[5], counts[6], counts[7],
            runtime.binder_overflow.load());
    }
}
bool publish_binding(Runtime& r, std::uint64_t epoch, const Binding& binding) noexcept {
    if (r.binding_busy.test_and_set(std::memory_order_acquire)) return false;
    const bool current = r.epoch.load() == epoch && GetTickCount64() <= r.expires.load();
    if (current) {
        r.binding = binding;
        r.binding_epoch.store(epoch, std::memory_order_release);
    }
    r.binding_busy.clear(std::memory_order_release);
    return current;
}
bool refresh_binding_hint(Runtime& r, std::uint64_t epoch) noexcept {
    if (r.hint_epoch == epoch || r.binding_busy.test_and_set(std::memory_order_acquire)) return false;
    const auto previous = r.binding;
    r.binding_busy.clear(std::memory_order_release);
    r.hint_epoch = epoch;
    Read read; Binding next{};
    return rebind_schema_hint(read, previous, next) && publish_binding(r, epoch, next);
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
        if (bind_schema(read, objects, binding)) publish_binding(r, epoch, binding);
    }
    // Restart a completed unsuccessful pass later; never expose a partial map.
    if (r.cursor.complete && r.binding_epoch.load() != epoch) r.scan_epoch = 0;
    if (lowered) SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_END);
}
void report_schema(Runtime& r, std::uint64_t epoch, std::uint64_t now) {
    if (!epoch || now < r.schema_report_at || r.binding_epoch.load() != epoch ||
        now > r.expires.load()) return;
    Binding binding{};
    if (r.binding_busy.test_and_set(std::memory_order_acquire)) return;
    const bool bound = r.epoch.load() == epoch && r.binding_epoch.load() == epoch;
    if (bound) binding = r.binding;
    r.binding_busy.clear(std::memory_order_release);
    if (!bound) return;
    r.schema_report_at = now + 1000;
    const auto slot = r.schema_gate.begin(binding);
    if (slot == SchemaSnapshotGate::denied) return;
    const auto current = [&] {
        const auto expiry = r.expires.load();
        return expiry && GetTickCount64() <= expiry && r.epoch.load() == epoch &&
            r.binding_epoch.load() == epoch;
    };
    Read read;
    auto snapshot = snapshot_score_schema(read, binding, current);
    const auto serial = ++r.schema_serial;
    unsigned emitted{};
    if (snapshot.status == SchemaSnapshotStatus::complete) {
        for (unsigned role = 0; role < snapshot.types.size(); ++role) {
            const auto& type = snapshot.types[role];
            for (unsigned i = 0; i < type.count; ++i) {
                const auto& member = type.members[i];
                logging::log(logging::Level::info, logging::Channel::skater,
                    "ARCADE NATIVE SCORE SCHEMA FIELD: seq={} epoch={} role={} index={} type_hash={} type_size={} field_hash={} kind={} size={} ref_type_hash={}.",
                    serial, epoch, role + 1, i, type.hash, type.size, member.hash,
                    member.kind, member.size, member.type_hash);
                ++emitted;
            }
        }
    }
    // A missing footer or non-complete status invalidates all rows for this
    // snapshot. Completing a schema snapshot never proves event ownership.
    if (!current()) snapshot.status = SchemaSnapshotStatus::inactive;
    if (snapshot.status != SchemaSnapshotStatus::complete) ++r.schema_incomplete;
    logging::log(logging::Level::info, logging::Channel::skater,
        "ARCADE NATIVE SCORE SCHEMA: seq={} ms={} epoch={} status={} line_count={} sequence_count={} emitted={} incomplete={} owner_verified=false.",
        serial, GetTickCount64(), epoch, static_cast<unsigned>(snapshot.status),
        snapshot.types[0].count, snapshot.types[1].count, emitted, r.schema_incomplete);
    r.schema_gate.finish(slot, snapshot.status);
}
void run_worker(Runtime& r) noexcept {
    std::uint64_t previous_epoch{};
    std::uint64_t health_epoch{}, health_report_at{};
    bool health_active{}, health_bound{};
    // Process lifetime matches the existing runtime workers. All formatting and
    // file logging happen here, after the nonblocking interpreter publication.
    for (;;) {
        try {
            const auto now = GetTickCount64(), epoch = r.epoch.load();
            if (epoch != previous_epoch) { previous_epoch = epoch; r.next_scan = 0; }
            const bool active = epoch && now <= r.expires.load();
            report_binder(r, now, active);
            if (active && r.binding_epoch.load() != epoch && now >= r.next_scan) {
                if (!refresh_binding_hint(r, epoch)) scan_slice(r, epoch);
                r.next_scan = GetTickCount64() + 100;
            }
            report_schema(r, epoch, now);
            std::array<Event, 16> rows{};
            const auto count = r.events.take(rows);
            for (std::size_t i = 0; i < count; ++i) {
                const auto& row = rows[i];
                const auto delivered_at = GetTickCount64();
                const auto delivered_epoch = r.epoch.load();
                const auto expiry = r.expires.load();
                const auto disposition = native_score_capture_status(row, delivered_epoch, delivered_at,
                    expiry && delivered_at <= expiry && delivered_epoch == r.epoch.load());
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
                if (row.route == Route::sequence_end) {
                    const auto& d = row.sequence;
                    logging::log(logging::Level::info, logging::Channel::skater,
                        "ARCADE NATIVE SCORE DETAIL: seq={} scalar_status={} category_status={} sequence_kind={} action_score={} base_score={} landing={} current_category={} authoritative=false.",
                        row.serial, static_cast<unsigned>(d.scalars), static_cast<unsigned>(d.category), d.kind,
                        d.action_score, d.base_score, d.landing, d.current_category);
                }
            }
            if (epoch && now >= r.report_at) {
                r.report_at = now + 5000;
                logging::log(logging::Level::info, logging::Channel::skater,
                    "ARCADE NATIVE SCORE STATUS: epoch={} active={} bound={} dropped={} rejected={} contended={} stale={} owner_verified=false.",
                    epoch, active, active && r.binding_epoch.load() == epoch, r.events.dropped(),
                    r.rejected.load(), r.contended.load(), r.stale);
            }
            // Sampled worker health is not a complete transition journal. These
            // lifetime counters also retain gaps that occurred between samples.
            const auto health_ms = GetTickCount64(), sampled_epoch = r.epoch.load();
            const auto sampled_expiry = r.expires.load();
            const bool sampled_active = sampled_epoch && sampled_expiry && health_ms <= sampled_expiry;
            const bool sampled_bound = sampled_active && r.binding_epoch.load() == sampled_epoch;
            if (sampled_epoch == r.epoch.load() && (sampled_epoch != health_epoch ||
                sampled_active != health_active || sampled_bound != health_bound ||
                (sampled_epoch && health_ms >= health_report_at))) {
                for (const auto& graph : graphs) {
                    const auto skipped = r.capture_health.snapshot(graph.route);
                    logging::log(logging::Level::info, logging::Channel::skater,
                        "ARCADE NATIVE SCORE HEALTH: ms={} epoch={} active={} bound={} route={} unbound={} expired={} changed={} overflow={}.",
                        health_ms, sampled_epoch, sampled_active, sampled_bound, static_cast<unsigned>(graph.route),
                        skipped.unbound, skipped.expired, skipped.changed, r.capture_health.overflow());
                }
                health_epoch = sampled_epoch; health_active = sampled_active; health_bound = sampled_bound;
                health_report_at = health_ms + 5000;
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
    if (now > r->expires.load() || !epoch) {
        r->capture_health.skip(graph->route, CaptureSkip::expired); return;
    }
    if (r->binding_epoch.load(std::memory_order_acquire) != epoch) {
        r->capture_health.skip(graph->route, CaptureSkip::unbound); return;
    }
    // The profiled interpreter may enter the ordinary interpreter. Suppress
    // same-VM ancestor entries; exceeding the bounded nesting walk fails closed.
    auto* scope = profile_runtime::expression_scope;
    scope = scope ? scope->parent : nullptr;
    if (!capture_scope_allowed(scope, vm, r->rejected)) return;
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
    if (!current) { r->capture_health.skip(graph->route, CaptureSkip::changed); return; }
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
        if (graph->route == Route::sequence_end) row.sequence = decode_sequence_diagnostics(read, binding, payload);
    }
    // No raw addresses, text, or identity leave the callback. Different listener
    // routes can report the same end; this stream deliberately makes no bank claim.
    if (r->epoch.load() == epoch && now <= r->expires.load()) (void)r->events.push(row);
    else r->capture_health.skip(graph->route, CaptureSkip::changed);
}
}

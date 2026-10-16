#include "Extension/ArcadeMode/arcade_fixture_geometry.h"
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
using namespace dingosdk::arcade_mode;
unsigned checks{};
void check(bool value, const char* message) {
    ++checks; if (!value) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
FixtureGeometryContext context() { return {{1, 2, 3, 4, 5}, 2, 7, 11, 31, true}; }
template<class Hit> FixtureGeometry measure(Hit hit) {
    FixtureGeometry g; const auto c = context();
    check(g.begin(c, 100), "valid diagnostic starts");
    for (unsigned i = 0; i < FixtureGeometry::samples; ++i) {
        const auto ray = g.next(c, 100 + i * 16);
        check(ray && ray->index == i, "one ordered bounded ray per response");
        check(!g.next(c, 100 + i * 16), "unconsumed query cannot be overwritten");
        const bool ours = hit(*ray);
        g.accept(i, true, true, ours ? c.object : 0, ours ? .75f : 0.f);
    }
    check(!g.next(c, 2200) && g.count == FixtureGeometry::samples, "total ray budget cannot grow");
    return g;
}
}
int main() {
    using namespace dingosdk::arcade_mode;
    const auto c = context();
    auto g = measure([](auto ray) { return std::abs(ray.x) <= .25f && std::abs(ray.z) <= 3.f; });
    check(g.status == GeometryStatus::complete && g.low[0] == -.25f && g.high[0] == .25f &&
        g.low[1] == -3.f && g.high[1] == 3.f && g.height_min == .75f && g.height_max == .75f,
        "collision samples retain both cross sections and measured height");
    check(g.result_current(c), "completed result remains associated with its exact original context");
    for (unsigned change = 0; change < 5; ++change) {
        auto stale = c;
        if (change == 0) ++stale.generation;
        if (change == 1) ++stale.revision;
        if (change == 2) ++stale.owner[0];
        if (change == 3) ++stale.physics_changes;
        if (change == 4) stale.valid = false;
        check(!g.result_current(stale), "completed geometry is never current after lifecycle or validity changes");
    }
    g.cancel(); check(!g.result_current(c), "explicit reset/cancel also retires completed geometry");
    auto clipped = measure([](auto) { return true; });
    check(clipped.status == GeometryStatus::clipped, "hits at scan boundary are not complete dimensions");
    auto hole = measure([](auto ray) { return std::abs(ray.x) <= .25f &&
        std::abs(ray.z) <= 3.f && ray.z != 1.f; });
    check(hole.status == GeometryStatus::discontinuous, "separated collision islands cannot imply solid rail extents");
    for (unsigned change = 0; change < 10; ++change) {
        FixtureGeometry pending; check(pending.begin(c, 100) && pending.next(c, 100).has_value(), "pending fixture sample");
        auto changed = c;
        if (change < 5) ++changed.owner[change];
        if (change == 5) ++changed.generation;
        if (change == 6) ++changed.revision;
        if (change == 7) ++changed.object;
        if (change == 8) ++changed.physics_changes;
        if (change == 9) changed.valid = false;
        check(!pending.current(changed, 116) && pending.status == GeometryStatus::cancelled,
            "owner, world, edit, object, reset and invalid context cancel pending diagnostic");
        pending.accept(0, true, true, c.object, .75f);
        check(!pending.count && !pending.next(c, 132), "cancelled response cannot restart or publish a sample");
    }
    FixtureGeometry pending; check(pending.begin(c, 100), "timeout fixture begins");
    check(pending.next(c, 100).has_value() && !pending.begin(c, 110), "active run cannot be restarted to extend budget");
    pending.accept(1, true, true, c.object, .75f);
    check(!pending.count && pending.pending(), "wrong response index cannot advance measurement");
    check(pending.current(c, 350) && !pending.current(c, 351) && pending.status == GeometryStatus::timeout,
        "response timeout retains exact protective bound");
    check(pending.begin(c, 500) && !pending.current(c, 499) && pending.status == GeometryStatus::cancelled,
        "backward monotonic clock cancels");
    check(pending.begin(c, 1000) && !pending.current(c, 6001) && pending.status == GeometryStatus::timeout,
        "whole diagnostic has a five-second wall budget");
    for (unsigned failure = 0; failure < 5; ++failure) {
        FixtureGeometry bad; check(bad.begin(c, 100) && bad.next(c, 100).has_value(), "invalid result fixture begins");
        bad.accept(0, failure != 0, failure != 1, failure == 2 ? 22 : failure == 3 ? 0 : c.object,
            failure == 4 ? std::numeric_limits<float>::quiet_NaN() : .75f);
        check(bad.status == GeometryStatus::unavailable && !bad.count,
            "unavailable, missing center, foreign/unknown center identity and nonfinite height fail closed");
    }
    FixtureGeometry inactive; auto invalid = c; invalid.valid = false;
    check(!inactive.begin(invalid, 100) && inactive.status == GeometryStatus::idle,
        "unverified initial context leaves feature inactive");
    check(inactive.begin(c, 100), "explicit start after invalid request remains possible");
    inactive.unavailable(); check(!inactive.next(c, 101), "queue rejection cannot enqueue further rays");
    FixtureGeometry armed;
    check(armed.arm(c, 100) && !armed.arm(c, 200), "armed request cannot renew its deadline");
    armed.activate(c, 200, false);
    check(armed.status == GeometryStatus::armed && !armed.next(c, 200) && !armed.count,
        "console-visible armed state performs no native query");
    armed.activate(c, 1000, true);
    check(armed.status == GeometryStatus::sampling && armed.next(c, 1000).has_value(),
        "fresh gameplay admits first ray after console closure");
    armed.cancel(); check(armed.arm(c, 2000), "cancel permits an explicit new request");
    armed.activate(c, 7001, true);
    check(armed.status == GeometryStatus::timeout && !armed.next(c, 7001),
        "late console closure cannot revive an expired arm");
    for (unsigned change = 0; change < 5; ++change) {
        FixtureGeometry waiting; check(waiting.arm(c, 100), "new guarded armed fixture");
        auto invalid_context = c;
        if (change == 0) ++invalid_context.generation;
        if (change == 1) ++invalid_context.owner[1];
        if (change == 2) ++invalid_context.physics_changes;
        if (change == 3) invalid_context.valid = false;
        waiting.activate(invalid_context, change == 4 ? 99 : 116, true);
        check(waiting.status == GeometryStatus::cancelled && !waiting.next(c, 132),
            "armed owner/reset/invalid-context/clock changes cancel before any ray");
    }
    std::cout << checks << " fixture geometry checks passed\n";
}

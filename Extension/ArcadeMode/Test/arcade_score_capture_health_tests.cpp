#include "../arcade_score_capture_health.h"
#include <cstdlib>
#include <iostream>
#include <thread>

using namespace dingosdk::arcade_mode::native_score_detail;
namespace {
unsigned checks{};
void check(bool value, const char* message) {
    ++checks;
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}
}
int arcade_score_capture_health_tests() {
    CaptureHealth health;
    for (const auto& graph : graphs) {
        const auto empty = health.snapshot(graph.route);
        check(!empty.unbound && !empty.expired && !empty.changed, "each route starts empty");
        check(health.skip(graph.route, CaptureSkip::unbound), "known route counted");
    }
    check(!health.skip(Route::none, CaptureSkip::unbound), "none is not a scoring route");
    check(!health.skip(static_cast<Route>(255), CaptureSkip::unbound), "invalid route rejected");
    check(!health.skip(Route::line_start, static_cast<CaptureSkip>(255)), "invalid reason rejected");
    health.skip(Route::line_end, CaptureSkip::expired);
    health.skip(Route::sequence_end, CaptureSkip::changed);
    for (const auto& graph : graphs) {
        const auto row = health.snapshot(graph.route);
        check(row.unbound == 1 && row.expired == (graph.route == Route::line_end) &&
              row.changed == (graph.route == Route::sequence_end), "route and reason isolation");
    }
    // Snapshotting does not consume gaps or reset them at an observer epoch.
    check(health.snapshot(Route::line_start).unbound == 1, "snapshot retains cumulative gaps");
    std::array<std::thread, 4> writers;
    for (auto& writer : writers) writer = std::thread([&] {
        for (unsigned i = 0; i < 10000; ++i) health.skip(Route::line_start, CaptureSkip::unbound);
    });
    for (auto& writer : writers) writer.join();
    check(health.snapshot(Route::line_start).unbound == 40001, "concurrent hook skips are not lost");
    check(!health.overflow(), "ordinary increments preserve health");
    std::atomic<std::uint64_t> counter{UINT64_MAX - 1};
    std::atomic<bool> overflow{};
    count_capture_skip(counter, overflow);
    check(counter.load() == UINT64_MAX && !overflow.load(), "last representable count remains exact");
    count_capture_skip(counter, overflow);
    check(!counter.load() && overflow.load(), "wrap explicitly invalidates counter interpretation");
    count_capture_skip(counter, overflow);
    check(counter.load() == 1 && overflow.load(), "overflow uncertainty remains sticky");
    std::cout << checks << " score capture health checks passed\n";
    return 0;
}
#ifndef ARCADE_CAPTURE_HEALTH_COMBINED_TESTS
int main() { return arcade_score_capture_health_tests(); }
#endif

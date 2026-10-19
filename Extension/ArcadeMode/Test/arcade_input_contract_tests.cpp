#include "Extension/ArcadeMode/arcade_input_events.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value, const char* message) {
    ++checks;
    if (!value) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
constexpr InputToken a{InputSource::buttons, 12, InputEdge::press};
constexpr InputToken b{InputSource::buttons, 13, InputEdge::press};
constexpr InputToken x{InputSource::buttons, 14, InputEdge::press};
constexpr InputToken y{InputSource::buttons, 15, InputEdge::press};
constexpr InputToken left{InputSource::stick, 4, InputEdge::press};
constexpr InputToken right{InputSource::stick, 6, InputEdge::press};
constexpr InputToken diagonal{InputSource::stick, 7, InputEdge::press};
constexpr InputToken release(InputToken token) { token.edge = InputEdge::release; return token; }
void air_queries() {
    for (unsigned age : {499u, 500u, 501u}) {
        InputHistory h; h.record(left, 0); h.record(x, 1000);
        const auto direction_used = h.match({std::span<const InputToken>(&left, 1), 2000}, 1000);
        check(h.consume(direction_used, 8), "prior owner may consume held direction");
        const auto match = h.air_action(x, left, true, 500, 1000 + age);
        check(static_cast<bool>(match) == (age < 500), "held directional action tests fresh face age, not old held-edge age");
        if (match) {
            check(match.count == 2 && match.held_reuse == h.events()[0].sequence && h.consume(match, 1),
                "held direction is explicitly reusable while fresh face ownership remains transactional");
            check(h.events()[0].owners == 9 && h.events()[1].owners == 1, "air query exact category footprint");
            check(!h.air_action(x, left, true, 500, 1000 + age), "consumed face cannot repeat from held direction alone");
        }
    }
    InputHistory baseline; baseline.record(x, 20);
    const auto held = baseline.air_action(x, left, true, 500, 20);
    check(held.count == 1 && baseline.consume(held, 1), "held startup direction needs no fabricated historical press");
    for (unsigned position = 0; position < 3; ++position) {
        InputHistory h;
        if (position == 0) h.record(y, 0);
        h.record(left, 10);
        if (position == 1) h.record(y, 20);
        h.record(x, 30);
        if (position == 2) h.record(y, 40);
        const auto query = h.air_action(x, left, false, 500, 50);
        check(static_cast<bool>(query) == (position != 1), "released direction branch rejects only intervening unrelated press");
        if (!query) for (const auto& event : h.events()) check(event.owners == 0, "failed air query commits no ownership");
    }
}
void release_queries() {
    for (unsigned age : {199u, 200u, 201u}) {
        InputHistory h; h.record(release(a), 0); h.record(b, 20); h.record(release(b), 40);
        check(static_cast<bool>(h.release_and_tap(a, b, 200, age)) == (age < 200), "release-and-tap strict oldest-event boundary");
    }
    for (const auto inserted : {left, right, diagonal, x}) {
        InputHistory h; h.record(release(a), 0); h.record(b, 20); h.record(inserted, 30); h.record(release(b), 40);
        const auto query = h.release_and_tap(a, b, 200, 50);
        const bool permitted = inserted == left || inserted == right;
        check(static_cast<bool>(query) == permitted, "release-and-tap filters cardinals but keeps diagonal and face interventions");
        if (query) {
            check(query.count == 3 && h.consume(query, 4), "release-and-tap consumes only the three selected IDs");
            check(h.events()[2].owners == 0, "ignored cardinal remains available to another category");
        }
    }
    for (bool final_press : {false, true}) for (bool repress : {false, true}) {
        InputHistory h;
        h.record(release(a), 0);
        if (repress) h.record(b, 10);
        h.record(release(b), 20);
        if (final_press) h.record(x, 30);
        const auto query = h.released_pair(a, b, 200, 40, x, final_press);
        check(static_cast<bool>(query) == !repress, "released pair cannot cross the latest repress barrier");
        if (query) check(query.count == (final_press ? 3u : 2u) && h.consume(query, 2), "released pair exact selected event footprint");
        else for (const auto& event : h.events()) check(event.owners == 0, "failed release query is atomic");
    }
}
void extra_queries() {
    for (unsigned age : {299u, 300u, 301u}) {
        InputHistory h; h.record(left, 0); h.record(x, 20);
        const auto query = h.extra_action(x, left, 300, age);
        check(static_cast<bool>(query), "fresh extra face can survive expiry of an older matching direction");
        check(query.count == (age < 300 ? 2u : 1u), "extra direction footprint obeys its own strict age");
    }
    for (bool wrong : {false, true}) {
        InputHistory h; h.record(left, 0); h.record(x, 20); h.record(wrong ? right : left, 40); h.record(x, 60);
        const auto query = h.extra_action(x, left, 300, 80);
        check(static_cast<bool>(query) == !wrong, "extra matching rejects any fresh different direction without killing events");
        if (query) {
            check(query.count == 3 && h.consume(query, 16), "extra query consumes all matching directions and one face");
            check(h.events()[1].owners == 16 && h.events()[3].owners == 0,
                "extra query preserves the specified oldest eligible face selection quirk");
        } else for (const auto& event : h.events()) check(event.owners == 0, "failed extra query leaves its footprint reusable");
    }
}
}
int main() {
    air_queries(); release_queries(); extra_queries();
    std::cout << checks << " source input contract checks passed\n";
}

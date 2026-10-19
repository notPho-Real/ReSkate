#include "Extension/ArcadeMode/arcade_input_dispatch.h"
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value, const char* reason) {
    ++checks;
    if (!value) { std::cerr << "FAIL: " << reason << '\n'; std::exit(1); }
}
constexpr InputToken a{InputSource::buttons, 12, InputEdge::press};
constexpr InputToken b{InputSource::buttons, 13, InputEdge::press};
constexpr InputToken c{InputSource::buttons, 14, InputEdge::press};
constexpr InputToken z{InputSource::buttons, 15, InputEdge::press};
constexpr InputToken release(InputToken token) { token.edge = InputEdge::release; return token; }
struct Adapter {
    unsigned calls{};
    InputRequest last{};
    InputDisposition result{InputDisposition::started};
    static InputDisposition receive(void* state, const InputRequest& request) noexcept {
        auto& self = *static_cast<Adapter*>(state); ++self.calls; self.last = request; return self.result;
    }
    InputAdapter bind() { return {this, receive}; }
};
void journal_tests() {
    InputHistory history;
    const std::array pair{a, b};
    history.record(a, 0); history.record(z, 40); history.record(b, 80);
    check(static_cast<bool>(history.match({pair, 200}, 199)), "unrelated intervening press is permitted");
    check(!history.match({pair, 200}, 200), "query age rejects exact strict boundary");
    check(!history.match({pair, 200, ~std::uint32_t{0}, false}, 80), "adjacent policy rejects unrelated press");
    check(static_cast<bool>(history.match({pair, 200, ~std::uint32_t{0}, true, false, true}, 200)), "inclusive adaptation is explicit");
    const auto match = history.match({pair, 200}, 80);
    check(history.consume(match, 1), "manual category consumes match atomically");
    check(!history.match({pair, 200}, 80), "ordinary mask rejects consumed events");
    const auto reuse = history.match({pair, 200, ~std::uint32_t{1}}, 80);
    check(reuse && history.consume(reuse, 2, ~std::uint32_t{1}), "allowed manual prefix reuse adds category");
    check(!history.match({pair, 200, 2}, 80), "additional regular category prevents reuse");
    history.restore(1, 200, 200);
    check((history.events()[0].owners & 1) && !(history.events()[2].owners & 1), "restore is strict and category selective");
    history.reset(); history.record(a, 0); history.record(b, 0);
    check(static_cast<bool>(history.match({pair, 1}, 0)), "same-time forward ordinals match");
    history.reset(); history.record(b, 0); history.record(a, 0);
    check(!history.match({pair, 1}, 0), "same-time reversed ordinals do not match");
    history.reset(); history.record(a, 0); history.record(b, 40); history.record(c, 80);
    check(static_cast<bool>(history.triple(a, b, c, 200, 0, 80)), "ordered triple accepts three presses");
    check(!history.triple(a, b, c, 200, 1, 80), "strict triple requires releases");
    check(!history.triple(a, b, c, 200, 2, 80), "alternate-tail triple requires first release");
    history.reset(); history.record(a, 0); history.record(release(a), 20);
    history.record(b, 40); history.record(release(b), 60); history.record(c, 80);
    check(static_cast<bool>(history.triple(a, b, c, 200, 1, 80)), "strict triple accepts releases before following presses");
    history.reset(); history.record(a, 0); history.record(release(a), 20); history.record(c, 40); history.record(b, 80);
    check(static_cast<bool>(history.triple(a, b, c, 200, 2, 80)), "alternate-tail triple permits reversed final presses");
    check(!history.triple(a, b, c, 200, 0, 80), "ordered triple cannot reverse final presses");
    history.reset(); history.record(a, 0); history.record(z, 20); history.record(b, 40);
    check(!history.overlap(a, b, 200, 40, true, true, true), "current-hold pair rejects intervening unrelated press");
    check(static_cast<bool>(history.overlap(a, b, 200, 40)), "historical overlap permits unrelated press");
    history.reset(); history.record(a, 0); history.record(b, 40); history.record(z, 80);
    check(static_cast<bool>(history.overlap(a, b, 200, 80, true, true, true)), "current-hold pair ignores newer unrelated press");
    InputHistory held_pair;
    held_pair.record(a, 0); held_pair.record(b, 20); held_pair.record(release(b), 40); held_pair.record(b, 60);
    check(static_cast<bool>(held_pair.overlap(a, b, 200, 60, true, true, true)),
        "current-hold matching ignores intervening releases when current state proves both held");
    history.record(release(a), 90);
    check(!history.overlap(a, b, 200, 90, true, false, true), "current-hold pair requires both currently held");
    check(static_cast<bool>(history.overlap(a, b, 200, 90)), "historical overlap survives later release");
    history.reset(); history.record(a, 0); history.record(release(a), 20); history.record(b, 40);
    check(!history.overlap(a, b, 200, 40), "release separating presses prevents overlap");
    InputHistory historical_hold;
    historical_hold.record(a, 0); historical_hold.record(b, 20); historical_hold.record(c, 40);
    historical_hold.record(release(a), 80);
    check(static_cast<bool>(historical_hold.held_pair_then_press(a, b, c, 200, 80)),
        "held pair is evaluated at the final press, not at current released state");
    historical_hold.reset(); historical_hold.record(a, 0); historical_hold.record(release(a), 20);
    historical_hold.record(b, 40); historical_hold.record(c, 80);
    check(!historical_hold.held_pair_then_press(a, b, c, 200, 80), "release before final press blocks historical hold");
    history.reset(); history.record(a, 0);
    check(!history.match({pair, 200}, 0) && history.events()[0].owners == 0, "failed query is atomic");
    for (std::uint64_t i = 1; i <= 100; ++i) history.record(b, i);
    check(history.events().size() == InputHistory::capacity && history.overflows() == 1, "journal capacity and loss counter bounded");
    check(!history.consume(match, 1), "stale match cannot consume overwritten identities");
    history.advance(2000); check(!history.match({pair, 200}, 2000), "old events remain stored but cannot match expired windows");
    history.record(a, 2001); check(!history.advance(1000) && history.events().empty(), "clock rollback clears continuity");
}
void overlap_footprint_tests() {
    for (bool current : {false, true}) {
        InputHistory h;
        h.record(a, 0); h.record(b, 20); h.record(release(b), 30); h.record(b, 40);
        const auto match = h.overlap(a, b, 200, 40, current, true, true);
        check(match.count == (current ? 3u : 4u), "overlap footprint retains all encountered relevant transitions");
        check(h.consume(match, 4), "complete overlap footprint consumes atomically");
        for (std::size_t i = 0; i < h.events().size(); ++i)
            check(h.events()[i].owners == ((!current || i != 2) ? 4u : 0u),
                "overlap consumes the exact press/release ID set for its predicate");
        h.record(release(a), 50); h.record(a, 60);
        check(!h.overlap(a, b, 200, 60, current, true, true),
            "previously encountered repeated press cannot be reused after another button rearms");
    }
    InputHistory full;
    full.record(a, 0);
    for (unsigned i = 1; i < 100; ++i) full.record(b, i);
    const auto match = full.overlap(a, b, 200, 100, true, true, true);
    check(match.count == 100 && full.consume(match, 8), "overlap footprint can span the entire bounded journal");
    for (const auto& event : full.events()) check(event.owners == 8, "full footprint includes every encountered repeated press ID");
}

void normalization_tests() {
    InputEvents input;
    input.sample(0, {0x1001, 0, 25000, 255, 255});
    input.sample(10, {0x1001, 0, 25000, 255, 255});
    check(input.history().events().empty(), "held startup input is baseline, not press");
    input.sample(20, {});
    check(input.history().events().size() == 6, "one general direction, two explicit Manual projections and three physical controls release once");
    input.discard_intent(); input.sample(30, {});
    check(input.history().events().empty(), "discarding intent retains physical baseline");
    input.sample(40, {0x4000, 25000, 25000, 30, 128});
    check(input.history().events().size() == 3, "diagonal is one token and trigger thresholds remain strict");
    input.sample(50, {0x4000, 25000, 25000, 31, 129});
    check(input.history().events().size() == 5, "triggers rise at their independent thresholds");
    input.sample(60, {0x4000, 25000, 25000, 31, 129});
    check(input.history().events().size() == 5, "held samples do not duplicate events");
    input.reset(); input.sample(100, {}); input.sample(110, {0x0001, 0, 25000}, true);
    const std::array<InputToken, 3> held{{{InputSource::buttons, 0, InputEdge::press},
        {InputSource::stick, InputEvents::encode(0, 1), InputEdge::press},
        {InputSource::vertical_stick, 1, InputEdge::press}}};
    check(input.history().events().empty() && input.held_all(held),
        "ignored balance controls retain current held state without adding history");
    input.sample(1000, {0x0001, 0, 25000}, true);
    check(input.held_all(held), "held-state predicates do not require fresh historical edges");

}
void queue_tests() {
    InputRequestQueue queue;
    Adapter adapter;
    unsigned reports{};
    auto report = [&](InputDispatchEvent) { ++reports; };
    check(queue.push({InputAction::manual, InputContext::air, 100, 800, 1, 0, 1, false, true}), "manual queue accepts recognized time");
    adapter.result = InputDisposition::waiting;
    queue.dispatch(800, 1, InputContext::ground, adapter.bind(), report);
    check(adapter.calls == 1 && !reports, "manual pending remains eligible at exact lifetime");
    queue.dispatch(801, 1, InputContext::ground, adapter.bind(), report);
    check(adapter.calls == 1 && reports == 1, "manual expires after lifetime without extending it on landing");
    queue.push({InputAction::flip, InputContext::air, 0, 200, 1});
    queue.dispatch(200, 1, InputContext::air, adapter.bind(), report);
    check(adapter.calls == 1 && reports == 2, "readiness expiry is half-open");
    queue.push({InputAction::grab, InputContext::air, 300, 500, 1});
    queue.dispatch(301, 2, InputContext::air, adapter.bind(), report);
    check(adapter.calls == 1 && reports == 3, "session changes cancel queued work");
    for (std::size_t i = 0; i < InputRequestQueue::capacity; ++i)
        check(queue.push({InputAction::flip, InputContext::ground, 0, 1000, 2}), "ordinary FIFO accepts distinct repeated requests");
    check(!queue.push({InputAction::rail_request, InputContext::ground, 0, 1000, 2}), "full ordinary FIFO rejects newest request");
    check(queue.push({InputAction::manual, InputContext::air, 0, 700, 2}) &&
          queue.push({InputAction::nose_manual, InputContext::air, 1, 701, 2}) &&
          queue.slots()[0].action == InputAction::nose_manual, "manual selection replaces its separate pending slot even when ordinary queue is full");
    queue.clear_ordinary();
    std::size_t ordinary{};
    for (std::size_t i = 1; i < queue.slots().size(); ++i) ordinary += queue.slots()[i].action != InputAction::none;
    check(ordinary == 0 && queue.slots()[0].action == InputAction::nose_manual,
        "table installation clears ordinary pending work independently of the manual selection");

    queue.clear(); check(queue.slots()[0].action == InputAction::none, "clear empties pending queue");
}
void queue_lifecycle_tests() {
    struct Reentrant {
        InputRequestQueue* queue;
        std::array<unsigned, 4> order{};
        unsigned calls{};
        bool clear{}, observed_removed{};
        static InputDisposition receive(void* state, const InputRequest& request) noexcept {
            auto& self = *static_cast<Reentrant*>(state);
            self.order[self.calls++] = request.presses;
            self.observed_removed = true;
            for (const auto& pending : self.queue->slots())
                if (pending.action != InputAction::none && pending.presses == request.presses) self.observed_removed = false;
            if (self.clear) { self.queue->clear(); return InputDisposition::waiting; }
            return InputDisposition::started;
        }
    };
    InputRequestQueue queue; Reentrant adapter{&queue};
    for (unsigned i = 1; i <= 3; ++i)
        queue.push({InputAction::flip, InputContext::air, 10, 100, 1, 0, i});
    unsigned reports{};
    const auto report = [&](InputDispatchEvent) { ++reports; };
    for (unsigned now = 11; now <= 13; ++now)
        queue.dispatch(now, 1, InputContext::air, {&adapter, &Reentrant::receive}, report);
    check(adapter.calls == 3 && adapter.order[0] == 1 && adapter.order[1] == 2 && adapter.order[2] == 3,
        "ordinary FIFO preserves insertion order across native ownership polls");
    check(adapter.observed_removed && reports == 3, "request is removed before native callback boundary");
    adapter.clear = true;
    queue.push({InputAction::flip, InputContext::air, 20, 100, 1, 0, 4});
    queue.dispatch(21, 1, InputContext::air, {&adapter, &Reentrant::receive}, report);
    queue.dispatch(22, 1, InputContext::air, {&adapter, &Reentrant::receive}, report);
    check(adapter.calls == 4 && reports == 4, "callback cancellation cannot resurrect a waiting request");
    queue.push({InputAction::manual, InputContext::air, 30, 730, 1});
    queue.dispatch(31, 1, InputContext::rail, {}, report);
    check(reports == 5 && queue.slots()[0].action == InputAction::none, "grind context cancels pending manual rather than extending landing admission");
}

void queue_action_cancellation_tests() {
    struct Cancelling {
        InputRequestQueue* queue;
        InputAction target;
        bool replace;
        unsigned calls{};
        static InputDisposition receive(void* state, const InputRequest& request) noexcept {
            auto& self = *static_cast<Cancelling*>(state);
            ++self.calls;
            if (self.calls > 1) return InputDisposition::waiting;
            self.queue->cancel(self.target);
            if (self.replace) {
                auto replacement = request;
                replacement.recognized_at = 30;
                self.queue->push(replacement);
            }
            return InputDisposition::waiting;
        }
    };
    for (InputAction action : {InputAction::grab, InputAction::manual, InputAction::rail_select})
    for (bool same_action : {false, true}) for (bool replace : {false, true}) {
        InputRequestQueue queue;
        const auto unrelated = action == InputAction::manual ? InputAction::nose_manual : InputAction::flip;
        Cancelling sink{&queue, same_action ? action : unrelated, replace};
        queue.push({action, InputContext::air, 10, 100, 1});
        unsigned cancelled{};
        queue.dispatch(20, 1, InputContext::air, {&sink, Cancelling::receive},
            [&](InputDispatchEvent event) { cancelled += event.disposition == InputDisposition::cancelled; });
        unsigned original{}, replacement{};
        for (const auto& request : queue.slots()) if (request.action == action) {
            original += request.recognized_at == 10;
            replacement += request.recognized_at == 30;
        }
        const bool dedicated = action != InputAction::grab;
        check(original == (same_action || (dedicated && replace) ? 0u : 1u), "action cancellation during a waiting callback cannot restore cancelled intent or discard unrelated intent");
        check(replacement == (replace ? 1u : 0u), "cancellation preserves replacement requests inserted after the boundary");
        check(cancelled == (same_action || (dedicated && replace) ? 1u : 0u), "cancelled or replaced in-flight intent is reported exactly once");
        if (same_action) check(sink.calls == (replace && !dedicated ? 2u : 1u), "a cancelled request cannot reenter the adapter; an ordinary replacement can advance immediately");
    }
}

void router_tests() {
    UnifiedInput input;
    Adapter adapter;
    const InputEnvironment ground{InputContext::ground, true};
    const InputEnvironment air{InputContext::air, true};
    auto sample = [&](std::uint64_t now, PhysicalInputSample pad, InputEnvironment env) {
        input.poll(now, pad, env, adapter.bind());
    };
    sample(0, {}, ground); sample(10, {0x1000}, ground);
    check(adapter.last.action == InputAction::crouch, "ground A enters crouch context");
    sample(20, {}, ground);
    check(adapter.last.action == InputAction::ollie_release, "fresh A release requests native ollie release");
    sample(30, {0x4000, -25000, 0}, air);
    check(adapter.last.action == InputAction::flip && adapter.last.presses == 1, "air flip needs no held A");
    sample(40, {}, air); sample(50, {0x4000, -25000, 0}, air);
    check(adapter.last.presses == 2, "fresh repeated flip press advances variant");
    sample(60, {}, air); sample(70, {0x2000}, air);
    check(adapter.last.action == InputAction::grab, "air B requests grab separately");
    input.reset(); sample(100, {}, ground); sample(110, {0, 0, 25000}, ground);
    sample(310, {0, 0, -25000}, ground);
    check(adapter.last.action == InputAction::manual, "ground manual pair reaches adapter");
    input.reset(); const auto calls = adapter.calls;
    sample(400, {0x4000}, ground); sample(410, {0x4000}, air);
    check(adapter.calls == calls, "held face button through context transition is not fresh");
    input.reset(); sample(500, {}, ground); sample(510, {0x2000, 0, 25000}, {InputContext::ground, true, true});
    sample(520, {0, 0, 25000}, ground); sample(530, {0, 0, -25000}, ground);
    check(adapter.calls == calls, "held direction through rejected gap cannot become first press");
    input.reset(); sample(600, {}, ground); sample(610, {0x2000}, {InputContext::ground, true, true});
    sample(620, {0, 0, 25000}, ground); sample(820, {0, 0, -25000}, ground);
    check(adapter.last.action == InputAction::manual, "brake release with same-sample direction preserves fresh edge");
    input.reset(); adapter.result = InputDisposition::unsupported;
    const InputEnvironment balance{InputContext::balance, true};
    sample(900, {}, balance); sample(910, {0x4000}, balance); sample(920, {}, balance); sample(930, {0x8000}, balance);
    check(adapter.last.action == InputAction::balance_branch_one &&
        input.diagnostic().dispatch[0].disposition == InputDisposition::unsupported, "unsupported contextual branch is explicit");
    sample(940, {}, {InputContext::balance, true, false, false, 1});
    sample(950, {0x8000}, {InputContext::balance, true, false, false, 1});
    check(adapter.last.action == InputAction::balance_branch_one, "consumed branch prefix cannot accidentally advance context");
    sample(960, {}, {InputContext::balance, true, false, false, 1});
    sample(970, {0x8000}, {InputContext::balance, true, false, false, 1});
    check(adapter.last.action == InputAction::balance_branch_two, "observer-admitted branch table accepts two fresh presses");
    const auto count = adapter.calls;
    sample(980, {}, {InputContext::unknown, false}); sample(990, {0x8000}, air);
    check(adapter.calls == count, "OFF restoration cancels and rebaselines held commands");
}
void deferred_recognition_tests() {
    UnifiedInput input; Adapter adapter;
    InputEnvironment air{InputContext::air, true}; air.deferred_grabs = true;
    input.poll(0, {}, air, adapter.bind());
    input.poll(10, {0x2000, 25000, 0}, air, adapter.bind());
    input.poll(100, {0, 25000, 0}, air, adapter.bind());
    input.poll(150, {0x2000, 25000, 0}, air, adapter.bind());
    input.poll(329, {0x2000, 25000, 0}, air, adapter.bind());
    check(adapter.calls == 0, "deferred repeated grab waits from the final fresh press");
    input.poll(330, {0x2000, 25000, 0}, air, adapter.bind());
    check(adapter.calls == 1 && adapter.last.presses == 2 && adapter.last.recognized_at == 150 && adapter.last.ready_at == 330,
        "deferred grab preserves repeated selection and separate recognition/readiness clocks");
    input.poll(800, {0x2000, 25000, 0}, air, adapter.bind());
    check(adapter.calls == 1, "held deferred grab cannot repeat after queue removal");
    input.reset(); adapter.calls = 0;
    input.poll(900, {}, air, adapter.bind()); input.poll(910, {0x2004}, air, adapter.bind());
    input.poll(930, {4}, air, adapter.bind()); input.poll(1160, {0x2004}, air, adapter.bind());
    check(adapter.calls == 1 && adapter.last.recognized_at == 910,
        "late new press flushes completed prior selection before starting a fresh command");
    input.poll(1340, {4}, air, adapter.bind());
    check(adapter.calls == 2 && adapter.last.recognized_at == 1160,
        "late new press also retains its independent pending command");
    input.reset(); adapter.calls = 0;
    input.poll(1000, {}, air, adapter.bind()); input.poll(1010, {0x2000}, air, adapter.bind());
    input.poll(1100, {}, {InputContext::ground, true}, adapter.bind());
    input.poll(1300, {}, air, adapter.bind());
    check(adapter.calls == 0, "landing cancels deferred selection before native readiness");
}

void sampled_replay_tests() {
    for (const unsigned step : {17u, 34u}) for (unsigned phase = 0; phase < step; ++phase)
        for (const unsigned gap : {150u, 220u, 250u}) for (const bool pad : {false, true})
            for (const int sign : {-1, 1}) {
                UnifiedInput input; Adapter adapter;
                const InputEnvironment ground{InputContext::ground, true};
                for (unsigned now = phase; now < 600; now += step) {
                    const int dir = now < 100 ? 0 : now < 100 + gap ? sign : -sign;
                    PhysicalInputSample sample{};
                    if (pad) sample.buttons = static_cast<std::uint16_t>(dir > 0 ? 1 : dir < 0 ? 2 : 0);
                    else sample.y = static_cast<std::int16_t>(dir * 25000);
                    input.poll(now, sample, ground, adapter.bind());
                }
                check(adapter.calls == 1 && adapter.last.action == (sign > 0 ? InputAction::manual : InputAction::nose_manual),
                    "sampling-phase manual replay preserves direction and starts once");
            }
    for (const unsigned gap : {300u, 350u, 800u}) {
        UnifiedInput input; Adapter adapter;
        const InputEnvironment ground{InputContext::ground, true};
        input.poll(0, {}, ground, adapter.bind());
        input.poll(10, {0, 0, 25000}, ground, adapter.bind());
        input.poll(10 + gap, {0, 0, -25000}, ground, adapter.bind());
        check(adapter.calls == 0, "observed-edge long steering negative cannot request manual");
    }
    for (const unsigned gap : {399u, 400u, 401u}) {
        UnifiedInput input; Adapter adapter; adapter.result = InputDisposition::waiting;
        const InputEnvironment air{InputContext::air, true};
        input.poll(0, {}, air, adapter.bind()); input.poll(10, {0, 0, 25000}, air, adapter.bind());
        input.poll(10 + gap, {0, 0, -25000}, air, adapter.bind());
        check((adapter.calls != 0) == (gap < 400), "air query rejects its exact400ms endpoint independently of the ground sampling allowance");
    }
    for (const unsigned gap : {100u, 499u, 500u, 600u}) {
        UnifiedInput input; Adapter adapter;
        const InputEnvironment air{InputContext::air, true};
        input.poll(0, {}, air, adapter.bind()); input.poll(10, {0x4000}, air, adapter.bind());
        check(adapter.calls == 0, "face-first unmatched flip owns no adapter");
        input.poll(10 + gap, {0, -25000, 0}, air, adapter.bind());
        check((adapter.calls == 1) == (gap < 500), "face-first direction match has bounded independent deadline");
    }
}

void recognition_capture_opt_in_tests(){
    static_assert(sizeof(InputRequest)<=256,"requests must not embed full100-ID evidence arrays");
    static_assert(sizeof(UnifiedInput)<32768,"ordinary input storage remains close to its original fixed footprint");
    UnifiedInput input;Adapter adapter;adapter.result=InputDisposition::waiting;
    const InputEnvironment ground{InputContext::ground,true};
    input.poll(0,{},ground,adapter.bind());input.poll(1,{0x1000},ground,adapter.bind());
    check(input.diagnostic().recognized==1&&input.diagnostic().recognition_count==0&&!adapter.last.recognition,
        "ordinary gameplay recognizes and queues without allocating detached capture evidence");
    input.set_recognition_capture(true);input.poll(2,{},ground,adapter.bind());
    check(input.diagnostic().recognition_count==1&&input.diagnostic().recognition[0].request.recognition,
        "explicit diagnostic opt-in captures new attempts only");
    const auto identity=input.diagnostic().recognition[0].request.recognition;
    check(identity->footprint.count==2&&identity->footprint.first_at==1&&identity->footprint.last_at==2,
        "enabling diagnostics does not fabricate a fresh initiating press");
    input.set_recognition_capture(false);
    check(input.diagnostic().recognition_count==0&&identity->footprint.last_at==2,
        "disabling capture releases unread diagnostics without mutating retained immutable evidence");
    input.poll(3,{0x1000},ground,adapter.bind());
    check(input.diagnostic().recognition_count==0&&!adapter.last.recognition,
        "later ordinary recognition remains metadata-free after disabling capture");
}

void recognition_capture_tests() {
    // Two recognitions in one physical sample retain independent exact edges.
    for (const std::uint64_t cadence : {17u,34u}) {
        UnifiedInput input; input.set_recognition_capture(true); Adapter adapter; adapter.result=InputDisposition::waiting;
        InputEnvironment ground{InputContext::ground,true};
        const auto poll=[&](std::uint64_t now,PhysicalInputSample physical) {
            ground.native_context={true,true,{1,1,1},now+1,now,false};
            input.poll(now,physical,ground,adapter.bind());
        };
        poll(0,{}); poll(cadence,{0x1000,0,0,0,255});
        const auto& d=input.diagnostic();
        check(d.recognition_count==2 && d.recognition_omitted==0, "one poll exports each crouch and revert attempt separately at30/60Hz");
        std::uint64_t press_id{};
        for(std::size_t i=0;i<d.recognition_count;++i) {
            const auto& attempt=d.recognition[i]; const auto& r=attempt.request;
            check(attempt.queued && r.recognition->sequence && r.session==d.session && r.recognized_at==cadence,
                "attempt retains stable sequence action session and original recognition time");
            check(r.recognition->footprint_kind==InputFootprintKind::events && r.recognition->footprint.count==1 &&
                r.recognition->footprint.first_at==cadence && r.recognition->footprint.last_at==cadence,
                "single-edge commands publish actual sequence footprint and time bounds");
            const auto id=r.recognition->footprint.events[0]; bool exact{};
            for(const auto& event:input.history().events()) if(event.sequence==id)
                exact=event.token==(r.action==InputAction::crouch ? a : InputToken{InputSource::triggers,1,InputEdge::press});
            check(exact && r.recognition->observation_known && r.recognition->observation==ground.native_context,
                "attempt proof is the actual immutable observation and exact initiating edge");
            if(r.action==InputAction::crouch) press_id=id;
            if(i)check(r.recognition->sequence==d.recognition[i-1].request.recognition->sequence+1,
                "multiple attempts in one poll have distinct ordered identities");
        }
        poll(2*cadence,{});
        check(input.diagnostic().recognition_count==1, "release emits one new attempt without republishing old recognition");
        const auto release=input.diagnostic().recognition[0].request;
        check(release.action==InputAction::ollie_release && release.recognition->footprint.count==2 &&
            release.recognition->footprint.events[0]==press_id && release.recognition->footprint.first_at==cadence &&
            release.recognition->footprint.last_at==2*cadence,
            "release preserves both original crouch press and physical release rather than synthetic poll edges");
        poll(3*cadence,{});
        check(input.diagnostic().recognition_count==0, "held or neutral polls do not renew recognition identity");
    }
    for (const std::uint64_t cadence : {17u,34u}) {
        UnifiedInput input; input.set_recognition_capture(true); Adapter adapter; adapter.result=InputDisposition::waiting;
        InputEnvironment air{InputContext::air,true};
        auto poll=[&](std::uint64_t now,PhysicalInputSample physical) {
            air.native_context={true,true,{1,1,1},now+1,now,false};
            input.poll(now,physical,air,adapter.bind());
        };
        poll(0,{}); poll(cadence,{0,-25000}); poll(2*cadence,{0x4000,-25000});
        const auto original=input.diagnostic().recognition[0].request;
        check(original.recognition->footprint.count==2 && original.recognition->footprint.held_reuse &&
            original.recognition->footprint.first_at==cadence && original.recognition->footprint.last_at==2*cadence,
            "held-direction reuse exports exact matcher IDs and original bounds");
        const auto direction_id=original.recognition->footprint.held_reuse;
        bool exact{}; for(const auto& event:input.history().events())if(event.sequence==direction_id)
            exact=event.token==InputToken{InputSource::direction,4,InputEdge::press};
        check(exact, "held reuse refers to a real original direction event");
        poll(3*cadence,{0,-25000}); adapter.result=InputDisposition::started;
        poll(1000,{0,-25000});
        check(adapter.last.recognition==original.recognition && adapter.last.recognized_at==original.recognized_at &&
            input.diagnostic().recognition_count==0 && input.diagnostic().dispatch[0].request.recognition==original.recognition,
            "waiting and dispatch preserve original proof IDs and times across new observation polls");
    }
    {
        UnifiedInput input; input.set_recognition_capture(true); Adapter adapter; adapter.result=InputDisposition::waiting;
        InputEnvironment air{InputContext::air,true};
        input.poll(0,{},air,adapter.bind()); input.poll(1,{0,-25000},air,adapter.bind());
        for(std::uint64_t i=0;i<=InputRequestQueue::capacity;++i) {
            input.poll(2+2*i,{0x4000,-25000},air,adapter.bind());
            const auto& d=input.diagnostic();
            check(d.recognition_count==1 && d.recognition[0].queued==(i<InputRequestQueue::capacity),
                "queue-full attempt remains observable with its own identity and exact admission result");
            check(!d.recognition[0].request.recognition->observation_known,
                "pure-model missing native proof remains explicitly unknown");
            input.poll(3+2*i,{0,-25000},air,adapter.bind());
        }
        check(input.diagnostic().queue_full==1, "queue-full diagnostic counts the rejected attempt exactly once");
    }
    {
        UnifiedInput input; input.set_recognition_capture(true); Adapter adapter; InputEnvironment lip{InputContext::lip,true};
        lip.rail.phase=InputRailPhase::lip_selection; lip.rail.fresh=true; lip.rail.generation=1;
        lip.rail.selection_frame=100;
        input.poll(0,{},lip,adapter.bind());
        const std::array<std::array<std::int16_t,2>,8> directions{{{-25000,25000},{25000,25000},{-25000,-25000},
            {25000,-25000},{-25000,0},{25000,0},{0,-25000},{0,25000}}};
        std::uint64_t now{};
        for(const auto& direction:directions) {
            input.poll(++now,{0,direction[0],direction[1]},lip,adapter.bind()); input.poll(++now,{},lip,adapter.bind());
        }
        lip.rail.gameframe=100;
        input.poll(++now,{0x8000},lip,adapter.bind());
        const auto& d=input.diagnostic();
        check(d.recognition_count==8 && d.recognition_omitted==1 && d.recognition_dropped==1,
            "nine actual selection attempts retain eight records with explicit exact omitted count");
        input.poll(++now,{},lip,adapter.bind());
        check(input.diagnostic().recognition_omitted==0 && input.diagnostic().recognition_dropped==1,
            "per-poll omission resets while cumulative loss survives");
    }
    {
        UnifiedInput input; input.set_recognition_capture(true); Adapter adapter; InputEnvironment lip{InputContext::lip,true};
        lip.rail.phase=InputRailPhase::lip_selection; lip.rail.fresh=true; lip.rail.generation=1;
        lip.rail.gameframe=2;
        input.poll(0,{},lip,adapter.bind());
        const auto& r=input.diagnostic().recognition[0].request;
        check(input.diagnostic().recognition_count==1 && r.recognition->footprint_kind==InputFootprintKind::input_free &&
            !r.recognition->footprint.count && !r.recognition->footprint.first_at && !r.recognition->observation_known,
            "autonomous default is explicitly input-free without invented edges or proof");
        UnifiedInput held; held.set_recognition_capture(true); InputEnvironment surface{InputContext::air,true}; surface.surface.fresh=surface.surface.can_push=true;
        held.poll(0,{},surface,adapter.bind()); surface.surface.physics_step=1; held.poll(10,{0x8000},surface,adapter.bind());
        check(held.diagnostic().recognition_count==1 &&
            held.diagnostic().recognition[0].request.recognition->footprint_kind==InputFootprintKind::unknown,
            "unspecified held-control footprint stays unknown instead of being mislabeled input-free");
    }
    {
        UnifiedInput input; input.set_recognition_capture(true); Adapter adapter; const InputEnvironment ground{InputContext::ground,true};
        input.poll(0,{},ground,adapter.bind());input.poll(1,{0x1000},ground,adapter.bind());
        const auto press=input.diagnostic().recognition[0].request.recognition->footprint;
        for(std::uint64_t now=2;now<122;++now)
            input.poll(now,{0x1000,static_cast<std::int16_t>(now%2 ? -25000 : 25000)},ground,adapter.bind());
        bool evicted=true;for(const auto& event:input.history().events())evicted=evicted && event.sequence!=press.events[0];
        input.poll(122,{},ground,adapter.bind());
        const auto& release=input.diagnostic().recognition[0].request;
        check(evicted && release.action==InputAction::ollie_release && release.recognition->footprint.count==2 &&
            release.recognition->footprint.events[0]==press.events[0] && release.recognition->footprint.first_at==1,
            "release keeps original press identity even after bounded history rotation");
    }
    {
        std::uint64_t counter=std::numeric_limits<std::uint64_t>::max()-1;
        check(input_diagnostic_increment(counter) && counter==std::numeric_limits<std::uint64_t>::max() &&
            !input_diagnostic_increment(counter) && counter==std::numeric_limits<std::uint64_t>::max(),
            "diagnostic identity allocator saturates without rollover");
        UnifiedInput input; input.set_recognition_capture(true); Adapter adapter; const InputEnvironment ground{InputContext::ground,true};
        input.poll(0,{},ground,adapter.bind());
        auto& d=const_cast<UnifiedInput::Diagnostic&>(input.diagnostic());
        d.recognized=std::numeric_limits<std::uint64_t>::max();
        input.poll(10,{0x1000},ground,adapter.bind());
        check(d.recognition_count==1 && d.recognition_overflow && d.recognition[0].request.recognition->sequence_overflow &&
            !d.recognition[0].request.recognition->sequence,
            "exhausted attempt counter exports explicit unavailable identity without changing recognition or wrapping");
    }
}

}
int main() {
    recognition_capture_tests();
    recognition_capture_opt_in_tests();
    journal_tests(); overlap_footprint_tests(); normalization_tests(); queue_tests(); queue_lifecycle_tests(); queue_action_cancellation_tests(); router_tests(); deferred_recognition_tests(); sampled_replay_tests();
    std::cout << checks << " event architecture checks passed\n";
}

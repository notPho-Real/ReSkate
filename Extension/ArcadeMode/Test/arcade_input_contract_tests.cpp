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
void held_clocks() {
    InputEvents input; const InputToken up{InputSource::direction,8,InputEdge::press};
    input.sample(0,{},false,false,InputPolicy::strict()); input.sample(10,{1},true,false,InputPolicy::strict());
    for (unsigned age : {129u,130u,131u}) check(input.held_longer(up,130,10+age) == (age>130), "held-duration predicate uses strict greater-than");
    input.discard_intent();
    check(input.history().events().empty() && input.held_longer(up,130,150), "intent clearing cannot erase independently observed held duration");
    for (unsigned i=0;i<120;++i) input.sample(160+i,{static_cast<std::uint16_t>(1u | (i%2 ? 0x4000u : 0u))},true,false,InputPolicy::strict());
    check(input.held_longer(up,130,300), "held duration survives ring overwrite without adding fake events");
    input.sample(310,{},false,false,InputPolicy::strict()); input.sample(320,{1},false,false,InputPolicy::strict());
    check(!input.held_longer(up,130,450) && input.held_longer(up,130,451), "release and repress establish a new held clock");
    input.sample(100,{1},false,false,InputPolicy::strict());
    check(!input.held_longer(up,130,230) && input.held_longer(up,130,231), "clock rollback establishes a fresh observation baseline");
    check(input.history().events().empty(), "held rollback is not a new press");
    InputEvents vertical;
    vertical.sample(0,{}); vertical.sample(100,{0,0,-25000});
    check(!vertical.held_longer({InputSource::vertical_stick,-1,InputEdge::press},130,230), "negative projection cannot alias the neutral hold clock");
}

PhysicalInputSample sample_direction(std::int16_t code) {
    const auto x=code==4 || code==7 || code==1 ? -25000 : code==6 || code==9 || code==3 ? 25000 : 0;
    const auto y=code>=7 ? 25000 : code<=3 ? -25000 : 0;
    return {0,static_cast<std::int16_t>(x),static_cast<std::int16_t>(y)};
}
void event_ordinals_and_recovery() {
    constexpr std::array<std::int16_t,8> order{8,2,4,6,7,9,1,3};
    for(std::size_t prior=0;prior<order.size();++prior) for(std::size_t next=0;next<order.size();++next) {
        if(prior==next)continue;
        InputEvents input; input.sample(0,sample_direction(order[prior]),false,false,InputPolicy::strict());
        input.sample(10,sample_direction(order[next]),false,false,InputPolicy::strict());
        const auto events=input.history().events();
        check(events.size()==2 && events[0].token.edge==(next<prior ? InputEdge::press : InputEdge::release),
            "same-poll direction transitions follow fixed button ordinals rather than a universal release-first rule");
    }
    InputEvents input; input.sample(0,{},false,false,InputPolicy::strict());
    input.sample(10,{0xf3c0,0,0,255,255},false,false,InputPolicy::strict());
    constexpr std::array<int,10> codes{13,14,15,12,8,0,6,9,1,7};
    const auto events=input.history().events();check(events.size()==10,"one event per mapped physical edge");
    for(std::size_t i=0;i<events.size();++i)check(events[i].token.code==codes[i] &&
        events[i].token.source==((i==5 || i==8) ? InputSource::triggers : InputSource::buttons),
        "face/shoulder/trigger same-poll recording order is deterministic");
    InputHistory history;history.record(a,0);history.record(release(a),1);history.record(b,2);
    const auto first=history.match({std::span<const InputToken>(&a,1),1000},2);check(history.consume(first,1),"recovery positive control includes already used events");
    check(history.recovery_events(400)==2 && history.events()[0].owners==1,"recovery strict400ms excludes exact boundary but includes presses and releases regardless of use");
    check(history.recovery_events(400)==2,"recovery recount does not compound an animation-rate accumulator");
    history.restore(16,400,1000);
    check(!history.match({std::span<const InputToken>(&b,1),1000},400),"restoring one category cannot reactivate recovery-owned input");
    history.reset();history.record(a,0);history.record(release(a),100);history.discard_older(a,100,100);
    check(history.events()[0].owners==~std::uint32_t{0} && history.events()[1].owners==0,"older-than invalidation includes its boundary and both edge kinds");
}

}
int main() {
    air_queries(); release_queries(); extra_queries(); held_clocks(); event_ordinals_and_recovery();
    std::cout << checks << " source input contract checks passed\n";
}

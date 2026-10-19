#include "Extension/ArcadeMode/arcade_input_session.h"
#include "Extension/ArcadeMode/arcade_gesture.h"
#include <iostream>
#include <vector>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{}, failures{};
void check(bool value, const char* reason) {
    ++checks;
    if (!value) { ++failures; std::cerr << "FAIL: " << reason << '\n'; }
}
struct Receiver {
    InputDisposition result{InputDisposition::waiting};
    std::vector<InputRequest> requests;
    static InputDisposition receive(void* state, const InputRequest& request) noexcept {
        auto& self = *static_cast<Receiver*>(state);
        self.requests.push_back(request); return self.result;
    }
    InputAdapter adapter() noexcept { return {this,receive}; }
};
void ordinary_table_order() {
    for (bool pad : {false,true}) {
        UnifiedInput input; Receiver sink; InputEnvironment env{InputContext::air,true};
        input.set_recognition_capture(true);
        const auto sample = [pad](std::uint16_t buttons) {
            return PhysicalInputSample{static_cast<std::uint16_t>(buttons | (pad ? 4u : 0u)),
                static_cast<std::int16_t>(pad ? 0 : -25000)};
        };
        input.poll(0,{},env,sink.adapter()); input.poll(10,sample(0),env,sink.adapter());
        input.poll(20,sample(0x6000),env,sink.adapter());
        const auto& d = input.diagnostic();
        check(d.recognition_count == 2 && d.recognition[0].request.action == InputAction::flip &&
            d.recognition[1].request.action == InputAction::grab,
            "installed air table order precedes physical face-button recording order");
        bool flip_face{}, grab_face{};
        for (const auto& event : input.history().events()) {
            if (event.token.source != InputSource::buttons) continue;
            if (event.token.code == 14) flip_face = event.owners == InputConsumption::regular;
            if (event.token.code == 13) grab_face = event.owners == InputConsumption::regular;
        }
        check(flip_face && grab_face,"independent ordinary matches commit their own physical face IDs");
        sink.requests.clear(); sink.result = InputDisposition::unsupported;
        input.poll(21,sample(0x6000),env,sink.adapter());
        check(sink.requests.size() == 2 && sink.requests[0].action == InputAction::flip &&
            sink.requests[1].action == InputAction::grab && sink.requests[0].recognized_at == 20 &&
            sink.requests[1].recognized_at == 20,
            "waiting preserves both independent ordinary requests in table order without renewing their origin");
    }
    UnifiedInput input; Receiver sink; InputEnvironment env{InputContext::air,true};
    input.poll(0,{},env,sink.adapter()); input.poll(10,{0,-25000},env,sink.adapter());
    input.poll(20,{0x4000,-25000},env,sink.adapter()); input.poll(30,{0x8000,-25000},env,sink.adapter());
    sink.requests.clear(); sink.result = InputDisposition::unsupported;
    input.poll(31,{0x8000,-25000},env,sink.adapter());
    check(sink.requests.size() == 2 && sink.requests[0].action == InputAction::flip &&
        sink.requests[1].action == InputAction::rail_request,
        "a rail request is recognition, not native table installation or permission to discard a queued flip");
}
void observed_table_replacement() {
    for (unsigned replacement : {2u,999u}) {
        UnifiedInput input; Receiver sink; InputEnvironment env{InputContext::balance,true};
        input.poll(0,{},env,sink.adapter()); input.poll(5,{0,0,0,0,255},env,sink.adapter());
        input.poll(10,{0x4000},env,sink.adapter()); input.poll(15,{},env,sink.adapter());
        env.balance_branch = replacement;
        sink.requests.clear(); sink.result = InputDisposition::unsupported;
        input.poll(20,{0x4000},env,sink.adapter());
        check(replacement == 2 ? sink.requests.size() == 1 && sink.requests[0].variant == 42 : sink.requests.empty(),
            "observed branch table replacement cancels old waiting extra requests before selecting the new table");
        check(replacement != 999 || input.diagnostic().branch_context_unavailable,
            "an unknown installed branch stays explicitly unavailable");
    }
    UnifiedInput input; Receiver sink; InputEnvironment env{InputContext::balance,true};
    input.poll(0,{},env,sink.adapter()); input.poll(10,{0,0,0,0,255},env,sink.adapter());
    env.branch_ready = false; sink.requests.clear();
    input.poll(20,{0,0,0,0,255},env,sink.adapter());
    check(sink.requests.size() == 1 && sink.requests[0].recognized_at == 10,
        "temporary actor unreadiness alone cannot invent a new installed table or cancel its request");
}
void session_table_routing() {
    InputSession session; InputAdmission admission{};
    admission.gameplay = admission.authored = admission.action_fresh = admission.inspected_stance = true;
    admission.pose = SkatePose::air; admission.action_mode = ActionExperiment::classic;
    session.poll(0,{},0,0,admission); session.poll(10,{0,-25000},0,0,admission);
    const auto out = session.poll(20,{0x6000,-25000},0,0,admission);
    const auto& d = session.diagnostic();
    check(d.recognized == 2 && d.dispatched == 1 && d.dispatch[0].request.action == InputAction::flip &&
        session.pending_grab().action == InputAction::grab && session.pending_grab().recognized_at == 20,
        "the actual session caller evaluates both air rows in installed order");
    check(!out.authored_playback && !out.action.owned && !out.phase,
        "independent recognition with missing native capabilities produces no substitute output");
}
void restoration_and_exclusions() {
    for (std::uint64_t exit : {109u,110u,111u}) {
        UnifiedInput input; Receiver sink; InputEnvironment env{InputContext::balance,true};
        input.poll(0,{},env,sink.adapter()); input.poll(10,{0x4000},env,sink.adapter());
        input.poll(15,{},env,sink.adapter()); input.poll(20,{0x4000},env,sink.adapter());
        env.context = InputContext::ground; input.poll(exit,{},env,sink.adapter());
        env.context = InputContext::balance; env.balance_branch = 2;
        sink.requests.clear(); sink.result = InputDisposition::unsupported;
        input.poll(exit+1,{},env,sink.adapter());
        check((sink.requests.size() == 1 && sink.requests[0].variant == 42) == (exit == 109),
            "actual table exit restores only Extra events strictly younger than100ms for the next installed table");
    }
    for (unsigned origin : {0u,1u,13u}) {
        UnifiedInput input; Receiver sink; sink.result = InputDisposition::unsupported;
        InputEnvironment env{InputContext::balance,true}; env.balance_branch = origin;
        input.poll(0,{},env,sink.adapter()); input.poll(10,{0x4000},env,sink.adapter());
        input.poll(15,{},env,sink.adapter()); input.poll(20,{0x2000},env,sink.adapter());
        check(sink.requests.size() == 1 && sink.requests[0].variant == (origin == 1 ? 13 : 1),
            "installed table order applies current-target exclusions before consuming an overlapping fallback");
    }
}
void dedicated_slots_and_boundaries() {
    UnifiedInput input; Receiver sink; InputEnvironment env{InputContext::air,true};
    env.manual_safe = false; env.manual_prepare = true;
    input.poll(0,{},env,sink.adapter()); input.poll(10,{0,0,25000},env,sink.adapter());
    input.poll(50,{0,0,-25000},env,sink.adapter());
    input.poll(60,{},env,sink.adapter()); input.poll(70,{0,0,-25000},env,sink.adapter());
    input.poll(100,{0,0,25000},env,sink.adapter());
    check(input.pending_manual().action == InputAction::nose_manual && input.pending_manual().recognized_at == 100,
        "fresh independent Manual match replaces the dedicated slot");
    env.context = InputContext::ground;
    input.poll(800,{},env,sink.adapter());
    check(input.pending_manual().recognized_at == 100,"air Manual retains its original inclusive700ms deadline through contact");
    input.poll(801,{},env,sink.adapter());
    check(input.pending_manual().action == InputAction::none,"expired dedicated Manual is removed even while an adapter waits");
    UnifiedInput rail; Receiver rail_sink; InputEnvironment air{InputContext::air,true};
    rail.poll(0,{},air,rail_sink.adapter()); rail.poll(10,{0,0,25000},air,rail_sink.adapter());
    rail.poll(20,{},air,rail_sink.adapter()); rail.poll(30,{0,0,25000},air,rail_sink.adapter());
    rail.poll(40,{0x8000,0,25000},air,rail_sink.adapter());
    check(rail.pending_rail().variant == 20 && rail.pending_rail().recognized_at == 40,
        "the installed approach table fills its independent extra-grind slot");
    rail.poll(50,{},air,rail_sink.adapter()); rail.poll(60,{0,0,-25000},air,rail_sink.adapter());
    rail.poll(70,{},air,rail_sink.adapter()); rail.poll(80,{0,0,-25000},air,rail_sink.adapter());
    rail.poll(90,{0x8000,0,-25000},air,rail_sink.adapter());
    check(rail.pending_rail().variant == 21 && rail.pending_rail().recognized_at == 90,
        "a later independent selection replaces rather than appends to the extra-grind slot");
    rail.poll(1090,{},air,rail_sink.adapter());
    check(rail.pending_rail().recognized_at == 90,"extra-grind replacement retains its own inclusive1000ms lifetime");
    rail.poll(1091,{},air,rail_sink.adapter());
    check(rail.pending_rail().action == InputAction::none,"extra-grind expiry cannot be renewed by continued polling");
    for (unsigned boundary = 0; boundary < 3; ++boundary) {
        UnifiedInput test; Receiver pending; InputEnvironment air{InputContext::air,true};
        if (boundary == 2) air.native_context = {true,true,{1,1,1},1,0,false};
        test.poll(0,{},air,pending.adapter());
        if (boundary == 2) { air.native_context.physics_step = 2; air.native_context.sampled_at = 10; }
        test.poll(10,{0,-25000},air,pending.adapter());
        if (boundary == 2) { air.native_context.physics_step = 3; air.native_context.sampled_at = 20; }
        test.poll(20,{0x6000,-25000},air,pending.adapter());
        if (boundary == 0) air.gameplay = false;
        if (boundary == 2) air.native_context = {true,true,{2,2,2},4,30,false};
        pending.requests.clear();
        test.poll(30,{static_cast<std::uint16_t>(boundary == 1 ? 0x6010 : 0x6000),-25000},air,pending.adapter());
        air.gameplay = true;
        if (boundary == 2) { air.native_context.physics_step = 5; air.native_context.sampled_at = 40; }
        test.poll(40,{0x6000,-25000},air,pending.adapter());
        check(pending.requests.empty(),"owner/menu/gameplay boundary clears installed work and baselines held-through faces");
    }
    ArcadeInput arcade; arcade.enable(true);
    arcade.poll(0,0,0,0,0,0,0,true,true); arcade.enable(false);
    const auto off = arcade.poll(0,17,0x6004,-25000,12000,-9000,8000,true,true);
    check(off.buttons == 0x6004 &&
        off.right_x == -9000 && off.right_y == 8000 && !off.phase,
        "Arcade OFF restores the physical sample after installed input work is removed");
}
}
int main() {
    ordinary_table_order(); observed_table_replacement(); session_table_routing(); restoration_and_exclusions(); dedicated_slots_and_boundaries();
    std::cout << checks << " installed table checks, " << failures << " failed\n";
    return failures ? 1 : 0;
}

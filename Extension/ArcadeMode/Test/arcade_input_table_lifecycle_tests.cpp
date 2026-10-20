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
InputNativeContext selector_observation(std::uint64_t now) {
    return {true,true,{1,1,1},now+1,now,false,true,1,1};
}
void contextual_table_lifecycle() {
    for(bool grab:{false,true})for(unsigned boundary=0;boundary<16;++boundary) {
        UnifiedInput input;Receiver sink;InputEnvironment env{InputContext::air,true};
        const auto kind=grab?InputAirTableKind::grab_extra:InputAirTableKind::pressure;
        const auto set_table=[&](std::uint64_t now) {
            env.native_context=selector_observation(now);
            InputAirTable table{kind,1,40,env.native_context,true};
            env.air_tables=grab?InputAirTables{table,{}}:InputAirTables{{},table};
        };
        set_table(0);input.poll(0,{},env,sink.adapter());
        set_table(10);input.poll(10,{static_cast<std::uint16_t>(grab?0x2000:0x4000),25000},env,sink.adapter());
        check(sink.requests.size()==1 && sink.requests[0].form==InputForm::contextual,
            "qualified parent table selects the contextual command through the production input consumer");
        const auto original=sink.requests.front();sink.requests.clear();
        set_table(20);input.poll(20,{0,25000},env,sink.adapter());
        check(sink.requests.size()==1 && sink.requests[0]==original,
            "waiting survives release with the original physical footprint, metadata and lifetime");
        sink.requests.clear();set_table(boundary==0?40:30);
        auto& table=grab?env.air_tables.extra:env.air_tables.pressure;
        switch(boundary) {
        case 0:break; // Exact exclusive original expiry.
        case 1:table.installed=false;break;
        case 2:++table.generation;break;
        case 3:++table.expires_at;break;
        case 4:table.kind=InputAirTableKind::none;break;
        case 5:++env.native_context.generations.action;break;
        case 6:++env.native_context.generations.contact;break;
        case 7:++env.native_context.air_departure;break;
        case 8:env.context=InputContext::ground;env.native_context.airborne=false;break;
        case 9:env.context=InputContext::unknown;break;
        case 10:env.gameplay=false;break;
        case 11:++env.native_context.generations.owner;break;
        case 12:table.observation.valid=false;break;
        case 13:++env.native_context.generations.action;table.observation=env.native_context;break;
        case 14:++env.native_context.generations.contact;table.observation=env.native_context;break;
        case 15:++env.native_context.air_departure;table.observation=env.native_context;break;
        }
        input.poll(boundary==0?40:30,{0,25000},env,sink.adapter());
        check(sink.requests.empty(),"expiry, replacement, changed proof, landing, bail, menu or owner invalidates waiting contextual work before native callback");
        check(input.pending_grab().form!=InputForm::contextual,"invalidated extra grab cannot retain physical ownership");
    }
    for(bool grab:{false,true})for(unsigned invalid=0;invalid<10;++invalid) {
        UnifiedInput input;Receiver sink;sink.result=InputDisposition::unsupported;
        InputEnvironment env{InputContext::air,true};env.named_grabs=true;
        input.poll(0,{},env,sink.adapter());env.native_context=selector_observation(10);
        const auto kind=grab?InputAirTableKind::grab_extra:InputAirTableKind::pressure;
        InputAirTable table{kind,1,1000,env.native_context,true};
        switch(invalid) {
        case 0:table={};break;
        case 1:table.kind=InputAirTableKind::none;break;
        case 2:table.generation=0;break;
        case 3:table.expires_at=10;break;
        case 4:table.observation.required=false;break;
        case 5:table.observation.valid=false;break;
        case 6:table.observation.generations.action=2;break;
        case 7:table.observation.airborne_episode=2;break;
        case 8:table.observation.physics_step=100;break;
        case 9:table.observation.sampled_at=11;break;
        }
        env.air_tables=grab?InputAirTables{table,{}}:InputAirTables{{},table};
        input.poll(10,{static_cast<std::uint16_t>(grab?0x2000:0x4000),25000},env,sink.adapter());
        bool contextual{};for(const auto& request:sink.requests)contextual=contextual||request.form==InputForm::contextual;
        check(!contextual,"AIR and ordinary labels cannot replace exact fresh installed table proof");
    }
    for(bool grab:{false,true}) {
        UnifiedInput input;Receiver sink;sink.result=InputDisposition::unsupported;
        InputEnvironment env{InputContext::air,true};
        const auto kind=grab?InputAirTableKind::grab_extra:InputAirTableKind::pressure;
        const auto poll=[&](std::uint64_t now,std::uint16_t face,bool installed,std::uint64_t generation,std::uint64_t expiry) {
            env.native_context=selector_observation(now);
            InputAirTable table{kind,generation,expiry,env.native_context,installed};
            env.air_tables=grab?InputAirTables{table,{}}:InputAirTables{{},table};
            input.poll(now,{face,25000},env,sink.adapter());
        };
        poll(0,0,true,1,1000);poll(10,0,false,1,1000);
        const auto face=static_cast<std::uint16_t>(grab?0x2000:0x4000);
        poll(20,face,true,1,2000);
        bool contextual{};for(const auto& request:sink.requests)contextual=contextual||request.form==InputForm::contextual;
        check(!contextual,"removed generation cannot reinstall itself or renew its original expiry");
        poll(30,0,true,2,2000);sink.requests.clear();poll(40,face,true,2,2000);
        check(sink.requests.size()==1 && sink.requests[0].form==InputForm::contextual && sink.requests[0].table_generation==2,
            "only a newly qualified generation rearms the removed contextual table");
    }
    for(bool nollie:{false,true}) {
        UnifiedInput input;Receiver sink;sink.result=InputDisposition::unsupported;InputEnvironment env{InputContext::air,true};
        env.nollie=nollie;env.native_context=selector_observation(0);
        env.air_tables.pressure={InputAirTableKind::pressure,1,1000,env.native_context,nollie};
        input.poll(0,{},env,sink.adapter());env.native_context=selector_observation(10);
        env.air_tables.pressure.observation=env.native_context;
        input.poll(10,{0x4000,25000},env,sink.adapter());
        check(sink.requests.size()==1 && sink.requests[0].form==InputForm::ordinary,
            "ordinary and nollie contexts do not establish a pressure table");
    }
    // A face already committed by the ordinary table cannot be borrowed by
    // a later installed contextual table, even when direction reuse is legal.
    for(bool grab:{false,true}) {
        UnifiedInput input;Receiver sink;sink.result=InputDisposition::unsupported;
        InputEnvironment env{InputContext::air,true};env.named_grabs=true;
        env.native_context=selector_observation(0);input.poll(0,{},env,sink.adapter());
        env.native_context=selector_observation(10);
        const auto face=static_cast<std::uint16_t>(grab?0x2000:0x4000);
        input.poll(10,{face,25000},env,sink.adapter());
        check(sink.requests.size()==1 && sink.requests[0].form==InputForm::ordinary,
            "ordinary table commits the original face before any extra table exists");
        sink.requests.clear();env.native_context=selector_observation(20);
        InputAirTable table{grab?InputAirTableKind::grab_extra:InputAirTableKind::pressure,1,1000,env.native_context,true};
        env.air_tables=grab?InputAirTables{table,{}}:InputAirTables{{},table};
        input.poll(20,{face,25000},env,sink.adapter());
        check(sink.requests.empty(),"installing a table cannot reconsume an already committed category or synthesize a held press");
    }
    for(bool grab:{false,true}) {
        UnifiedInput input;InputEnvironment env{InputContext::air,true};
        const auto poll=[&](std::uint64_t now,PhysicalInputSample physical) {
            env.native_context=selector_observation(now);
            InputAirTable table{grab?InputAirTableKind::grab_extra:InputAirTableKind::pressure,1,1000,env.native_context,true};
            env.air_tables=grab?InputAirTables{table,{}}:InputAirTables{{},table};
            input.poll(now,physical,env);
        };
        poll(0,{});poll(10,{static_cast<std::uint16_t>(grab?0x2000:0x4000),25000});
        check(input.diagnostic().dispatched==1 && input.diagnostic().dispatch[0].request.form==InputForm::contextual &&
            input.diagnostic().dispatch[0].disposition==InputDisposition::unsupported && input.pending_grab().action==InputAction::none,
            "the default missing native adapter reports unsupported and retains no contextual owner");
        poll(20,{static_cast<std::uint16_t>(grab?0x2000:0x4000),25000});
        check(input.diagnostic().dispatched==0,"unsupported dispatch cannot reuse its held face on a later sample");
    }
    check(native_flip_id(ArcadeTrick::pressure_turn_flip,false)==0 && native_flip_id(ArcadeTrick::pressure_turn_flip,true)==0 &&
        native_flip_id(ArcadeTrick::compact_grab,false)==0 && grab_plan(ArcadeTrick::compact_grab,true).readiness==ActionReadiness::method_missing,
        "contextual identities have no fallback native flip or named grab mapping");
}
void contextual_queue_priority() {
    const auto native=selector_observation(10);
    const InputAirTable table{InputAirTableKind::grab_extra,1,1000,native,true};
    const InputAirTables tables{table,{}};
    InputRequest ordinary{};ordinary.action=InputAction::flip;ordinary.context=InputContext::air;ordinary.session=1;ordinary.unbounded=true;
    auto extra=ordinary;extra.action=InputAction::grab;extra.form=InputForm::contextual;
    extra.selection=*contextual_trick(InputAirTableKind::grab_extra);extra.table_generation=1;extra.table_expires_at=1000;
    InputRequestQueue queue;Receiver sink;sink.result=InputDisposition::unsupported;
    check(queue.push(ordinary)&&queue.push(extra)&&queue.push(ordinary),"contextual priority shares the bounded ordinary queue");
    queue.dispatch(10,1,InputContext::air,sink.adapter(),[](InputDispatchEvent){},native,false,tables);
    check(sink.requests.size()==3 && sink.requests[0]==extra && sink.requests[1]==ordinary && sink.requests[2]==ordinary,
        "extra runs before already waiting ordinary work, preserving ordinary insertion order");
    for(unsigned rotation=0;rotation<3;++rotation) {
        // Advance the ring without rebuilding it, then fill all100 positions.
        for(unsigned i=0;i<37;++i) {
            queue.push(ordinary);
            queue.dispatch(10,1,InputContext::air,sink.adapter(),[](InputDispatchEvent){},native,false,tables);
        }
        for(unsigned i=0;i<98;++i) {
            auto request=ordinary;request.variant=static_cast<std::uint8_t>(i);
            check(queue.push(request),"ordinary queue preserves its original bounded capacity");
        }
        auto first=extra;first.variant=100;auto second=extra;second.variant=101;
        check(queue.push(first)&&queue.push(second)&&!queue.push(extra),
            "priority extras occupy normal capacity and cannot overflow a full queue");
        sink.requests.clear();
        queue.dispatch(10,1,InputContext::air,sink.adapter(),[](InputDispatchEvent){},native,false,tables);
        bool ordered=sink.requests.size()==100 && sink.requests[0]==first && sink.requests[1]==second;
        for(unsigned i=0;i<98 && ordered;++i)ordered=sink.requests[i+2].variant==i;
        check(ordered,"ring wrap preserves FIFO inside both extra and ordinary priority groups");
    }
    struct Reentrant {
        InputRequestQueue* queue{};InputRequest extra{};unsigned calls{};
        static InputDisposition receive(void* state,const InputRequest&) noexcept {
            auto& self=*static_cast<Reentrant*>(state);
            if(++self.calls==1)self.queue->push(self.extra);
            return InputDisposition::waiting;
        }
    } reentrant{&queue,extra};
    queue.push(ordinary);
    queue.dispatch(10,1,InputContext::air,{&reentrant,Reentrant::receive},[](InputDispatchEvent){},native,false,tables);
    sink.requests.clear();queue.dispatch(10,1,InputContext::air,sink.adapter(),[](InputDispatchEvent){},native,false,tables);
    check(sink.requests.size()==2 && sink.requests[0]==extra && sink.requests[1]==ordinary,
        "ordinary callback restoration cannot place itself ahead of a newly queued extra");
    struct Canceller {
        InputRequestQueue* queue{};
        static InputDisposition receive(void* state,const InputRequest&) noexcept {
            static_cast<Canceller*>(state)->queue->cancel_table(InputAirTableKind::grab_extra);
            return InputDisposition::waiting;
        }
    } canceller{&queue};
    queue.push(extra);queue.push(ordinary);unsigned cancelled{};
    queue.dispatch(10,1,InputContext::air,{&canceller,Canceller::receive},[&](InputDispatchEvent event){cancelled+=event.disposition==InputDisposition::cancelled;},native,false,tables);
    sink.requests.clear();queue.dispatch(10,1,InputContext::air,sink.adapter(),[](InputDispatchEvent){},native,false,tables);
    check(cancelled==1 && sink.requests.size()==1 && sink.requests[0]==ordinary,
        "reentrant table cancellation retires its waiting reservation without cancelling unrelated ordinary work");    for(unsigned invalid=0;invalid<3;++invalid) {
        auto malformed=extra;
        if(invalid==0)malformed.form=InputForm::ordinary;
        if(invalid==1)malformed.selection.trick=ArcadeTrick::basic_bs_grab;
        if(invalid==2)malformed.action=InputAction::flip;
        queue.push(malformed);sink.requests.clear();unsigned cancelled_bad{};
        queue.dispatch(10,1,InputContext::air,sink.adapter(),[&](InputDispatchEvent event){cancelled_bad+=event.disposition==InputDisposition::cancelled;},native,false,tables);
        check(sink.requests.empty() && cancelled_bad==1,"contextual dispatch rejects a forged row, ordinary form or mismatched action before native admission");
    }

}

}
int main() {
    contextual_table_lifecycle(); contextual_queue_priority();
    ordinary_table_order(); observed_table_replacement(); session_table_routing(); restoration_and_exclusions(); dedicated_slots_and_boundaries();
    std::cout << checks << " installed table checks, " << failures << " failed\n";
    return failures ? 1 : 0;
}

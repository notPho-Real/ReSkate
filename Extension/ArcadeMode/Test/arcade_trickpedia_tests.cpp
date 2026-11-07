#include "Extension/ArcadeMode/arcade_trickpedia.h"
#include "Extension/ArcadeMode/arcade_registry_validation.h"
#include "Extension/ArcadeMode/arcade_native_mapping.h"
#include "Extension/ArcadeMode/arcade_native_grab_lease.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
void require(bool good,const char* message){if(!good){std::cerr<<message<<'\n';std::exit(1);}}
std::string label(std::uint32_t button){return button==0x1000?"A":button==0x4000?"X":button==0x2000?"B":"other";}
}
#include "arcade_evidence_tests.inl"
int main() {
    const auto errors=validate_registry(trick_registry);for(const auto& e:errors)std::cerr<<e<<'\n';
    require(errors.empty(),"registry invalid");
    for(const auto& t:trick_registry) {
        require(trick_definition(t.id)==&t,"identity lookup");
        require(trick_status(t)!=TrickStatus::verified,"candidate has no full gameplay receipt");
        require(native_flip_id(t.id,false)==t.native_flip,"production native mapping");
        for(unsigned i=0;i<t.binding_count;++i) {
            const auto& b=t.bindings[i];
            require(!trickpedia::diagram(t,b).steps.empty(),"missing input diagram");
            if(b.pattern==BindingPattern::named) {
                const auto family=t.category==TrickCategory::flips?TrickFamily::flip:t.category==TrickCategory::grabs?TrickFamily::grab:TrickFamily::grind;
                require(select_named_trick(family,b.direction,b.presses)==b.variant,"production binding differs");
            }
        }
        for(unsigned i=0;i<t.grab_variant_count;++i) {
            const auto& p=t.grab_variants[i];
            require(native_grab::pilot(p.trick)==&native_grab::pilots[p.actor_index],"native animation slot changed");
        }
    }
    auto changed=trick_registry;
    changed[0].bindings[0].direction=TrickDirection::none;
    require(select_registry_trick(changed,TrickFamily::flip,TrickDirection::left,1)==ArcadeTrick::none,"old gameplay binding survived edit");
    require(trickpedia::diagram_text(trickpedia::diagram(changed[0],changed[0].bindings[0]),label)=="X","guide did not follow edited binding");
    changed=trick_registry;
    for(auto& t:changed)if(t.id==ArcadeTrick::crossbone)for(unsigned i=0;i<t.binding_count;++i)
        if(t.bindings[i].pattern==BindingPattern::extended_grab) {
            t.bindings[i].direction=TrickDirection::none;
            require(extended_grab_binding(changed,0,ArcadeTrick::double_grab)==t.id,"extended gameplay binding did not follow direction edit");
            const auto d=trickpedia::diagram(t,t.bindings[i]);
            require(trickpedia::diagram_text(d,label).find("Up-left")==std::string::npos,"extended guide retained old direction");
        }
    changed=trick_registry;changed[1].id=changed[0].id;
    require(!validate_registry(changed).empty(),"duplicate identity/status accepted");
    changed=trick_registry;changed[1].bindings[0]=changed[0].bindings[0];
    require(!validate_registry(changed).empty(),"conflict accepted");
    changed=trick_registry;changed[0].native_flip=0;
    require(!validate_registry(changed).empty(),"missing native handler accepted");
    changed=trick_registry;changed[0].name={};
    require(!validate_registry(changed).empty(),"missing name accepted");
    changed=trick_registry;changed[0].executor=TrickExecutor::none;
    require(!validate_registry(changed).empty(),"Planned entry retained executable mapping");
    evidence_tests();
    for(bool enabled:{false,true,false,true,false})require(trickpedia::show_arcade(enabled,true)==enabled,"same-session OFF restoration policy");
    require(!trickpedia::show_arcade(true,false) && !trickpedia::show_arcade(true,true,true),"focus/load leak");
    require(trickpedia::browse({"kickflip",-1,-1}).size()>=3,"search");
    trickpedia::Query controller_query;
    for(unsigned key:{10u,8u,2u,10u,5u,11u,8u,15u})trickpedia::search_key(controller_query,key);
    require(trickpedia::browse(controller_query)==trickpedia::browse({"kickflip",-1,-1}),"controller and keyboard searches differ");
    require(trickpedia::search_key(controller_query,39) && controller_query.search=="KICKFLIP","finishing search lost query");
    trickpedia::search_key(controller_query,37);require(controller_query.search=="KICKFLI","controller erase");
    trickpedia::search_key(controller_query,38);require(controller_query.search.empty(),"controller clear");
    controller_query.search=std::string(64,'A');trickpedia::search_key(controller_query,0);
    require(controller_query.search.size()==64,"controller query exceeded field bound");
    controller_query.search="caf\xc3\xa9";trickpedia::search_key(controller_query,37);
    require(controller_query.search=="caf","controller erase split physical keyboard UTF-8 input");
    require(trickpedia::browse({"",-1,0}).empty(),"unverified family advertised verified");
    const auto specials=trickpedia::browse({"",7,2});require(specials.size()>=3,"planned specials missing");
    for(const auto* t:specials)require(t->executor==TrickExecutor::none,"planned special substitutes handler");
    auto proposed=*specials.front();proposed.binding_count=1;
    auto& sequence=proposed.bindings[0];sequence.pattern=BindingPattern::sequence;sequence.variant=proposed.id;
    sequence.sequence={{{{StepKind::direction,0,TrickDirection::down},{StepKind::then},
        {StepKind::hold,TrickControls::mask(TrickControls::grab)},{StepKind::together},
        {StepKind::press,TrickControls::mask(TrickControls::flip)},{StepKind::then},
        {StepKind::release,TrickControls::mask(TrickControls::grab)}}},7,350};
    proposed.special={ArcadeTrick::ollie,"airborne","","continue the current combo"};
    const auto special_diagram=trickpedia::diagram(proposed,sequence);
    require(special_diagram.proposed && special_diagram.window_ms==350 &&
        trickpedia::diagram_text(special_diagram,label)=="Down -> hold B + X -> release B","special sequence diagram");
    require(special_diagram.condition.find("after Ollie")!=std::string::npos,"preceding trick missing");
    require(trick_status(proposed)==TrickStatus::planned && registry_handler_present(proposed),"proposed sequence became executable");
    require(manual_pair_trick(1,-1)==ArcadeTrick::manual && manual_pair_trick(-1,1)==ArcadeTrick::nose_manual,"manual registry binding");
    require(neutral_flip_trick()==ArcadeTrick::kickflip,"neutral production binding");
    const auto* turn=trick_definition(ArcadeTrick::ground_half_turn);
    require(turn && turn->executor==TrickExecutor::ground_turn,"executable ground turn missing");
    const auto turn_diagram=trickpedia::diagram(*turn,turn->bindings[0]);
    require(turn_diagram.window_ms==333 && turn_diagram.steps.back().buttons==(1u<<9) &&
        turn_diagram.steps.front().direction==TrickDirection::down,"ground turn guide differs from recognizer descriptor");
    const auto* boneless=trick_definition(ArcadeTrick::boneless);
    const auto steps=trickpedia::diagram(*boneless,boneless->bindings[0]);
    require(steps.window_ms==footplant_repeat_ms && trickpedia::diagram_text(steps,label)=="Up -> release direction -> Up -> A","multi-stage diagram");
    require(trick_definition(ArcadeTrick::nosegrab_alternate)==trick_definition(ArcadeTrick::nosegrab),"duplicate pose identity");
    std::cout<<trick_registry.size()<<" canonical tricks: registry and guide checks passed; native UI/gameplay acceptance remains separate\n";
}

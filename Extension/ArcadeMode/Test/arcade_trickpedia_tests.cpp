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
    changed=trick_registry;changed[1].id=changed[0].id;
    require(!validate_registry(changed).empty(),"duplicate identity/status accepted");
    changed=trick_registry;changed[1].bindings[0]=changed[0].bindings[0];
    require(!validate_registry(changed).empty(),"conflict accepted");
    changed=trick_registry;changed[0].native_flip=0;
    require(!validate_registry(changed).empty(),"missing native handler accepted");
    changed=trick_registry;changed[0].name={};
    require(!validate_registry(changed).empty(),"missing name accepted");
    auto receipt=trick_registry[0];receipt.evidence.accepted=31;
    require(trick_status(receipt,20260929,receipt.evidence.revision)==TrickStatus::verified,"complete pinned receipt rejected");
    require(trick_status(receipt,20260930,receipt.evidence.revision)==TrickStatus::experimental,"wrong build verified");
    require(trick_status(receipt,20260929,"other revision")==TrickStatus::experimental,"stale revision verified");
    receipt.evidence.invalidated=true;
    require(trick_status(receipt,20260929,receipt.evidence.revision)==TrickStatus::experimental,"regression verified");
    for(bool enabled:{false,true,false,true,false})require(trickpedia::show_arcade(enabled,true)==enabled,"same-session OFF restoration policy");
    require(!trickpedia::show_arcade(true,false) && !trickpedia::show_arcade(true,true,true),"focus/load leak");
    require(trickpedia::browse({"kickflip",-1,-1}).size()>=3,"search");
    require(trickpedia::browse({"",-1,0}).empty(),"unverified family advertised verified");
    const auto specials=trickpedia::browse({"",7,2});require(specials.size()>=3,"planned specials missing");
    for(const auto* t:specials)require(t->executor==TrickExecutor::none,"planned special substitutes handler");
    const auto* boneless=trick_definition(ArcadeTrick::boneless);
    const auto steps=trickpedia::diagram(*boneless,boneless->bindings[0]);
    require(steps.window_ms==footplant_repeat_ms && trickpedia::diagram_text(steps,label)=="Up -> release direction -> Up -> A","multi-stage diagram");
    require(trick_definition(ArcadeTrick::nosegrab_alternate)==trick_definition(ArcadeTrick::nosegrab),"duplicate pose identity");
    std::cout<<trick_registry.size()<<" canonical tricks: registry and guide checks passed; native UI/gameplay acceptance remains separate\n";
}

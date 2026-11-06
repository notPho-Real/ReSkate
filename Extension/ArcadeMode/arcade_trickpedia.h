#pragma once
#include "arcade_trick_registry.h"
#include "arcade_input_policy.h"
#include <algorithm>
#include <cctype>
#include <span>
#include <string>
#include <vector>

namespace dingosdk::arcade_mode::trickpedia {
inline constexpr std::array category_names{"Flip Tricks", "Grab Tricks", "Grinds and Slides",
    "Manuals and Reverts", "Footplants", "Wall Tricks", "Flatground Tricks", "Special Tricks"};
inline constexpr std::array status_names{"Verified", "Experimental", "Planned"};
struct InputDiagram {
    std::vector<InputStep> steps;
    std::uint64_t window_ms{};
    std::string condition;
    bool proposed{};
};
inline std::string direction_name(TrickDirection d) {
    constexpr std::array names{"Neutral", "Left", "Up", "Right", "Down", "Up-left", "Up-right", "Down-left", "Down-right", "Invalid"};
    return names[static_cast<unsigned>(d)];
}
inline std::string context_name(BindingContext context) {
    switch(context) {
    case BindingContext::air:return "Airborne; native action must be ready";
    case BindingContext::ground:return "Grounded, on board";
    case BindingContext::ground_or_air:return "Grounded, or prepare in air before landing";
    case BindingContext::rail_approach:return "Approaching a compatible rail; geometry selects the result";
    case BindingContext::established_rail:return "Established grind; compatible source contact required";
    }
    return {};
}
inline std::string grab_family_name(int family) {
    for(const auto& t:trick_registry)for(unsigned i=0;i<t.grab_variant_count;++i) {
        const auto& p=t.grab_variants[i];
        if(p.family==family && !p.one_footer && p.x==0 && p.y==0)return std::string(t.name);
    }
    return "compatible grab";
}
// Structured diagrams are rendered with the current controller's glyph labels by the UI.
// Timings come from the same policy and binding descriptor as the production recognizers.
inline InputDiagram diagram(const TrickDefinition& trick,const TrickBinding& b,
                            ArcadeTrick selected=ArcadeTrick::double_grab) {
    InputDiagram out;out.proposed=trick.executor==TrickExecutor::none;
    if(b.context==BindingContext::rail_approach)out.proposed=true;
    out.condition=context_name(b.context);
    const auto step=[&](StepKind k,std::uint32_t buttons=0,TrickDirection d=TrickDirection::none){out.steps.push_back({k,buttons,d});};
    const auto direction=[&](TrickDirection d){step(StepKind::direction,0,d);};
    const auto then=[&]{step(StepKind::then);};
    const auto together=[&]{step(StepKind::together);};
    switch(b.pattern) {
    case BindingPattern::rail_entry:
        if(b.direction!=TrickDirection::none){direction(b.direction);then();}
        if(b.parameter==2){step(StepKind::release_direction);then();direction(b.direction);then();}
        step(StepKind::press,TrickControls::mask(TrickControls::grind));
        out.window_ms=InputTiming::direction_ms;
        break;
    case BindingPattern::sequence:
        out.steps.assign(b.sequence.steps.begin(),b.sequence.steps.begin()+std::min<std::size_t>(b.sequence.count,b.sequence.steps.size()));
        out.window_ms=b.sequence.window_ms;
        if(trick.special.preceding_trick!=ArcadeTrick::none)out.condition+="; after "+std::string(trick_name(trick.special.preceding_trick));
        if(!trick.special.required_state.empty())out.condition+="; "+std::string(trick.special.required_state);
        if(!trick.special.combo_requirement.empty())out.condition+="; "+std::string(trick.special.combo_requirement);
        break;
    case BindingPattern::rail_change:
        step(StepKind::press,1u<<b.first_button);then();
        if(b.first_button==b.second_button){step(StepKind::release,1u<<b.first_button);then();}
        step(StepKind::press,1u<<b.second_button);out.window_ms=InputTiming::extra_press_ms;
        out.condition+="; destination: "+std::string(grind_change_name(grind_change_target(static_cast<std::uint8_t>(b.parameter))))+"; original rail geometry and source contact gate execution";
        break;
    case BindingPattern::fingerflip:
        step(StepKind::hold,TrickControls::mask(TrickControls::grab));together();
        if(b.direction!=TrickDirection::none){direction(b.direction);together();}
        step(StepKind::press,TrickControls::mask(TrickControls::flip));
        out.condition+="; already holding "+grab_family_name(trick.fingerflip.family)+"; both feet on board; keep B through catch, then release";
        break;
    case BindingPattern::ollie_release:
        step(StepKind::hold,TrickControls::mask(TrickControls::ollie));then();step(StepKind::release,TrickControls::mask(TrickControls::ollie));
        out.condition+="; release launches; hold controls crouch";break;
    case BindingPattern::manual_pair:
        direction(b.direction);then();direction(b.direction==TrickDirection::up?TrickDirection::down:TrickDirection::up);
        out.window_ms=InputTiming::manual_throw_pair_ms;
        out.condition+="; left stick or exact D-pad vertical strokes; keep balance after entry";break;
    case BindingPattern::footplant:
        direction(b.direction);
        if(b.presses==2){then();step(StepKind::release_direction);then();direction(b.direction);}
        then();step(StepKind::press,TrickControls::mask(TrickControls::ollie));
        out.window_ms=b.presses==2?footplant_repeat_ms:footplant_single_ms;
        out.condition+="; ordinary stance; native push/grab preparation must complete";break;
    case BindingPattern::revert:
        step(StepKind::press,TrickControls::trigger_mask(TrickControls::revert_trigger));out.condition+="; also during an admitted manual";break;
    case BindingPattern::ground_grab:
        step(StepKind::hold,TrickControls::mask(TrickControls::grab_layer));together();
        if(b.direction!=TrickDirection::none){direction(b.direction);together();}
        step(StepKind::hold,TrickControls::mask(TrickControls::grab));out.condition+="; release B to exit";break;
    case BindingPattern::selected_grab:
    case BindingPattern::extended_grab:
        step(StepKind::hold,TrickControls::mask(TrickControls::grab_layer));together();
        if(b.direction!=TrickDirection::none){direction(b.direction);together();}
        step(StepKind::hold,TrickControls::mask(TrickControls::grab));
        if(b.pattern==BindingPattern::selected_grab) {
            out.condition+="; neutral direction; choose this pose below";
            if(selected==b.variant)out.condition+=" (currently selected)";
            else out.condition+=" (not currently selected)";
        }
        out.condition+="; release B to exit";break;
    default: {
        const bool grab=trick.category==TrickCategory::grabs;
        const auto face=grab?TrickControls::mask(TrickControls::grab):trick.category==TrickCategory::grinds?TrickControls::mask(TrickControls::grind):TrickControls::mask(TrickControls::flip);
        if(b.direction!=TrickDirection::none){direction(b.direction);together();}
        step(grab?StepKind::hold:StepKind::press,face);
        for(unsigned i=1;i<b.presses;++i){then();step(StepKind::release,face);then();step(grab?StepKind::hold:StepKind::press,face);}
        out.window_ms=b.presses>1?InputTiming::extra_press_ms:InputTiming::direction_ms;
        if(b.presses>1 && grab)out.window_ms=grab_repeat_ms;
        else if(b.presses>1 && trick.executor!=TrickExecutor::held_flip)out.window_ms=flip_repeat_ms;
        if(trick.executor==TrickExecutor::held_flip)out.condition+="; same flight, one repeat, ordinary stance; native held preparation still open";
        else if(grab)out.condition+="; hold B; release B to exit";
        break;
    }
    }
    return out;
}
template<class ButtonLabel> std::string diagram_text(const InputDiagram& value,ButtonLabel&& label) {
    std::string out;
    for(const auto& step:value.steps) {
        switch(step.kind) {
        case StepKind::then:out+=" -> ";break;
        case StepKind::together:out+=" + ";break;
        case StepKind::direction:out+=direction_name(step.direction);break;
        case StepKind::release_direction:out+="release direction";break;
        case StepKind::hold:out+="hold "+label(step.buttons);break;
        case StepKind::release:out+="release "+label(step.buttons);break;
        case StepKind::press:out+=label(step.buttons);break;
        }
    }
    return out;
}
inline std::string fold(std::string_view text) {
    std::string out(text);for(auto& c:out)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));return out;
}
struct Query { std::string search; int category{-1}, status{-1}; };
inline std::vector<const TrickDefinition*> browse(const Query& query,unsigned game_build=20260929,std::string_view implementation=gameplay_fingerprint) {
    std::vector<const TrickDefinition*> out;const auto search=fold(query.search);
    for(const auto& t:trick_registry) {
        if(query.category>=0 && query.category!=static_cast<int>(t.category))continue;
        if(query.status>=0 && query.status!=static_cast<int>(trick_status(t,game_build,implementation)))continue;
        if(!search.empty() && fold(std::string(t.name)+" "+std::string(t.key)+" "+category_names[static_cast<unsigned>(t.category)]).find(search)==std::string::npos)continue;
        out.push_back(&t);
    }
    std::sort(out.begin(),out.end(),[](const auto* a,const auto* b){return a->name<b->name;});return out;
}
// No state or text-store mutation: the native page remains authoritative while Arcade is off.
constexpr bool show_arcade(bool enabled,bool native_page_focused,bool loading=false) noexcept {
    return enabled && native_page_focused && !loading;
}
} // namespace dingosdk::arcade_mode::trickpedia

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
inline constexpr std::array maturity_names{"Implemented", "Observed", "Repeatable", "Acceptance Pending"};
inline constexpr std::array criterion_state_names{"Not tested", "Observed once", "Satisfied repeatedly", "Failed", "Pass and failure recorded", "Tested; inconclusive"};
inline EvidenceSummary evidence_summary(const TrickDefinition& t,unsigned build=20260929,
                                      std::string_view fingerprint=gameplay_fingerprint) {
    return evaluate_evidence(t.evidence,acceptance_policy(t),build,fingerprint);
}
inline std::string evidence_label(const TrickDefinition& t,unsigned build=20260929,
                                  std::string_view fingerprint=gameplay_fingerprint) {
    const auto status=trick_status(t,build,fingerprint);
    std::string result=status_names[static_cast<unsigned>(status)];
    if(status==TrickStatus::experimental) {
        const auto e=evidence_summary(t,build,fingerprint);
        result+=" / ";result+=maturity_names[static_cast<unsigned>(e.current.maturity)];
        if(e.current.regression)result+=" / failures recorded";
    }
    return result;
}
inline std::string evidence_row_label(const TrickDefinition& t,unsigned build=20260929,
                                      std::string_view fingerprint=gameplay_fingerprint) {
    const auto status=trick_status(t,build,fingerprint);
    if(status!=TrickStatus::experimental)return status_names[static_cast<unsigned>(status)];
    const auto e=evidence_summary(t,build,fingerprint);
    constexpr std::array compact{"Implemented", "Observed", "Repeatable", "Pending"};
    return "Exp / "+std::string(compact[static_cast<unsigned>(e.current.maturity)])+(e.current.regression?" !":"");
}
inline std::vector<std::string> detail_chunks(std::string_view text) {
    // Each fragment is separately focusable. A long capture receipt must not
    // become one taller-than-viewport item that controller focus skips over.
    std::vector<std::string> result;
    while(!text.empty()) {
        std::size_t end=std::min<std::size_t>(180,text.size());
        if(end<text.size()) {
            while(end && (static_cast<unsigned char>(text[end])&0xc0)==0x80)--end;
            const auto space=text.substr(0,end).find_last_of(" \n");
            if(space!=std::string_view::npos && space>90)end=space+1;
        }
        result.emplace_back(text.substr(0,end));text.remove_prefix(end);
    }
    return result;
}
// The same evaluated evidence feeds the detail view and presentation tests.
inline std::vector<std::string> evidence_details(const TrickDefinition& t,bool technical=false,
                                               unsigned build=20260929,std::string_view fingerprint=gameplay_fingerprint) {
    std::vector<std::string> lines;
    const auto e=evidence_summary(t,build,fingerprint);const auto policy=acceptance_policy(t);
    lines.emplace_back("Applicable game build: "+std::to_string(build));
    if(t.executor==TrickExecutor::none)lines.emplace_back("No executable implementation in this build.");
    else {
        lines.emplace_back("Current compatible evidence: "+std::string(maturity_names[static_cast<unsigned>(e.current.maturity)]));
        lines.emplace_back(std::to_string(e.current.successful_trials)+" independent successful executions in the strongest comparable context; not a count of all attempts.");
        if(!e.current.context.empty())lines.emplace_back("Context: "+std::string(e.current.context));
        if(e.current.regression)lines.emplace_back("Known failure/regression: see failed criteria and trial details. Earlier successes do not establish reliability.");
    }
    if(e.historical_records)lines.emplace_back("Historical evidence: "+std::string(maturity_names[static_cast<unsigned>(e.historical.maturity)])+
        " ("+std::to_string(e.historical.successful_trials)+" independent successes in one context); not certified compatible with this build.");
    if(e.historical.regression)lines.emplace_back("Historical failures also recorded; see trial details. Historical successes are not a reliability claim.");
    if(e.invalidated_records)lines.emplace_back(std::to_string(e.invalidated_records)+" invalidated records retained; excluded from acceptance.");
    if(e.incomplete_records)lines.emplace_back(std::to_string(e.incomplete_records)+" incomplete/non-gameplay records; excluded from native trial counts.");
    if(e.comparison_records)lines.emplace_back(std::to_string(e.comparison_records)+" original-control comparison records; not Arcade acceptance.");
    if(policy.required)lines.emplace_back("Applicable checklist across current-compatible contexts. Verification additionally requires repeatable results in comparable conditions.");
    for(unsigned c=0;c<static_cast<unsigned>(Criterion::count);++c)if(policy.required&(Criteria{1}<<c))
        lines.emplace_back(std::string(criterion_names[c])+": "+criterion_state_names[static_cast<unsigned>(e.current_checklist.state(static_cast<Criterion>(c)))]);
    if(!technical)return lines;
    lines.emplace_back("Required criteria: "+std::string(policy.rationale));
    lines.emplace_back("Policy source: "+std::string(policy.source));
    lines.emplace_back("Current implementation fingerprint: "+std::string(fingerprint.empty()?"unavailable":fingerprint));
    for(const auto& trial:t.evidence.trials) {
        const auto compatible=current_trial(trial,build,fingerprint) && !t.evidence.invalidated;
        const auto scope=trial.invalidated || t.evidence.invalidated?"Invalidated":
            trial.kind==EvidenceKind::native_control?"Original-control comparison":!native_metadata(trial)?"Incomplete legacy record":compatible?"Current-compatible":"Historical";
        lines.emplace_back(std::string(scope)+" / "+std::string(trial.trial_id)+" / build "+std::to_string(trial.game_build));
        lines.emplace_back("Tested source: "+std::string(trial.revision)+"; "+std::string(trial.context));
        lines.emplace_back(std::string(trial.observation));
        for(unsigned c=0;c<static_cast<unsigned>(Criterion::count);++c)if((trial.passed|trial.failed)&(Criteria{1}<<c))
            lines.emplace_back(std::string(criterion_names[c])+((trial.failed&(Criteria{1}<<c))?": Failed":": Observed"));
        for(unsigned c=0;c<static_cast<unsigned>(Criterion::count);++c)if(trial.inconclusive&(Criteria{1}<<c))
            lines.emplace_back(std::string(criterion_names[c])+": Tested; inconclusive");
        if(!trial.limitations.empty())lines.emplace_back("Open / limited: "+std::string(trial.limitations));
        lines.emplace_back("Capture: "+std::string(trial.reference)+"; SHA-256 "+std::string(trial.capture_digest));
        if(compatible && trial.implementation_fingerprint!=fingerprint)
            lines.emplace_back("Compatibility review: "+std::string(trial.compatibility_reference)+"; "+std::string(trial.compatibility_scope));
    }
    return lines;
}
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
    case BindingPattern::ground_turn:
        direction(b.direction);then();step(StepKind::release_direction);then();direction(b.direction);
        then();step(StepKind::press,TrickControls::mask(b.first_button));out.window_ms=b.parameter;
        out.condition+="; riding on four wheels; no grab, manual, trigger hold or competing action; Arcade rotation enabled";
        break;
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
            if(trick.grab_variant_count>1)for(unsigned i=0;i<trick.grab_variant_count;++i)
                if(trick.grab_variants[i].trick==b.variant)out.condition+="; pose "+std::to_string(i+1)+" of "+std::to_string(trick.grab_variant_count);
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
inline constexpr std::array<std::string_view,40> search_keys{
    "A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P",
    "Q","R","S","T","U","V","W","X","Y","Z","0","1","2","3","4","5",
    "6","7","8","9","SPACE","ERASE","CLEAR","DONE"};
// The controller keypad uses the same query as physical keyboard input.
inline bool search_key(Query& query,unsigned key) {
    if(key>=search_keys.size())return false;
    if(key==39)return true;
    if(key==38)query.search.clear();
    else if(key==37 && !query.search.empty()) {
        auto start=query.search.size()-1;
        while(start && (static_cast<unsigned char>(query.search[start])&0xc0)==0x80)--start;
        query.search.erase(start);
    } else if(key<37 && query.search.size()<64)query.search+=key==36?" ":search_keys[key];
    return false;
}
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

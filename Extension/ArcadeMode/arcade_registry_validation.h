#pragma once
#include "arcade_trick_registry.h"
#include "arcade_fingerflip.h"
#include <string>
#include <vector>

namespace dingosdk::arcade_mode {
inline bool registry_handler_present(const TrickDefinition& t) {
    switch(t.executor) {
    case TrickExecutor::none:return true;
    case TrickExecutor::native_flip:return t.native_flip && t.nollie_flip;
    case TrickExecutor::native_grab:return t.grab_variant_count>0;
    case TrickExecutor::held_flip:return t.held_base==2 || t.held_base==3;
    case TrickExecutor::native_ollie:return t.id==ArcadeTrick::ollie;
    case TrickExecutor::native_manual:return t.id==ArcadeTrick::manual || t.id==ArcadeTrick::nose_manual;
    case TrickExecutor::native_revert:return t.id==ArcadeTrick::revert;
    case TrickExecutor::native_footplant:return t.id==ArcadeTrick::no_comply || t.id==ArcadeTrick::boneless;
    case TrickExecutor::native_ground_grab:return t.id==ArcadeTrick::coffin;
    case TrickExecutor::native_rail:
        for(unsigned i=0;i<t.binding_count;++i)if(t.bindings[i].pattern==BindingPattern::rail_change)return true;
        return false;
    case TrickExecutor::native_fingerflip:return t.fingerflip.identity && t.fingerflip.family && t.fingerflip.gestures && t.fingerflip.destination;
    case TrickExecutor::ground_turn:return t.id==ArcadeTrick::ground_half_turn && t.binding_count==1 && t.bindings[0].pattern==BindingPattern::ground_turn;
    }
    return false;
}
inline bool conflicting_bindings(const TrickDefinition& a,const TrickBinding& x,
                                 const TrickDefinition& b,const TrickBinding& y) {
    if(x.context!=y.context || x.pattern!=y.pattern || x.direction!=y.direction || x.presses!=y.presses)return false;
    if(x.pattern==BindingPattern::selected_grab)return x.variant==y.variant;
    if(x.pattern==BindingPattern::fingerflip)return a.fingerflip.family==b.fingerflip.family;
    if(x.pattern==BindingPattern::rail_change)return x.first_button==y.first_button && x.second_button==y.second_button;
    if(x.pattern==BindingPattern::named && a.category!=b.category)return false;
    if(x.pattern==BindingPattern::rail_entry)return x.parameter==y.parameter && a.id==b.id;
    return true;
}
inline std::vector<std::string> validate_registry(std::span<const TrickDefinition> catalog) {
    std::vector<std::string> errors;
    for(std::size_t i=0;i<catalog.size();++i) {
        const auto& t=catalog[i];const auto fail=[&](const char* why){errors.emplace_back(std::string(t.key)+": "+why);};
        if(t.id==ArcadeTrick::none || t.key.empty() || t.name.empty())fail("missing display metadata or identity");
        if(t.binding_count>t.bindings.size() || t.grab_variant_count>t.grab_variants.size()){fail("descriptor capacity exceeded");continue;}
        if(!registry_handler_present(t))fail("missing production handler");
        if(t.executor==TrickExecutor::none && (t.native_flip || t.nollie_flip || t.held_base || t.grab_variant_count || t.fingerflip.identity))
            fail("unsupported trick retains an executable native mapping");
        if(t.executor!=TrickExecutor::none && !t.binding_count)fail("orphaned executable definition");
        if(t.evidence.reference.empty() || t.evidence.revision.size()!=40 || t.evidence.context.empty())fail("missing evidence provenance");
        for(std::size_t j=i+1;j<catalog.size();++j)
            if(t.id==catalog[j].id || t.key==catalog[j].key)fail("duplicate canonical identity/status record");
        for(unsigned x=0;x<t.binding_count;++x) {
            const auto& bind=t.bindings[x];bool owned=bind.variant==t.id;
            if(bind.pattern==BindingPattern::fingerflip &&
               (bind.parameter!=fingerflip_input(bind.direction) || !(native_grab::finger_gesture(static_cast<int>(bind.parameter))&t.fingerflip.gestures)))
                fail("fingerflip diagram differs from production input or native transition");
            if(bind.pattern==BindingPattern::sequence &&
               (!bind.sequence.count || bind.sequence.count>bind.sequence.steps.size() || t.executor!=TrickExecutor::none))
                fail("sequence lacks an executable specialized handler or valid proposed steps");
            for(unsigned v=0;v<t.grab_variant_count;++v)owned=owned || t.grab_variants[v].trick==bind.variant;
            if(!owned)fail("orphaned binding variant");
            for(std::size_t j=i;j<catalog.size();++j) {
                const auto& other=catalog[j];if(other.binding_count>other.bindings.size())continue;
                for(unsigned y=j==i?x+1:0;y<other.binding_count;++y)
                    if(conflicting_bindings(t,bind,other,other.bindings[y]))fail("conflicting binding in same context");
            }
        }
        const auto policy=acceptance_policy(t);
        constexpr auto valid_criteria=(Criteria{1}<<static_cast<unsigned>(Criterion::count))-1;
        if(t.executor!=TrickExecutor::none && (!policy.execution || (policy.execution&policy.core)!=policy.execution ||
            (policy.core&policy.required)!=policy.core || (policy.required&~valid_criteria) || policy.source.empty() || policy.rationale.empty()))
            fail("missing or inconsistent applicable acceptance policy");
        for(std::size_t x=0;x<t.evidence.trials.size();++x) {
            const auto& trial=t.evidence.trials[x];
            if(((trial.passed|trial.failed|trial.inconclusive)&~valid_criteria) || (trial.passed&trial.failed) ||
                ((trial.passed|trial.failed)&trial.inconclusive))fail("conflicting criterion results");
            if(!trial.compatible_fingerprint.empty() && (!hex_digest(trial.compatible_fingerprint,64) ||
                trial.compatibility_reference.empty() || trial.compatibility_scope.empty() || !native_metadata(trial)))
                fail("incomplete source compatibility review");
            if(t.executor==TrickExecutor::none && current_trial(trial,20260929,gameplay_fingerprint))
                fail("unsupported trick claims current gameplay acceptance");
            for(std::size_t y=x+1;y<t.evidence.trials.size();++y)if(same_trial(trial,t.evidence.trials[y]))
                fail("duplicate or conflicting native trial record");
        }
        for(unsigned v=0;v<t.grab_variant_count;++v) {
            const auto& variant=t.grab_variants[v];
            if(variant.actor_index>=native_grab::pilots.size())fail("orphaned native actor slot");
            for(std::size_t j=i;j<catalog.size();++j)for(unsigned w=j==i?v+1:0;w<catalog[j].grab_variant_count && w<3;++w) {
                const auto& other=catalog[j].grab_variants[w];
                if(variant.trick==other.trick || variant.actor_index==other.actor_index)fail("duplicate pose/animation association");
            }
        }
    }
    for(unsigned id=1;id<=static_cast<unsigned>(ArcadeTrick::ground_half_turn);++id) {
        bool found{};
        for(const auto& t:catalog) {
            found=found || static_cast<unsigned>(t.id)==id;
            for(unsigned v=0;v<t.grab_variant_count && v<t.grab_variants.size();++v)
                found=found || static_cast<unsigned>(t.grab_variants[v].trick)==id;
        }
        for(const auto& selector:rail_selectors)found=found || static_cast<unsigned>(selector.request)==id;
        if(!found)errors.emplace_back("orphaned Arcade identity");
    }
    for(const auto& selector:rail_selectors)for(const auto outcome:selector.outcomes) {
        bool found{};for(const auto& t:catalog)found=found || t.id==outcome;
        if(!found)errors.emplace_back("orphaned rail selector outcome");
    }
    for(const auto& context:contextual_trick_table) {
        bool found{};for(const auto& t:catalog)found=found || t.id==context.trick;
        if(!found)errors.emplace_back("orphaned contextual prototype definition");
    }
    return errors;
}
}

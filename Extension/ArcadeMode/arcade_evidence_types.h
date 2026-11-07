#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace dingosdk::arcade_mode {
enum class Criterion : unsigned {
    input, identity, actor_pose, trajectory, landing, combo, rotation, catch_action,
    timing, hold, release, contact, balance, stance, hud, transition, exit_pop,
    prerequisite, planted_foot, single_launch, wall_collision, rebound, continuation, count
};
using Criteria=std::uint32_t;
constexpr Criteria criterion(Criterion item) noexcept {return Criteria{1}<<static_cast<unsigned>(item);}
inline constexpr std::array<std::string_view,static_cast<unsigned>(Criterion::count)> criterion_names{
    "Fresh recognized input", "Correct native identity", "Exact native actor / pose", "Airborne trajectory",
    "Clean landing", "Scoring / combo continuity", "Correct board rotation", "Native catch", "Timing / repetition coverage",
    "Native hold", "Release / native exit", "Correct physical contact", "Balance lifetime", "Supported stance / variations",
    "Native HUD", "Native transition", "Exit / pop", "Native prerequisite", "Planted-foot action", "Single legitimate launch",
    "Actual wall collision", "Bounded rebound", "Airborne continuation"};
enum class EvidenceKind : std::uint8_t { none, native_gameplay, source_asset, automated_test, native_control };
enum class EvidenceMaturity : std::uint8_t { implemented, observed, repeatable, acceptance_pending };
enum class CriterionState : std::uint8_t { not_tested, observed, satisfied, failed, regressed, inconclusive };
struct AcceptancePolicy {
    Criteria execution{}, core{}, required{};
    std::string_view source, rationale;
};
struct NativeTrial {
    // One source-documented execution segment. Multiple observers/rows for this
    // digest + segment never establish multiple independent executions.
    std::string_view capture_digest, trial_id, reference, revision, context, comparable_context;
    unsigned game_build{};
    EvidenceKind kind{EvidenceKind::none};
    bool arcade{}, invalidated{};
    Criteria passed{}, failed{};
    std::string_view observation, limitations;
    std::string_view implementation_fingerprint;
    // An exact-target source review may carry old observations forward. It must
    // name the review and its behavioral scope, not merely say "compatible".
    std::string_view compatible_fingerprint, compatibility_reference, compatibility_scope;
    Criteria inconclusive{};
};
struct TrickEvidence {
    std::string_view revision, context, reference, remaining;
    std::span<const NativeTrial> trials{};
    bool invalidated{};
};
constexpr bool hex_digest(std::string_view value,std::size_t size) noexcept {
    if(value.size()!=size)return false;
    for(char c:value)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;
    return true;
}
constexpr bool native_metadata(const NativeTrial& t) noexcept {
    return t.kind==EvidenceKind::native_gameplay && t.arcade && hex_digest(t.capture_digest,64) &&
        hex_digest(t.revision,40) && !t.trial_id.empty() && !t.reference.empty() && !t.context.empty() &&
        !t.observation.empty() && t.game_build!=0;
}
constexpr bool same_trial(const NativeTrial& a,const NativeTrial& b) noexcept {
    return !a.capture_digest.empty() && a.capture_digest==b.capture_digest &&
        (a.trial_id==b.trial_id || (!a.reference.empty() && a.reference==b.reference));
}
constexpr bool current_trial(const NativeTrial& t,unsigned build,std::string_view fingerprint) noexcept {
    if(!native_metadata(t) || t.invalidated || t.game_build!=build || !hex_digest(fingerprint,64))return false;
    return t.implementation_fingerprint==fingerprint ||
        (t.compatible_fingerprint==fingerprint && !t.compatibility_reference.empty() && !t.compatibility_scope.empty());
}
struct EvidenceView {
    EvidenceMaturity maturity{EvidenceMaturity::implemented};
    unsigned successful_trials{}, core_trials{};
    std::array<unsigned,static_cast<unsigned>(Criterion::count)> passes{}, failures{}, inconclusive{};
    std::string_view context, revision;
    bool verified{}, regression{}, conflicting{};
    constexpr CriterionState state(Criterion c)const noexcept {
        const auto i=static_cast<unsigned>(c);
        if(failures[i])return passes[i]?CriterionState::regressed:CriterionState::failed;
        if(inconclusive[i])return CriterionState::inconclusive;
        return passes[i]>=2?CriterionState::satisfied:passes[i]?CriterionState::observed:
            CriterionState::not_tested;
    }
};
struct EvidenceSummary {
    EvidenceView current, historical;
    EvidenceView current_checklist;
    unsigned historical_records{}, invalidated_records{}, incomplete_records{}, comparison_records{};
};
constexpr bool comparable(const NativeTrial& a,const NativeTrial& b,bool current) noexcept {
    if(a.comparable_context.empty() || b.comparable_context.empty())return same_trial(a,b);
    return a.comparable_context==b.comparable_context && (current || a.revision==b.revision) && a.game_build==b.game_build;
}
constexpr EvidenceView evidence_group(const TrickEvidence& evidence,AcceptancePolicy policy,const NativeTrial& anchor,
                                     unsigned build,std::string_view fingerprint,bool current) noexcept {
    EvidenceView out;out.context=anchor.context;out.revision=anchor.revision;
    for(std::size_t i=0;i<evidence.trials.size();++i) {
        const auto& t=evidence.trials[i];
        if(!native_metadata(t) || t.invalidated || current_trial(t,build,fingerprint)!=current ||
            !comparable(t,anchor,current))continue;
        bool duplicate{},invalidated{};
        for(std::size_t j=0;j<evidence.trials.size();++j)if(j!=i && same_trial(t,evidence.trials[j])) {
            const auto& other=evidence.trials[j];
            duplicate=duplicate || j<i;invalidated=invalidated || other.invalidated;
            if(t.passed!=other.passed || t.failed!=other.failed || t.inconclusive!=other.inconclusive ||
                t.revision!=other.revision)out.conflicting=true;
        }
        if(invalidated || duplicate)continue;
        for(unsigned c=0;c<static_cast<unsigned>(Criterion::count);++c) {
            if(t.passed&(Criteria{1}<<c))++out.passes[c];
            if(t.failed&(Criteria{1}<<c))++out.failures[c];
            if(t.inconclusive&(Criteria{1}<<c))++out.inconclusive[c];
        }
        if(policy.execution && (t.passed&policy.execution)==policy.execution && !(t.failed&policy.execution))++out.successful_trials;
        if(policy.core && (t.passed&policy.core)==policy.core && !(t.failed&policy.core))++out.core_trials;
    }
    out.maturity=out.successful_trials>=2?EvidenceMaturity::repeatable:
        out.successful_trials?EvidenceMaturity::observed:EvidenceMaturity::implemented;
    if(out.core_trials>=2)out.maturity=EvidenceMaturity::acceptance_pending;
    out.verified=policy.required && policy.core && !policy.source.empty() && !policy.rationale.empty() &&
        out.core_trials>=2 && !out.conflicting && !evidence.invalidated;
    for(unsigned c=0;c<static_cast<unsigned>(Criterion::count);++c) {
        if(out.failures[c] && (policy.required&(Criteria{1}<<c)))out.regression=true;
        if((policy.required&(Criteria{1}<<c)) && (out.passes[c]<2 || out.failures[c] || out.inconclusive[c]))out.verified=false;
    }
    return out;
}
constexpr EvidenceSummary evaluate_evidence(const TrickEvidence& evidence,AcceptancePolicy policy,
                                           unsigned build,std::string_view fingerprint) noexcept {
    EvidenceSummary out;
    bool current_conflict{},historical_conflict{};
    for(const auto& t:evidence.trials) {
        if(t.invalidated || evidence.invalidated){++out.invalidated_records;continue;}
        if(t.kind==EvidenceKind::native_control){++out.comparison_records;continue;}
        if(!native_metadata(t)){++out.incomplete_records;continue;}
        const bool current=current_trial(t,build,fingerprint);
        if(!current)++out.historical_records;
        const auto group=evidence_group(evidence,policy,t,build,fingerprint,current);
        (current?current_conflict:historical_conflict)|=group.conflicting;
        auto& best=current?out.current:out.historical;
        // Keep one comparable context; never assemble a pass from unrelated runs.
        // A larger entry-only batch must not hide a complete acceptance context.
        if(best.context.empty() || (group.verified && !best.verified) ||
            (group.verified==best.verified && (group.maturity>best.maturity ||
                (group.maturity==best.maturity && group.successful_trials>best.successful_trials))))best=group;
    }
    out.current.conflicting|=current_conflict;out.historical.conflicting|=historical_conflict;
    if(current_conflict)out.current.verified=false;
    if(historical_conflict)out.historical.verified=false;
    // Checklist observations span contexts for transparency, but certification
    // still requires one comparable group. Do not label an out-of-group test
    // "Not tested" merely because another context has more execution trials.
    for(std::size_t i=0;i<evidence.trials.size();++i) {
        const auto& t=evidence.trials[i];
        if(evidence.invalidated || !current_trial(t,build,fingerprint))continue;
        bool skip{};
        for(std::size_t j=0;j<evidence.trials.size();++j)
            if(j!=i && same_trial(t,evidence.trials[j]) && (j<i || evidence.trials[j].invalidated))skip=true;
        if(skip)continue;
        for(unsigned c=0;c<static_cast<unsigned>(Criterion::count);++c) {
            if(t.passed&(Criteria{1}<<c))++out.current_checklist.passes[c];
            if(t.failed&(Criteria{1}<<c))++out.current_checklist.failures[c];
            if(t.inconclusive&(Criteria{1}<<c))++out.current_checklist.inconclusive[c];
        }
    }
    // Applicable failures in another context still block global verification.
    for(const auto& t:evidence.trials)if(!evidence.invalidated && !t.invalidated && native_metadata(t) && (t.failed&policy.required)) {
        auto& view=current_trial(t,build,fingerprint)?out.current:out.historical;
        view.regression=true;view.verified=false;
        for(unsigned c=0;c<static_cast<unsigned>(Criterion::count);++c)
            if((t.failed&(Criteria{1}<<c)) && !view.failures[c])view.failures[c]=1;
    }
    for(const auto& t:evidence.trials)if(!evidence.invalidated && !t.invalidated && native_metadata(t) && (t.inconclusive&policy.required))
        (current_trial(t,build,fingerprint)?out.current:out.historical).verified=false;
    return out;
}
} // namespace dingosdk::arcade_mode

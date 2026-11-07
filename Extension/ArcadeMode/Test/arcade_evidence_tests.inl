// Synthetic trials exercise the policy, never confer acceptance on a catalog entry.
void evidence_tests() {
    constexpr std::string_view fingerprint="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    constexpr std::string_view source="bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    constexpr std::string_view capture="cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc";
    auto t=*trick_definition(ArcadeTrick::ollie);
    const auto policy=acceptance_policy(t);
    NativeTrial first{capture,"attempt-1","fixture/native",source,"documented native context","ordinary/30Hz",
        20260929,EvidenceKind::native_gameplay,true,false,policy.execution,0,"native observation","",fingerprint,"","",""};
    auto second=first;second.trial_id="attempt-2";
    std::array trials{first,second};
    t.evidence.trials={};
    auto summary=[&]{return trickpedia::evidence_summary(t,20260929,fingerprint);};
    auto label_for=[&]{return trickpedia::evidence_label(t,20260929,fingerprint);};
    require(summary().current.maturity==EvidenceMaturity::implemented && label_for()=="Experimental / Implemented","empty native evidence");
    t.evidence.trials=std::span{trials}.first(1);
    require(summary().current.successful_trials==1 && label_for()=="Experimental / Observed","one native success");
    t.evidence.trials=trials;
    require(summary().current.successful_trials==2 && label_for()=="Experimental / Repeatable","two independent successes");
    trials[1]=trials[0];
    require(summary().current.successful_trials==1,"one capture segment counted twice");
    trials[1]=second;trials[1].comparable_context="different/rate";
    require(summary().current.successful_trials==1,"incomparable contexts pooled");
    trials[1]=second;
    for(auto& trial:trials)trial.passed=policy.core;
    require(label_for()=="Experimental / Acceptance Pending","repeated core flow maturity");
    for(auto& trial:trials)trial.passed=policy.required;
    require(trick_status(t,20260929,fingerprint)==TrickStatus::verified && label_for()=="Verified","all applicable repeated criteria");
    trials[0].failed=criterion(Criterion::wall_collision);
    require(summary().current.verified,"non-applicable criterion blocked verification");
    trials[0].failed=0;
    require(!(policy.required&criterion(Criterion::wall_collision)) && !(policy.required&criterion(Criterion::release)),"inapplicable Ollie requirements");
    const auto flip=acceptance_policy(*trick_definition(ArcadeTrick::kickflip));
    const auto grab=acceptance_policy(*trick_definition(ArcadeTrick::benihana));
    require(!(flip.required&criterion(Criterion::release)) && (grab.required&criterion(Criterion::release)),"trick specific release applicability");
    require(acceptance_policy(*trick_definition(ArcadeTrick::manual)).required&criterion(Criterion::balance),"manual balance omitted");
    require(acceptance_policy(*trick_definition(ArcadeTrick::no_comply)).required&criterion(Criterion::planted_foot),"footplant action omitted");
    trials[0].passed&=~criterion(Criterion::combo);trials[1].passed&=~criterion(Criterion::combo);
    require(summary().current.state(Criterion::combo)==CriterionState::not_tested,"missing criterion treated as failure");
    trials[0].inconclusive=criterion(Criterion::combo);
    require(summary().current.state(Criterion::combo)==CriterionState::inconclusive,"inconclusive telemetry reported untested or failed");
    trials[0].inconclusive=0;
    trials[0].failed=criterion(Criterion::combo);
    require(summary().current.state(Criterion::combo)==CriterionState::failed && !summary().current.verified,"observed failure ignored");
    const auto failed_lines=trickpedia::evidence_details(t,false,20260929,fingerprint);
    require(std::find(failed_lines.begin(),failed_lines.end(),"Scoring / combo continuity: Failed")!=failed_lines.end(),"failed checklist display missing");
    trials[1].passed|=criterion(Criterion::combo);
    require(summary().current.state(Criterion::combo)==CriterionState::regressed && label_for().find("failures recorded")!=std::string::npos,"mixed pass/failure hidden");
    trials[0].failed=0;trials[0].passed=policy.required;
    require(trick_status(t,20260930,fingerprint)==TrickStatus::experimental,"wrong game build certified");
    require(trickpedia::evidence_summary(t,20260929,capture).current.successful_trials==0 &&
        trickpedia::evidence_summary(t,20260929,capture).historical.successful_trials==2,"stale fingerprint certified or history lost");
    for(auto& trial:trials){trial.compatible_fingerprint=capture;trial.compatibility_reference="source-review";trial.compatibility_scope="reviewed path/context";}
    require(trick_status(t,20260929,capture)==TrickStatus::verified,"explicit exact-target review ignored");
    trials[0].compatibility_reference={};
    require(trick_status(t,20260929,capture)==TrickStatus::experimental,"partial compatibility metadata certified");
    trials[0]=first;trials[1]=second;for(auto& trial:trials)trial.passed=policy.required;
    trials[0].invalidated=true;
    require(summary().invalidated_records==1 && !summary().current.verified,"invalidated trial certified");
    trials[1]=trials[0];trials[1].invalidated=false;
    require(summary().current.successful_trials==0,"duplicate restored invalidated trial");
    trials[0]=first;trials[1]=second;trials[0].capture_digest={};
    require(summary().incomplete_records==1 && summary().current.successful_trials==1,"legacy missing metadata granted repeatability");
    trials[0]=first;trials[1]=second;
    for(auto kind:{EvidenceKind::source_asset,EvidenceKind::automated_test,EvidenceKind::native_control}) {
        for(auto& trial:trials)trial.kind=kind;
        require(summary().current.successful_trials==0,"non Arcade observation granted gameplay maturity");
    }
    trials[0]=first;trials[1]=second;trials[1].arcade=false;
    require(summary().current.successful_trials==1,"original controls granted Arcade maturity");
    trials[1]=first;trials[1].passed|=criterion(Criterion::landing);
    require(summary().current.conflicting && !summary().current.verified,"conflicting duplicate results ignored");
    auto changed=trick_registry;changed[0].evidence.trials=trials;
    require(!validate_registry(changed).empty(),"duplicate evidence records accepted by validator");
    t.evidence.invalidated=true;
    require(summary().current.successful_trials==0 && summary().invalidated_records==2,"record-level invalidation ignored");
    t.evidence.invalidated=false;trials[0]=first;trials[1]=second;
    const auto before=t.bindings;
    for(const auto mask:{Criteria{},policy.execution,policy.core,policy.required}) {
        for(auto& trial:trials)trial.passed=mask;
        const auto lines=trickpedia::evidence_details(t,true,20260929,fingerprint);
        require(!lines.empty() && lines.back().find("fixture/native")!=std::string::npos,"technical capture reference missing");
        require(t.bindings[0].pattern==before[0].pattern && t.bindings[0].direction==before[0].direction,"evidence changed runtime binding");
    }
    auto planned=*trick_definition(ArcadeTrick::wallplant);planned.evidence=t.evidence;
    require(trick_status(planned,20260929,fingerprint)==TrickStatus::planned,"evidence created planned executor");
    unsigned imported{},compatible{};
    for(const auto& record:trick_registry) {
        const auto view=trickpedia::evidence_summary(record,20260929,evidence_reviewed_fingerprint);
        if(view.current.verified) {
            require(view.current.core_trials>=2 && !view.current.regression && !view.current.conflicting,
                    "verified import lacks repeatable current core evidence");
            const auto required=acceptance_policy(record).required;
            for(unsigned c=0;c<static_cast<unsigned>(Criterion::count);++c)if(required&(Criteria{1}<<c))
                require(view.current.passes[c]>=2 && !view.current.failures[c],"verified import lacks applicable criterion");
        }
        for(const auto& trial:record.evidence.trials) {
            ++imported;
            require(trial.kind==EvidenceKind::native_control || native_metadata(trial),"curated import has incomplete native metadata");
            if(current_trial(trial,20260929,evidence_reviewed_fingerprint))++compatible;
        }
    }
    require(imported>0 && compatible>0 && compatible<imported,"current versus historical import lost");
    for(const auto mask:{Criteria{},policy.execution,policy.core,policy.required}) {
        for(auto& trial:trials)trial.passed=mask;
        require(trickpedia::evidence_row_label(t,20260929,fingerprint).size()<=20,"row maturity label crowds trick name");
    }
    const auto receipt_text=std::string(179,'x')+"\xc3\xa9 "+std::string(500,'y');
    std::string reconstructed;
    for(const auto& chunk:trickpedia::detail_chunks(receipt_text)) {
        require(chunk.size()<=180 && (static_cast<unsigned char>(chunk.front())&0xc0)!=0x80,"controller detail splits UTF-8 or exceeds focus fragment");
        reconstructed+=chunk;
    }
    require(reconstructed==receipt_text,"controller detail fragment dropped evidence text");
}

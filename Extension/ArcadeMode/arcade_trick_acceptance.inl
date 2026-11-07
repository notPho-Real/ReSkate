// Reviewed obligations follow the production executor and its original native
// owners. These are applicability rules, never per-trick success/status flags.
constexpr AcceptancePolicy acceptance_policy(const TrickDefinition& t) noexcept {
    using C=Criterion;
    const auto m=[](std::initializer_list<C> values){Criteria out{};for(auto c:values)out|=criterion(c);return out;};
    const auto base=m({C::input,C::identity});
    const auto complete=m({C::combo,C::stance});
    switch(t.executor) {
    case TrickExecutor::native_ollie: {
        const auto execution=base|m({C::trajectory});
        const auto core=execution|m({C::single_launch,C::landing});
        return {execution,core,core|complete,"Extension/ArcadeMode/arcade_air_motion.h; Extension/ArcadeMode/arcade_launch_descriptor.h",
            "Original launch and airborne trajectory, fresh input, landing and combo; no wall or grab-release requirement."};
    }
    case TrickExecutor::native_flip: case TrickExecutor::held_flip: {
        const auto execution=base|m({C::rotation,C::actor_pose});
        const auto core=execution|m({C::catch_action,C::landing});
        return {execution,core,core|complete|m({C::timing}),"contrib/arcade-mode/research/native-ollie-flip-transition.md; Extension/ArcadeMode/arcade_held_flip.h",
            "Original board rotation/actor, catch and landing; repeated inputs and timing remain separate from scoring."};
    }
    case TrickExecutor::native_grab: case TrickExecutor::native_fingerflip: {
        const auto execution=base|m({t.executor==TrickExecutor::native_fingerflip?C::actor_pose:C::hold});
        auto core=execution|m({C::actor_pose,C::hold,C::release,C::landing});
        if(t.executor==TrickExecutor::native_fingerflip)core|=m({C::catch_action,C::transition});
        return {execution,core,core|complete|m({C::timing}),"contrib/arcade-mode/research/native-grab-transition.md; contrib/arcade-mode/research/native-fingerflip-entry.md",
            "Exact grab actor/pose and native hold/exit; finger transitions additionally require native catch and destination completion."};
    }
    case TrickExecutor::native_manual: case TrickExecutor::native_revert: {
        const auto execution=base|m({C::contact});
        const auto core=execution|m({C::balance,C::transition});
        return {execution,core,core|complete|m({C::actor_pose,C::hud,C::timing}),"contrib/arcade-mode/research/rc3-native-manual-lifetime-observations.json; Extension/ArcadeMode/arcade_manual_input.h",
            "Ground contact, sustained balance, stance/revert transitions, actual pose, native HUD and combo; airborne landing is not a manual requirement."};
    }
    case TrickExecutor::native_rail: {
        const auto execution=base|m({C::contact,C::transition});
        const auto core=execution|m({C::exit_pop,C::landing});
        return {execution,core,core|complete|m({C::timing}),"Extension/ArcadeMode/arcade_grind_change.h; contrib/arcade-mode/research/grind-exit-results.json",
            "Named rail contact and transition, original exit/pop and landing, combo continuity; unnamed contact cannot certify a named grind."};
    }
    case TrickExecutor::native_footplant: {
        const auto execution=base|m({C::prerequisite,C::trajectory});
        const auto core=execution|m({C::planted_foot,C::single_launch,C::landing});
        return {execution,core,core|complete|m({C::timing}),"contrib/arcade-mode/research/native-footplant-entry.md",
            "Native push/grab prerequisite, planted-foot action and exactly one launch; intent alone does not establish animation or launch classification."};
    }
    case TrickExecutor::native_ground_grab: {
        const auto execution=base|m({C::actor_pose,C::contact});
        const auto core=execution|m({C::hold,C::release});
        return {execution,core,core|complete,"contrib/arcade-mode/research/native-ground-grab-entry.md",
            "Ground pose/contact and clean release; no airborne landing requirement for this grounded execution."};
    }
    case TrickExecutor::ground_turn: {
        const auto execution=base|m({C::contact,C::rotation});
        return {execution,execution|m({C::transition}),execution|complete|m({C::transition,C::timing}),
            "Extension/ArcadeMode/arcade_ground_rotation.h","Grounded bounded half-turn, maintained contact and clean return to ordinary steering."};
    }
    case TrickExecutor::none:
        if(t.category==TrickCategory::walls) {
            const auto core=base|m({C::wall_collision,C::planted_foot,C::rebound,C::continuation});
            return {base|m({C::wall_collision,C::planted_foot}),core,core|complete|m({C::landing}),
                "contrib/arcade-mode/research/trickpedia-implementation.md","Wall contact, planted-foot animation and bounded rebound are future acceptance obligations; no integrated executor exists."};
        }
        return {};
    }
    return {};
}

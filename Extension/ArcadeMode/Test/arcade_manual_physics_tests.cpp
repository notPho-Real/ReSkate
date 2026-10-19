#include "Extension/ArcadeMode/arcade_manual_physics.h"
#include "Extension/ArcadeMode/arcade_combo_session.h"
#include "Extension/ArcadeMode/arcade_input_session.h"
#include "Extension/ArcadeMode/arcade_manual_labels.h"
#include "Extension/ArcadeMode/arcade_manual_observation.h"
#include "Extension/ArcadeMode/arcade_balance_input.h"
#include "Extension/ArcadeMode/arcade_input_telemetry.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value, const char* why) {
    ++checks;
    if (!value) { std::cerr << "FAIL: " << why << '\n'; std::exit(1); }
}
bool near(float a, float b) { return std::abs(a - b) < .00002f; }
}
int main() {
    for (bool nose : {false, true}) {
        ManualPhysics physics;
        check(physics.begin(nose), "independently admitted manual starts selected physical dynamics");
        const auto initial = physics.sample(1.0f / 60, nose ? 1.0f : -1.0f);
        const auto sign = nose ? -1.0f : 1.0f;
        check(initial.valid && initial.active && near(initial.position, sign * (1 + .07f / 60) / 4096),
              "position integrates the existing velocity and selected instability before correction");
        check(near(initial.velocity, -sign * 9 / 4096), "correction uses ten internal units per reference step");
        check(near(initial.repeat_pressure, 693), "repeat pressure decays by the frame-scaled seven internal units");
        check(physics.begin(nose), "same active balance request is idempotent rather than a history reset");
        const auto continued = physics.sample(1.0f / 60, nose ? 1.0f : -1.0f);
        check(continued.elapsed > initial.elapsed && continued.repeat_pressure < initial.repeat_pressure,
              "same active request preserves time and repeat-pressure decay");
        physics.stop();
        check(physics.begin(nose), "a physical hop can re-enter while retaining repeat history");
        const auto repeat = physics.sample(1.0f / 60, nose ? 1.0f : -1.0f);
        check(repeat.elapsed > continued.elapsed, "hop does not erase accumulated physical instability time");
        physics.reset(); physics.begin(nose);
        const auto fresh = physics.sample(1.0f / 60, nose ? 1.0f : -1.0f);
        check(near(fresh.position, initial.position) && near(fresh.elapsed, initial.elapsed),
              "landed, bail or ownership reset clears physical repeat history");
    }
    for (bool nose : {false, true}) for (float correction : {-1.0f, 1.0f}) {
        ManualPhysics physics; ComboSession session;
        const auto pose = nose ? SkatePose::nose_manual : SkatePose::manual;
        physics.begin(nose);
        PhysicalBalanceSample sample{};
        for (unsigned i = 0; i < 10000 && physics.active(); ++i) {
            sample = physics.sample(1.0f / 60, correction);
            const bool bailed = session.step(pose, 1.0f / 60, correction, false, &sample);
            check(bailed == (sample.exit == BalanceExit::bail), "native lifecycle consumes the physical endpoint result");
            if (sample.active) check(near(session.model().state().balance, sample.position),
                "physical balance publishes its actual normalized lean without a second dynamics pass");
        }
        check(sample.exit != BalanceExit::none && physics.closed(), "both physical endpoints close the section");
        check(!physics.begin(nose), "continuing native contact cannot reopen a terminated physical section");
        check(session.manual_putdown() == (sample.exit == BalanceExit::putdown),
              "safe physical endpoint remains distinct from native bail request");
        physics.stop(); physics.begin(nose);
        check(physics.active(), "an observed physical exit permits a later fresh section");
    }
    for (bool nose : {false, true}) {
        ManualPhysics zero_draw({}, 7); zero_draw.begin(nose);
        check(zero_draw.sample(1.0f / 60, 0).velocity == 0,
              "deterministic neutral draw can set either signed initial drift to zero");
        check(near(zero_draw.sample(1.0f / 60, 0).velocity, 7.0f / 4096),
              "neutral drift recovers positively from a zero random draw instead of sticking at zero");
        ManualPhysics zero_velocity({}, 1); zero_velocity.begin(nose);
        check(zero_velocity.sample(1.0f / 600, nose ? 1.0f : -1.0f).velocity == 0,
              "physical correction can exactly cancel initial drift");
        check(near(zero_velocity.sample(1.0f / 60, 0).velocity, 15.0f / 4096),
              "zero velocity selects positive neutral drift on the next nonzero draw");
        ManualPhysics signed_drift({}, 1); signed_drift.begin(nose);
        check(near(signed_drift.sample(1.0f / 60, 0).velocity, (nose ? -15.0f : 15.0f) / 4096),
              "nonzero positive and negative drift retain their existing signs");
    }
    for (bool nose : {false,true}) for (std::uint64_t elapsed : {1u,999u,1000u}) {
        ManualPhysics held({},1); held.begin(nose);
        const float toward=nose?1.0f:-1.0f;
        const auto prior=held.sample(1.0f/60,toward); // Lean and velocity now have opposite signs.
        BalanceControl controls;
        controls.poll(0,BalanceKind::manual,prior.position,0,0,true);
        controls.poll(elapsed,BalanceKind::manual,prior.position,0,nose?-32768:32767,true);
        const auto crossed=held.sample(1.0f/60,controls.manual_input());
        check(crossed.position*toward>0, "physical lean crosses zero during integration before safe-period direction is tested");
        check(near(crossed.velocity,(elapsed<1000?9.0f:19.0f)*toward/4096),
              "post-integration outward hold is suppressed before1000ms and acts at the exact boundary");
        if(elapsed<1000) {
            const auto noise=held.sample(1.0f/60,0);
            check(near(noise.velocity,9.19f*toward/4096), "suppressed held correction preserves the neutral RNG footprint");
        }
        ManualPhysics neutral({},1);neutral.begin(nose);neutral.sample(1.0f/60,toward);
        check(near(neutral.sample(1.0f/60,0).velocity,9.19f*toward/4096),
              "true neutral consumes its random drift update rather than behaving as a suppressed hold");
    }
    for(bool nose : {false,true}) {
        ManualPhysics entry({},1);entry.begin(nose);
        BalanceControl controls;controls.poll(0,BalanceKind::manual,0,0,nose?-32768:32767,true);
        check(!controls.manual_input().released && near(entry.sample(1.0f/60,controls.manual_input()).velocity,
              (nose?-15.0f:15.0f)/4096), "entry-held unreleased input retains the source neutral branch until ignore suppression ends");
    }
    for(bool original_nose : {false,true}) {
        ManualPhysics typed;typed.begin(original_nose);
        const float direction=original_nose?1.0f:-1.0f;
        typed.sample(1.0f/60,direction);
        const auto prior=typed.sample(1.0f/60,direction);
        check(typed.begin(!original_nose)&&typed.nose()!=original_nose,
              "independently admitted opposite manual type installs its physical endpoint policy");
        const auto next=typed.sample(1.0f/60,-direction);
        check(near(next.velocity,-direction*5.25f/4096)&&next.elapsed>prior.elapsed,
              "typed transition applies repeat multiplier to retained velocity and preserves accumulated instability");
        check(near(next.repeat_pressure,693)&&near(prior.repeat_pressure,686),
              "a changed manual type performs new setup and resets section repeat pressure");
        typed.begin(!original_nose);
        check(near(typed.sample(1.0f/60,-direction).repeat_pressure,686),
              "same-type continuation remains idempotent and cannot renew repeat pressure");
        BalanceControl controls;controls.poll(0,BalanceKind::manual,0,0,0,true);
        controls.poll(1100,BalanceKind::manual,0,0,32767,true);controls.reset_manual();
        controls.poll(1117,BalanceKind::manual,0,0,32767,true);
        check(!controls.manual_input().released&&controls.manual_input().elapsed_ms==0,
              "changed-type setup restarts entry-held suppression without erasing physical history");
    }
    ManualPhysics a({}, 19), b({}, 19);
    a.begin(false); b.begin(false);
    for (unsigned i = 0; i < 30; ++i) {
        const auto x = a.sample(1.0f / 60, 0), y = b.sample(1.0f / 60, 0);
        check(x.position == y.position && x.velocity == y.velocity, "fixed-seed physical noise is independently replayable");
    }
    check(!a.sample(0, 0).valid && !a.active(), "missing simulation step releases physical dynamics");
    a.begin(false);
    check(!a.sample(.3f, 0).valid, "an oversized simulation gap cannot integrate an unobserved balance");
    for (bool nose : {false, true}) for (bool early : {false, true}) {
        InputSession input; ManualHistory history; ManualHistoryLabels labels; ManualObservation identity;
        ManualPhysics physics; ComboSession session; BalanceControl controls;
        const auto wanted = nose ? SkatePose::nose_manual : SkatePose::manual;
        const std::array rows{ManualHistoryRow{10, 20, nose ? 2u : 1u}};
        const auto first = static_cast<std::int16_t>(nose ? -25000 : 25000);
        auto sample = [&](std::uint64_t now, SkatePose raw, SkatePose contact, std::int16_t y, bool has_history) {
            labels.observe(history.observe(has_history ? std::span<const ManualHistoryRow>(rows) : std::span<const ManualHistoryRow>{},
                {now, 1, 2, 3, true, true}), now);
            const auto observed = identity.poll(raw, labels.take(now), {now, 1, true, true, true, false, false, contact});
            const auto correction = controls.poll(now, balance_kind(observed.pose), session.model().state().balance, 0, y, true);
            PhysicalBalanceSample physical{true, false};
            if (observed.confirmed) {
                if (!physics.active() && !physics.closed()) physics.begin(nose);
                physical = physics.sample(1.0f / 60, controls.manual_input());
            } else if (observed.pose == SkatePose::air) physics.stop();
            else physics.reset();
            const bool failed = session.step(observed.pose, 1.0f / 60, correction, false, &physical);
            if (failed || session.manual_putdown()) input.cancel_manual();
            InputAdmission admission{};
            admission.gameplay = admission.authored = admission.manual_safe = true;
            admission.pose = observed.confirmed ? observed.pose : raw; admission.tick = now / 17;
            admission.manual_balance_admitted = observed.confirmed && session.model().state().balance_kind == BalanceKind::manual &&
                !session.failed_section();
            return input.poll(now, {0, 0, y}, 0, 0, admission);
        };
        sample(1000, SkatePose::riding, SkatePose::riding, 0, false);
        sample(1017, SkatePose::air, SkatePose::unknown, first, false);
        check(sample(1034, SkatePose::air, SkatePose::unknown, static_cast<std::int16_t>(-first), false).manual.owns,
              "production shared input recognizes the preparation before physical balance exists");
        if (early) sample(1051, SkatePose::air, SkatePose::unknown, 0, true);
        for (std::uint64_t now = early ? 1068 : 1051; now < 61000; now += 17) {
            const auto& state = session.model().state();
            const auto goal = std::clamp(-8 * state.balance - 3 * state.balance_velocity * 60, -1.0f, 1.0f);
            const auto magnitude = goal == 0 ? 0 : 7849 + std::abs(goal) * (32767 - 7849);
            const auto y = static_cast<std::int16_t>(goal < 0 ? magnitude : -magnitude);
            const auto out = sample(now, SkatePose::riding, wanted, y, true);
            check(out.manual.owns && !physics.closed() && session.model().state().balance_kind == BalanceKind::manual &&
                balance_meter(session.balance_display_state()).visible,
                "history/contact admission, timed physical correction, native ownership and meter remain coherent during a corrected long hold");
        }
        check(!physics.sample(0, 0).valid, "missing physics cadence cannot preserve an active physical balance");
    }
    ManualPhysicsEvidence evidence{1000, 1.0f / 60, 2, 400, .2f, .1f, 10};
    check(client_telemetry_valid(ClientTelemetry{evidence}), "physical telemetry carries finite cadence and selected profile");
    evidence.step = 0;
    check(!client_telemetry_valid(ClientTelemetry{evidence}), "missing physical cadence is not a valid dynamics sample");
    evidence.step = 1.0f / 60; evidence.profile = 0;
    check(!client_telemetry_valid(ClientTelemetry{evidence}), "unknown profile is not reported as selected physics");
    std::cout << checks << " physical manual dynamics checks passed\n";
}

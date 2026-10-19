#include "session_manual.h"
#include "Extension/ArcadeMode/arcade_manual_input.h"
#include "Extension/ArcadeMode/arcade_gesture.h"
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
unsigned checks{};
void check(bool value, const char* reason) {
    ++checks;
    if (!value) { std::cerr << "FAIL: " << reason << '\n'; std::exit(1); }
}
using namespace dingosdk::arcade_mode;
dingosdk::arcade_mode::test::SessionManual::Output poll(dingosdk::arcade_mode::test::SessionManual& manual, std::uint64_t ms, short y = 0,
        SkatePose pose = SkatePose::riding, std::uint16_t buttons = 0, short x = 0,
        bool safe = true, bool conflict = false, bool gameplay = true) {
    return manual.poll(ms, buttons, x, y, gameplay, safe, pose, conflict);
}
void start(dingosdk::arcade_mode::test::SessionManual& manual, std::uint64_t ms = 0, bool nose = false) {
    poll(manual, ms);
    poll(manual, ms + 10, nose ? -25000 : 25000);
    const auto out = poll(manual, ms + 20, nose ? 25000 : -25000);
    check(out.owns_stick && !out.confirmed && out.y == (nose ? 20000 : -20000),
        "a fresh directional pair requests the exact partial native deflection");
}
}

int run_arcade_manual_tests() {
    for (bool pad : {false, true}) for (bool nose : {false, true}) {
        const int first = nose ? -1 : 1;
        for (bool prior_neutral : {false, true}) for (unsigned gap : {0u, 1u, 2u}) {
            dingosdk::arcade_mode::test::SessionManual manual;
            auto sample = [&](unsigned ms, int direction, bool rejected) {
                return manual.poll(ms, (pad ? (direction > 0 ? 1u : direction < 0 ? 2u : 0u) : 0u) |
                    (rejected && gap == 0 ? 0x2000u : 0u), 0, static_cast<std::int16_t>(pad ? 0 : direction * 24000),
                    true, !rejected || gap != 1, SkatePose::riding, rejected && gap == 2);
            };
            if (prior_neutral) sample(0, 0, false);
            check(!sample(17, 0, true).owns_stick, "rejected sample never actuates");
            sample(51, first, false);
            const auto result = sample(251, -first, false);
            check(result.owns_stick && result.y == (nose ? 20000 : -20000),
                "sampled neutral during a reversible conflict preserves a fresh post-gap edge");
        }
        auto sample = [&](dingosdk::arcade_mode::test::SessionManual& m, unsigned ms, int direction, bool safe = true,
                          bool gameplay = true, SkatePose pose = SkatePose::riding) {
            return m.poll(ms, pad ? (direction > 0 ? 1u : direction < 0 ? 2u : 0u) : 0u,
                0, static_cast<std::int16_t>(pad ? 0 : direction * 24000), gameplay, safe, pose, false);
        };
        for (bool trigger : {false, true}) {
            ArcadeInput input; input.enable(true);
            auto integrated = [&](unsigned ms, int direction, bool rejected) {
                return input.poll(0, ms,
                    (pad ? (direction > 0 ? 1u : direction < 0 ? 2u : 0u) : 0u) |
                        (rejected && !trigger ? 0x2000u : 0u),
                    0, static_cast<std::int16_t>(pad ? 0 : direction * 24000), 0, 0, true, false,
                    false, 0, rejected && trigger, false, true, ms, false, {}, false,
                    true, SkatePose::riding);
            };
            check(!integrated(17, 0, true).phase,
                "brake or trigger owner retains neutral synthetic input during rejection");
            check(!integrated(51, first, false).phase,
                "first fresh direction on brake or trigger release only prepares the pair");
            const auto out = integrated(251, -first, false);
            check(out.phase && out.right_y == (nose ? 20000 : -20000),
                "public input integration recognizes the immediate post-conflict first edge");
        }
        {
            dingosdk::arcade_mode::test::SessionManual m;
            sample(m, 0, 0); sample(m, 17, first);
            check(sample(m, 51, -first).owns_stick, "opposing-pad cancellation has an active request control");
            const auto out = m.poll(68, 3, 0, 0, true, true, SkatePose::riding, false);
            check(!out.owns_stick && !out.suppress_reverse && !out.confirmed && !out.y,
                "opposing D-pad input cancels an active request without retaining stick ownership");
            check(!sample(m, 85, -first).owns_stick,
                "an opposing-pad cancellation cannot replay the canceled request");
        }
        {
            dingosdk::arcade_mode::test::SessionManual m;
            sample(m, 0, 0); sample(m, 17, first);
            sample(m, 34, first, false); sample(m, 51, first);
            check(!sample(m, 200, -first).owns_stick,
                "held first direction across eligibility gap cannot become a new press");
        }
        {
            dingosdk::arcade_mode::test::SessionManual m;
            sample(m, 0, first);
            check(!sample(m, 200, -first).owns_stick, "unknown startup hold is only a baseline");
        }
        for (unsigned reset : {0u, 1u, 2u, 3u, 4u, 5u}) {
            dingosdk::arcade_mode::test::SessionManual m;
            sample(m, 100, 0); sample(m, 117, first);
            if (reset == 0) sample(m, 134, 0, true, false); // Menu/focus.
            if (reset == 1) m.cancel(); // Owner/context reset published outside input poll.
            if (reset == 2) sample(m, 99, 0); // Clock rollback.
            if (reset == 3) sample(m, 134, 0, true, true, SkatePose::bail);
            if (reset == 4) sample(m, 134, 0, true, true, SkatePose::grind);
            if (reset == 5) sample(m, 134, 0, true, true, SkatePose::unknown);
            sample(m, 151, first);
            check(!sample(m, 200, -first).owns_stick,
                "hard context boundary restores unknown held baseline suppression");
        }
        {
            dingosdk::arcade_mode::test::SessionManual m;
            sample(m, 0, 0); sample(m, 17, first); sample(m, 34, -first);
            check(!sample(m, 51, 0, false).owns_stick && !sample(m, 68, -first).owns_stick,
                "completed request cannot survive eligibility loss or replay on return");
            m.cancel(); sample(m, 100, 0); sample(m, 117, first);
            check(!sample(m, 500, -first).owns_stick, "expired sequence remains expired");
        }
        // Exercise public integration resets without changing playback/face ownership.
        for (unsigned reset : {0u, 1u, 2u, 3u}) {
            ArcadeInput input; input.enable(true);
            auto integrated = [&](unsigned user, unsigned ms, int direction, bool gameplay = true,
                                  bool offboard = false) {
                return input.poll(user, ms,
                    pad ? (direction > 0 ? 1u : direction < 0 ? 2u : 0u) : 0u,
                    0, static_cast<std::int16_t>(pad ? 0 : direction * 24000), 0, 0, gameplay, false,
                    false, 0, false, false, true, ms, false, {}, false,
                    true, SkatePose::riding, false, false, offboard);
            };
            integrated(0, 0, 0); integrated(0, 17, first);
            unsigned user = 0;
            if (reset == 0) { input.enable(false); input.enable(true); }
            if (reset == 1) user = 1;
            if (reset == 2) integrated(0, 34, 0, false);
            if (reset == 3) integrated(0, 34, 0, true, true);
            integrated(user, 51, first);
            check(!integrated(user, 200, -first).phase,
                "toggle controller menu and offboard integration reset physical continuity");
        }
        {
            dingosdk::arcade_mode::test::SessionManual m;
            sample(m, 0, 0);
            // Both directions occur while another owner is active: no queued command.
            for (unsigned button : {0x1000u, 0x2000u, 0x4000u, 0x8000u, 0x0300u}) {
                m.cancel(); sample(m, 0, 0);
                auto rejected = [&](unsigned ms, int direction) {
                    return m.poll(ms, static_cast<std::uint16_t>(button | (pad ? (direction > 0 ? 1u : 2u) : 0u)),
                        0, static_cast<std::int16_t>(pad ? 0 : direction * 24000), true, true, SkatePose::riding, true);
                };
                check(!rejected(17, first).owns_stick && !rejected(51, -first).owns_stick &&
                      !sample(m, 68, -first).owns_stick,
                    "face trigger and active playback conflicts never queue or replay rejected pairs");
            }
            m.cancel(); sample(m, 0, 0);
            m.poll(17, 3, 0, 0, true, false, SkatePose::riding, false);
            sample(m, 51, first);
            check(!sample(m, 200, -first).owns_stick,
                "invalid opposing physical directions reset continuity even during eligibility loss");
        }
    }
    // Replay physical events on every input sampling phase, rather than
    // assigning their timestamps directly to idealized polls.
    for (const unsigned frame : {17u, 34u}) {
        for (unsigned phase = 0; phase < frame; ++phase) {
            // 300ms and250ms can both quantize to272ms at34ms polling.
            // Deliberate long holds must lie outside that ambiguous boundary.
            for (const unsigned duration : {150u, 220u, 250u, 350u, 800u}) {
                for (const bool nose : {false, true}) for (const bool pad : {false, true}) {
                    dingosdk::arcade_mode::test::SessionManual manual;
                    bool requested{};
                    const short first = nose ? -24000 : 24000;
                    for (unsigned ms = phase; ms < 220 + duration; ms += frame) {
                        const auto first_end = duration <= 250 ? 180u : 100 + duration;
                        const int direction = ms >= 100 && ms < first_end ? 1 :
                            ms >= 100 + duration && ms < 180 + duration ? -1 : 0;
                        const auto buttons = static_cast<std::uint16_t>(!pad || !direction ? 0 :
                            direction * first > 0 ? 1 : 2);
                        const auto out = poll(manual, ms, pad ? 0 : static_cast<short>(direction * first),
                            SkatePose::riding, buttons);
                        requested = requested || out.owns_stick;
                    }
                    if (requested != (duration <= 250))
                        std::cerr << "Sampling case: frame=" << frame << " phase=" << phase
                            << " duration=" << duration << " nose=" << nose << " pad=" << pad << '\n';
                    check(requested == (duration <= 250),
                        "physical150-250ms thumb events survive every30/60Hz phase;350/800ms steering holds do not");
                }
            }
        }
    }
    // Dpad override cannot create another canonical press when the same
    // logical direction was already held at startup by the stick.
    for (unsigned frame : {17u,34u}) for (bool held_up : {false,true}) {
        dingosdk::arcade_mode::test::SessionManual manual;
        bool requested=false;
        for(unsigned ms=0;ms<450;ms+=frame){
            const bool pad_held=(ms>=100&&ms<180)||(ms>=250&&ms<330);
            const auto buttons=static_cast<std::uint16_t>(pad_held ? (held_up?1:2) : 0);
            requested=requested||poll(manual,ms,held_up?25000:-25000,SkatePose::riding,buttons).owns_stick;
        }
        check(!requested,"startup-held stick and same-direction Dpad override cannot fabricate a fresh canonical Manual pair");
    }
    for (const auto pose : {SkatePose::riding, SkatePose::air}) {
        const auto window = pose == SkatePose::air ? 399u : 275u;
        for (const auto buttons : {true, false}) {
            for (const bool nose : {false, true}) {
                for (const auto offset : {0u, 1u}) {
                    dingosdk::arcade_mode::test::SessionManual manual;
                    const short first = nose ? -24000 : 24000;
                    poll(manual, 0, 0, pose);
                    poll(manual, 10, buttons ? 0 : first, pose, buttons ? (nose ? 2 : 1) : 0);
                    poll(manual, 50, 0, pose);
                    const auto out = poll(manual, 10 + window + offset, buttons ? 0 : -first,
                        pose, buttons ? (nose ? 1 : 2) : 0);
                    check(out.owns_stick == (offset == 0),
                        "ground220 plus55ms allowance and strict air400 have independent boundaries");
                }
            }
        }
    }
    {
        dingosdk::arcade_mode::test::SessionManual manual;
        poll(manual, 0, 0, SkatePose::air);
        poll(manual, 10, 24000, SkatePose::air);
        poll(manual, 360, -24000, SkatePose::air);
        check(poll(manual, 1059, 0, SkatePose::air).owns_stick &&
              poll(manual, 1060, 0, SkatePose::air).owns_stick &&
              !poll(manual, 1061, 0, SkatePose::air).owns_stick,
              "air request remains valid at700ms and expires afterward without a refreshed deadline");
        check(!poll(manual, 1080).owns_stick,
              "an expired airborne request cannot replay on landing");
    }
    for (const unsigned frame : {17u, 33u}) {
        for (const bool nose : {false, true}) {
            dingosdk::arcade_mode::test::SessionManual manual;
            poll(manual, 0);
            const short first = nose ? -24000 : 24000;
            poll(manual, frame, first, SkatePose::riding, 0, 16000);
            for (unsigned ms = frame * 2; ms < 180; ms += frame)
                poll(manual, ms, ms < 100 ? first : 0, SkatePose::riding, 0, ms < 100 ? 16000 : 0);
            const auto out = poll(manual, 180 + frame, -first, SkatePose::riding, 0, -16000);
            check(!out.owns_stick,
                "canonical diagonal thumb travel cannot fabricate vertical-only Manual presses at30/60Hz");
            manual.cancel(); poll(manual, 1000);
            poll(manual, 1010, first);
            poll(manual, 1800, first);
            check(!poll(manual, 1820, -first).owns_stick,
                "ordinary long forward or reverse hold cannot complete a stale manual pair");
        }
    }
    {
        dingosdk::arcade_mode::test::SessionManual manual;
        poll(manual, 0);
        poll(manual, 20, 24000);
        poll(manual, 50, 9000);
        check(!poll(manual, 80, 12000).owns_stick, "threshold jitter cannot create a second opposite tap");
        check(poll(manual, 200, -24000).owns_stick, "moderate stick taps do not require a full throw");
        manual.cancel(); poll(manual, 1000);
        poll(manual, 1020, 0, SkatePose::riding, 5);
        check(!poll(manual, 1220, 0, SkatePose::riding, 10).owns_stick,
            "canonical diagonal D-pad presses cannot become a second vertical command history");
    }
    for (const auto candidate : {SkatePose::manual, SkatePose::nose_manual}) {
        check(dingosdk::arcade_mode::manual_input_pose(candidate, true, false) == SkatePose::riding,
            "independently observed native one-wheel pushing cannot eat a manual command");
        check(dingosdk::arcade_mode::manual_input_pose(candidate, false, false) == candidate && dingosdk::arcade_mode::manual_input_pose(candidate, true, true) == candidate,
            "actual manual contact and independently established balance keep their input context");
    }
    check(dingosdk::arcade_mode::manual_input_pose(SkatePose::air, true, false) == SkatePose::air &&
          dingosdk::arcade_mode::manual_input_pose(SkatePose::bail, true, false) == SkatePose::bail,
          "push context normalization cannot fabricate riding permission outside a one-wheel candidate");
    {
        dingosdk::arcade_mode::test::SessionManual manual;
        poll(manual, 0);
        auto down = poll(manual, 20, -25000);
        check(down.suppress_reverse && !down.owns_stick,
            "fresh grounded Down is reserved before native revert can consume a Nose Manual pair");
        check(poll(manual, 100, -25000).suppress_reverse, "a rapid held first tap retains its pending ownership");
        const auto up = poll(manual, 160, 25000);
        check(up.owns_stick && up.y == 20000 && !up.suppress_reverse,
            "direct Down/Up starts the proven native Nose Manual without a native reverse transition");
        check(!poll(manual, 180, -25000, SkatePose::nose_manual).suppress_reverse,
            "established balance correction is never swallowed as a new first tap");
        manual.cancel(); poll(manual, 1000);
        poll(manual, 1020, -25000);
        check(poll(manual, 1295, -25000).suppress_reverse && !poll(manual, 1296, -25000).suppress_reverse,
            "a continued reverse hold passes through immediately after the275ms sampled ground window");
        check(!poll(manual, 1600, 25000).owns_stick, "an expired reverse hold cannot become a stale Nose Manual");
        manual.cancel(); poll(manual, 2000, 0, SkatePose::air);
        check(!poll(manual, 2020, -25000, SkatePose::air).suppress_reverse,
            "airborne movement is not masked by grounded reverse protection");
        manual.cancel(); poll(manual, 3000);
        check(!poll(manual, 3020, -25000, SkatePose::riding, 0x2000).suppress_reverse,
            "ordinary B braking cancels pending gesture ownership immediately");
        manual.cancel(); poll(manual, 4000);
        check(!poll(manual, 4020, -11000, SkatePose::riding, 0, 32767).suppress_reverse,
            "predominantly sideways steering remains native rather than becoming a vertical tap");
    }
    check(dingosdk::arcade_mode::manual_revert_preparation(true, false, 101, 1, true),
        "fresh measured recovery with accepted continuation permits preparation");
    for (unsigned state : {0u, 100u, 102u, 200u, 401u})
        check(!dingosdk::arcade_mode::manual_revert_preparation(true, false, state, 1, true),
            "preparation exception is restricted to measured state101");
    check(!dingosdk::arcade_mode::manual_revert_preparation(false, false, 101, 1, true) &&
          !dingosdk::arcade_mode::manual_revert_preparation(true, true, 101, 1, true) &&
          !dingosdk::arcade_mode::manual_revert_preparation(true, false, 101, 0, true) &&
          !dingosdk::arcade_mode::manual_revert_preparation(true, false, 101, 1, false),
        "stale owner bail other mode and absent native continuation reject preparation");
    for (const short first : {short(25000), short(-25000)}) {
        dingosdk::arcade_mode::test::SessionManual manual;
        auto recovery = [&](std::uint64_t ms, short y, bool prepare = true,
                            SkatePose pose = SkatePose::riding, bool conflict = false) {
            return manual.poll(ms, 0, 0, y, true, false, pose, conflict, prepare);
        };
        recovery(800, 0);
        check(!recovery(900, first).owns_stick, "first direction during recovery never owns synthetic input");
        check(!poll(manual, 941, first).owns_stick, "safe recovery preserves the held first edge");
        poll(manual, 960);
        auto out = poll(manual, 1024, -first);
        check(out.owns_stick && out.y == (first > 0 ? -20000 : 20000),
            "measured900/941/960/1024ms handoff initiates only after safe opposite edge");
        check(!recovery(1025, 0).owns_stick,
            "preparation exception cannot retain an already active synthetic request");
        manual.cancel(); recovery(2000, 0); recovery(2010, first);
        check(!recovery(2020, -first).owns_stick && poll(manual, 2030, -first).owns_stick,
            "recognized recovery pair waits for readiness before bounded actuation");
        manual.cancel(); recovery(3000, 0); recovery(3010, first);
        poll(manual, 3020);
        check(!poll(manual, 3511, -first).owns_stick, "preparation cannot extend the sampled ground direction deadline");
        for (const auto pose : {SkatePose::riding, SkatePose::unknown, SkatePose::bail, SkatePose::grind}) {
            manual.cancel(); recovery(4000, 0); recovery(4010, first);
            recovery(4020, 0, pose != SkatePose::riding, pose);
            poll(manual, 4030);
            check(!poll(manual, 4040, -first).owns_stick,
                "lost preparation or invalid pose discards the earlier direction");
        }
        manual.cancel(); recovery(5000, 0); recovery(5010, first);
        recovery(5020, 0, true, SkatePose::riding, true); poll(manual, 5030);
        check(!poll(manual, 5040, -first).owns_stick, "conflicting owner discards recovery preparation");
    }
    for (const bool nose : {false, true}) {
        dingosdk::arcade_mode::test::SessionManual manual;
        start(manual, 0, nose);
        const auto pose = nose ? SkatePose::nose_manual : SkatePose::manual;
        check(poll(manual, 30, 0, pose).confirmed, "only matching native pose confirms the request");
        for (unsigned i = 0; i < 10; ++i) {
            const auto out = poll(manual, 40 + i * 10, i % 2 ? 25000 : -25000, pose);
            check(out.confirmed && out.y == (nose ? 20000 : -20000),
                "balance corrections cannot reverse an active native manual request");
        }
        check(poll(manual, 279, 0, SkatePose::unknown).owns_stick,
            "brief observer gap retains the existing partial hold");
        check(!poll(manual, 280, 0, SkatePose::unknown).owns_stick,
            "confirmed hold expires exactly150ms after its last matching observation");
        check(!poll(manual, 281, 25000).owns_stick && !poll(manual, 282, -25000).owns_stick,
            "expired ownership cannot inherit a held direction as a fresh edge");
    }
    {
        dingosdk::arcade_mode::test::SessionManual manual;
        start(manual);
        check(poll(manual, 1219).owns_stick && !poll(manual, 1220).owns_stick,
            "unconfirmed request releases exactly1200ms after initiation");
        start(manual, 1300);
        check(poll(manual, 1330, 0, SkatePose::manual).confirmed, "fresh retry confirms");
        check(!poll(manual, 1331, 0, SkatePose::air).owns_stick,
            "observed takeoff immediately releases a confirmed manual");
    }
    for (auto pose : {SkatePose::manual, SkatePose::nose_manual, SkatePose::unknown,
                      SkatePose::bail, SkatePose::grind}) {
        dingosdk::arcade_mode::test::SessionManual manual;
        poll(manual, 0, 0, pose);
        poll(manual, 10, 25000, pose);
        check(!poll(manual, 20, -25000, pose).owns_stick,
            "correction or unknown incompatible pose cannot initiate a manual");
    }
    for (auto pose : {SkatePose::bail, SkatePose::grind, SkatePose::nose_manual}) {
        dingosdk::arcade_mode::test::SessionManual manual;
        start(manual);
        check(!poll(manual, 30, 0, pose).owns_stick,
            "independent incompatible native pose releases even if external eligibility lags");
    }
    constexpr std::uint16_t conflicting_buttons[]{0x1000u, 0x2000u, 0x4000u, 0x8000u, 0x0010u, 0x0020u, 0x0300u};
    for (const auto button : conflicting_buttons) {
        dingosdk::arcade_mode::test::SessionManual manual;
        start(manual);
        check(!poll(manual, 30, 0, SkatePose::manual, button).owns_stick,
            "face menu and board commands cannot retain manual synthetic ownership");
    }
    {
        dingosdk::arcade_mode::test::SessionManual manual;
        poll(manual, 0);
        poll(manual, 10, 25000);
        poll(manual, 20, 0, SkatePose::riding, 0, 25000);
        check(poll(manual, 30, -25000).owns_stick, "sideways center passage retains a fresh bounded vertical sequence");
        manual.cancel(); poll(manual, 100);
        poll(manual, 110, 25000);
        poll(manual, 120, 0, SkatePose::riding, 3);
        check(!poll(manual, 130, -25000).owns_stick, "opposing D-pad directions break the sequence");
        manual.cancel(); poll(manual, 200);
        poll(manual, 210, 25000);
        check(!poll(manual, 711, -25000).owns_stick, "a long hold exceeds the sampled ground direction deadline");
        manual.cancel(); poll(manual, 600);
        poll(manual, 610, 25000);
        check(poll(manual, 830, -25000).owns_stick, "direction sequence accepts the220ms boundary");
        check(!poll(manual, 829).owns_stick, "backwards time cancels manual state");
    }
    {
        dingosdk::arcade_mode::test::SessionManual manual;
        poll(manual, 0, 0, SkatePose::air);
        poll(manual, 10, 0, SkatePose::air, 1);
        auto out = poll(manual, 20, 0, SkatePose::air, 2);
        check(out.owns_stick && !out.confirmed && out.y == -20000,
            "airborne D-pad command can prepare a manual without claiming it landed");
        check(poll(manual, 21, 0, SkatePose::air, 2).y == -20000,
            "repeated held direction does not create another command");
        check(poll(manual, 22, 0, SkatePose::manual).confirmed, "actual manual landing confirms airborne preparation");
        check(!poll(manual, 23, 0, SkatePose::manual, 0, 0, true, true).owns_stick,
            "external owner conflict immediately releases partial input");
        start(manual, 100);
        check(!poll(manual, 121, 0, SkatePose::manual, 0, 0, true, false, false).owns_stick,
            "gameplay loss cancels synthetic ownership");
        start(manual, 200);
        check(!poll(manual, 221, 0, SkatePose::manual, 0, 0, false).owns_stick,
            "native eligibility loss cancels synthetic ownership");
    }
    {
        auto prepare = [](dingosdk::arcade_mode::test::SessionManual& manual) {
            poll(manual, 0, 0, SkatePose::air);
            poll(manual, 10, 25000, SkatePose::air);
            poll(manual, 20, -25000, SkatePose::air);
            check(poll(manual, 100, 0, SkatePose::manual).confirmed, "native initial wheel contact confirms prepared landing");
        };
        dingosdk::arcade_mode::test::SessionManual manual;
        prepare(manual);
        check(poll(manual, 117, 0, SkatePose::air).owns_stick &&
              poll(manual, 267, 0, SkatePose::manual).confirmed,
              "measured17ms contact and150ms rebound preserve the initial landing request");
        check(!poll(manual, 268, 0, SkatePose::manual, 0x1000).owns_stick,
              "intentional Ollie immediately wins over landing-settle ownership");
        manual.cancel(); prepare(manual);
        check(poll(manual, 299, 0, SkatePose::air).owns_stick &&
              !poll(manual, 300, 0, SkatePose::air).owns_stick,
              "unconfirmed rebound releases at200ms without new contact");
        manual.cancel(); prepare(manual);
        poll(manual, 250, 0, SkatePose::manual);
        check(!poll(manual, 350, 0, SkatePose::air).owns_stick,
              "settle opportunity expires250ms after first contact even with later contact");
        manual.cancel(); prepare(manual);
        check(!poll(manual, 250, 0, SkatePose::unknown).owns_stick,
              "unknown evidence retains the original150ms limit");
    }
    {
        auto prepare_nose = [](dingosdk::arcade_mode::test::SessionManual& manual) {
            poll(manual, 0, 0, SkatePose::air);
            poll(manual, 10, -25000, SkatePose::air);
            poll(manual, 20, 25000, SkatePose::air);
        };
        dingosdk::arcade_mode::test::SessionManual manual; prepare_nose(manual);
        auto out = poll(manual, 100, 0, SkatePose::manual);
        check(out.owns_stick && !out.confirmed && out.y == 20000,
              "opposite first wheel contact retains prepared input without claiming a match");
        check(poll(manual, 117, 0, SkatePose::air).owns_stick &&
              poll(manual, 317, 0, SkatePose::nose_manual).confirmed,
              "measured17ms wrong contact plus200ms rebound can reach the requested native pose");
        check(!poll(manual, 318, 0, SkatePose::manual).owns_stick,
              "opposite pose after actual confirmation still releases immediately");
        manual.cancel(); prepare_nose(manual); poll(manual, 100, 0, SkatePose::manual);
        check(poll(manual, 149, 0, SkatePose::manual).owns_stick &&
              !poll(manual, 150, 0, SkatePose::manual).owns_stick,
              "sustained wrong wheel contact expires at50ms");
        manual.cancel(); prepare_nose(manual); poll(manual, 100, 0, SkatePose::manual);
        check(poll(manual, 349, 0, SkatePose::air).owns_stick &&
              !poll(manual, 350, 0, SkatePose::air).owns_stick,
              "missing desired contact expires250ms after first wrong wheel");
        manual.cancel(); prepare_nose(manual); poll(manual, 100, 0, SkatePose::manual);
        check(!poll(manual, 117, 0, SkatePose::air, 0x1000).owns_stick,
              "intentional trick cancels even during initial wrong-wheel recovery");
    }
    {
        dingosdk::arcade_mode::test::SessionManual manual;
        const auto base = std::numeric_limits<std::uint64_t>::max() - 1300;
        start(manual, base);
        check(!poll(manual, base + 1220).owns_stick, "near-limit timestamps use bounded differences without overflow");
    }
    for (const unsigned frame_ms : {17u, 33u}) {
        for (const bool nose : {false, true}) {
            for (const bool neutral_between : {false, true}) {
                dingosdk::arcade_mode::test::SessionManual manual;
                // Steering has been held well beyond the direction-pair window.
                for (unsigned ms = 0; ms < 1000; ms += frame_ms)
                    check(!poll(manual, ms, 25000).owns_stick,
                        "ordinary sustained forward does not request a manual");
                const auto first = static_cast<std::uint16_t>(nose ? 2 : 1);
                const auto second = static_cast<std::uint16_t>(nose ? 1 : 2);
                poll(manual, 1000, 25000, SkatePose::riding, first);
                if (neutral_between) poll(manual, 1000 + frame_ms, 25000);
                const auto out = poll(manual, 1000 + 2 * frame_ms, 25000, SkatePose::riding, second);
                check(out.owns_stick == nose,
                    "fresh Dpad Down then effective Up qualifies Nose Manual, including release back to held stick; same logical Up alone supplies no new press");
            }
        }
        dingosdk::arcade_mode::test::SessionManual manual;
        poll(manual, 0); poll(manual, frame_ms, 25000);
        check(poll(manual, frame_ms * 2, 25000, SkatePose::riding, 2).owns_stick,
            "fresh canonical Up then Dpad Down share the one command history");
        manual.cancel(); poll(manual, 0);
        poll(manual, frame_ms, 0, SkatePose::riding, 1);
        check(poll(manual, frame_ms * 2, -25000).owns_stick,
            "fresh Dpad Up then newly moved stick Down share the one command history");
    }
    for (const bool nose : {false, true}) {
        dingosdk::arcade_mode::test::SessionManual manual;
        poll(manual, 0, 0, SkatePose::air);
        poll(manual, 17, nose ? -25000 : 25000, SkatePose::air);
        poll(manual, 50, nose ? 25000 : -25000, SkatePose::air);
        check(poll(manual, 700, 0, SkatePose::air).owns_stick,
            "early airborne preparation retains the native method through the jump");
        check(poll(manual, 740).owns_stick && poll(manual, 1600).owns_stick,
            "landing actuation gets its own bounded window instead of expiring from the apex");
        const auto pose = nose ? SkatePose::nose_manual : SkatePose::manual;
        check(poll(manual, 1617, 0, pose).confirmed,
            "only the independently observed requested manual confirms prepared landing");
        manual.cancel(); poll(manual, 0, 0, SkatePose::air);
        poll(manual, 17, 25000, SkatePose::air); poll(manual, 50, -25000, SkatePose::air);
        poll(manual, 740); poll(manual, 900, 0, SkatePose::air);
        check(!poll(manual, 2000).owns_stick,
            "later rebound cannot restart the1200ms failed-landing deadline");
    }
    std::cout << checks << " manual input checks passed\n";
    return 0;
}
#ifndef ARCADE_PLUMBING_COMBINED_TESTS
int main() { return run_arcade_manual_tests(); }
#endif

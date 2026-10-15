#include "Extension/ArcadeMode/arcade_model.h"
#include "Extension/ArcadeMode/arcade_combo_session.h"
#include "Extension/ArcadeMode/arcade_native_events.h"
#include "Extension/ArcadeMode/arcade_balance_input.h"
#include "Extension/ArcadeMode/arcade_observation.h"
#include "Extension/ArcadeMode/arcade_native_probe.h"
#include "Extension/ArcadeMode/arcade_ollie.h"
#include "Extension/ArcadeMode/arcade_flip.h"
#include "Extension/ArcadeMode/arcade_gesture.h"
#include "Extension/ArcadeMode/arcade_trick_table.h"
#include "Extension/ArcadeMode/arcade_native_trick.h"
#include "Extension/ArcadeMode/arcade_authored_playback.h"
#include "Extension/ArcadeMode/arcade_native_mapping.h"
#include "Extension/ArcadeMode/arcade_classic_input.h"
#include "Extension/ArcadeMode/arcade_camera.h"
#include "Extension/ArcadeMode/arcade_score_labels.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <type_traits>

namespace {
unsigned checks{};
void check(bool passed, const char* description) {
    ++checks;
    if (!passed) { std::cerr << "FAIL: " << description << '\n'; std::exit(1); }
}
}
#ifdef ARCADE_PLAYBACK_COMBINED_TESTS
int run_arcade_playback_tests();
#endif
int main() {
#ifdef ARCADE_PLAYBACK_COMBINED_TESTS
    run_arcade_playback_tests();
#endif
    using namespace dingosdk::arcade_mode;
    for (unsigned context = 0; context < 3; ++context) {
        const std::array catalog{PreparedGesture{1, false, GestureContext::ground,
            {{0, 0, -32767}, {18, 0, 32767}, {19, 0, 0}, {54, 0, 0}}}};
        ArcadeInput input;
        input.enable(true);
        const auto poll = [&](unsigned tick, std::uint16_t buttons, SkatePose pose) {
            return input.poll(0, tick * 17, buttons, 0, 0, 0, 0, true, true, false, 0,
                false, false, true, tick, false, catalog, false, false, pose, context != 0);
        };
        poll(0, 0, SkatePose::riding);
        poll(1, ClassicGesture::a, SkatePose::riding);
        poll(30, 0, SkatePose::air);
        const auto neutral = poll(31, 0, SkatePose::air);
        check(neutral.command_busy && neutral.authored_playback && neutral.right_y == 0,
              "authored neutral was delivered before grab handoff");
        const auto next = poll(32, 0x2000, context == 1 ? SkatePose::riding : SkatePose::air);
        check(next.command_busy == (context != 2) && next.authored_playback == (context != 2),
              "only enabled airborne B can end the neutral authored tail");
    }
    {
        ManualGesture manual;
        auto poll = [&](std::uint64_t ms, short y, SkatePose pose = SkatePose::riding,
                        bool safe = true, bool conflict = false, bool gameplay = true) {
            return manual.poll(ms, 0, 0, y, gameplay, safe, pose, conflict);
        };
        poll(0, 25000); poll(100, -25000);
        check(!poll(150, 0).owns_stick, "held direction on enable cannot form a manual request");
        poll(200, 25000);
        auto out = poll(350, -25000);
        check(out.owns_stick && out.y == -20000 && !out.confirmed,
            "up down requests the measured manual stick position without claiming native success");
        out = poll(400, 0, SkatePose::manual);
        check(out.confirmed && out.y == -20000, "only the observed matching pose confirms a manual");
        check(poll(540, 25000, SkatePose::unknown).owns_stick, "short observation gaps preserve native manual hold");
        check(!poll(550, -25000, SkatePose::riding).owns_stick, "an observed manual exit releases after bounded grace");
        poll(600, 0); poll(650, -25000); out = poll(850, 25000);
        check(out.owns_stick && out.y == 20000 && !out.confirmed,
            "down up requests the distinct nose manual position");
        check(!poll(900, 0, SkatePose::manual).confirmed, "an opposite native pose cannot confirm the request");
        check(!poll(2050, 0).owns_stick, "unconfirmed requests expire and neutralize");
        poll(2100, 0); poll(2150, 25000);
        check(!poll(2501, -25000).owns_stick, "late directional pairs are not manual commands");
        manual.cancel(); poll(3000, 0); poll(3050, 25000);
        check(!poll(3100, -25000, SkatePose::bail, false).owns_stick,
            "unsafe native state cancels a pending manual sequence");
        poll(3200, 0); poll(3250, 25000); poll(3300, -25000);
        check(!poll(3350, 0, SkatePose::manual, true, true).owns_stick,
            "a conflicting synthetic gesture immediately releases manual ownership");
        poll(3400, 0); poll(3450, 25000); poll(3500, -25000);
        check(!poll(3550, 0, SkatePose::manual, true, false, false).owns_stick,
            "menus immediately cancel a native manual hold");
        poll(3600, 0); poll(3650, 25000); poll(3700, -25000);
        check(!poll(3699, 0).owns_stick, "backwards time clears manual latches");
        manual.cancel(); poll(4000, 0);
        manual.poll(4050, 1, 0, 0, true, true, SkatePose::riding, false);
        out = manual.poll(4100, 2, 0, 0, true, true, SkatePose::riding, false);
        check(out.owns_stick && out.y == -20000, "D-pad sequences use the same exact manual request");
        manual.cancel(); poll(4200, 0); poll(4250, 25000);
        manual.poll(4300, 3, 0, 0, true, true, SkatePose::riding, false);
        check(!poll(4350, -25000).owns_stick, "contradictory directions break a pending manual sequence");
    }
    {
        check(score_label_id("Manual") == 1 && score_label_id("BS Revert") == 4,
            "existing score label IDs remain stable");
        check(score_label_id("Nose Grab") == 6 && score_label_id("Tail Grab") == 8 && score_label_id("Japan Air") == 9,
            "exact native guide spellings share their semantic diagnostic IDs");
        check(score_label_id("One-Foot Tail Grab") == 16 && score_label_id("Judo Air") == 18,
            "native one-foot guide spellings use the requested semantic identity");
        for (std::size_t i = 0; i < score_labels.size(); ++i)
            check(score_label_id(score_labels[i]) == i + 1, "score labels have distinct diagnostic IDs");
        check(score_label_id("private player text") == 0 && score_label_id("Melon private") == 0 &&
            score_label_id("melon") == 0, "unrecognized score text is never emitted");
    }
    {
        const std::array<float, 16> follow{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,2,-4,1};
        const std::array<float, 3> target{0,1,0};
        CameraOrbit orbit;
        const auto initial = orbit.update(follow, target, 0, 0, 0);
        check(initial && std::abs((*initial)[13] - 2) < .001f && std::abs((*initial)[14] + 4) < .001f,
            "camera orbit begins at the existing follow-camera position");
        for (unsigned i = 0; i < 300; ++i) {
            const auto camera = orbit.update(follow, target, 1, -1, .016f);
            check(camera.has_value(), "sustained camera input stays finite");
            const auto& m = *camera;
            const float radius_squared = m[12]*m[12] + (m[13]-1)*(m[13]-1) + m[14]*m[14];
            check(std::abs(radius_squared - 17) < .01f && m[3] == 0 && m[7] == 0 && m[11] == 0 && m[15] == 1,
                "camera orbit preserves follow distance and affine transform");
            const float orthogonal = m[0]*m[4] + m[1]*m[5] + m[2]*m[6];
            check(std::abs(orthogonal) < .001f, "camera basis stays orthogonal at pitch limits");
        }
        orbit.reset();
        const auto reset = orbit.update(follow, target, 0, 0, 0);
        check(reset == initial, "camera reset clears accumulated orbit state");
        check(!orbit.update(follow, target, std::numeric_limits<float>::quiet_NaN(), 0, .016f),
            "invalid camera input cannot reach native transform setter");
    }
    {
        const std::vector<AuthoredStickStep> steps{{0,0,-32767},{18,28000,28000},{22,0,0},{54,0,0}};
        const std::array<PreparedGesture, 4> catalog{{{1,false,GestureContext::ground,steps},
            {2,false,GestureContext::ground,steps}, {3,false,GestureContext::ground,steps},
            {17,true,GestureContext::ground,steps}}};
        ClassicGesture controls;
        auto out = controls.poll(0, 0, ClassicGesture::a, 0, 0, true, true, false, catalog);
        check(out.requested_id == 1 && out.owns_stick && out.right_y == -32767 && !out.buttons,
            "Classic A starts native authored crouch and consumes native push");
        out = controls.poll(1000, 60, ClassicGesture::a, 0, 0, true, false, false, catalog);
        check(out.right_y == -32767, "Classic A remains crouched until released");
        out = controls.poll(1100, 66, 0, 0, 0, true, false, false, catalog);
        check(out.right_y == 28000, "Classic A release begins native authored flick");
        controls.cancel();
        out = controls.poll(1200, 72, ClassicGesture::a, 0, 0, true, true, false, catalog);
        check(!out.owns_stick && !out.requested_id, "mode reset cannot convert held A into a new crouch");
        controls.poll(1300, 78, 0, 0, 0, true, true, false, catalog);
        out = controls.poll(1400, 84, ClassicGesture::x | 4, 0, 0, true, true, false, catalog);
        check(!out.requested_id && !(out.buttons & ClassicGesture::x), "X waits briefly for exact repeated-button selection");
        check(!(out.buttons & 15), "directional trick input cannot open a native gameplay menu");
        out = controls.poll(1600, 96, 0, 0, 0, true, true, false, catalog);
        check(out.requested_id == 2 && out.right_y == -32767, "left X selects the exact authored kickflip");
        controls.cancel(); controls.poll(1700, 102, 0, 0, 0, true, true, false, catalog);
        controls.poll(1800, 108, ClassicGesture::x | 4, 0, 0, true, true, false, catalog);
        controls.poll(1850, 111, 4, 0, 0, true, true, false, catalog);
        controls.poll(1900, 114, ClassicGesture::x | 4, 0, 0, true, true, false, catalog);
        out = controls.poll(2100, 126, 0, 0, 0, true, true, false, catalog);
        check(out.unsupported && !out.requested_id && !out.owns_stick,
            "unsupported double kickflip never executes a single kickflip substitute");
        controls.poll(2200, 132, ClassicGesture::x | 4, 0, 0, true, true, true, catalog);
        out = controls.poll(2400, 144, 0, 0, 0, true, true, true, catalog);
        check(out.requested_id == 17, "LT context selects the distinct authored nollie identifier");
        out = controls.poll(2500, 150, ClassicGesture::a | 4, 0, 0, false, true, true, catalog);
        check(out.buttons == (ClassicGesture::a | 4) && !out.owns_stick,
            "menus retain native buttons and directional navigation while cancelling synthetic ownership");
        controls.poll(2600, 156, 0, 0, 0, true, true, false, catalog);
        controls.poll(2700, 162, ClassicGesture::a, 0, 0, true, true, false, catalog);
        out = controls.poll(2800, 161, ClassicGesture::a | ClassicGesture::x | 4, 0, 0, true, true, false, catalog);
        check(!out.owns_stick && !controls.busy(), "game-tick regression cancels both held crouch and pending trick intent");
        out = controls.poll(2900, 170, ClassicGesture::a, 0, 0, true, true, false, catalog);
        check(!out.requested_id, "cancelled clock boundary requires release before rearming");
        check(classic_direction(5, 32767, 0) == TrickDirection::up_left &&
            classic_direction(3, 0, 32767) == TrickDirection::invalid,
            "D-pad overrides steering and contradictory directions are rejected");
        ArcadeInput integrated;
        out = {};
        auto packet = integrated.poll(0, 3000, ClassicGesture::a, 0, 0, 9000, 10000,
            true, true, false, 0, false, true, true, 180, false, catalog);
        check(packet.buttons == ClassicGesture::a && packet.right_x == 9000 && packet.right_y == 10000,
            "Arcade OFF preserves physical input with an authored catalog available");
        integrated.enable(true);
        packet = integrated.poll(0, 3100, ClassicGesture::a, 0, 0, 0, 0,
            true, true, false, 0, false, true, true, 186, false, catalog);
        check(packet.phase == 0 && !packet.requested_id, "enabling authored controls requires held A release");
        integrated.poll(0, 3200, 0, 0, 0, 0, 0, true, true, false, 0, false, true, true, 192, false, catalog);
        packet = integrated.poll(0, 3300, ClassicGesture::a, 0, 0, 0, 0,
            true, true, false, 0, false, true, true, 198, false, catalog);
        check(packet.requested_id == 1 && packet.phase == 1 && packet.right_y == -32767,
            "rearmed authored A owns native crouch through the combined adapter");
        integrated.enable(false);
        packet = integrated.poll(0, 3400, ClassicGesture::a, 0, 0, -9000, 12000,
            true, true, false, 0, false, true, true, 204, false, catalog);
        check(packet.phase == 0 && packet.buttons == ClassicGesture::a && packet.right_x == -9000 && packet.right_y == 12000,
            "disabling during authored crouch immediately restores physical buttons and axes");
        integrated.enable(true);
        const auto manual_packet = [&](std::uint64_t ms, short ly, std::uint16_t buttons = 0,
                                       SkatePose pose = SkatePose::riding) {
            return integrated.poll(0, ms, buttons, 0, ly, 9000, 12000, true, true, true, 0,
                false, true, true, ms / 16, false, catalog, false, true, pose);
        };
        manual_packet(4000, 0); manual_packet(4100, 25000);
        packet = manual_packet(4200, -25000);
        check(packet.phase == 1 && packet.right_x == 0 && packet.right_y == -20000 && !packet.buttons,
            "manual request owns the synthetic stick and suppresses automatic push");
        packet = manual_packet(4300, 0, ClassicGesture::a, SkatePose::manual);
        check(packet.requested_id == 1 && packet.right_y == -32767,
            "authored ollie takes priority over a manual hold");
        integrated.enable(false);
        packet = manual_packet(4400, -25000);
        check(packet.phase == 0 && packet.right_x == 9000 && packet.right_y == 12000,
            "manual cancellation on mode OFF restores physical camera axes unchanged");
    }
    {
        check(authored_name_tokens("gameplay/input/gestures/fliptricks/gesture_fliptrick_impossible") == 1 &&
            authored_name_tokens("gesture_fliptrick_front_foot_impossible_fromgrind") == 33 &&
            authored_name_tokens("gesture_fliptrick_no_comply") == 2,
            "authored label evidence uses a fixed numeric vocabulary");
        check(authored_name_tokens("unrelated/private/value") == 0,
            "unknown authored names cannot become diagnostic text");
        check(native_flip_id(ArcadeTrick::impossible, false) == 36 &&
            native_flip_id(ArcadeTrick::impossible, true) == 38,
            "current-game Impossible labels select exact regular and nollie identifiers");
        check(native_flip_id(ArcadeTrick::double_impossible, false) == 0 &&
            native_flip_id(ArcadeTrick::double_kickflip, false) == 0,
            "unestablished exact variants never fall back to basic flips");
        std::array<AuthoredGesture, 2> catalog{};
        catalog[0].native_id = 2;
        catalog[0].context = GestureContext::from_grind;
        catalog[1].native_id = 17;
        catalog[1].nollie = true;
        check(!exact_authored_gesture(catalog, 2, false, GestureContext::ground),
            "ground request cannot borrow a grind-exit recipe");
        check(exact_authored_gesture(catalog, 2, false, GestureContext::from_grind) == &catalog[0],
            "grind exit retains its authored context");
        check(!exact_authored_gesture(catalog, 17, false, GestureContext::ground) &&
            exact_authored_gesture(catalog, 17, true, GestureContext::ground) == &catalog[1],
            "native nollie context must agree with the authored gesture");
    }
    {
        AuthoredGesture authored{};
        authored.arcs = {{-10, 10, .8f, 1, 0}, {170, 190, .8f, 1, 0}};
        const auto steps = compile_authored_flick(authored);
        check(steps.size() == 4 && steps.front().tick == 0 && steps.front().y == -32767,
            "authored setup rotates data by half a circle and reaches full crouch");
        check(steps[1].tick == 18 && steps[1].y > 29000,
            "authored flick follows the existing eighteen tick setup recipe");
        check(steps[2].x == 0 && steps[2].y == 0 && steps[3].x == 0 && steps[3].y == 0,
            "authored playback includes neutral release and neutral completion");
        AuthoredPlayback playback;
        check(playback.start(steps, 100, true) && playback.sample(100).y == -32767 &&
            playback.sample(500).y == -32767, "held A can retain authored crouch without an automatic release");
        playback.release(501);
        check(playback.sample(501).y > 29000 && playback.sample(501).y > 29000,
            "A release begins the flick once; repeated input polls do not advance it");
        check(!playback.sample(600).owns_stick && !playback.busy(),
            "completed authored gesture relinquishes synthetic ownership");
        playback.start(steps, 700, false);
        playback.sample(710);
        check(!playback.sample(709).owns_stick && !playback.busy(),
            "backwards game ticks cancel authored input");
        playback.start(steps, 800, true);
        playback.cancel();
        check(!playback.sample(801).owns_stick, "toggle cancellation clears held authored input");
        const std::array<AuthoredStickStep, 3> lateral{{{0, -30000, -12000}, {18, 20000, 25000}, {24, 0, 0}}};
        playback.start(lateral, 900, true, true);
        check(playback.sample(900).x == 30000 && playback.sample(900).y == -12000,
            "stance reflection changes lateral setup without changing crouch depth");
        playback.release(930);
        check(playback.sample(930).x == -20000 && playback.sample(930).y == 25000 && lateral[1].x == 20000,
            "stance reflection preserves authored timing and the immutable catalog");
        check(!playback.sample(940).owns_stick, "reflected authored input returns to neutral completion");
        const std::array<AuthoredStickStep, 6> short_arcs{{
            {0, 0, -32767}, {18, 12000, 20000}, {19, 26000, 9000},
            {20, -22000, 15000}, {24, 0, 0}, {56, 0, 0}}};
        playback.start(short_arcs, 1000, false);
        check(playback.sample(1018).x == 12000 && playback.sample(1020).x == 26000,
            "a missed input poll cannot skip a one-tick authored arc");
        check(playback.sample(1020).x == 26000 && playback.sample(1021).x == -22000,
            "catch-up never collapses distinct arcs into repeated reads of one game tick");
        check(playback.sample(1025).x == 0 && !playback.sample(1057).owns_stick,
            "delayed arc delivery retains bounded neutral release and completion");
        playback.start(short_arcs, 2000, true);
        playback.sample(2050);
        playback.release(2051);
        check(playback.sample(2051).x == 12000 && playback.sample(2053).x == 26000 &&
            playback.sample(2054).x == -22000,
            "held A release also delivers every short authored arc after a missed poll");
        check(!playback.sample(2231).owns_stick, "late playback still expires at its absolute safety bound");
        authored.arcs.push_back({-10, 10, .8f, 1, 0});
        const auto turning = compile_authored_flick(authored);
        check(turning.size() >= 6 && turning[2].tick + 1 == turning[3].tick,
            "later half-circle movement receives intermediate game ticks");
        authored.arcs = {{350, 10, .8f, 1, 0}};
        check(compile_authored_flick(authored).front().y == -32767,
            "authored arc crossing zero uses the short angular interval");
        authored.arcs[0].start = std::numeric_limits<float>::infinity();
        check(compile_authored_flick(authored).empty(), "nonfinite authored angles cannot produce input");
        authored.arcs[0] = {0, 10, .9f, .1f, 0};
        check(compile_authored_flick(authored).empty(), "reversed authored magnitude limits cannot produce input");
        authored.arcs.resize(33);
        check(compile_authored_flick(authored).empty(), "authored playback remains bounded");
        authored.arcs.clear();
        check(compile_authored_flick(authored).empty(), "missing exact authored gesture produces no substitute input");
    }
    {
        namespace layout = dingosdk::game::build::v20260929::arcade_tricks;
        constexpr std::uintptr_t core = 0x100000, selection = 0x200000;
        unsigned pointer_reads{};
        bool replace{}, unreadable{};
        std::int32_t id = 2;
        auto read = [&](std::uintptr_t at, auto& value) noexcept {
            using T = std::remove_reference_t<decltype(value)>;
            if (at == core + layout::core_selection) {
                value = static_cast<T>(selection + (replace && pointer_reads++ ? 0x100 : 0));
                return true;
            }
            if (at == selection + layout::selection_flip && !unreadable) {
                value = static_cast<T>(id); return true;
            }
            return false;
        };
        check(read_flip_selection(read, core) == NativeFlipSelection{true, 2}, "read the actual native flip selection");
        replace = true;
        check(!read_flip_selection(read, core).valid, "selection replacement invalidates native trick evidence");
        replace = false; unreadable = true;
        check(!read_flip_selection(read, core).valid, "unreadable native selection remains unknown");
        unreadable = false;
        for (const int value : {-2, 64, 10000}) {
            id = value;
            check(!read_flip_selection(read, core).valid, "out-of-range native identifiers are rejected");
        }
        id = -1;
        check(read_flip_selection(read, core) == NativeFlipSelection{true, -1}, "native no-trick sentinel remains distinct from unknown");
        check(!read_flip_selection(read, 0).valid && !read_flip_selection(read, UINTPTR_MAX).valid,
            "native selection never follows null or overflowing core addresses");
    }
    {
        for (std::size_t i = 0; i < literal_trick_table.size(); ++i)
            for (std::size_t j = i + 1; j < literal_trick_table.size(); ++j) {
                const auto& a = literal_trick_table[i]; const auto& b = literal_trick_table[j];
                check(a.family != b.family || a.direction != b.direction || a.presses != b.presses,
                    "literal trick table has no ambiguous duplicate input");
            }
        for (const auto& row : literal_trick_table) {
            check(select_named_trick(row.family, row.direction, 0) == ArcadeTrick::none,
                  "zero presses cannot select a trick");
            check(select_named_trick(row.family, row.direction, 4) == ArcadeTrick::none,
                  "unassigned repetition cannot fall back to an easier variant");
        }
        check(select_named_trick(TrickFamily::flip, TrickDirection::up_left, 3) == ArcadeTrick::none,
              "unassigned triple diagonal remains unselected");
        check(select_named_trick(TrickFamily::grab, TrickDirection::right, 3) == ArcadeTrick::none,
              "unassigned triple grab cannot silently substitute");
        check(select_named_trick(TrickFamily::flip, TrickDirection::none, 1) == ArcadeTrick::none,
              "literal table does not invent a neutral flip default");
        check(select_grind(TrickDirection::none, 1, 1, RailRelationship::unknown) == ArcadeTrick::none,
              "unknown rail relationship cannot imply a 50-50");
        check(select_grind(TrickDirection::up, 1, 2, RailRelationship::unknown) == ArcadeTrick::nosebluntslide &&
              select_grind(TrickDirection::down, 1, 2, RailRelationship::unknown) == ArcadeTrick::bluntslide,
              "direction repetition is distinct from grind-button repetition");
        check(select_grind(TrickDirection::left, 1, 1, RailRelationship::parallel) ==
              select_grind(TrickDirection::right, 1, 1, RailRelationship::parallel),
              "lateral grind requests retain contextual pair identity");
        check(select_grind(TrickDirection::up, 2, 2, RailRelationship::unknown) == ArcadeTrick::none,
              "unassigned combination of button and direction repetition stays unselected");
    }
    {
        BoardChord chord;
        constexpr auto pair = BoardChord::pair;
        check(chord.poll(0, pair, true, true).buttons == pair, "held chord across enable cannot mount");
        chord.poll(1, 0, true, true);
        check(chord.poll(2, BoardChord::lb, true, true).buttons == BoardChord::lb, "LB alone passes through");
        auto out = chord.poll(100, pair, true, true);
        check(out.pulse && out.buttons == BoardChord::native_y, "small chord requests one native board press");
        check(chord.poll(219, pair, true, true).pulse, "board pulse remains bounded but long enough for native input");
        check(chord.poll(220, pair, true, true).buttons == 0, "native Y releases even with the chord held");
        check(!chord.poll(5000, pair, true, true).pulse, "held chord cannot oscillate after native state changes");
        chord.poll(5001, BoardChord::rb, true, true);
        check(!chord.poll(5002, pair, true, true).pulse, "partial release cannot rearm chord");
        chord.poll(5003, 0, true, true);
        check(chord.poll(5004, BoardChord::rb, true, true).buttons == BoardChord::rb, "RB alone passes through");
        check(chord.poll(5125, pair, true, true).buttons == pair, "late pair remains ordinary native input");
        chord.poll(5126, BoardChord::rb, true, true);
        check(!chord.poll(5127, pair, true, true).pulse, "late pair requires complete release before another attempt");
        for (unsigned cycle = 0; cycle < 40; ++cycle) {
            const auto now = 6000ull + cycle * 1000;
            chord.poll(now, 0, true, true);
            check(chord.poll(now + 1, pair, true, true).pulse, "fresh chord supports repeated native board transitions");
            check(!chord.poll(now + 2, pair, true, false).pulse, "lost safe native state immediately ends synthetic Y");
            check(!chord.poll(now + 3, pair, true, true).pulse, "state recovery cannot replay the same chord");
        }
        check(chord.poll(50000, pair, false, true).buttons == pair, "menus preserve physical bumpers");
        check(!chord.poll(50001, pair, true, true).pulse, "menu close requires release");
        chord.poll(50002, 0, true, true);
        check(!chord.poll(50003, pair | 1, true, true).pulse, "directional marker chord is not a board toggle");
        chord.reset(); chord.poll(1, 0, true, true);
        chord.poll(2, pair, true, true);
        check(!chord.poll(0, pair, true, true).pulse, "clock rollback neutralizes board request");
    }
    {
        ArcadeInput input;
        input.enable(true);
        auto poll = [&](std::uint64_t now, std::uint16_t buttons, bool gameplay = true) {
            return input.poll(0, now, buttons, 0, 0, 0, 0, gameplay, true, true, 0, false, true);
        };
        poll(0, 0);
        auto out = poll(1, BoardChord::pair);
        check(out.buttons == BoardChord::native_y && out.phase == 3, "board adapter emits native Y with distinct packet phase");
        out = poll(122, BoardChord::pair);
        check(out.buttons == 0 && out.phase == 0, "held chord releases synthetic Y without leaking bumpers");
        input.enable(false);
        check(poll(123, BoardChord::pair).buttons == BoardChord::pair, "OFF restores physical board chord immediately");
        input.enable(true);
        check(poll(124, BoardChord::pair).buttons == BoardChord::pair, "ON cannot treat a held board chord as fresh");
        poll(125, 0);
        poll(126, NativeGesture::a);
        check(poll(127, BoardChord::pair).phase != 3, "board action never overlaps synthetic trick preparation");
    }
    {
        ArcadeInput input;
        input.enable(true);
        auto poll = [&](std::uint64_t now, std::uint16_t buttons, bool trigger) {
            return input.poll(0, now, buttons, 0, 0, 0, 0, true, true, true, 0, trigger, true);
        };
        poll(0, 0, false);
        poll(300, 0, true);
        check(poll(800, BoardChord::pair, true).buttons == BoardChord::pair,
            "held trigger suppresses board translation");
        check(poll(2200, BoardChord::pair, false).phase != 3,
            "trigger release cannot resurrect a rejected chord");
        poll(2201, 0, false);
        check(poll(2202, BoardChord::pair, false).phase == 3,
            "full release rearms after a rejected trigger chord");
    }
    {
        AutomaticPush push;
        check(!push.poll(0, 0, 25000, 0, true, true), "held movement across enable does not auto push");
        push.poll(1, 0, 0, 0, true, true);
        check(push.poll(2, 0, 25000, 0, true, true), "fresh movement at rest requests native push");
        check(push.poll(181, 0, 25000, 4.5f, true, true), "acceleration hysteresis survives lower speed crossing");
        check(!push.poll(182, 0, 25000, 4.5f, true, true), "native push has a bounded hold");
        check(!push.poll(851, 0, 25000, 4.5f, true, true), "cadence leaves an actual release gap");
        check(push.poll(852, 0, 25000, 4.5f, true, true), "acceleration continues on the next native pulse");
        check(!push.poll(853, 0, 25000, 6.0f, true, true), "cruise speed immediately ends an active push");
        check(!push.poll(2000, 0, 25000, 5.0f, true, true), "cruise hysteresis prevents push chatter");
        check(!push.poll(3000, 0, 25000, 15.0f, true, true), "downhill speed is left alone");
        check(push.poll(4000, 0, 25000, 3.9f, true, true), "push resumes after slowing below lower threshold");
        check(!push.poll(4001, 0, 25000, 3.9f, true, false), "lost riding eligibility cancels a push immediately");
        check(!push.poll(5000, 0, -25000, 0, true, true), "backward stick intent never accelerates");
        check(!push.poll(6000, 0, 25000, std::numeric_limits<float>::quiet_NaN(), true, true), "unknown speed suppresses push");
        check(!push.poll(7000, 0, 25000, -1, true, true), "invalid negative speed suppresses push");
        check(push.poll(8000, 25000, 0, 1, true, true), "steering can resume native pushing");
        check(!push.poll(8001, 25000, 0, 1, false, true), "menu opening neutralizes push");
        check(!push.poll(9000, 25000, 0, 1, true, true), "menu closing with held movement requires release");
        push.poll(9001, 0, 0, 1, true, true);
        check(push.poll(9002, 25000, 0, 1, true, true), "released movement rearms after menu");
        check(!push.poll(5, 25000, 0, 1, true, true), "clock rollback cancels a pulse safely");
    }
    {
        ArcadeInput input;
        input.enable(true);
        auto poll = [&](std::uint64_t now, std::uint16_t buttons, short ly, bool allowed = true, bool trigger = false) {
            return input.poll(0, now, buttons, 0, ly, 0, 0, true, true, allowed, 0, trigger);
        };
        poll(0, 0, 0);
        auto out = poll(1, 0, 25000);
        check(out.buttons == NativeGesture::x && out.started == NativeTrick::none && !out.phase,
              "internal push reaches native button output without becoming a flip request");
        check(poll(2, 0, 25000, true, true).buttons == 0, "trigger context cancels internal push");
        check(poll(1000, NativeGesture::a, 25000).started == NativeTrick::ollie, "physical A retains trick priority");
        check(poll(1301, 0, 25000).buttons == 0, "gesture preparation remains exclusive after A release");
        check(poll(2100, 0, 25000).buttons == 0, "synthetic stick setup never overlaps automatic push");
        check(poll(2700, 0, 25000).buttons == 0, "synthetic flick never overlaps automatic push");
        input.enable(false);
        out = poll(2701, 0x2000, 25000);
        check(out.buttons == 0x2000 && !out.phase, "OFF preserves physical buttons and cancels push and gesture together");
        input.enable(true);
        check(poll(3000, 0, 25000).buttons == 0, "ON cannot inherit held movement from OFF");
        poll(3001, 0, 0);
        check(poll(3002, 0, 25000, false).buttons == 0, "air grind manual bail or stale native evidence cannot push");
    }
    {
        constexpr std::uint32_t physical_packet = 123;
        const std::array packets{
            physical_packet ^ synthetic_packet_mask(0, false),
            physical_packet ^ synthetic_packet_mask(0, true),
            physical_packet ^ synthetic_packet_mask(1, true),
            physical_packet ^ synthetic_packet_mask(2, true),
            physical_packet ^ synthetic_packet_mask(3, true)};
        for (std::size_t i = 0; i < packets.size(); ++i)
            for (std::size_t j = i + 1; j < packets.size(); ++j)
                check(packets[i] != packets[j], "every synthetic phase and native restoration changes an unchanged physical packet");
        check(synthetic_packet_mask(0, false) == 0, "disabled native packet remains unmodified");
    }
    {
        ArcadeInput input;
        auto poll = [&](unsigned user, std::uint64_t now, std::uint16_t buttons, short rx = 0, short ry = 0) {
            return input.poll(user, now, buttons, 0, 0, rx, ry, true, true);
        };
        auto out = poll(0, 0, NativeGesture::a, 12000, -9000);
        check(out.buttons == NativeGesture::a && out.right_x == 12000 && out.right_y == -9000 && !out.phase,
              "disabled input is physical input unchanged");
        for (unsigned cycle = 0; cycle < 20; ++cycle) {
            const auto now = 5000ull * cycle;
            input.enable(true);
            for (unsigned user = 0; user < 4; ++user) {
                check(poll(user, now, NativeGesture::a).started == NativeTrick::none,
                      "enabling with A held does not manufacture a press");
                poll(user, now + 1, 0);
                check(poll(user, now + 2, NativeGesture::a).started == NativeTrick::ollie,
                      "release rearms each controller");
                check(poll(user, now + 1100, NativeGesture::a).right_y == -32768,
                      "enabled gesture reaches synthetic setup");
            }
            input.enable(false);
            for (unsigned user = 0; user < 4; ++user) {
                out = poll(user, now + 1101, NativeGesture::a, 4000, 5000);
                check(out.buttons == NativeGesture::a && out.right_x == 4000 && out.right_y == 5000 && !out.phase,
                      "turning off cancels every synthetic gesture and preserves native axes and buttons");
            }
        }
        input.enable(true);
        poll(0, 110000, 0);
        poll(0, 110001, NativeGesture::a);
        out = input.poll(0, 111100, NativeGesture::a, 0, 0, 0, 0, false, true);
        check(!out.phase && out.right_y == 0 && out.buttons == NativeGesture::a,
              "menu boundary cancels synthetic setup and preserves menu confirm");
        check(poll(0, 111101, NativeGesture::a).started == NativeTrick::none,
              "closing menu with A held requires release");
    }
    check(observed_pose({true, true, true, false, 100, 1, 0x4000}) == SkatePose::manual,
          "measured scoring plus tail contact identifies manual");
    check(observed_pose({true, true, true, false, 100, 1, 0x8000}) == SkatePose::nose_manual,
          "measured scoring plus nose contact identifies nose manual");
    check(observed_pose({true, true, false, false, 100, 1, 0x4000}) == SkatePose::riding,
          "turn contact without scoring is not manual");
    check(observed_pose({true, true, true, false, 201, 2, 0}) == SkatePose::air,
          "ollie scoring activity is not manual");
    check(observed_pose({true, true, true, false, 100, 1, 0xc000}) == SkatePose::riding,
          "stale scoring after two-wheel landing is not manual");
    check(observed_pose({true, false, false, false, 100, 1, 0x4000}) == SkatePose::unknown,
          "missing score observation cannot invent a manual");
    check(observed_pose({false, true, true, false, 100, 1, 0x4000}) == SkatePose::unknown,
          "lost owner invalidates balance observation");
    check(observed_pose({true, true, true, true, 100, 1, 0x4000}) == SkatePose::bail,
          "actual native bail overrides stale balance fields");
    {
        ComboSession session;
        session.step(SkatePose::manual, 0, 0);
        session.step(SkatePose::manual, 0.2f, 0);
        const auto fatigue = session.model().state().manual_fatigue;
        for (const auto state : {100u, 103u}) {
            const NativeComboObservation observed{true, true, true, false, state, 0, 0x4000};
            check(observed_pose(observed) == SkatePose::riding && !action_input_eligible(observed),
                  "observed grounded preparation preserves pose without granting action eligibility");
            session.step(observed_pose(observed), 0.1f, 0);
        }
        session.step(observed_pose({true, true, true, false, 200, 0, 0}), 0.02f, 0);
        check(session.model().state().combo_active && session.model().state().manual_fatigue >= fatigue && fatigue > 0,
              "native manual through mode-zero preparation to takeoff preserves combo fatigue");
        check(observed_pose({true, true, true, true, 103, 0, 0}) == SkatePose::bail &&
              observed_pose({false, true, true, false, 100, 0, 0}) == SkatePose::unknown &&
              observed_pose({true, true, true, false, 100, 3, 0}) == SkatePose::unknown,
              "preparation mapping preserves bail, owner-loss and unknown-mode boundaries");
    }
    for (const auto first : {SkatePose::grind, SkatePose::manual}) {
        ComboSession session;
        const auto other = first == SkatePose::grind ? SkatePose::manual : SkatePose::grind;
        session.step(first, 0, 0);
        const float initial = session.model().difficulty();
        for (int i = 0; i < 120; ++i) session.step(first, 1.0f / 60, 0);
        session.step(SkatePose::air, 0, 0);
        check(session.model().state().balance_kind == BalanceKind::none,
              "native takeoff stops balance without banking");
        session.step(other, 0, 0);
        for (int i = 0; i < 120; ++i) session.step(other, 1.0f / 60, 0);
        session.step(SkatePose::air, 0, 0);
        session.step(first, 0, 0);
        check(session.model().difficulty() > initial,
              "semantic mixed-mode chain preserves accumulated difficulty");
        check(session.model().state().grind_fatigue > 0 && session.model().state().manual_fatigue > 0,
              "semantic mode transitions preserve both mode histories");
        session.step(SkatePose::riding, 0, 0);
        session.step(SkatePose::riding, 0.25f, 0);
        session.step(SkatePose::riding, 0.20f, 0);
        check(!session.model().state().combo_active && session.model().state().fatigue == 0,
              "semantic landing grace banks and resets fatigue");
    }
    {
        ComboSession session;
        session.step(SkatePose::air, 0, 0);
        session.step(SkatePose::riding, 0.1f, 0);
        check(session.model().state().landing_grace, "observed air-to-ground opens landing grace");
        session.step(SkatePose::riding, 0.2f, 0, true);
        check(session.model().state().revert_pending, "observed revert establishes pending link");
        session.step(SkatePose::manual, 0.2f, 0);
        check(session.model().state().balance_kind == BalanceKind::manual &&
              !session.model().state().revert_pending, "observed manual consumes revert link");
        const auto multiplier = session.model().state().multiplier;
        for (int i = 0; i < 30; ++i) session.step(SkatePose::manual, 1.0f / 60, 0);
        check(session.model().state().multiplier == multiplier, "continuous observed manual cannot farm multiplier");
        bool failure = false;
        for (int i = 0; i < 1200 && !failure; ++i) failure = session.step(SkatePose::manual, 1.0f / 60, 1);
        check(failure && session.failed_section(), "balance failure latches native section");
        for (int i = 0; i < 60; ++i) session.step(SkatePose::manual, 1.0f / 60, 0);
        check(!session.model().state().combo_active, "failed native section cannot restart combo every frame");
        session.step(SkatePose::riding, 0.01f, 0);
        session.step(SkatePose::manual, 0.01f, 0);
        check(session.model().state().combo_active, "genuine balance re-entry can start fresh after failure");
        session.step(SkatePose::unknown, 0.01f, 0);
        check(!session.model().state().combo_active, "lost native observation clears active HUD and combo");
    }
    check(balance_correction(BalanceKind::grind, -32768, 32767, true) == -1, "left corrects grind left");
    check(balance_correction(BalanceKind::grind, 32767, -32768, true) == 1, "right corrects grind right");
    check(balance_correction(BalanceKind::manual, -32768, 32767, true) == 1, "up corrects manual up");
    check(balance_correction(BalanceKind::manual, 32767, -32768, true) == -1, "down corrects manual down");
    check(balance_correction(BalanceKind::manual, 32767, 32767, false) == 0, "unavailable input is neutral");
    check(balance_correction(BalanceKind::grind, 7849, 32767, true) == 0, "balance deadzone suppresses idle noise");
    {
        Model m;
        check(!balance_meter(m.state()).visible, "idle meter hidden");
        m.begin_balance(BalanceKind::grind, 0.5f);
        auto view = balance_meter(m.state());
        check(view.visible && !view.vertical && view.cursor_x == 0.5f && view.cursor_y == 0, "grind meter horizontal and model-driven");
        m.begin_balance(BalanceKind::manual, 0.5f);
        view = balance_meter(m.state());
        check(view.visible && view.vertical && view.cursor_x == 0 && view.cursor_y == -0.5f, "manual meter vertical and up-positive");
        m.bail();
        check(!balance_meter(m.state()).visible, "bail hides meter");
    }
    for (const auto direction : {std::uint16_t{4}, std::uint16_t{8}}) {
        NativeGesture gesture;
        const auto expected = direction == 4 ? NativeTrick::kickflip : NativeTrick::heelflip;
        for (unsigned trial = 0; trial < 20; ++trial) {
            const auto t = 3000ull * trial;
            auto out = gesture.poll(t, NativeGesture::x | direction, 0, 0, 0, 0, true, true);
            check(out.started == expected && !(out.buttons & NativeGesture::x), "X direction starts once and consumes X");
            check(!(out.buttons & 15), "fallback directional trick input also suppresses native gameplay menus");
            out = gesture.poll(t + 1000, NativeGesture::x | direction, 0, 0, 0, 0, true, false);
            check(out.phase == 1 && out.right_y == -32768 && out.started == NativeTrick::none, "gesture continues across native crouch state");
            out = gesture.poll(t + 1600, NativeGesture::x | direction, 0, 0, 0, 0, true, false);
            check(out.phase == 2 && out.right_y == 28000 && out.right_x == (direction == 4 ? 28000 : -28000), "flip polarity matches discovered native gesture");
            out = gesture.poll(t + 1850, NativeGesture::x, 0, 0, 0, 0, true, true);
            check(out.phase == 0 && out.right_x == 0 && out.right_y == 0, "flip ends neutral at boundary");
            check(gesture.poll(t + 2200, NativeGesture::x, 0, 0, 0, 0, true, true).started == NativeTrick::none, "long hold never repeats");
            gesture.poll(t + 2500, 0, 0, 0, 0, 0, true, true);
        }
    }
    {
        NativeGesture gesture;
        check(gesture.poll(0, NativeGesture::x, 0, 0, 0, 0, true, false).started == NativeTrick::none, "air/invalid state refuses new flip");
        check(gesture.poll(1, NativeGesture::x, 0, 0, 0, 0, true, true).started == NativeTrick::none, "refused edge is not queued");
        gesture.poll(2, 0, 0, 0, 0, 0, true, true);
        check(gesture.poll(3, NativeGesture::x, 0, 0, 0, 0, true, true).started == NativeTrick::kickflip, "neutral X selects kickflip");
        auto out = gesture.poll(1003, NativeGesture::x, 0, 0, 9000, -10000, true, true);
        check(out.phase == 0 && out.right_x == 9000 && out.right_y == -10000, "physical right stick cancels without modification");
        check(gesture.poll(1603, NativeGesture::x, 0, 0, 0, 0, true, true).phase == 0, "cancelled hold cannot restart");
        gesture.poll(2000, 0, 0, 0, 0, 0, false, false);
        check(gesture.poll(2001, NativeGesture::x, 0, 0, 0, 0, true, true).started == NativeTrick::none, "menu return requires release");
        gesture.poll(2002, 0, 0, 0, 0, 0, true, true);
        check(gesture.poll(2003, NativeGesture::x | 1, 0, 0, 0, 0, true, true).started == NativeTrick::none, "unmeasured impossible is not faked");
    }
    {
        NativeGesture shared;
        OllieGesture old;
        for (const auto now : {0ull, 100ull, 999ull, 1000ull, 1599ull, 1600ull, 1749ull, 1750ull, 3000ull}) {
            const auto buttons = static_cast<std::uint16_t>(now < 100 ? 0x1000 : 0);
            const auto before = old.poll(now, buttons, 0, true, true);
            const auto after = shared.poll(now, buttons, 0, 0, 0, 0, true, true);
            check(before.right_y == after.right_y && before.buttons == after.buttons && before.phase == after.phase,
                  "shared A profile preserves measured ollie boundaries");
        }
    }
    check(flip_direction(0x0004, 0, 0) == FlipTrick::kickflip, "D-pad left selects kickflip");
    check(flip_direction(0x0008, 0, 0) == FlipTrick::heelflip, "D-pad right selects heelflip");
    check(flip_direction(0x0001, 0, 0) == FlipTrick::impossible, "D-pad up selects impossible");
    check(flip_direction(0x0002, 0, 0) == FlipTrick::pop_shove_it, "D-pad down selects pop shove-it");
    check(flip_direction(0x0005, 0, 0) == FlipTrick::hardflip, "D-pad top-left selects hardflip");
    check(flip_direction(0x0009, 0, 0) == FlipTrick::inward_heelflip, "D-pad top-right selects inward heelflip");
    check(flip_direction(0x0006, 0, 0) == FlipTrick::varial_kickflip, "D-pad bottom-left selects varial kickflip");
    check(flip_direction(0x000a, 0, 0) == FlipTrick::varial_heelflip, "D-pad bottom-right selects varial heelflip");
    check(flip_direction(0, -32768, 0) == FlipTrick::kickflip, "left stick left selects kickflip");
    check(flip_direction(0, 32767, 0) == FlipTrick::heelflip, "left stick right selects heelflip");
    check(flip_direction(0, 0, 32767) == FlipTrick::impossible, "left stick up selects impossible");
    check(flip_direction(0, 0, -32768) == FlipTrick::pop_shove_it, "left stick down selects pop shove-it");
    check(flip_direction(0, -25000, 25000) == FlipTrick::hardflip, "left stick top-left selects hardflip");
    check(flip_direction(0, 25000, 25000) == FlipTrick::inward_heelflip, "left stick top-right selects inward heelflip");
    check(flip_direction(0, -25000, -25000) == FlipTrick::varial_kickflip, "left stick bottom-left selects varial kickflip");
    check(flip_direction(0, 25000, -25000) == FlipTrick::varial_heelflip, "left stick bottom-right selects varial heelflip");
    check(flip_direction(0, 12000, 12000) == FlipTrick::none, "stick deadzone remains unselected");
    check(flip_direction(0x000c, 0, 0) == FlipTrick::none, "opposing D-pad directions are rejected");
    check(flip_direction(0x0004, 32767, 0) == FlipTrick::kickflip, "D-pad overrides steering stick");
    OllieGesture ollie;
    auto frame = ollie.poll(0, 0, 0, true, true);
    check(frame.buttons == 0 && frame.phase == 0, "idle leaves native controller alone");
    frame = ollie.poll(10, OllieGesture::a_button, 0, true, false);
    check(frame.buttons == OllieGesture::a_button && !frame.started,
        "A stays native when off board");
    frame = ollie.poll(20, 0, 0, true, true);
    frame = ollie.poll(30, OllieGesture::a_button, 0, true, true);
    check(frame.started && frame.buttons == 0 && frame.phase == 0,
        "on-board A edge starts one gesture and suppresses native A");
    frame = ollie.poll(1029, OllieGesture::a_button, 0, true, false);
    check(!frame.started && frame.right_y == 0 && frame.buttons == 0,
        "holding A neither repeats nor aborts when board state changes");
    frame = ollie.poll(1030, OllieGesture::a_button, 0, true, false);
    check(frame.phase == 1 && frame.right_y == -32768, "native crouch phase begins");
    frame = ollie.poll(1630, 0, 0, true, false);
    check(frame.phase == 2 && frame.right_y == 32767, "native release phase begins");
    frame = ollie.poll(1780, 0, 0, true, false);
    check(frame.phase == 0 && frame.right_y == 0, "gesture always returns stick to neutral");
    frame = ollie.poll(1900, OllieGesture::a_button, 15000, true, true, true);
    check(!frame.started && frame.buttons == OllieGesture::a_button && frame.right_y == 15000,
        "physical right-stick control takes precedence");
    frame = ollie.poll(1910, 0, 0, true, true);
    frame = ollie.poll(1920, OllieGesture::a_button, 0, true, true);
    frame = ollie.poll(2920, 0, 15000, true, false, true);
    check(frame.phase == 0 && frame.right_y == 15000, "player stick cancels synthetic gesture");
    frame = ollie.poll(2930, OllieGesture::a_button, 0, false, false);
    check(frame.buttons == OllieGesture::a_button && frame.phase == 0,
        "menu input capture cancels synthetic gesture");
    frame = ollie.poll(2940, 0, 0, true, true);
    frame = ollie.poll(2950, OllieGesture::a_button, 0, true, true);
    frame = ollie.poll(2949, OllieGesture::a_button, 0, true, false);
    check(frame.phase == 0, "backwards time cancels synthetic gesture");
    NativeRequestCapture capture;
    NativeRequestSample native{true, 100, 0, {1,2,3,4,5,6,7,8}};
    check(!capture.sample(100, native), "native capture requires explicit arm");
    capture.arm(100);
    check(capture.sample(100, native) && capture.count() == 1, "capture includes first sample");
    check(!capture.sample(110, native), "unchanged native request suppressed");
    native.words[7] ^= 1;
    check(capture.sample(120, native), "request change recorded without state change");
    native.valid = false;
    check(capture.sample(130, native) && capture.last() == NativeRequestSample{},
        "ownership loss clears all native fields");
    check(!capture.sample(140, {}), "repeated unavailable capture suppressed");
    native.valid = true;
    check(capture.sample(150, native), "recovered owner creates fresh sample");
    check(!capture.sample(5100, native) && !capture.active(), "five-second capture expires");
    capture.arm(100);
    check(!capture.sample(99, native) && !capture.active(), "backwards clock cancels capture");
    capture.arm(100);
    for (unsigned i = 0; i < NativeRequestCapture::limit; ++i) {
        native.state = i;
        check(capture.sample(100+i, native), "changing samples accepted up to limit");
    }
    ++native.state;
    check(!capture.sample(300, native) && capture.count() == NativeRequestCapture::limit &&
        !capture.active(), "native trace cannot exceed sample limit");
    capture.arm(1000);
    check(capture.sample(1000, native) && capture.count() == 1, "explicit rearm resets capture");
    ObservationTracker observer;
    check(!observer.update({false, 200, 1, 0}), "invalid watch cannot publish stale state");
    check(!observer.update({true, 0, 1, 0}), "uninitialized selector is unavailable");
    check(observer.update({true, 100, 1, 0}), "first native sample recorded");
    check(!observer.update({true, 100, 1, 0}), "unchanged sample does not spam logs");
    check(observer.update({true, 100, 3, 0}), "counter exposes intervening missed transitions");
    check(observer.update({true, 100, 3, 1}), "wipeout counter change retained");
    check(observer.update({false, 100, 3, 1}) && !observer.state().valid,
        "expired ownership discards stale sample");
    check(!observer.update({false, 300, 4, 2}), "unavailable samples stay quiet");
    check(observer.update({true, 200, 4, 2}), "watch recovery starts a new sample");
    Model combo;
    combo.trick(100);
    check(combo.state().combo_active && combo.state().multiplier == 1, "first trick starts combo");
    combo.trick(200);
    check(combo.state().raw_points == 300 && combo.state().multiplier == 2, "tricks accumulate");
    check(combo.bank() == 600 && !combo.state().combo_active && combo.state().fatigue == 0.0f, "bank and reset");
    check(combo.bank() == 0, "empty bank");
    combo.link();
    check(combo.state().fatigue == 0.0f, "idle link does nothing");

    combo.trick(100);
    combo.begin_balance(BalanceKind::grind);
    combo.tick(0.25f);
    const auto fatigue = combo.state().fatigue;
    combo.end_balance();
    check(combo.state().fatigue == fatigue, "balance exit preserves fatigue");
    combo.link();
    check(std::abs(combo.state().fatigue - fatigue - 0.12f) < 0.00001f, "link adds fatigue");
    const auto linked_fatigue = combo.state().fatigue;
    combo.begin_balance(BalanceKind::manual);
    check(combo.state().fatigue >= linked_fatigue, "manual entry preserves accumulated fatigue");
    Model fresh;
    fresh.trick(100);
    fresh.begin_balance(BalanceKind::manual);
    combo.tick(0.25f);
    fresh.tick(0.25f);
    check(combo.state().balance > fresh.state().balance, "tired combo drifts farther");
    combo.bail();
    check(!combo.state().combo_active && combo.state().raw_points == 0 &&
        combo.state().multiplier == 0 && combo.state().balance_kind == BalanceKind::none &&
        combo.state().fatigue == 0.0f, "bail clears combo");

    for (float sign : {-1.0f, 1.0f}) {
        combo.trick(1);
        combo.begin_balance(BalanceKind::grind, sign * 0.99f);
        check(combo.tick(1.0f, 0.0f, sign) && !combo.state().combo_active, "both balance limits bail");
    }
    combo.trick(1);
    combo.begin_balance(BalanceKind::manual, 1.0f);
    check(!combo.state().combo_active, "initial balance at limit fails");
    combo.trick(1);
    combo.begin_balance(BalanceKind::manual, 0.5f);
    combo.tick(0.1f, -1.0f, 0.0f);
    check(combo.state().balance < 0.5f, "signed correction restores balance");
    combo.tick(-1.0f);
    combo.tick(std::numeric_limits<float>::infinity());
    check(std::abs(combo.state().combo_age_s - 0.1f) < 0.00001f, "invalid time ignored");
    combo.tick(0.1f, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity());
    check(std::isfinite(combo.state().balance), "nonfinite controls sanitized");
    combo.end_balance();
    combo.landing();
    combo.tick(100.0f);
    check(!combo.state().combo_active && combo.take_banked_points() == 1,
        "expired landing grace banks score exactly once");
    check(combo.take_banked_points() == 0, "bank notification consumed once");
    Tuning invalid;
    invalid.fatigue_per_second = std::numeric_limits<float>::quiet_NaN();
    invalid.balance_drift_rate = -2.0f;
    Model safe(invalid);
    safe.trick(1);
    safe.begin_balance(BalanceKind::manual);
    safe.tick(1.0f);
    check(safe.state().fatigue == 0.12f && safe.state().balance == 0.0f, "unsafe tuning sanitized; valid link fatigue retained");
    for (int i = 0; i < 1000; ++i) safe.link();
    check(safe.state().fatigue == 1.0f, "fatigue bounded");
    for (bool continuation : {false, true}) {
        Model m;
        m.trick(100);
        m.tick(1);
        check(m.state().combo_active, "airborne combo has no landing timeout");
        check(!m.revert(), "early revert rejected");
        m.landing();
        m.tick(0.3f);
        check(m.revert(), "revert in landing grace accepted");
        const auto points = m.state().raw_points;
        const auto multiplier = m.state().multiplier;
        for (int i = 0; i < 20; ++i) {
            check(!m.revert(), "repeat revert cannot refresh grace");
            m.tick(0.01f);
        }
        check(m.state().raw_points == points && m.state().multiplier == multiplier, "revert does not farm score");
        if (continuation) m.begin_balance(BalanceKind::manual);
        m.tick(0.3f);
        check(m.state().combo_active == continuation, "only actual continuation consumes pending revert");
        if (!continuation) check(m.take_banked_points() == 100 && !m.revert(), "expired revert banks and late revert rejected");
    }
    for (auto kind : {BalanceKind::grind, BalanceKind::manual}) {
        Model m;
        m.trick(10);
        m.begin_balance(kind);
        const auto first = m.difficulty();
        m.tick(1, 0, 0);
        m.tick(1, 0, 0);
        const auto mode_fatigue = kind == BalanceKind::grind ? m.state().grind_fatigue : m.state().manual_fatigue;
        m.trick(10);
        m.begin_balance(kind == BalanceKind::grind ? BalanceKind::manual : BalanceKind::grind);
        m.tick(1, 0, 0);
        m.tick(1, 0, 0);
        m.trick(10);
        m.begin_balance(kind);
        check(m.difficulty() > first, "second balance section harder across other mode");
        check((kind == BalanceKind::grind ? m.state().grind_fatigue : m.state().manual_fatigue) == mode_fatigue,
              "mode fatigue persists through other mode");
        const auto count = m.state().multiplier;
        for (int i = 0; i < 100; ++i) m.begin_balance(kind);
        check(m.state().multiplier == count, "continuous balance entry cannot farm multiplier");
        m.bank();
        check(m.state().fatigue == 0 && m.state().grind_fatigue == 0 && m.state().manual_fatigue == 0,
              "bank resets all fatigue");
    }
    float positions[3]{};
    unsigned index = 0;
    for (int hz : {30, 60, 120}) {
        Model m({}, 42), identical({}, 42);
        m.trick(1); identical.trick(1);
        m.begin_balance(BalanceKind::manual); identical.begin_balance(BalanceKind::manual);
        for (int i = 0; i < hz * 3; ++i) {
            m.tick(1.0f / static_cast<float>(hz), -0.05f);
            identical.tick(1.0f / static_cast<float>(hz), -0.05f);
        }
        check(m.state().combo_active && m.state().balance == identical.state().balance, "same seed reproduces exact run");
        positions[index++] = m.state().balance;
    }
    check(std::abs(positions[0] - positions[1]) < 0.00001f && std::abs(positions[1] - positions[2]) < 0.00001f,
          "30/60/120 Hz materially equivalent");
    {
        Model m;
        for (int i = 0; i < 100000; ++i) m.trick(std::numeric_limits<std::uint32_t>::max());
        check(m.bank() == std::numeric_limits<std::uint64_t>::max(), "score product saturates without overflow");
        m.begin_balance(BalanceKind::grind);
        check(m.state().combo_active, "observed fresh grind starts a combo");
        m.tick(0.2f, 1);
        const auto x = m.state().balance;
        m.tick(0.02f, 0, 0);
        check(m.state().balance > x, "velocity persists after correction releases");
        m.bail();
        check(m.state().fatigue == 0 && m.state().grind_fatigue == 0 && m.state().manual_fatigue == 0 &&
              !m.state().revert_pending && m.take_banked_points() == 0, "bail resets every fatigue and loses score");
    }
    {
        Model fresh_model, tired_model;
        fresh_model.trick(1); tired_model.trick(1);
        for (int i = 0; i < 20; ++i) tired_model.link();
        fresh_model.begin_balance(BalanceKind::grind, 0.5f);
        tired_model.begin_balance(BalanceKind::grind, 0.5f);
        fresh_model.tick(0.25f, -1, 0); tired_model.tick(0.25f, -1, 0);
        check(fresh_model.state().balance < tired_model.state().balance, "fatigue reduces recovery authority");
        Model neutral;
        neutral.begin_balance(BalanceKind::manual);
        bool failed = false;
        for (int i = 0; i < 3600 && !failed; ++i) failed = neutral.tick(1.0f / 60.0f);
        check(failed && !neutral.state().combo_active, "uncorrected balance eventually fails deterministically");
    }
    {
        Model early, late, direct;
        early.trick(10); late.trick(10); direct.trick(10);
        early.landing(); late.landing(); direct.landing();
        early.tick(0.449f); late.tick(0.45f); direct.tick(0.3f);
        check(early.revert() && !late.revert(), "449ms revert links; 450ms boundary has banked");
        direct.begin_balance(BalanceKind::manual);
        direct.tick(0.3f, 0, 0);
        check(direct.state().combo_active && !direct.state().landing_grace, "direct landing-to-manual consumes grace");
        const auto remaining = early.state().link_grace_s;
        early.landing();
        check(early.state().link_grace_s == remaining && early.state().revert_pending, "repeated landing cannot refresh pending revert");
    }
    {
        Model m;
        std::uint32_t sequence = 12345;
        for (unsigned i = 0; i < 10000; ++i) {
            sequence = sequence * 1664525u + 1013904223u;
            switch ((sequence >> 16u) % 8u) {
            case 0: m.trick(sequence); break;
            case 1: m.landing(); break;
            case 2: m.revert(); break;
            case 3: m.begin_balance(BalanceKind::grind); break;
            case 4: m.begin_balance(BalanceKind::manual); break;
            case 5: m.end_balance(); break;
            case 6: m.bail(); break;
            default: m.bank(); break;
            }
            const float dt = i % 17 == 0 ? std::numeric_limits<float>::quiet_NaN() : 1.0f / 30.0f;
            m.tick(dt, sequence & 256u ? 1.0f : -1.0f);
            const auto& s = m.state();
            check(std::isfinite(s.balance) && std::isfinite(s.balance_velocity) && std::abs(s.balance) < 1,
                  "adversarial transitions preserve finite bounded motion");
            check(s.fatigue >= 0 && s.fatigue <= 1 && s.grind_fatigue >= 0 && s.grind_fatigue <= 1 &&
                  s.manual_fatigue >= 0 && s.manual_fatigue <= 1, "adversarial transitions preserve fatigue bounds");
            check(s.combo_active || (s.raw_points == 0 && s.multiplier == 0 && !s.revert_pending &&
                  s.balance_kind == BalanceKind::none), "inactive state never retains a combo or HUD");
        }
    }
    std::cout << "Arcade Mode tests: " << checks << " checks passed, 0 failed\n";
}

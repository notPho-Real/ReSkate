#ifdef ARCADE_INPUT_BASELINE
#include "ReferenceInput/arcade_gesture.h"
#include "ReferenceInput/arcade_action_runtime.h"
namespace tested = dingosdk::arcade_mode::baseline;
#else
#include "Extension/ArcadeMode/arcade_gesture.h"
namespace tested = dingosdk::arcade_mode;
#endif
#include <array>
#include <iostream>
#include <string_view>
#include <vector>
using namespace dingosdk::arcade_mode;
namespace {
struct Frame {
    std::uint64_t now{};
    std::uint16_t buttons{};
    std::int16_t x{}, y{};
    std::uint8_t lt{}, rt{};
    SkatePose pose{SkatePose::riding};
    bool game{true}, safe{true}, start{true}, fresh{true}, flip_ready{true}, offboard{}, enabled{true}, prepare{};
    float speed{};
    bool owner{};
    unsigned user{};
};
std::vector<tested::PreparedGesture> catalog() {
    std::vector<tested::PreparedGesture> result;
    for (const auto id : {1u, 2u, 3u, 4u, 5u, 16u, 17u, 18u, 19u}) {
        const bool nollie = id >= 16;
        const std::int16_t setup = nollie ? 32767 : -32767;
        const std::int16_t x = id == 2 || id == 17 ? 28000 : id == 3 || id == 18 ? -28000 : 0;
        result.emplace_back(static_cast<std::uint8_t>(id), nollie, GestureContext::ground,
            std::vector<AuthoredStickStep>{{0, 0, setup}, {18, x, 28000}, {21, 0, 0}, {24, 0, 0}});
    }
    result.emplace_back(1, false, GestureContext::from_grind,
        std::vector<AuthoredStickStep>{{0, 0, -32767}, {18, 0, 32767}, {21, 0, 0}, {24, 0, 0}});
    return result;
}
void replay(std::string_view name, const std::vector<Frame>& frames, unsigned step = 17) {
    tested::ArcadeInput input;
#ifdef ARCADE_INPUT_BASELINE
    std::array<tested::ActionRuntime, 4> auxiliary;
#endif
    const auto prepared = catalog();
    bool enabled{};
    for (std::size_t i = 0; i < frames.size(); ++i) {
        const auto& frame = frames[i];
        if (enabled != frame.enabled) {
            input.enable(frame.enabled);
#ifdef ARCADE_INPUT_BASELINE
            for (auto& action : auxiliary) action.reset();
#endif
            enabled = frame.enabled;
        }
        const auto tick = frame.now / step;
#ifdef ARCADE_INPUT_BASELINE
        auto& action_input = auxiliary[frame.user];
        const bool busy = action_input.busy() || frame.owner;
        auto out = input.poll(frame.user, frame.now, frame.buttons, frame.x, frame.y, 0, 0,
            frame.game, frame.start, !busy, frame.speed, frame.lt > 30 || frame.rt > 30, true,
            true, tick, frame.lt > 30, prepared, false, frame.safe && !busy, frame.pose,
            false, frame.prepare && !busy, frame.offboard, busy, frame.flip_ready, false);
        const tested::ActionPhysicalPad physical{frame.buttons, frame.x, frame.y, frame.lt, frame.rt};
        const auto action = action_input.poll(tested::ActionExperiment::classic, frame.now, physical,
            enabled, frame.fresh && frame.game && !frame.offboard, frame.pose, true, out.command_busy);
#else
        // This input-only replay supplies no independent manual identity.
        // Contact cannot waive preparation expiry; admitted long holds are
        // exercised by the composed manual lifecycle corpus instead.
        auto out = input.poll(frame.user, frame.now, frame.buttons, frame.x, frame.y, 0, 0,
            frame.game, frame.start, true, frame.speed, frame.lt > 30 || frame.rt > 30, true,
            true, tick, frame.lt > 30, prepared, false, frame.safe, frame.pose,
            false, frame.prepare, frame.offboard, frame.owner, frame.flip_ready, false,
            frame.lt, frame.rt, ActionExperiment::classic, frame.fresh && frame.game && !frame.offboard, true);
        const auto& action = out.action;
#endif
        auto lx = frame.x, ly = frame.y;
        auto lt = frame.lt, rt = frame.rt;
        if (out.suppress_reverse) ly = 0;
        out.buttons &= static_cast<std::uint16_t>(~action.consume_buttons);
        if (action.consume_rt) rt = 0;
        if (action.pad.owns_right_stick) { out.right_x = action.pad.right_x; out.right_y = action.pad.right_y; }
        if (action.pad.owns_left_stick) { lx = action.pad.left_x; ly = action.pad.left_y; }
        if (action.pad.owns_triggers) { lt = action.pad.left_trigger; rt = action.pad.right_trigger; }
        if (action.pad.owns_face_buttons) out.buttons = static_cast<std::uint16_t>((out.buttons & ~0xf000u) | action.pad.face_buttons);
        // Match the runtime's normal nollie-context masking.
        if (enabled && frame.game && !frame.offboard && !action.pad.owns_triggers) lt = 0;
        std::cout << name << ',' << i << ',' << frame.now << ',' << out.buttons << ',' << lx << ',' << ly << ','
            << static_cast<unsigned>(lt) << ',' << static_cast<unsigned>(rt) << ',' << out.right_x << ',' << out.right_y << ','
            << static_cast<unsigned>(out.requested_id) << ',' << out.unsupported << ',' << out.phase << ','
            << out.suppress_reverse << ',' << out.movement_intent << ',' << action.requested << ',' << action.owned << ','
            << static_cast<unsigned>(action.trick) << ',' << action.request_kind << ',' << static_cast<unsigned>(action.result) << ','
            << out.manual.confirmed << '\n';
    }
}
}
int main() {
    for (unsigned step : {17u, 34u}) {
        for (unsigned phase = 0; phase < step; ++phase) for (bool pad : {false, true}) for (int sign : {-1, 1})
            for (unsigned gap : {150u, 200u, 220u, 250u, 350u, 800u}) {
                std::vector<Frame> frames;
                for (unsigned now = phase; now < 1500; now += step) {
                    Frame f; f.now = now; f.speed = 8;
                    int direction = now < 100 ? 0 : now < 100 + gap ? sign : now < 1100 ? -sign : 0;
                    if (pad) f.buttons = static_cast<std::uint16_t>(direction > 0 ? 1 : direction < 0 ? 2 : 0);
                    else f.y = static_cast<std::int16_t>(direction * 25000);
                    if (now > 100 + gap + step && gap <= 250) f.pose = sign > 0 ? SkatePose::manual : SkatePose::nose_manual;
                    frames.push_back(f);
                }
                const auto name = std::string("manual-") + std::to_string(step) + '-' + std::to_string(phase) + '-' +
                    std::to_string(pad) + '-' + std::to_string(sign) + '-' + std::to_string(gap);
                replay(name, frames, step);
            }
        for (unsigned scenario = 0; scenario < 48; ++scenario) {
            std::vector<Frame> frames;
            for (unsigned now = 0; now < 2000; now += step) {
                Frame f; f.now = now;
                if (scenario == 0) { f.y = now >= 100 && now < 1200 ? 25000 : 0; f.speed = now < 500 ? 0 : 8; }
                if (scenario == 1) { f.buttons = now >= 100 && now < 500 ? 0x1000 : 0; if (now >= 500 && now < 1200) f.pose = SkatePose::air; }
                if (scenario >= 2 && scenario <= 7) {
                    f.pose = now >= 400 && now < 1400 ? SkatePose::air : SkatePose::riding;
                    const unsigned face_at = scenario == 2 ? 450 : scenario == 3 ? 750 : scenario == 4 ? 1250 : 500;
                    if (now >= face_at && now < face_at + 70) f.buttons = scenario == 5 ? 0x2000 : scenario == 6 ? 0x8000 : 0x4000;
                    if (now >= face_at - 50 && now < face_at + 100) f.x = -25000;
                    if (scenario == 6) f.x = 0;
                    if (scenario == 7) f.flip_ready = now >= 700;
                }
                if (scenario == 8) { f.rt = now >= 100 && now < 250 ? 255 : 0; }
                if (scenario == 9) { f.buttons = now >= 100 && now < 600 ? 0x8000 : 0; f.pose = now >= 300 && now < 700 ? SkatePose::grind : SkatePose::air; }
                if (scenario == 10) { f.buttons = now >= 100 && now < 500 ? 0x0300 : 0; }
                if (scenario == 11) { f.buttons = now >= 100 && now < 700 ? 0x4000 : 0; f.game = !(now >= 200 && now < 400); f.pose = SkatePose::air; }
                if (scenario == 12) { f.y = now >= 100 && now < 300 ? 25000 : now >= 300 && now < 900 ? -25000 : 0; f.buttons = now < 100 ? 0x2000 : 0; }
                if (scenario == 13) { f.y = now >= 50 && now < 300 ? 25000 : now >= 300 && now < 900 ? -25000 : 0; f.buttons = now < 100 ? 0x2000 : 0; }
                if (scenario == 14) { f.enabled = now < 400 || now >= 800; f.buttons = now >= 100 && now < 900 ? 0x1000 : 0; }
                if (scenario == 15) { f.pose = SkatePose::manual; if (now >= 100 && now < 170) f.buttons = 0x4000; if (now >= 250 && now < 320) f.buttons = 0x8000; }
                if (scenario == 16) { f.buttons = now >= 100 && now < 700 ? 0x1000 : 0; f.offboard = true; f.safe = f.start = false; }
                if (scenario == 17) { f.pose = SkatePose::air; f.buttons = now >= 100 && now < 200 ? 0xa000 : 0; }
                if (scenario == 18) { // Neutral/diagonal thumb arc.
                    f.y = now >= 100 && now < 200 ? 24000 : now >= 250 && now < 700 ? -24000 : 0;
                    f.x = now >= 200 && now < 250 ? 20000 : 0;
                }
                if (scenario == 19) { // Face first, then direction.
                    f.pose = SkatePose::air; f.buttons = now >= 100 && now < 170 ? 0x4000 : 0;
                    f.x = now >= 250 && now < 400 ? -25000 : 0;
                }
                if (scenario == 20) { // Expired unmatched face.
                    f.pose = SkatePose::air; f.buttons = now >= 100 && now < 170 ? 0x4000 : 0;
                    f.x = now >= 800 ? -25000 : 0;
                }
                if (scenario == 21) { // Early grab during Ollie delivery.
                    f.pose = now >= 200 && now < 1000 ? SkatePose::air : SkatePose::riding;
                    f.buttons = now >= 100 && now < 200 ? 0x1000 : now >= 250 && now < 320 ? 0x2000 : 0;
                }
                if (scenario == 22) { // Landing cancels a readiness request.
                    f.pose = now >= 100 && now < 250 ? SkatePose::air : SkatePose::riding;
                    f.buttons = now >= 150 && now < 200 ? 0x4000 : 0; f.x = now >= 100 && now < 300 ? -25000 : 0;
                    f.flip_ready = now >= 400;
                }
                if (scenario == 23) { // Long-held pre-takeoff modifier expires.
                    f.pose = now >= 700 && now < 1400 ? SkatePose::air : SkatePose::riding;
                    f.x = now >= 100 && now < 200 ? -25000 : 0; f.buttons = now >= 750 && now < 820 ? 0x4000 : 0;
                }
                if (scenario == 24) { // Recent released pre-takeoff modifier remains valid.
                    f.pose = now >= 300 && now < 1400 ? SkatePose::air : SkatePose::riding;
                    f.x = now >= 200 && now < 250 ? -25000 : 0; f.buttons = now >= 350 && now < 420 ? 0x4000 : 0;
                }
                if (scenario == 25) { // Repeated fresh air faces, same held direction.
                    f.pose = SkatePose::air; f.x = now >= 100 && now < 1000 ? -25000 : 0;
                    f.buttons = (now >= 150 && now < 200) || (now >= 350 && now < 400) || (now >= 550 && now < 600) ? 0x4000 : 0;
                }
                if (scenario == 26) { // Trigger and shoulder exclusions on manual intent.
                    f.y = now >= 100 && now < 300 ? 24000 : now >= 300 && now < 800 ? -24000 : 0;
                    f.lt = now >= 50 && now < 200 ? 255 : 0;
                }
                if (scenario == 27) { // Revert cannot start without a fresh action observation.
                    f.rt = now >= 100 && now < 300 ? 255 : 0; f.fresh = now >= 500;
                }
                if (scenario == 28) { // Prepared air manual retains its request through landing.
                    f.pose = now >= 50 && now < 500 ? SkatePose::air : now >= 550 && now < 1200 ? SkatePose::manual : SkatePose::riding;
                    f.y = now >= 100 && now < 350 ? 24000 : now >= 350 && now < 1000 ? -24000 : 0;
                }
                if (scenario == 29) { // Prepared manual expires without landing.
                    f.pose = SkatePose::air; f.y = now >= 100 && now < 300 ? 24000 : now >= 300 ? -24000 : 0;
                }
                if (scenario == 30) { // Same held direction cannot rearm across an unsafe gap.
                    f.safe = !(now >= 150 && now < 250);
                    f.y = now >= 100 && now < 300 ? 24000 : now >= 300 && now < 900 ? -24000 : 0;
                }
                if (scenario == 31) { // Opposing pad directions are hard invalidation.
                    f.buttons = now >= 100 && now < 200 ? 1 : now >= 200 && now < 250 ? 3 : now >= 250 && now < 400 ? 2 : 0;
                }
                if (scenario == 32) { // Native push to brake/manual transition.
                    f.y = now >= 100 && now < 800 ? 24000 : now >= 800 && now < 1400 ? -24000 : 0;
                    f.buttons = now >= 300 && now < 600 ? 0x2000 : 0; f.speed = 8;
                }
                if (scenario == 33) { // Grind exit A retains grind context.
                    f.pose = now < 500 ? SkatePose::grind : now < 1200 ? SkatePose::air : SkatePose::riding;
                    f.buttons = now >= 100 && now < 300 ? 0x1000 : now >= 650 && now < 720 ? 0x4000 : 0;
                    f.x = now >= 650 && now < 720 ? -25000 : 0;
                }
                if (scenario == 34) { // Nollie crouch captures the release context.
                    f.lt = now >= 50 && now < 200 ? 255 : 0; f.buttons = now >= 100 && now < 350 ? 0x1000 : 0;
                    f.pose = now >= 400 && now < 1300 ? SkatePose::air : SkatePose::riding;
                }
                if (scenario == 35) { // Held Y survives air-to-rail and releases cleanly.
                    f.pose = now < 400 ? SkatePose::air : now < 1000 ? SkatePose::grind : SkatePose::riding;
                    f.buttons = now >= 100 && now < 800 ? 0x8001 : 0;
                }
                if (scenario == 36) { // Action owner history cannot become fresh after menu.
                    f.pose = SkatePose::air; f.buttons = now >= 100 && now < 800 ? 0x2000 : 0;
                    f.game = !(now >= 200 && now < 400);
                }
                if (scenario == 37) { // Readiness expires exactly at its deadline.
                    f.pose = SkatePose::air; f.x = now >= 100 && now < 700 ? -25000 : 0;
                    f.buttons = now >= 100 && now < 170 ? 0x4000 : 0; f.flip_ready = now >= 500;
                }
                if (scenario == 38) {
                    f.pose = SkatePose::air; f.buttons = now >= 100 && now < 500 ? 0x4000 : 0;
                    f.x = now >= 50 && now < 700 ? -25000 : 0; f.owner = now >= 80 && now < 300;
                }
                if (scenario == 39) {
                    f.y = now >= 100 && now < 400 ? 24000 : now >= 400 ? -24000 : 0;
                    if (now >= 300) f.now = now - 250;
                }
                if (scenario == 40) {
                    f.pose = SkatePose::air; f.buttons = now >= 100 && now < 500 ? 0x4000 : 0;
                    f.x = now >= 50 && now < 700 ? -25000 : 0; f.user = now >= 300 ? 1 : 0;
                }
                if (scenario == 41) {
                    f.y = now >= 100 && now < 400 ? 24000 : now >= 400 ? -24000 : 0;
                    if (now >= 200 && now < 300) f.pose = SkatePose::unknown;
                }
                if (scenario == 42) {
                    f.pose = SkatePose::air; f.x = now >= 50 && now < 100 ? -25000 : 0;
                    f.buttons = (now >= 150 && now < 200) || (now >= 550 && now < 600) ? 0x4000 : 0;
                }
                if (scenario == 43) {
                    f.pose = SkatePose::air; f.buttons = now >= 100 && now < 300 ? 0x3000 : now >= 300 && now < 400 ? 0x2000 : 0;
                }
                if (scenario == 44) f.rt = (now >= 100 && now < 200) || (now >= 500 && now < 600) ? 255 : 0;
                if (scenario == 45) {
                    f.pose = now >= 200 && now < 1200 ? SkatePose::air : SkatePose::riding;
                    f.buttons = now >= 100 && now < 200 ? 0x1000 : now >= 210 && now < 280 ? 0x2000 : 0;
                }
                if (scenario == 46) {
                    f.pose = SkatePose::air; f.buttons = now >= 100 && now < 400 ? 0x2000 : 0;
                    f.lt = now >= 50 && now < 500 ? 255 : 0;
                }
                if (scenario == 47) {
                    f.pose = SkatePose::air; f.buttons = now >= 100 && now < 500 ? 0x2000 : 0;
                    if (now >= 200 && now < 600) f.pose = SkatePose::bail;
                }
                frames.push_back(f);
            }
            replay("action-" + std::to_string(step) + '-' + std::to_string(scenario), frames, step);
        }
    }
}

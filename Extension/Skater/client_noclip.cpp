#include "client_source_spawn.h"
#include "skater_push.h"
#include "client_source_spawn_internal.h"
#include "no_bail.h"
#include "Engine/Game/Multiplayer/session_tools.h"
#include "offboard_flight.h"
#include "Engine/Core/Hooks/hooks.h"
#include "Engine/Core/Log/logging.h"
#include "Engine/Game/Build/20260929/engine.h"
#include "Engine/Game/Build/20260929/no_bail.h"
#include "Engine/Game/Build/20260929/offboard_flight.h"
#include "Engine/Game/Build/20260929/arcade_native_probe.h"
#include "Engine/Game/Build/20260929/skater_state.h"
#include "Engine/Game/Build/20260929/skater_body.h"
#include "Engine/Game/Build/20260929/rigid_bodies.h"
#include "Engine/Game/Abi/orientation_target.h"
#include "Engine/Game/Multiplayer/session_tools.h"
#include "Extension/ArcadeMode/arcade_movement.h"
#include "Extension/ArcadeMode/arcade_mode.h"
#include "Extension/ArcadeMode/arcade_push.h"
#include "Extension/ArcadeMode/arcade_ground_motion_step.h"
#include "Extension/ArcadeMode/arcade_ground_writer_observation.h"
#include "Extension/ArcadeMode/arcade_air_motion.h"
#include "Extension/ArcadeMode/arcade_wallplant_runtime.h"
#include "Extension/ArcadeMode/arcade_wallplant_hold.h"
#include "Extension/ArcadeMode/arcade_ground_rotation.h"
#include "free_flight.h"
#include "Extension/Trainer/trainer_jump.h"
#include "Engine/Game/Multiplayer/session_competition.h"
#include <cmath>
#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace dingosdk::client_source::detail {
namespace {
// Physics bodies already verified writable (a VirtualQuery each), so the
// every-frame and every-simulation-step body reads below skip that system call.
// A body at a new address, and every body once a second, is checked again.
// Only touched with SourceState::busy held, which every caller holds.
struct WritableBodies {
    std::array<std::uintptr_t, 32> bodies{};
    ULONGLONG until{};
};
WritableBodies& writable_bodies() { static WritableBodies value; return value; }
bool body_writable(std::size_t index, std::uintptr_t body) {
    auto& cache = writable_bodies();
    const auto now = GetTickCount64();
    if (now >= cache.until) {
        cache.bodies = {};
        cache.until = now + 1000;
    }
    if (cache.bodies[index] == body) return true;
    // Validate the complete flags/linear/angular region before either writer.
    if (!source_writable(body + 0x60, 0x40)) return false;
    cache.bodies[index] = body;
    return true;
}
bool copy_to(std::uintptr_t address, const void* data, std::size_t size) noexcept {
    __try {
        std::memcpy(reinterpret_cast<void*>(address), data, size);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}
// debug_write for a physics body field inside the region debug_noclip_bodies
// verified writable: a guarded copy and read-back instead of VirtualQuery,
// WriteProcessMemory and a ReadProcessMemory on every simulation step.
template<class T> void body_write(std::uintptr_t address, const T& value) {
    source_require(source_range(address, sizeof(T)) && copy_to(address, &value, sizeof(T)), "Debug setting write failed.");
    T written{};
    source_require(memory::peek(address, written) && written == value, "Debug setting write could not be verified.");
}
// Shared by ordinary motion and the pending-only chooser handoff. All callers
// stage and revalidate their entire original snapshot before this write loop.
template<class Parts,class Vectors,class Flags,class Completed>
void write_arcade_bodies(const Parts& parts,const Vectors& velocities,const Vectors& spins,
        const Flags& flags,bool linear,bool angular,Completed completed) {
    for(std::size_t i=0;i<parts.size();++i) {
        if(linear)body_write(parts[i]+0x70,velocities[i]);
        if(angular)body_write(parts[i]+0x90,spins[i]);
        body_write(parts[i]+0x60,flags[i]|(linear?8u:0u)|(angular?0x20u:0u));
        completed();
    }
}
}
void debug_stop_noclip(InteractiveDebug& debug) noexcept {
    debug.noclip = false;
    debug.noclip_entity = 0;
    debug.noclip_velocity.valid = false;
    clear_no_bail_flight();
}
NoclipBodies debug_noclip_bodies(std::uintptr_t base, std::uintptr_t client, std::uintptr_t entity) {
    overlay::DebugModel skater;
    const auto transform = debug_skater(base, client, skater);
    source_require(skater.skater_identity == entity, "Skater changed; flight stopped.");
    SourceReader reader;
    const auto collection = reader.pointer(entity, 0x70);
    const auto component = reader.pointer(entity, 0x628);
    source_require(reader.pointer(component) == base + addr::engine::skater_component_vtable && reader.pointer(component, 0x18) == collection,
        "Skater physics ownership changed.");
    NoclipBodies result;
    result.root = skater.skater_position;
    result.forward = {transform[8], 0, transform[10]};
    result.up = {transform[4], transform[5], transform[6]};
    result.matrix = transform;
    result.core = reader.pointer(component, 0x70);
    source_require(reader.pointer(result.core) == base + addr::no_bail::bail_core_vtable, "Skater physics is unavailable.");
    const auto board = reader.pointer(reader.pointer(result.core, 0x430), 0x18);
    result.rig_wrapper = reader.pointer(result.core, 0x438);
    const auto rig = reader.pointer(result.rig_wrapper, 0x2f10);
    result.context = reader.pointer(result.core, 0x3c0);
    source_require(reader.pointer(result.rig_wrapper) == result.context && reader.pointer(result.rig_wrapper, 0x4630) == result.core,
        "Skater motion ownership changed.");
    result.seconds = reader.value<float>(result.context, 0x17ec);
    source_require(std::isfinite(result.seconds) && result.seconds >= 0 && result.seconds <= .1f, "Invalid physics timestep.");
    result.offboard = reader.pointer(reader.pointer(result.core, 0x3b0)) == base + addr::offboard_flight::offboard_flight_vtable;
    // Read the committed native state, not the selector's proposed next state
    // (which is remapped by its caller and may be stale between selections).
    const auto physics = reader.value<std::uint32_t>(result.context, 0x1414);
    result.wipeout = physics == addr::no_bail::wipeout_physics_state ||
        physics == addr::no_bail::offboard_physics_state;
    source_require(reader.pointer(board) == base + spawn::board_physics_vtable && reader.pointer(rig) == base + spawn::rig_physics_vtable,
        "Unsupported board or skeleton physics.");
    const auto board_parts = reader.pointer(board, 0x20), rig_parts = reader.pointer(rig, 0x20);
    source_require(reader.value<std::uint32_t>(board_parts, 0) == 9 && reader.value<std::uint32_t>(rig_parts, 0) == 26,
        "Unsupported physics body layout.");
    for (std::size_t i = 0; i < result.parts.size(); ++i) {
        const auto body = i < 9 ? board_parts + i * 0x130 : rig_parts + (i - 8) * 0x130;
        source_require(reader.pointer(body, 0x10) == (i < 9 ? board : rig) && body_writable(i, body),
            "Physics body ownership changed.");
        const auto velocity = reader.value<std::array<float, 3>>(body, 0x70);
        for (const auto v : velocity) source_require(std::isfinite(v) && std::abs(v) <= 100000,
            "Invalid physics velocity.");
        result.parts[i] = body;
    }
    result.board_height = reader.value<float>(board_parts, 0x54);
    source_require(std::isfinite(result.board_height) && std::abs(result.board_height) <= 1000000, "Invalid board altitude.");
    reader.verify();
    return result;
}
namespace {
void noclip_apply_velocity(std::uintptr_t core) noexcept {
    SourceLastError error;
    auto& state = source_state();
    if (!state.initialized.load(std::memory_order_acquire) || !state.velocity_guard_active.load(std::memory_order_acquire) ||
        state.busy.test_and_set(std::memory_order_acquire)) return;
    SourceBusyScope scope{state.busy};
    auto& debug = state.trial.debug;
    const auto apply_boost = [&](InteractiveDebug::VelocityRequest& boost, std::uint64_t& updates, bool up, bool offboard = false) {
        if (!boost.valid || boost.core != core) return;
        try {
            source_require(session_boosts_allowed(), "Boosts are disabled while competition physics are on.");
            if (offboard && boost.applied && GetTickCount64() >= boost.expires) {
                boost.valid = false;
                state.offboard_boost_core.store(0, std::memory_order_release);
                return;
            }
            source_require(GetTickCount64() < boost.expires, "Velocity boost timed out.");
            const auto bodies = debug_noclip_bodies(state.trial.base, boost.client, boost.entity);
            source_require(bodies.core == core, "Skater physics was replaced before the velocity boost could apply.");
            source_require(offboard ? (boost.applied || bodies.offboard || bodies.wipeout) : !bodies.offboard,
                offboard ? "Off-board Up Boost requires walking, falling or gliding." :
                "Velocity boosts require the skater to be on the board.");
            source_require(!debug.noclip && !debug.park_editor, "Velocity boost cancelled while editing or flying.");
            SourceReader reader;
            source_require(reader.value<std::uint8_t>(boost.entity, 0x7e0) == 0,
                "Wait for the current teleport before using velocity boosts.");
            std::array<std::array<float, 3>, 32> velocities{};
            std::array<std::uint32_t, 32> flags{};
            const std::size_t first = offboard ? 9 : 0; // Leave the carried/detached board alone.
            if (offboard && !boost.applied) {
                const auto current_up = reader.value<float>(bodies.parts[9], 0x74);
                boost.velocity[1] = std::max(boost.velocity[1], current_up + boost.velocity[1]);
            }
            for (std::size_t i = first; i < bodies.parts.size(); ++i) {
                velocities[i] = reader.value<std::array<float, 3>>(bodies.parts[i], 0x70);
                flags[i] = reader.value<std::uint32_t>(bodies.parts[i], 0x60);
                for (std::size_t axis = 0; axis < 3; ++axis) {
                    if (offboard && axis == 1) velocities[i][axis] = std::max(velocities[i][axis], boost.velocity[1]);
                    else velocities[i][axis] += boost.velocity[axis];
                    source_require(std::isfinite(velocities[i][axis]) && std::abs(velocities[i][axis]) <= 100000,
                        "Velocity boost produced an invalid physics velocity.");
                }
            }
            reader.verify();
            if (offboard && bodies.offboard)
                source_require(ensure_offboard_up_velocity(state.trial.base, core, bodies.context,
                    bodies.rig_wrapper, boost.velocity[1]), "Off-board boost motion ownership changed.");
            for (std::size_t i = first; i < bodies.parts.size(); ++i) {
                body_write(bodies.parts[i] + 0x70, velocities[i]);
                body_write(bodies.parts[i] + 0x60, flags[i] | 8u);
            }
            if (offboard) {
                if (!boost.applied) {
                    ++updates;
                    boost.applied = true;
                    boost.expires = GetTickCount64() + 180;
                    debug.status = "Off-board up velocity added.";
                }
            } else {
                boost.valid = false;
                ++updates;
                debug.status = up ? "Up velocity added." : "Forward velocity added.";
            }
        } catch (const SourceGuard& issue) {
            boost.valid = false;
            if (offboard) state.offboard_boost_core.store(0, std::memory_order_release);
            debug.status = issue.message;
            if (offboard) logging::write(logging::Level::warning, logging::Channel::skater, issue.message);
        } catch (...) {
            boost.valid = false;
            if (offboard) state.offboard_boost_core.store(0, std::memory_order_release);
            debug.status = "Velocity boost failed during the physics update.";
            if (offboard) logging::write(logging::Level::warning, logging::Channel::skater, debug.status);
        }
    };
    apply_boost(debug.forward_velocity, debug.forward_velocity_updates, false);
    apply_boost(debug.up_velocity, debug.up_velocity_updates, true);
    apply_boost(debug.offboard_up_velocity, debug.offboard_up_velocity_updates, true, true);
    const auto& request = debug.noclip_velocity;
    if (!debug.noclip || !request.valid || request.core != core) return;
    try {
        source_require(session_noclip_allowed(), "Noclip is disabled while competition physics are on.");
        if (GetTickCount64() >= request.expires) { debug_stop_noclip(debug); debug.status = "Flight input timed out."; return; }
        const auto bodies = debug_noclip_bodies(state.trial.base, request.client, request.entity);
        source_require(bodies.core == core, "Skater physics was replaced; flight stopped.");
        SourceReader reader;
        source_require(reader.value<std::uint8_t>(request.entity, 0x7e0) == 0,
            "A teleport started; flight stopped.");
        for (float v : request.velocity) source_require(std::isfinite(v) && std::abs(v) <= 6000, "Invalid flight input.");
        reader.verify();
        if (bodies.seconds == 0) { debug.noclip_altitude_valid = false; return; }
        // Off-board the native drive only chases the motion target through a
        // spring, at a capped walking speed while grounded. Give the skeleton
        // bodies the flight velocity directly so the root (and the camera) keeps
        // up at any speed and stops dead on release. The carried/detached board
        // is left alone; its bodies only receive velocity while riding.
        const float height = bodies.offboard ? bodies.root[1] : bodies.board_height;
        source_require(std::isfinite(height) && std::abs(height) <= 1000000, "Invalid flight altitude.");
        if (!debug.noclip_altitude_valid || debug.noclip_altitude_offboard != bodies.offboard) {
            debug.noclip_altitude = height;
            debug.noclip_altitude_offboard = bodies.offboard;
        }
        debug.noclip_altitude_valid = true;
        debug.noclip_altitude = std::clamp(debug.noclip_altitude, height - 3.0f, height + 3.0f);
        auto velocity = request.velocity;
        // Correct gravity drift without changing world gravity or teleporting.
        velocity[1] += std::clamp((debug.noclip_altitude - height) * 12.0f, -8.0f, 8.0f);
        debug.noclip_altitude += request.velocity[1] * bodies.seconds;
        // Match native velocity writers: XYZ at +70 and dirty bit 8 at +60.
        // Preserve W, angular velocity, transforms, contacts and other flags.
        // Runs after the simulation's movement-state update.
        for (std::size_t i = bodies.offboard ? 9 : 0; i < bodies.parts.size(); ++i) {
            const auto body = bodies.parts[i];
            const auto flags = reader.value<std::uint32_t>(body, 0x60);
            body_write(body + 0x70, velocity);
            body_write(body + 0x60, flags | 8u);
        }
        ++debug.noclip_velocity_updates;
    } catch (const SourceGuard& issue) { debug_stop_noclip(debug); debug.status = issue.message; }
      catch (...) { debug_stop_noclip(debug); debug.status = "Flight stopped after a physics error."; }
}
// Trainer: called after each physics update (see set_physics_step_callback).
std::atomic<void (*)() noexcept> physics_step_callback{};
struct JumpScale {
    std::atomic<bool> pending{};
    std::uintptr_t client{}, entity{}, core{};
    ULONGLONG expires{};
    float factor{1};
    std::atomic<int> outcome{};
    std::atomic<float> up_speed{};
};
JumpScale& jump_scale() { static auto* value = new JumpScale; return *value; }
// The trainer's hippy jump height: scale the upward velocity the game gave the off-board skater.
void trainer_apply_jump_scale(std::uintptr_t core) noexcept {
    auto& j = jump_scale();
    if (!j.pending.load(std::memory_order_acquire) || j.core != core) return;
    SourceLastError error;
    auto& state = source_state();
    if (!state.initialized.load(std::memory_order_acquire) || state.busy.test_and_set(std::memory_order_acquire)) return;
    SourceBusyScope scope{state.busy};
    j.pending.store(false, std::memory_order_release);
    try {
        if (GetTickCount64() >= j.expires) { j.outcome.store(-3); return; }
        const auto bodies = debug_noclip_bodies(state.trial.base, j.client, j.entity);
        source_require(bodies.core == core, "Skater physics was replaced.");
        // On the board the game drives a jump along its own path and undoes a velocity change.
        if (!bodies.offboard) { j.outcome.store(-2); return; }
        float before{};
        const bool scaled = scale_offboard_up_velocity(state.trial.base, core, bodies.context, bodies.rig_wrapper, j.factor, &before);
        j.up_speed.store(before);
        j.outcome.store(scaled ? 2 : before < 0.5f ? -1 : -2);
    } catch (...) { j.outcome.store(-3); }
}
// The trainer's push speed. On the board the game steers the skater to the speed its trick
// scripts ask for (context +0x17f4) and never runs its own code for the push tuning values.
// Measured: a tapped push asks for the skater's own speed kept between 4 and about 9.5 m/s, a
// held one walks up 4.95, 6.98, 8.5 and 9.1 m/s, and the game holds that speed for about six
// seconds after the last push before the skater coasts. A multiplier on that target would
// feed back (the target follows the speed: x2 ran away to 17 m/s), so after each physics step
// of a riding skater:
//  - `factor` over 1: once the game has the skater at its tap speed it is carried on to
//    4 m/s x factor; a held push that has reached the game's last step is carried on to that
//    step x factor, and stays there while the game holds its own;
//  - `factor` under 1: the skater is held down to the game's target x factor (it then never
//    reaches the speed the next step would need, so nothing feeds back).
// Auto push: the game hands its flag to the animation and nothing comes of it (measured: the
// same coast-down with it on). With `cruise` set, a rolling skater that is not braking gains
// speed up to it.
constexpr float push_gain = 0.2f;    // m/s the game's own push gains each physics step (0 to 4 m/s in 19 steps)
constexpr float tap_speed = 4.0f;    // m/s the game's tapped push settles at
constexpr float last_step = 0.95f;   // of the stock top pushing speed: the game's last step is 9.1 of 9.25 m/s
constexpr float cruise_gain = 0.15f; // m/s each physics step: the game steers the speed back toward its own, so less does nothing
constexpr int held_steps = 36;       // a push flagged this long is held (a tap lasts 19 steps)
constexpr int hold_steps = 360;      // how long after a push the game holds its speed
struct PushSpeed {
    std::atomic<float> factor{1}, stock{9.25f}, cruise{};
    std::atomic<std::uintptr_t> client{}, entity{};
    std::atomic<ULONGLONG> expires{};
    std::atomic<ULONGLONG> arcade_expires{};
    std::atomic<bool> arcade_crouched{};
    std::atomic<std::uint64_t> arcade_epoch{};
    // Physics thread only.
    int pushing{}, since_push{hold_steps};
    bool carried{}; // a held push is being carried past the game's last step
    arcade_mode::GroundDrive arcade_drive;
    arcade_mode::GroundMotionStep arcade_drive_steps;
    arcade_mode::NativePushContact arcade_push_contacts;
    NoBailSkater arcade_owner{};
    std::uint64_t drive_epoch{};
    // A plugin's cruise control (set_plugin_cruise): the speed to ride at (0 none), whether it may
    // ease the skater off, and until when it holds.
    std::atomic<float> plugin{};
    std::atomic<bool> plugin_slow{};
    std::atomic<ULONGLONG> plugin_expires{};
    // Physics thread only.
    float reached{}; // the speed pushing has been carried to past the game's own ceiling (0: none)
};
PushSpeed& push_speed() { static auto* value = new PushSpeed; return *value; }
bool read_arcade_air(arcade_mode::AirMotionIntent& out, arcade_mode::NativeManualContact* manual = nullptr,
                     arcade_mode::GroundTurnCommand* ground_turn = nullptr) noexcept;
void trainer_push_speed(std::uintptr_t core) noexcept {
    auto& p = push_speed();
    const auto now = GetTickCount64();
    const auto arcade_until = p.arcade_expires.load(std::memory_order_acquire);
    const bool arcade = arcade_until >= now && arcade_until - now <= 150 && !multiplayer_session_active();
    const float factor = p.factor.load(std::memory_order_relaxed), cruise = p.cruise.load(std::memory_order_relaxed);
    const ULONGLONG ticks = GetTickCount64();
    const float plugin = ticks < p.plugin_expires.load(std::memory_order_acquire) ? p.plugin.load(std::memory_order_relaxed) : 0.0f;
    const bool plugin_slow = plugin > 0 && p.plugin_slow.load(std::memory_order_relaxed);
    if ((!arcade && factor == 1 && cruise <= 0 && plugin <= 0) || ticks >= p.expires.load(std::memory_order_acquire)) return;
    const auto watch = watched_physics_state();
    if (!watch.valid || (watch.state != 100 && !(arcade && watch.state == 103))) {
        p.arcade_drive.reset(); p.arcade_push_contacts.reset();
        return;
    }
    SourceLastError error;
    auto& state = source_state();
    if (!state.initialized.load(std::memory_order_acquire) || state.busy.test_and_set(std::memory_order_acquire)) return;
    SourceBusyScope scope{state.busy};
    try {
        const auto bodies = debug_noclip_bodies(state.trial.base, p.client, p.entity);
        if (bodies.core != core || bodies.offboard || bodies.parts.empty()) {
            p.arcade_drive.reset(); p.arcade_push_contacts.reset(); return;
        }
        SourceReader reader;
        const float target = reader.value<float>(bodies.context, 0x17f4);
        // What the game's brake code tests: a brake held (bit 2) or one squeezed by some amount (bit 1, +0x1880).
        const auto requests = reader.value<std::uint32_t>(bodies.context, 0x13c4);
        const bool braking = (requests & 4u) != 0 || ((requests & 2u) != 0 && std::abs(reader.value<float>(bodies.context, 0x1880)) > 0.05f);
        bool arcade_safe{}, arcade_start{}, arcade_crouched{};
        arcade_mode::NativeInputFrame drive_frame{};
        if (arcade) {
            const auto epoch = p.arcade_epoch.load(std::memory_order_relaxed);
            if (epoch != p.drive_epoch) {
                p.arcade_drive.reset(); p.arcade_push_contacts.reset(); p.drive_epoch = epoch;
            }
            namespace native = game::build::v20260929::arcade_native_probe;
            const auto contacts = reader.value<std::uint32_t>(bodies.context, native::request_words + 4) & 0xc000u;
            const auto physics = reader.value<std::uint32_t>(bodies.context, native::selected_state);
            const auto mode = reader.value<std::uint32_t>(bodies.rig_wrapper, native::crouch_mode);
            NoBailSkater owner{};
            if (!no_bail_skater(owner)) { p.arcade_drive.reset(); p.arcade_push_contacts.reset(); return; }
            if (owner != p.arcade_owner) {
                p.arcade_drive.reset(); p.arcade_push_contacts.reset(); p.arcade_owner = owner;
            }
            arcade_mode::NativeRequestSample sampled{};sampled.valid=true;sampled.state=physics;sampled.crouch_mode=mode;
            sampled.words[0]=requests;sampled.words[1]=contacts;
            drive_frame = arcade_mode::native_air_context(now,{owner.base,owner.entity,owner.core,owner.context,owner.rig},sampled);
            const bool contact_recovery = p.arcade_push_contacts.poll(now, true, physics, mode, contacts,
                (requests & 0x40u) != 0, false);
            const auto ground = arcade_mode::ground_motion_context(physics, mode, contacts,
                (requests & 0x40u) || contact_recovery, braking, p.arcade_crouched.load(std::memory_order_relaxed));
            arcade_start = ground.may_start; arcade_crouched = ground.crouched;
            arcade_safe = ground.eligible && !state.trial.debug.park_editor && !bodies.wipeout &&
                reader.value<std::uint8_t>(p.entity, 0x7e0) == 0 && no_bail_skater(owner) &&
                owner.base == state.trial.base && owner.entity == p.entity && owner.core == core &&
                owner.context == bodies.context && owner.rig == bodies.rig_wrapper;
        }
        std::array<std::array<float, 3>, 32> velocities{};
        std::array<std::uint32_t, 32> flags{};
        const auto count = std::min<std::size_t>(bodies.parts.size(), velocities.size());
        for (std::size_t i = 0; i < count; ++i) {
            velocities[i] = reader.value<std::array<float, 3>>(bodies.parts[i], 0x70);
            flags[i] = reader.value<std::uint32_t>(bodies.parts[i], 0x60);
        }
        reader.verify();
        if (arcade) {
            if (!arcade_safe) { p.arcade_drive.reset(); p.arcade_push_contacts.reset(); return; }
            arcade_mode::AirMotionIntent controls{};
            const bool controls_current = read_arcade_air(controls) && controls.enabled &&
                now >= controls.sampled_at && now-controls.sampled_at <= 150 &&
                controls.angular_proof.same_owner_observation(drive_frame,now) && controls.ground_controls.known;
            const auto drive_step = p.arcade_drive_steps.consume(drive_frame,now,controls_current);
            if (!drive_step) {
                if (drive_step.status == arcade_mode::GroundStepStatus::unavailable) p.arcade_drive.reset();
                return;
            }
            if (drive_step.baseline) p.arcade_drive.reset();
            if (!controls.ground_controls.can_kick(std::hypot(std::hypot(velocities[0][0],velocities[0][2]),velocities[0][1]),arcade_crouched)) {
                p.arcade_drive.reset(); return;
            }
            const float change = p.arcade_drive.step(drive_step.seconds, bodies.root, velocities[0],
                arcade_crouched, arcade_start);
            const float speed = std::hypot(velocities[0][0], velocities[0][2]);
            if (!(change > .0001f)) return;
            const auto direction = speed >= .05f ?
                std::array<float, 3>{velocities[0][0], 0, velocities[0][2]} : bodies.forward;
            const float direction_length = std::hypot(direction[0], direction[2]);
            if (!std::isfinite(direction_length) || direction_length < .05f ||
                (speed < .05f && (direction_length < .5f || direction_length > 1.5f))) {
                p.arcade_drive.reset(); return;
            }
            const float dx = direction[0] / direction_length * change, dz = direction[2] / direction_length * change;
            // Validate the complete batch before any native write; uniform
            // additions preserve articulated relative and vertical momentum.
            if (!arcade_mode::GroundDrive::apply_bodies({velocities.data(),count},dx,dz)) {
                p.arcade_drive.reset(); p.arcade_push_contacts.reset(); return;
            }
            reader.verify();
            NoBailSkater after{};
            source_require(no_bail_skater(after) && after == p.arcade_owner,
                "Ground acceleration ownership changed before writing.");
            for (std::size_t i = 0; i < count; ++i) {
                body_write(bodies.parts[i] + 0x70, velocities[i]);
                body_write(bodies.parts[i] + 0x60, flags[i] | 8u);
            }
            return;
        }
        if (requests & 0x40u) { // the push request the game's own push code tests
            ++p.pushing;
            p.since_push = 0;
        } else {
            p.pushing = 0;
            if (p.since_push < hold_steps) ++p.since_push;
        }
        const float speed = std::sqrt(velocities[0][0] * velocities[0][0] + velocities[0][2] * velocities[0][2]);
        if (!(speed >= 0.5f) || !(speed < 1000.0f)) return;
        const float stock = p.stock.load(std::memory_order_relaxed);
        const bool pushed = target > 0.5f && target < 100.0f && p.since_push < hold_steps; // the game is holding a push speed
        const bool at_last_step = pushed && target >= stock * last_step && speed >= target * 0.9f;
        if (braking || !at_last_step) p.carried = false;
        else if (p.pushing >= held_steps) p.carried = true;
        float change = 0;
        // The push class's speeds (the trainer raises those) are what a push aims for, but the
        // game's speed model settles near 10 to 11 m/s however high they are (measured with them at
        // 360 to 860 m/s: its own target stayed at 9.6 to 10 m/s). Past the stock ceiling a push is
        // carried on here, at the game's own gain, to the stock ceiling x factor, and that speed
        // is kept for as long as the game keeps a pushed speed.
        const float ceiling = stock * factor;
        if (braking || factor <= 1 || p.since_push >= hold_steps || speed < stock * 0.85f) p.reached = 0;
        if (braking) {
        } else if (factor > 1 && p.pushing > 0 && speed >= stock * 0.85f && speed < ceiling) {
            change = std::min(push_gain, ceiling - speed);
            p.reached = std::max(p.reached, speed + change);
        } else if (factor > 1 && p.reached > speed && p.since_push < hold_steps) {
            change = std::min(push_gain, p.reached - speed);
        } else if (pushed && factor < 1 && speed > target * factor && speed <= target * 1.1f) {
            change = -std::min(push_gain, speed - target * factor); // pushed speed only: a hill's is faster than the target
        } else if (plugin_slow && !pushed && speed > plugin + 0.3f) {
            change = -std::min(0.1f, speed - plugin); // riding up on what it keeps pace with: ease off (never while pushing)
        } else if (plugin > 0 && speed >= 1.0f && speed < plugin) {
            change = std::min(cruise_gain, plugin - speed); // a plugin's cruise control, before auto push
        } else if (cruise > 0 && speed >= 1.0f && speed < cruise) {
            change = std::min(cruise_gain, cruise - speed);
        }
        if (std::abs(change) < 0.005f) return;
        const float scale = (speed + change) / speed;
        // Like the native velocity writers: XYZ at +70 and the dirty bit 8 at +60.
        for (std::size_t i = 0; i < count; ++i) {
            velocities[i][0] *= scale;
            velocities[i][2] *= scale;
            body_write(bodies.parts[i] + 0x70, velocities[i]);
            body_write(bodies.parts[i] + 0x60, flags[i] | 8u);
        }
    } catch (...) { p.arcade_drive.reset(); p.arcade_push_contacts.reset(); }
}
// The command channel has one serialized Arcade producer. Native callbacks
// take one coherent snapshot; they never wait for an input-thread publication.
// Payload fields share their enclosing command/readiness seqlock. These are
// copied original physics facts, not a native action clock or a fresh sample.
struct AtomicAirSpinProof {
    std::array<std::atomic<std::uintptr_t>,5> owner{};
    std::atomic<std::uint64_t> generation{}, episode{}, departure{}, step{}, sampled{};
    std::atomic<bool> valid{}, airborne{};
    void store(const arcade_mode::NativeAirSpinProof& p) noexcept {
        const std::array links{p.owner.base,p.owner.entity,p.owner.core,p.owner.context,p.owner.rig};
        for(std::size_t i=0;i<links.size();++i)owner[i].store(links[i],std::memory_order_relaxed);
        generation.store(p.owner_generation,std::memory_order_relaxed);episode.store(p.episode,std::memory_order_relaxed);
        departure.store(p.departure,std::memory_order_relaxed);step.store(p.physics_step,std::memory_order_relaxed);
        sampled.store(p.sampled_at,std::memory_order_relaxed);valid.store(p.valid,std::memory_order_relaxed);airborne.store(p.airborne,std::memory_order_relaxed);
    }
    arcade_mode::NativeAirSpinProof load() const noexcept {
        return {{owner[0].load(std::memory_order_relaxed),owner[1].load(std::memory_order_relaxed),owner[2].load(std::memory_order_relaxed),
            owner[3].load(std::memory_order_relaxed),owner[4].load(std::memory_order_relaxed)},generation.load(std::memory_order_relaxed),
            episode.load(std::memory_order_relaxed),departure.load(std::memory_order_relaxed),step.load(std::memory_order_relaxed),
            sampled.load(std::memory_order_relaxed),valid.load(std::memory_order_relaxed),airborne.load(std::memory_order_relaxed)};
    }
};
struct ArcadeAir {
    std::atomic<std::uint64_t> version{}, sampled{}, release_sequence{}, release_at{}, ready_until{}, epoch{};
    std::atomic<unsigned> charge{}, ground_controls{}, spin_mode{}, spin_facts{};
    std::atomic<std::int32_t> native_launch_id{}, native_followup_id{};
    std::atomic<std::uint64_t> spin_epoch{},spin_serial{},left_pressed_at{},right_pressed_at{};
    std::atomic<std::int64_t> spin_balance{};
    std::atomic<float> yaw_rate{};
    std::atomic<bool> enabled{}, angular_allowed{};
    std::array<std::atomic<std::uintptr_t>,5> manual_owner{};
    std::atomic<std::uint64_t> manual_sampled{};
    std::atomic<std::uint32_t> manual_contacts{};
    std::atomic<bool> manual_admitted{};
    std::array<std::atomic<std::uint64_t>,3> manual_generations{};
    AtomicAirSpinProof spin_proof, ready_proof;
    arcade_mode::AtomicLaunchDescriptor launch;
    std::atomic<std::uint64_t> turn_serial{},turn_at{},turn_step{},turn_sampled{};
    std::array<std::atomic<std::uint64_t>,3> turn_generations{};
    std::atomic<std::uint64_t> ready_version{};
    // SourceState::busy protects the remaining physics/motion-thread state.
    arcade_mode::AirMotion controller;
    arcade_mode::GroundResistance ground_resistance;
    arcade_mode::GroundMotionStep ground_steps;
    arcade_mode::GroundYaw ground_yaw;
    arcade_mode::GroundTurnAngularLease turn_write, air_write;
    arcade_mode::GroundYawOutput ground_output{};
    arcade_mode::AirMotionOutput output{};
    arcade_mode::NativeAirSpinProof output_proof{};
    NoBailSkater owner{};
    std::uintptr_t client{};
    std::uint64_t stepped_at{}, generation{};
};
ArcadeAir& arcade_air() { static auto* value = new ArcadeAir; return *value; }
bool read_arcade_air(arcade_mode::AirMotionIntent& out, arcade_mode::NativeManualContact* manual,
                     arcade_mode::GroundTurnCommand* ground_turn) noexcept {
    auto& a = arcade_air();
    const auto before = a.version.load(std::memory_order_acquire);
    if (before & 1u) return false;
    arcade_mode::AirMotionIntent candidate = {a.enabled.load(std::memory_order_relaxed), a.angular_allowed.load(std::memory_order_relaxed),
        a.sampled.load(std::memory_order_relaxed), a.release_sequence.load(std::memory_order_relaxed),
        a.release_at.load(std::memory_order_relaxed), a.charge.load(std::memory_order_relaxed),
        a.yaw_rate.load(std::memory_order_relaxed), a.epoch.load(std::memory_order_relaxed)};
    candidate.angular_proof = a.spin_proof.load();
    candidate.launch = a.launch.load();
    candidate.native_launch_id = a.native_launch_id.load(std::memory_order_relaxed);
    candidate.native_followup_id = a.native_followup_id.load(std::memory_order_relaxed);
    const arcade_mode::GroundTurnCommand candidate_turn{a.turn_serial.load(std::memory_order_relaxed),a.turn_at.load(std::memory_order_relaxed),
        {a.turn_generations[0].load(std::memory_order_relaxed),a.turn_generations[1].load(std::memory_order_relaxed),a.turn_generations[2].load(std::memory_order_relaxed)},
        a.turn_step.load(std::memory_order_relaxed),a.turn_sampled.load(std::memory_order_relaxed)};
    candidate.ground_controls = arcade_mode::GroundControl::decode(a.ground_controls.load(std::memory_order_relaxed));
    candidate.spin = {a.spin_mode.load(std::memory_order_relaxed)==1 ? arcade_mode::SpinMode::tap : arcade_mode::SpinMode::held,
        a.spin_epoch.load(std::memory_order_relaxed),a.spin_serial.load(std::memory_order_relaxed),a.spin_balance.load(std::memory_order_relaxed),
        arcade_mode::SpinFacts::decode(a.spin_facts.load(std::memory_order_relaxed)),
        a.left_pressed_at.load(std::memory_order_relaxed),a.right_pressed_at.load(std::memory_order_relaxed),
        (a.spin_facts.load(std::memory_order_relaxed)&0x100u)!=0};
    arcade_mode::NativeManualContact candidate_manual{};
    if (manual) {
        for (std::size_t i=0;i<candidate_manual.owner.size();++i) candidate_manual.owner[i]=a.manual_owner[i].load(std::memory_order_relaxed);
        candidate_manual.sampled_at=a.manual_sampled.load(std::memory_order_relaxed);
        candidate_manual.contacts=a.manual_contacts.load(std::memory_order_relaxed);
        candidate_manual.admitted=a.manual_admitted.load(std::memory_order_relaxed);
        candidate_manual.owner_generation=a.manual_generations[0].load(std::memory_order_relaxed);
        candidate_manual.contact_generation=a.manual_generations[1].load(std::memory_order_relaxed);
        candidate_manual.action_generation=a.manual_generations[2].load(std::memory_order_relaxed);
    }
    if (before != a.version.load(std::memory_order_acquire)) return false;
    out = candidate;
    if (manual) *manual = candidate_manual;
    if (ground_turn) *ground_turn = candidate_turn;
    return true;
}
void wallplant_read_current_intent(arcade_mode::AirMotionIntent& intent,std::uint64_t& now,
        std::uint64_t generation) {
    source_require(read_arcade_air(intent,nullptr,nullptr),"wallplant_intent_publication_busy");
    // Input can publish after controller arbitration releases its lock. Compare
    // the completed snapshot against a subsequent clock read, never the clock
    // from before that publication. The original frame/hold deadlines stay fixed.
    now=GetTickCount64();
    source_require(intent.enabled,"wallplant_intent_disabled");
    source_require(!generation || intent.generation==generation,"wallplant_intent_generation_changed");
    source_require(now>=intent.sampled_at,"wallplant_intent_clock_reversed");
    source_require(now-intent.sampled_at<=150,"wallplant_intent_expired");
    source_require(!multiplayer_session_active(),"wallplant_intent_multiplayer");
}
struct WallplantBodyObservation {
    std::array<float,3> root{},board{},forward{},velocity0{},velocity8{};
    std::uint32_t pending0{},pending8{};
};
// Entity placement is a drive reference, not a physical-body measurement.
// Read the same owned body records used by the native transform getter.
WallplantBodyObservation wallplant_body_observation(const NoclipBodies& bodies,SourceReader& reader) {
    namespace body=game::build::v20260929::skater_state;
    WallplantBodyObservation out;
    std::array<float,16> root{},board{};
    for(unsigned row=0;row<4;++row) {
        const auto root_row=reader.value<std::array<float,4>>(bodies.parts[body::board_root_body],
            addr::skater_body::body_transform_offset+row*16);
        const auto board_row=reader.value<std::array<float,4>>(bodies.parts[spawn::reference_body_index],
            addr::skater_body::body_transform_offset+row*16);
        std::copy(root_row.begin(),root_row.end(),root.begin()+row*4);
        std::copy(board_row.begin(),board_row.end(),board.begin()+row*4);
    }
    source_require(valid_flight_transform(root) && valid_flight_transform(board),"hold_physical_transform");
    for(unsigned axis=0;axis<3;++axis) {
        out.root[axis]=root[12+axis];out.board[axis]=board[12+axis];out.forward[axis]=board[8+axis];
    }
    out.velocity0=reader.value<std::array<float,3>>(bodies.parts[body::board_root_body],body::body_velocity_offset);
    out.velocity8=reader.value<std::array<float,3>>(bodies.parts[spawn::reference_body_index],body::body_velocity_offset);
    for(unsigned axis=0;axis<3;++axis)
        source_require(std::isfinite(out.velocity0[axis]) && std::isfinite(out.velocity8[axis]),"hold_physical_velocity");
    out.pending0=reader.value<std::uint32_t>(bodies.parts[body::board_root_body],body::body_pending_flags_offset);
    out.pending8=reader.value<std::uint32_t>(bodies.parts[spawn::reference_body_index],body::body_pending_flags_offset);
    // Include the transform and velocity reads in one checked snapshot.
    reader.verify();
    return out;
}
struct WallplantPhysicalHold {
    arcade_mode::WallplantCommand command{};
    NoBailSkater owner{};
    std::uintptr_t client{},entity{};
    std::uint64_t started{},intent_generation{},targets{},first_target{},last_target{};
    std::array<float,3> entry_root{},last_root{},last_velocity{},readback0{},readback8{},actual_forward{};
    float maximum_drift{};
    WallplantBodyObservation physical{},entry_physical{};
    game::orientation_target::Result orientation{};
    float orientation_limit{};
    std::uintptr_t native_departure{};
    bool departure_attempted{},departure_submitted{};
};
// Payload is protected by SourceState::busy; idle physics/target callbacks only
// read the atomic hint. Runtime cancellation never needs the source lock.
WallplantPhysicalHold& wallplant_physical_hold() {static WallplantPhysicalHold value;return value;}
std::atomic<std::uintptr_t> wallplant_hold_core{};
struct WallplantReleasedDeparture {
    WallplantPhysicalHold release{};
    arcade_mode::NativeInputFrame frame{};
    std::uintptr_t object{};
    std::uint64_t at{};
};
WallplantReleasedDeparture& wallplant_released_departure() {static WallplantReleasedDeparture value;return value;}
std::atomic<std::uintptr_t> wallplant_released_core{};
// A short read-only window across the same release. This never grants a writer
// or trajectory request and cannot prolong the physical hold.
struct WallplantReleaseObservation {
    WallplantPhysicalHold release{};
    std::uint64_t at{};
    unsigned remaining{};
};
WallplantReleaseObservation& wallplant_release_observation() {static WallplantReleaseObservation value;return value;}
std::atomic<std::uintptr_t> wallplant_observed_release_core{};
void wallplant_hold_log(const WallplantPhysicalHold& hold,const char* phase,const char* reason,
        std::uint64_t now,std::uint32_t selected) noexcept {
    try {
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE WALLPLANT HOLD: occurrence={} contact={} phase={} reason={} ms={} started={} elapsed={} state={} targets={} first_target={} last_target={} entity_root={},{},{} anchor={},{},{} maximum_entity_target_error={} velocity={},{},{} exit={},{},{} pending_readback0={},{},{} pending_readback8={},{},{} entry_entity_root={},{},{} incoming={},{},{} reflected={},{},{} hold_forward={},{},{} target_normal={},{},{} entity_forward={},{},{} native_orientation_limit={} requested_orientation_error={} applied_orientation_error={} orientation_fraction={} native_drive=spring hard_anchor_verified=false.",
            hold.command.input_sequence,hold.command.contact,phase,reason,now,hold.started,
            now>=hold.started?now-hold.started:0,selected,hold.targets,hold.first_target,hold.last_target,
            hold.last_root[0],hold.last_root[1],hold.last_root[2],hold.command.anchor[0],
            hold.command.anchor[1],hold.command.anchor[2],hold.maximum_drift,
            hold.last_velocity[0],hold.last_velocity[1],hold.last_velocity[2],
            hold.command.after[0],hold.command.after[1],hold.command.after[2],
            hold.readback0[0],hold.readback0[1],hold.readback0[2],hold.readback8[0],hold.readback8[1],hold.readback8[2],
            hold.entry_root[0],hold.entry_root[1],hold.entry_root[2],
            hold.command.incoming[0],hold.command.incoming[1],hold.command.incoming[2],
            hold.command.reflected[0],hold.command.reflected[1],hold.command.reflected[2],
            hold.command.hold_forward[0],hold.command.hold_forward[1],hold.command.hold_forward[2],
            hold.command.contact_normal[0],hold.command.contact_normal[1],hold.command.contact_normal[2],
            hold.actual_forward[0],hold.actual_forward[1],hold.actual_forward[2],
            hold.orientation_limit,hold.orientation.requested_error,hold.orientation.applied_error,hold.orientation.fraction);
        const auto& p=hold.physical;
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE WALLPLANT BODY: occurrence={} contact={} phase={} ms={} state={} root={},{},{} board={},{},{} forward={},{},{} velocity0={},{},{} velocity8={},{},{} pending0={} pending8={} entry_root={},{},{} entry_board={},{},{} physical_integration_verified=false.",
            hold.command.input_sequence,hold.command.contact,phase,now,selected,
            p.root[0],p.root[1],p.root[2],p.board[0],p.board[1],p.board[2],p.forward[0],p.forward[1],p.forward[2],
            p.velocity0[0],p.velocity0[1],p.velocity0[2],p.velocity8[0],p.velocity8[1],p.velocity8[2],p.pending0,p.pending8,
            hold.entry_physical.root[0],hold.entry_physical.root[1],hold.entry_physical.root[2],
            hold.entry_physical.board[0],hold.entry_physical.board[1],hold.entry_physical.board[2]);
    } catch(...) {}
}
void wallplant_hold_cancel(const char* reason,std::uint64_t now,std::uint32_t selected=0) noexcept {
    auto& hold=wallplant_physical_hold();
    if(!hold.command)return;
    const auto cancelled=hold;hold={};wallplant_hold_core.store(0,std::memory_order_release);
    arcade_mode::wallplant_runtime::finish_hold(cancelled.command,false,now);
    wallplant_hold_log(cancelled,"cancel",reason,now,selected);
}
struct WallplantOrientationSnapshot {
    std::array<float,16> actual{},local_root{};
    std::array<float,3> root_spin{},reference_spin{};
    float limit{};
    std::uintptr_t root_body{},reference_body{};
};
WallplantOrientationSnapshot wallplant_orientation_snapshot(const arcade_mode::NativeInputOwner& owner,
        SourceReader& reader) {
    namespace native=addr::skater_state;
    for(const auto& contract:spawn::orientation_limit_contracts) {
        std::array<unsigned char,32> bytes{};
        source_require(reader.raw(owner.base+contract.rva,bytes.data(),bytes.size()) &&
            bytes==contract.bytes,"hold_orientation_contract");
    }
    const auto board=reader.pointer(reader.pointer(owner.core,native::board_holder_offset),native::board_physics_offset);
    source_require(reader.pointer(board)==owner.base+native::board_physics_vtable,"hold_orientation_board");
    const auto parts=reader.pointer(board,native::physics_bodies_offset);
    source_require(reader.value<std::uint32_t>(parts,0)==native::board_body_count,"hold_orientation_layout");
    const auto at=parts+spawn::reference_body_index*native::physics_body_size;
    source_require(reader.pointer(at,native::body_owner_offset)==board,"hold_orientation_body_owner");
    WallplantOrientationSnapshot result;
    result.root_body=parts;result.reference_body=at;
    source_require(reader.pointer(parts,native::body_owner_offset)==board,"hold_orientation_root_owner");
    result.root_spin=reader.value<std::array<float,3>>(parts,native::body_spin_offset);
    result.reference_spin=reader.value<std::array<float,3>>(at,native::body_spin_offset);
    for(unsigned row=0;row<4;++row) {
        const auto actual=reader.value<std::array<float,4>>(at,addr::skater_body::body_transform_offset+row*16);
        const auto local=reader.value<std::array<float,4>>(owner.rig,spawn::rig_local_root+row*16);
        std::copy(actual.begin(),actual.end(),result.actual.begin()+row*4);
        std::copy(local.begin(),local.end(),result.local_root.begin()+row*4);
    }
    const auto configuration=reader.pointer(owner.base+spawn::physics_configuration)+spawn::orientation_configuration;
    result.limit=3.14159265358979323846f;
    for(auto offset:spawn::orientation_limit_offsets) {
        const auto limit=reader.value<float>(configuration,offset);
        source_require(std::isfinite(limit) && limit>0 && limit<=3.14159265358979323846f,
            "hold_orientation_limit");
        result.limit=std::min(result.limit,limit);
    }
    // Keep these watches in the caller's transaction. A separately verified
    // snapshot can expire while the target is composed, before it is written.
    return result;
}
bool wallplant_hold_target_matrix(const arcade_mode::WallplantCommand& command,
        const std::array<float,16>& supplied,std::array<float,16>& target) noexcept {
    if(!command.anchor_verified || !valid_flight_transform(supplied))return false;
    for(unsigned axis=0;axis<3;++axis) {
        if(!std::isfinite(command.anchor[axis]) || std::abs(command.anchor[axis])>1000000 ||
           !std::isfinite(command.hold_forward[axis]))return false;
    }
    const float length=std::hypot(command.hold_forward[0],command.hold_forward[2]);
    if(!std::isfinite(length) || length<.001f || command.hold_forward[1]!=0)return false;
    const float x=command.hold_forward[0]/length,z=command.hold_forward[2]/length;
    target=supplied;
    // The plant establishes an upright outgoing frame. A local yaw would
    // retain the incoming bank and pitch, aiming the drive away from that frame.
    target[0]=z;target[1]=0;target[2]=-x;
    target[4]=0;target[5]=1;target[6]=0;
    target[8]=x;target[9]=0;target[10]=z;
    for(unsigned axis=0;axis<3;++axis)target[12+axis]=command.anchor[axis];
    return valid_flight_transform(target);
}
// Move the native target pair together, preserving the relative articulated
// pose and world-origin translation. These are drive targets, not body poses.
bool wallplant_ground_target_matrices(const std::array<float,48>& supplied,
        const std::array<float,3>& origin,const std::array<float,16>& target,
        std::array<float,48>& result) noexcept {
    std::array<float,16> local{},world{},inverse{};
    std::copy_n(supplied.begin(),16,local.begin());
    std::copy_n(supplied.begin()+16,16,world.begin());
    std::copy_n(supplied.begin()+32,16,inverse.begin());
    if(!valid_flight_transform(local) || !valid_flight_transform(world) ||
       !valid_flight_transform(inverse) || !valid_flight_transform(target))return false;
    for(float v:origin)if(!std::isfinite(v) || std::abs(v)>1000000)return false;
    // Refuse a target set whose inverse belongs to a different preparation.
    for(unsigned column=0;column<3;++column)for(unsigned axis=0;axis<3;++axis)
        if(std::abs(inverse[column*4+axis]-world[axis*4+column])>.05f)return false;
    for(unsigned axis=0;axis<3;++axis) {
        float v=inverse[12+axis];
        for(unsigned j=0;j<3;++j)v+=world[axis*4+j]*world[12+j];
        if(!std::isfinite(v) || std::abs(v)>.05f)return false;
    }
    std::array<float,16> moved=local,new_inverse=inverse;
    const auto rotate=[&](const std::array<float,3>& vector) {
        std::array<float,3> out{};
        for(unsigned column=0;column<3;++column) {
            float coefficient{};
            for(unsigned axis=0;axis<3;++axis)coefficient+=world[column*4+axis]*vector[axis];
            for(unsigned axis=0;axis<3;++axis)out[axis]+=target[column*4+axis]*coefficient;
        }
        return out;
    };
    for(unsigned column=0;column<3;++column) {
        const auto basis=rotate({local[column*4],local[column*4+1],local[column*4+2]});
        for(unsigned axis=0;axis<3;++axis)moved[column*4+axis]=basis[axis];
    }
    const auto displacement=rotate({local[12]+origin[0]-world[12],
        local[13]+origin[1]-world[13],local[14]+origin[2]-world[14]});
    for(unsigned axis=0;axis<3;++axis) {
        moved[12+axis]=target[12+axis]+displacement[axis]-origin[axis];
        new_inverse[12+axis]=0;
        for(unsigned j=0;j<3;++j) {
            new_inverse[axis*4+j]=target[j*4+axis];
            new_inverse[12+axis]-=target[axis*4+j]*target[12+j];
        }
    }
    if(!valid_flight_transform(moved) || !valid_flight_transform(new_inverse))return false;
    result=supplied;
    std::copy(moved.begin(),moved.end(),result.begin());
    std::copy(target.begin(),target.end(),result.begin()+16);
    std::copy(new_inverse.begin(),new_inverse.end(),result.begin()+32);
    return true;
}
// The requested basis is the published outgoing reference. Convert it back
// through the articulated root and local target to obtain the motion target;
// assigning it directly to the motion target would apply those rotations again.
bool wallplant_ground_orientation(const std::array<float,48>& supplied,
        const std::array<float,3>& origin,const std::array<float,16>& local_root,
        const std::array<float,16>& actual,float limit,std::array<float,16>& target,
        game::orientation_target::Result& report,float target_limit=0) noexcept {
    namespace orientation=game::orientation_target;
    std::array<float,16> local{},world{};
    std::copy_n(supplied.begin(),16,local.begin());
    std::copy_n(supplied.begin()+16,16,world.begin());
    if(!orientation::basis_valid(local_root) || !orientation::basis_valid(local) ||
       !orientation::basis_valid(actual) || !std::isfinite(limit) || !(limit>0) ||
       limit>3.14159265358979323846f)return false;
    const auto original_reference=orientation::compose_basis(local_root,local);
    // An already invalid native pose remains a genuine cancellation. Never
    // move the reference to hide a bail which predates this target request.
    if(orientation::error(actual,original_reference)>limit)return false;
    if(target_limit==0)target_limit=limit;
    if(!std::isfinite(target_limit) || target_limit<=0 || target_limit>limit)return false;
    if(!valid_flight_transform(target))return false;
    const auto reference=target;
    // Converting back through the native float matrices can round the result
    // past the boundary. Tighten only by that measured numerical overshoot;
    // this is not another gameplay angle or turn-rate setting.
    float numerical_limit=target_limit;
    for(unsigned attempt=0;attempt<24;++attempt) {
        std::array<float,16> bounded{};
        orientation::Result result;
        if(!orientation::bound(actual,reference,numerical_limit,bounded,result))return false;
        const auto correction=orientation::compose_basis(orientation::inverse_basis(original_reference),bounded);
        auto candidate=orientation::compose_basis(world,correction);
        for(unsigned axis=0;axis<3;++axis)candidate[12+axis]=target[12+axis];
        std::array<float,48> requested{};
        if(!wallplant_ground_target_matrices(supplied,origin,candidate,requested))return false;
        std::copy_n(requested.begin(),16,local.begin());
        result.applied_error=orientation::error(actual,orientation::compose_basis(local_root,local));
        if(result.applied_error<=target_limit) {target=candidate;report=result;return true;}
        numerical_limit=std::nextafter(numerical_limit-(result.applied_error-target_limit),0.f);
    }
    return false;
}
// Ground motion publishes a reference and then prepares a separate board
// target from the old physical pose. Route the same bounded reference to that
// target, only when both still belong to this exact preparation. The native
// constraint drive, rather than a physical-pose write, performs the movement.
bool wallplant_ground_drive_target(const std::array<float,16>& local_root,
        const std::array<float,16>& local_target,const std::array<float,3>& origin,
        const std::array<float,16>& actual,const std::array<float,16>& reference,
        const std::array<float,16>& supplied,float limit,std::array<float,16>& result) noexcept {
    for(const auto& matrix:{local_root,local_target,actual,reference,supplied})
        if(!valid_flight_transform(matrix))return false;
    for(float value:origin)if(!std::isfinite(value) || std::abs(value)>1000000)return false;
    if(!std::isfinite(limit) || limit<=0 || limit>3.14159265358979323846f ||
       game::orientation_target::error(actual,reference)>limit)return false;
    auto expected=game::orientation_target::compose_basis(local_root,local_target);
    for(unsigned axis=0;axis<3;++axis) {
        expected[12+axis]=local_target[12+axis]+origin[axis];
        for(unsigned row=0;row<3;++row)
            expected[12+axis]+=local_root[12+row]*local_target[row*4+axis];
    }
    auto candidate=supplied;
    for(unsigned row=0;row<4;++row)for(unsigned axis=0;axis<3;++axis) {
        const auto lane=row*4+axis;
        // Float matrix publication tolerance, not a movement allowance.
        if(std::abs(reference[lane]-expected[lane])>.001f ||
           supplied[lane]!=actual[lane])return false;
        candidate[lane]=reference[lane];
    }
    result=candidate;
    return true;
}
struct WallplantHandoff {
    arcade_mode::WallplantExecutionProof execution{};
    arcade_mode::wallplant_runtime::WriterProbe probe{};
    arcade_mode::WallplantSelectionTicket ticket{};
    std::uint64_t intent_generation{};
};
// A different scheduler/thread is rejected; no cross-thread frame is invented.
WallplantHandoff& wallplant_handoff() { static thread_local WallplantHandoff value;return value; }
void wallplant_selection_phase(std::uintptr_t selector,std::uint32_t selected,unsigned stage,const char* reason) noexcept {
    const auto probe=arcade_mode::wallplant_runtime::retained_writer_probe();
    if(!probe.contact)return;
    SourceLastError error;
    std::uintptr_t owned_selector{};
    if(memory::peek(probe.native.owner.core+game::build::v20260929::arcade_native_probe::core_selector,owned_selector) && owned_selector==selector)
        arcade_mode::wallplant_runtime::retained_writer_event(probe,stage,reason,selected,true,probe.native.owner.core);
}
std::uint64_t arcade_wallplant_before_selection(std::uintptr_t selector,std::uint32_t current) noexcept {
    // Pinned chooser contract: core+440 owns selector, selector+8 is context.
    // Its caller 47da1b0 commits the returned state only after this call. Native
    // body +70 / dirty8 remains the same verified synchronization facility;
    // this does not clear contact inputs or change the chooser's return value.
    wallplant_selection_phase(selector,current,0,"before_selection");
    auto& handoff=wallplant_handoff();
    if(handoff.ticket.selector==selector)handoff={};
    auto retained=arcade_mode::wallplant_runtime::writer_probe();
    if(!retained.input) {
        const auto contact=arcade_mode::wallplant_runtime::geometry_probe();
        if(!contact.contact)return 0;
        retained={0,contact.contact,contact.raw,contact.native};
    }
    SourceLastError error;
    auto& state=source_state();
    if(!state.initialized.load(std::memory_order_acquire) ||
       !state.velocity_guard_active.load(std::memory_order_acquire) ||
       !state.motion_original.load(std::memory_order_acquire))return 0;
    // Foreign selectors must neither consume a pending request nor its receipt.
    std::uintptr_t core_selector{};
    if(!memory::peek(retained.native.owner.core+0x440,core_selector) || core_selector!=selector)return 0;
    handoff={};
    std::uint64_t now{};
    const char* frame_reason="frame_not_read";
    arcade_mode::WallplantExecutionProof execution{};
    // remap_xinput holds ollie_mutex through request publication. The chooser
    // must arbitrate before deciding that no pending A exists, or it can pass
    // native201 while that same dispatch is still publishing the request.
    const auto probe=arcade_mode::wallplant_pending_after_arbitration(retained,execution,[&] {
        return arcade_mode::native_wallplant_context(retained.native.owner,now,&frame_reason);
    },[] {return arcade_mode::wallplant_runtime::writer_probe();});
    if(!probe.input) {
        arcade_mode::wallplant_runtime::writer_event(retained,12,frame_reason,current,true,retained.native.owner.core);
        return 0;
    }
    const auto note=[&](unsigned stage,const char* reason) {
        arcade_mode::wallplant_runtime::writer_event(probe,stage,reason,current,true,probe.native.owner.core);
    };
    note(10,"before_selection");
    const auto& frame=execution.native;
    note(12,frame_reason);
    if(!frame.valid){note(15,"Wallplant selection native admission changed.");return 0;}
    if(state.busy.test_and_set(std::memory_order_acquire)){note(11,"selection_source_busy");return 0;}
    SourceBusyScope scope{state.busy};
    try {
        arcade_mode::AirMotionIntent intent{};
        wallplant_read_current_intent(intent,now,0);
        source_require(!state.trial.debug.noclip && !state.trial.debug.park_editor,
            "Wallplant selection yields to editing or flight.");
        const auto& identity=push_speed();
        source_require(now<identity.expires.load(std::memory_order_acquire),"Wallplant selection identity is stale.");
        const auto client=identity.client.load(std::memory_order_relaxed),entity=identity.entity.load(std::memory_order_relaxed);
        const auto bodies=debug_noclip_bodies(state.trial.base,client,entity);
        NoBailSkater owner{};
        source_require(no_bail_skater(owner) && owner.core==probe.native.owner.core && bodies.core==owner.core &&
            !bodies.offboard && !bodies.wipeout && owner.base==state.trial.base && owner.entity==entity &&
            owner.context==bodies.context && owner.rig==bodies.rig_wrapper,
            "Wallplant selection local ownership changed.");
        SourceReader reader;
        source_require(reader.value<std::uint8_t>(entity,0x7e0)==0,"Wallplant selection yields to teleport.");
        namespace native=game::build::v20260929::arcade_native_probe;
        const auto selected=reader.value<std::uint32_t>(bodies.context,native::selected_state);
        const arcade_mode::NativeInputOwner input_owner{owner.base,owner.entity,owner.core,owner.context,owner.rig};
        const auto owned_selector=reader.pointer(owner.core,0x440),selector_context=reader.pointer(selector,8);
        source_require(arcade_mode::wallplant_selection_current(probe.native,input_owner,selector,owned_selector,
            selector_context,current,selected,now),"Wallplant selection frame or selector changed.");
        source_require(arcade_mode::wallplant_selection_current(frame,input_owner,selector,owned_selector,
            selector_context,current,selected,now),"Wallplant selection native admission changed.");
        arcade_mode::NativeRequestSample live{};live.valid=true;live.state=selected;
        live.crouch_mode=reader.value<std::uint32_t>(bodies.rig_wrapper,native::crouch_mode);
        live.words[0]=reader.value<std::uint32_t>(bodies.context,native::request_words);
        live.words[1]=reader.value<std::uint32_t>(bodies.context,native::request_words+4);
        source_require(arcade_mode::wallplant_execution_current(execution,input_owner,live,now),
            "Wallplant selection native sample changed after admission.");
        reader.verify();NoBailSkater after{};
        source_require(no_bail_skater(after) && after==owner,"Wallplant selection owner changed.");
        static thread_local std::uint64_t invocation{};
        source_require(invocation!=UINT64_MAX,"Wallplant chooser invocation exhausted.");
        handoff={execution,probe,{++invocation,probe.input,selector,current,0,false},intent.generation};
        note(13,"selection_handoff_ready");
        return invocation;
    } catch(...) {
        try{throw;}catch(const SourceGuard& issue){note(15,issue.message);}
        catch(...){note(15,"selection_exception");}
    }
    return 0;
}
void arcade_wallplant_after_reconstruction(const WallplantHandoff& handoff,std::uintptr_t core) noexcept {
    if(!handoff.ticket.selected || core!=handoff.execution.native.owner.core)return;
    SourceLastError error;
    std::uint64_t now{};
    const auto note=[&](unsigned stage,const char* reason,std::uint32_t selected=0) {
        arcade_mode::wallplant_runtime::writer_event(handoff.probe,stage,reason,selected,selected!=0,core);
    };
    note(16,"reconstruction_writer_entry");
    if(!arcade_mode::native_wallplant_execution_authorized(handoff.execution.native,now)) {
        note(19,"reconstruction_controller_authority");return;
    }
    auto& state=source_state();
    if(state.busy.test_and_set(std::memory_order_acquire)){note(19,"reconstruction_source_busy");return;}
    SourceBusyScope scope{state.busy};
    arcade_mode::WallplantCommand command{};
    try {
        source_require(state.initialized.load(std::memory_order_acquire) &&
            state.velocity_guard_active.load(std::memory_order_acquire) &&
            state.motion_original.load(std::memory_order_acquire),"Wallplant reconstruction writer unavailable.");
        arcade_mode::AirMotionIntent intent{};
        wallplant_read_current_intent(intent,now,handoff.intent_generation);
        source_require(!state.trial.debug.noclip && !state.trial.debug.park_editor,
            "Wallplant reconstruction yields to editing or flight.");
        const auto& identity=push_speed();
        source_require(now<identity.expires.load(std::memory_order_acquire),"Wallplant reconstruction identity stale.");
        const auto client=identity.client.load(std::memory_order_relaxed),entity=identity.entity.load(std::memory_order_relaxed);
        const auto bodies=debug_noclip_bodies(state.trial.base,client,entity);
        NoBailSkater owner{};
        source_require(no_bail_skater(owner) && owner.core==core && bodies.core==core &&
            !bodies.offboard && !bodies.wipeout && owner.base==state.trial.base && owner.entity==entity &&
            owner.context==bodies.context && owner.rig==bodies.rig_wrapper,
            "Wallplant reconstruction owner changed.");
        SourceReader reader;
        source_require(reader.value<std::uint8_t>(entity,0x7e0)==0,"Wallplant reconstruction yields to teleport.");
        const arcade_mode::NativeInputOwner input_owner{owner.base,owner.entity,owner.core,owner.context,owner.rig};
        namespace native=game::build::v20260929::arcade_native_probe;
        const auto selector=reader.pointer(core,0x440);
        source_require(reader.pointer(selector,8)==owner.context,"Wallplant reconstruction selector changed.");
        arcade_mode::NativeRequestSample live{};live.valid=true;
        live.state=reader.value<std::uint32_t>(bodies.context,native::selected_state);
        live.crouch_mode=reader.value<std::uint32_t>(bodies.rig_wrapper,native::crouch_mode);
        live.words[0]=reader.value<std::uint32_t>(bodies.context,native::request_words);
        live.words[1]=reader.value<std::uint32_t>(bodies.context,native::request_words+4);
        const auto previous=reader.value<std::uint32_t>(bodies.context,native::previous_selected_state);
        source_require(arcade_mode::wallplant_reconstruction_current(handoff.ticket,handoff.execution,
            input_owner,selector,previous,live,now),"Wallplant native reconstruction or chooser result changed.");
        const auto pending=arcade_mode::wallplant_runtime::writer_probe();
        source_require(pending.input==handoff.probe.input && pending.contact==handoff.probe.contact &&
            pending.raw==handoff.probe.raw && arcade_mode::wallplant_same_observation(
                handoff.execution.native,pending.native),"Wallplant reconstruction input or episode changed.");
        std::array<std::array<float,3>,32> velocities{},spins{};
        std::array<std::uint32_t,32> flags{};
        for(std::size_t i=0;i<bodies.parts.size();++i) {
            velocities[i]=reader.value<std::array<float,3>>(bodies.parts[i],0x70);
            spins[i]=reader.value<std::array<float,3>>(bodies.parts[i],0x90);
            flags[i]=reader.value<std::uint32_t>(bodies.parts[i],0x60);
            for(float v:spins[i])source_require(std::isfinite(v)&&std::abs(v)<1000,"Invalid native angular velocity.");
        }
        // Consume once, using actual reconstructed velocities and the unchanged
        // original AIR observation. The current native100 state is only a
        // completion phase of this recorded selector invocation, never admission.
        const arcade_mode::WallplantReconstructionProof reconstruction{
            handoff.ticket,handoff.execution,live,selector,previous,handoff.probe.contact};
        command=arcade_mode::wallplant_runtime::consume(now,handoff.execution.native,velocities[0],handoff.ticket.input,&reconstruction);
        note(17,command?"reconstruction_command_ready":"reconstruction_consume_empty",live.state);
        if(!command)return;
        source_require(arcade_mode::wallplant_hold_anchor_current(command,bodies.root),
            "Wallplant anchor moved beyond validated adjustment.");
        std::array<float,16> hold_target{};
        source_require(wallplant_hold_target_matrix(command,bodies.matrix,hold_target),
            "Wallplant anchor or orientation is unavailable.");
        source_require(!wallplant_physical_hold().command,"Wallplant physical phase already owns motion.");
        arcade_mode::WallplantHoldContinuity admission;
        source_require(admission.begin(command,now),"Wallplant hold origin is invalid.");
        arcade_mode::AirMotionOutput brake{};brake.ready=brake.vertical=brake.horizontal=true;
        brake.delta_x=-velocities[0][0];brake.delta_z=-velocities[0][2];
        source_require(arcade_mode::AirMotion::apply_bodies(brake,velocities,spins,bodies.up),
            "Wallplant entry brake is invalid.");
        reader.verify();NoBailSkater after{};
        source_require(no_bail_skater(after) && after==owner,"Wallplant reconstruction ownership changed before writing.");
        source_require(arcade_mode::wallplant_runtime::current(command,now,handoff.execution.native,command.before),
            "Wall contact changed before hold entry.");
        write_arcade_bodies(bodies.parts,velocities,spins,flags,true,false,[]{});
        auto started=GetTickCount64();
        SourceReader written;
        const auto readback0=written.value<std::array<float,3>>(bodies.parts[0],0x70);
        const auto readback8=written.value<std::array<float,3>>(bodies.parts[8],0x70);
        const auto entry_physical=wallplant_body_observation(bodies,written);
        written.verify();
        auto& air=arcade_air();
        air.controller.yield_ordinary_motion();air.output.vertical=air.output.horizontal=false;
        source_require(arcade_mode::wallplant_runtime::begin_hold(command,started),
            "Wallplant hold authority changed after entry brake.");
        auto& hold=wallplant_physical_hold();
        wallplant_released_departure()={};wallplant_released_core.store(0,std::memory_order_release);
        wallplant_release_observation()={};wallplant_observed_release_core.store(0,std::memory_order_release);
        hold={};hold.command=command;hold.owner=owner;hold.client=client;hold.entity=entity;
        hold.started=started;hold.intent_generation=intent.generation;hold.entry_root=hold.last_root=bodies.root;
        hold.last_velocity=velocities[0];
        hold.actual_forward={bodies.matrix[8],bodies.matrix[9],bodies.matrix[10]};
        hold.readback0=readback0;hold.readback8=readback8;
        hold.physical=hold.entry_physical=entry_physical;
        hold.maximum_drift=std::hypot(bodies.root[0]-command.anchor[0],bodies.root[1]-command.anchor[1],bodies.root[2]-command.anchor[2]);
        wallplant_hold_core.store(core,std::memory_order_release);
        wallplant_hold_log(hold,"entry","braked",started,live.state);
        note(18,"hold_entry_committed",live.state);
    } catch(...) {
        if(command)arcade_mode::wallplant_runtime::completed(command,false);
        try{throw;}catch(const SourceGuard& issue){note(19,issue.message);}
        catch(...){note(19,"reconstruction_exception");}
    }
}
struct WallplantReleaseTrajectory {
    std::uintptr_t object{};
    trainer::NativeTrajectoryParameters parameters{};
};
WallplantReleaseTrajectory wallplant_release_trajectory(std::uintptr_t base,
        const NoBailSkater& owner, const std::array<float,3>& velocity, SourceReader& reader,
        std::uintptr_t native_departure=0) {
    namespace native = addr::trajectory;
    WallplantReleaseTrajectory result;
    result.object=reader.pointer(owner.core,native::core_object);
    source_require(source_object(result.object) &&
        reader.pointer(result.object,native::object_context)==owner.context &&
        reader.pointer(owner.rig)==owner.context,"release_trajectory_owner");
    if(native_departure) {
        source_require(result.object==native_departure,"release_native_departure_owner");
        reader.verify();
        return result;
    }
    // The ordinary solver may leave a completed query awaiting selection at
    // this callback. Let its own selector consume that result once, without
    // clearing pending work or extending the hold when the result is not ready.
    reader.verify();
    source_require(trainer::consume_ready_native_trajectory(base,result.object),
        "release_trajectory_busy");
    reader.verify();
    source_require(trainer::prepare_native_trajectory(base,owner.rig,result.parameters),
        "release_trajectory_code");
    // The native rig supplies both transforms and the current query origin.
    // Only replace its requested launch, never a cached collision result.
    const auto& values=result.parameters.values;
    source_require(std::all_of(values.begin(),values.begin()+native::parameter_launch/sizeof(float),
        [](float value){return std::isfinite(value);}) &&
        std::all_of(velocity.begin(),velocity.end(),[](float value){return std::isfinite(value);}),
        "release_trajectory_parameters");
    std::copy(velocity.begin(),velocity.end(),result.parameters.values.begin()+native::parameter_velocity/sizeof(float));
    reinterpret_cast<unsigned char*>(result.parameters.values.data())[native::parameter_launch]=1;
    reader.verify();
    return result;
}
void arcade_wallplant_hold_step(std::uintptr_t core) noexcept {
    if(wallplant_hold_core.load(std::memory_order_acquire)!=core)return;
    // Serialize current controller permission before SourceState::busy. The
    // original admitted AIR frame is never renewed to authorize a later step.
    const auto held=arcade_mode::wallplant_runtime::held_command();
    std::uint64_t now=GetTickCount64();
    const auto frame=held?arcade_mode::native_wallplant_hold_context(held.native.owner,now):arcade_mode::NativeInputFrame{};
    auto& state=source_state();
    if(state.busy.test_and_set(std::memory_order_acquire))return;
    SourceBusyScope scope{state.busy};
    auto& hold=wallplant_physical_hold();
    if(!hold.command || hold.owner.core!=core)return;
    WallplantPhysicalHold attempted_release{};
    try {
        source_require(state.initialized.load(std::memory_order_acquire) &&
            state.velocity_guard_active.load(std::memory_order_acquire) &&
            state.motion_original.load(std::memory_order_acquire),"hold_writer_unavailable");
        arcade_mode::AirMotionIntent intent{};
        wallplant_read_current_intent(intent,now,hold.intent_generation);
        bool stale_query{};
        const bool authority=frame.valid && arcade_mode::wallplant_runtime::hold_current(hold.command,now,frame,&stale_query);
        if(stale_query)return;
        source_require(held && held.input_sequence==hold.command.input_sequence &&
            held.contact==hold.command.contact && authority,
            "hold_authority");
        source_require(!state.trial.debug.noclip && !state.trial.debug.park_editor,"hold_control_owner");
        const auto bodies=debug_noclip_bodies(state.trial.base,hold.client,hold.entity);
        NoBailSkater owner{};
        source_require(no_bail_skater(owner) && owner==hold.owner && bodies.core==core &&
            bodies.context==owner.context && bodies.rig_wrapper==owner.rig &&
            !bodies.offboard && !bodies.wipeout,"hold_body_owner");
        SourceReader reader;
        source_require(reader.value<std::uint8_t>(hold.entity,0x7e0)==0,"hold_teleport");
        const auto selected=reader.value<std::uint32_t>(bodies.context,
            game::build::v20260929::arcade_native_probe::selected_state);
        source_require(selected==100 || selected==200 || selected==201,"hold_native_state");
        hold.physical=wallplant_body_observation(bodies,reader);
        hold.last_root=bodies.root;
        hold.actual_forward={bodies.matrix[8],bodies.matrix[9],bodies.matrix[10]};
        const auto drift=std::hypot(bodies.root[0]-held.anchor[0],bodies.root[1]-held.anchor[1],bodies.root[2]-held.anchor[2]);
        source_require(std::isfinite(drift),"hold_root_invalid");hold.maximum_drift=std::max(hold.maximum_drift,drift);
        source_require(now>=hold.started,"hold_clock");
        if(now-hold.started<arcade_mode::WallplantHoldContinuity::duration_ms)return;
        source_require(hold.targets>0,"hold_target_never_applied");
        std::array<std::array<float,3>,32> velocities{},spins{};
        std::array<std::uint32_t,32> flags{};
        for(std::size_t i=0;i<bodies.parts.size();++i) {
            velocities[i]=reader.value<std::array<float,3>>(bodies.parts[i],0x70);
            spins[i]=reader.value<std::array<float,3>>(bodies.parts[i],0x90);
            flags[i]=reader.value<std::uint32_t>(bodies.parts[i],0x60);
        }
        hold.last_velocity=velocities[0];
        const auto original_velocities=velocities,original_spins=spins;
        arcade_mode::AirMotionOutput release{};release.ready=release.vertical=release.horizontal=true;
        release.velocity_y=held.after[1];release.delta_x=held.after[0]-velocities[0][0];
        release.delta_z=held.after[2]-velocities[0][2];
        source_require(arcade_mode::AirMotion::apply_bodies(release,velocities,spins,bodies.up),"hold_release_velocity");
        source_require(!hold.departure_attempted || (hold.departure_submitted && hold.native_departure),
            "release_native_departure_failed");
        auto trajectory=wallplant_release_trajectory(state.trial.base,owner,held.after,reader,hold.native_departure);
        reader.verify();NoBailSkater after{};
        source_require(no_bail_skater(after) && after==owner,"hold_release_owner");
        const bool current=arcade_mode::wallplant_runtime::hold_current(hold.command,now,frame,&stale_query);
        if(stale_query)return;
        source_require(current,"hold_release_revalidation");
        // Take the physical ticket before touching bodies: recursive callbacks
        // and future ticks cannot turn a single release into repeated writes.
        auto releasing=hold;attempted_release=releasing;
        hold={};wallplant_hold_core.store(0,std::memory_order_release);
        // The same single-release ticket owns the planner request and body
        // write. A busy/failed planner cannot be retried as a second impulse.
        if(!releasing.departure_attempted)
            source_require(trainer::submit_native_trajectory(state.trial.base,trajectory.object,trajectory.parameters,true),
                "release_trajectory_submit");
        source_require(no_bail_skater(after) && after==owner,"release_trajectory_owner_changed");
        SourceReader unchanged;
        for(std::size_t i=0;i<bodies.parts.size();++i) {
            source_require(unchanged.value<std::array<float,3>>(bodies.parts[i],0x70)==original_velocities[i] &&
                unchanged.value<std::array<float,3>>(bodies.parts[i],0x90)==original_spins[i] &&
                unchanged.value<std::uint32_t>(bodies.parts[i],0x60)==flags[i],"release_trajectory_body_changed");
        }
        unchanged.verify();
        write_arcade_bodies(bodies.parts,velocities,spins,flags,true,false,[]{});
        SourceReader written;
        releasing.readback0=written.value<std::array<float,3>>(bodies.parts[0],0x70);
        releasing.readback8=written.value<std::array<float,3>>(bodies.parts[8],0x70);
        releasing.physical=wallplant_body_observation(bodies,written);
        written.verify();
        SourceReader planned;
        const auto curve_position=planned.value<std::array<float,3>>(trajectory.object,addr::trajectory::curve_position);
        const auto curve_velocity=planned.value<std::array<float,3>>(trajectory.object,addr::trajectory::curve_velocity);
        const auto request_velocity=planned.value<std::array<float,3>>(trajectory.object,addr::trajectory::requested_velocity);
        const auto pending=planned.value<std::uint8_t>(trajectory.object,addr::trajectory::query_pending);
        const auto selected_curve=planned.value<std::uint8_t>(trajectory.object,addr::trajectory::curve_selected);
        planned.verify();
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE WALLPLANT PLAN ms={} input={} contact={} state={} submitted=true pending={} selected={} request_velocity=[{},{},{}] curve_position=[{},{},{}] curve_velocity=[{},{},{}] physical_integration_verified=false",
            now,releasing.command.input_sequence,releasing.command.contact,selected,unsigned(pending),unsigned(selected_curve),
            request_velocity[0],request_velocity[1],request_velocity[2],curve_position[0],curve_position[1],curve_position[2],
            curve_velocity[0],curve_velocity[1],curve_velocity[2]);
        arcade_mode::wallplant_runtime::finish_hold(releasing.command,true,now);
        wallplant_release_observation()={releasing,now,4};
        wallplant_observed_release_core.store(core,std::memory_order_release);
        // The impulse is already spent. Retain only enough provenance for the
        // first native100->200 query to use consumed motion, not a new kick.
        if(selected==100 && frame.state==100 && !frame.airborne) {
            wallplant_released_departure()={releasing,frame,trajectory.object,now};
            wallplant_released_core.store(core,std::memory_order_release);
        }
        wallplant_hold_log(releasing,"release","committed",now,selected);
    } catch(...) {
        // If a write failed after taking the local ticket, still withdraw the
        // runtime receipt. No repair write or delayed retry is manufactured.
        if(!hold.command) {
            arcade_mode::wallplant_runtime::finish_hold(held,false,now);
            if(attempted_release.command)wallplant_hold_log(attempted_release,"cancel","release_write_failed",now,0);
        }
        try{throw;}catch(const SourceGuard& issue){wallplant_hold_cancel(issue.message,now);}
        catch(...){wallplant_hold_cancel("hold_exception",now);}
    }
}
bool wallplant_released_native_departure(std::uintptr_t object,
        trainer::NativeWorldTrajectoryOverride& request,bool completed) noexcept {
    if(completed || !wallplant_released_core.load(std::memory_order_acquire))return false;
    const auto receipt=arcade_mode::wallplant_runtime::latest_completed();
    std::uint64_t now=GetTickCount64();
    const auto frame=receipt?arcade_mode::native_wallplant_hold_context(receipt.native.owner,now):arcade_mode::NativeInputFrame{};
    auto& state=source_state();
    if(state.busy.test_and_set(std::memory_order_acquire))return false;
    SourceBusyScope scope{state.busy};
    auto& ticket=wallplant_released_departure();
    if(!ticket.release.command || object!=ticket.object)return false;
    // Consume before native execution, including refusals. No later query,
    // retry or callback can turn this continuity proof into another impulse.
    const auto departed=ticket;ticket={};wallplant_released_core.store(0,std::memory_order_release);
    const auto& release=departed.release;
    try {
        SourceReader reader;
        const auto& call=addr::trajectory::pop_query_call;
        std::array<unsigned char,32> call_bytes{};
        source_require(state.initialized.load(std::memory_order_acquire) &&
            state.velocity_guard_active.load(std::memory_order_acquire) &&
            request.caller==call.rva && reader.raw(state.trial.base+call.rva,call_bytes.data(),call_bytes.size()) &&
            call_bytes==call.bytes,
            "released_departure_route");
        source_require(receipt && receipt.input_sequence==release.command.input_sequence &&
            receipt.contact==release.command.contact,"released_departure_receipt");
        arcade_mode::AirMotionIntent intent{};
        wallplant_read_current_intent(intent,now,release.intent_generation);
        source_require(!state.trial.debug.noclip && !state.trial.debug.park_editor,"released_departure_control");
        NoBailSkater owner{};
        source_require(no_bail_skater(owner) && owner==release.owner,"released_departure_owner");
        const auto bodies=debug_noclip_bodies(state.trial.base,release.client,release.entity);
        source_require(bodies.core==owner.core && bodies.context==owner.context && bodies.rig_wrapper==owner.rig &&
            !bodies.offboard && !bodies.wipeout,"released_departure_bodies");
        const auto selected=reader.value<std::uint32_t>(owner.context,game::build::v20260929::arcade_native_probe::selected_state);
        const auto previous=reader.value<std::uint32_t>(owner.context,game::build::v20260929::arcade_native_probe::previous_selected_state);
        source_require(reader.pointer(owner.core,addr::trajectory::core_object)==object &&
            reader.pointer(object,addr::trajectory::object_context)==owner.context && reader.pointer(owner.rig)==owner.context &&
            reader.value<std::uint8_t>(release.entity,addr::rigid_bodies::entity_teleport)==0 &&
            reader.value<std::uint8_t>(object,addr::trajectory::query_pending)==0,"released_departure_native_owner");
        const auto observation=wallplant_body_observation(bodies,reader);
        source_require(!observation.pending0 && !observation.pending8 &&
            arcade_mode::wallplant_release_velocity_current(release.command,observation.velocity0) &&
            arcade_mode::wallplant_release_velocity_current(release.command,observation.velocity8),
            "released_departure_unconsumed_motion");
        reader.verify();
        NoBailSkater current{};
        source_require(no_bail_skater(current) && current==owner,"released_departure_owner_changed");
        now=GetTickCount64();
        source_require(arcade_mode::wallplant_release_departure_current(release.command,departed.frame,
            frame,departed.at,now,selected,previous),"released_departure_continuity");
        request.velocity=observation.velocity0;request.receipt=release.command.input_sequence;
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE WALLPLANT DEPARTURE: occurrence={} contact={} ms={} released={} caller={} state={} previous={} consumed_velocity=[{},{},{}] body_write=false candidate_acceptance_verified=false",
            request.receipt,release.command.contact,now,departed.at,request.caller,selected,previous,
            request.velocity[0],request.velocity[1],request.velocity[2]);
        return true;
    } catch(...) {
        const auto report=[&](const char* reason) {
            logging::log(logging::Level::info,logging::Channel::skater,
                "ARCADE WALLPLANT DEPARTURE REFUSED: occurrence={} contact={} ms={} released={} caller={} reason={} body_write=false",
                release.command.input_sequence,release.command.contact,now,departed.at,request.caller,reason);
        };
        try{throw;}catch(const SourceGuard& issue){report(issue.message);}
        catch(...){report("released_departure_exception");}
        return false;
    }
}
bool wallplant_native_departure(std::uintptr_t object,
        trainer::NativeWorldTrajectoryOverride& request,bool completed) noexcept {
    if(!wallplant_hold_core.load(std::memory_order_acquire))
        return wallplant_released_native_departure(object,request,completed);
    const auto held=arcade_mode::wallplant_runtime::held_command();
    std::uint64_t now=GetTickCount64();
    const auto frame=held?arcade_mode::native_wallplant_hold_context(held.native.owner,now):arcade_mode::NativeInputFrame{};
    auto& state=source_state();
    if(state.busy.test_and_set(std::memory_order_acquire))return false;
    SourceBusyScope scope{state.busy};
    auto& hold=wallplant_physical_hold();
    if(!hold.command || !held || hold.command.input_sequence!=held.input_sequence ||
       hold.command.contact!=held.contact)return false;
    if(completed) {
        if(!hold.departure_attempted || hold.native_departure!=object ||
           request.receipt!=hold.command.input_sequence)return false;
        hold.departure_submitted=request.submitted;
        return true;
    }
    // A natural departure may submit its query before the +58 body callback.
    // Supply the saved exit at that original submission, once the hold is due,
    // instead of competing with its pending query after the callback returns.
    if(hold.departure_attempted || now<hold.started ||
       now-hold.started<arcade_mode::WallplantHoldContinuity::duration_ms)return false;
    try {
        source_require(state.initialized.load(std::memory_order_acquire) &&
            state.velocity_guard_active.load(std::memory_order_acquire) && hold.targets>0,
            "departure_writer_unavailable");
        arcade_mode::AirMotionIntent intent{};
        wallplant_read_current_intent(intent,now,hold.intent_generation);
        bool stale{};
        source_require(frame.valid && arcade_mode::wallplant_runtime::hold_current(hold.command,now,frame,&stale),
            "departure_authority");
        source_require(!state.trial.debug.noclip && !state.trial.debug.park_editor,
            "departure_control_owner");
        NoBailSkater owner{};
        source_require(no_bail_skater(owner) && owner==hold.owner,"departure_body_owner");
        SourceReader reader;
        const auto selected=reader.value<std::uint32_t>(owner.context,
            game::build::v20260929::arcade_native_probe::selected_state);
        source_require((selected==100 || selected==200 || selected==201) &&
            reader.value<std::uint8_t>(hold.entity,0x7e0)==0 &&
            reader.pointer(owner.core,addr::trajectory::core_object)==object &&
            reader.pointer(object,addr::trajectory::object_context)==owner.context &&
            reader.pointer(owner.rig)==owner.context &&
            reader.value<std::uint8_t>(object,addr::trajectory::query_pending)==0,"departure_native_owner");
        reader.verify();
        request.velocity=hold.command.after;request.receipt=hold.command.input_sequence;
        hold.native_departure=object;hold.departure_attempted=true;hold.departure_submitted=false;
        return true;
    } catch(...) { return false; }
}
bool arcade_wallplant_hold_target(std::uintptr_t rig,std::uintptr_t context,
        const std::array<float,16>* supplied,std::array<float,16>& target,
        const arcade_mode::NativeInputFrame& frame,std::uint64_t now) {
    auto& hold=wallplant_physical_hold();
    if(!hold.command)return false;
    if(rig!=hold.owner.rig || context!=hold.owner.context)return false;
    try {
        bool stale_query{};
        const bool authority=frame.valid && arcade_mode::wallplant_runtime::hold_current(hold.command,now,frame,&stale_query);
        if(stale_query)return false;
        source_require(authority,"target_authority");
        if(now<hold.started)return false;
        auto& state=source_state();
        source_require(!state.trial.debug.noclip && !state.trial.debug.park_editor && !multiplayer_session_active(),"target_control_owner");
        const auto bodies=debug_noclip_bodies(state.trial.base,hold.client,hold.entity);
        NoBailSkater owner{};
        source_require(no_bail_skater(owner) && owner==hold.owner && bodies.core==owner.core &&
            !bodies.offboard && !bodies.wipeout,"target_body_owner");
        SourceReader reader;std::array<float,16> original{};
        source_require(reader.value<std::uint8_t>(hold.entity,0x7e0)==0 &&
            reader.raw(reinterpret_cast<std::uintptr_t>(supplied),original.data(),sizeof(original)) &&
            wallplant_hold_target_matrix(hold.command,original,target),"target_transform");
        const auto physical=wallplant_body_observation(bodies,reader);
        reader.verify();NoBailSkater after{};
        source_require(no_bail_skater(after) && after==owner,"target_owner");
        const bool current=arcade_mode::wallplant_runtime::hold_current(hold.command,now,frame,&stale_query);
        if(stale_query)return false;
        source_require(current,"target_revalidation");
        // hold_current enforces the original deadline plus one native step.
        // The due release still owns this callback's target until its single
        // body write consumes the receipt after original motion returns.
        if(now<hold.started)return false;
        hold.last_root=bodies.root;
        hold.physical=physical;
        hold.maximum_drift=std::max(hold.maximum_drift,std::hypot(bodies.root[0]-hold.command.anchor[0],
            bodies.root[1]-hold.command.anchor[1],bodies.root[2]-hold.command.anchor[2]));
        if(!hold.targets)hold.first_target=now;
        if(hold.targets!=UINT64_MAX)++hold.targets;
        hold.last_target=now;
        return true;
    } catch(const SourceGuard& issue) {wallplant_hold_cancel(issue.message,now);}
      catch(...) {wallplant_hold_cancel("target_exception",now);}
    return false;
}
void arcade_air_apply(std::uintptr_t core,bool wallplant_handoff_active=false) noexcept {
    const auto wall_probe=arcade_mode::wallplant_runtime::writer_probe();
    std::uint32_t wall_probe_state{};
    bool wall_probe_state_known{};
    const auto wall_writer=[&](unsigned stage,const char* reason) {
        arcade_mode::wallplant_runtime::writer_event(wall_probe,stage,reason,wall_probe_state,wall_probe_state_known,core);
    };
    wall_writer(2,"writer_entry");
    auto& a = arcade_air();
    arcade_mode::AirMotionIntent intent{};
    arcade_mode::NativeManualContact manual{};
    arcade_mode::GroundTurnCommand ground_turn{};
    const auto now = GetTickCount64();
    auto& state = source_state();
    if (!state.initialized.load(std::memory_order_acquire)) {wall_writer(3,"source_uninitialized");return;}
    if (!state.velocity_guard_active.load(std::memory_order_acquire)) {wall_writer(3,"velocity_guard");return;}
    if (!state.motion_original.load(std::memory_order_acquire)) {wall_writer(3,"motion_original");return;}
    if (state.busy.test_and_set(std::memory_order_acquire)) {wall_writer(3,"source_busy");return;}
    SourceBusyScope scope{state.busy};
    if (!read_arcade_air(intent,&manual,&ground_turn)) {
        wall_writer(3,"intent_snapshot");
        // A failed snapshot has no command identity. Retire accepted work only;
        // never spend a serial from an interleaved input publication.
        a.ground_yaw.cancel(); a.ground_output={}; a.output={};
        a.ready_until.store(0, std::memory_order_release); return;
    }
    if (now < intent.sampled_at || now - intent.sampled_at > 150 || multiplayer_session_active()) {
        wall_writer(3,"intent_age_or_multiplayer");
        a.ground_yaw.cancel(&ground_turn); a.ground_output={}; a.output={}; a.turn_write.clear(); a.air_write.clear();
        a.ready_until.store(0, std::memory_order_release); return;
    }
    try {
        source_require(!state.trial.debug.noclip && !state.trial.debug.park_editor, "Air motion yields to editing or flight.");
        const auto& identity = push_speed();
        source_require(now < identity.expires.load(std::memory_order_acquire), "Air motion identity is stale.");
        const auto client = identity.client.load(std::memory_order_relaxed), entity = identity.entity.load(std::memory_order_relaxed);
        const auto bodies = debug_noclip_bodies(state.trial.base, client, entity);
        NoBailSkater owner{};
        source_require(no_bail_skater(owner) && owner.core == core && bodies.core == core && !bodies.offboard && !bodies.wipeout &&
            owner.base == state.trial.base && owner.entity == entity && owner.context == bodies.context && owner.rig == bodies.rig_wrapper,
            "Air motion local ownership changed.");
        SourceReader reader;
        source_require(reader.value<std::uint8_t>(entity, 0x7e0) == 0, "Air motion yields to teleport.");
        namespace native = game::build::v20260929::arcade_native_probe;
        const auto physics = reader.value<std::uint32_t>(bodies.context, native::selected_state);
        wall_probe_state=physics;wall_probe_state_known=true;
        std::array<std::array<float, 3>, 32> velocities{}, spins{};
        std::array<std::uint32_t, 32> flags{};
        for (std::size_t i = 0; i < bodies.parts.size(); ++i) {
            velocities[i] = reader.value<std::array<float, 3>>(bodies.parts[i], 0x70);
            spins[i] = reader.value<std::array<float, 3>>(bodies.parts[i], 0x90);
            flags[i] = reader.value<std::uint32_t>(bodies.parts[i], 0x60);
            for (float v : spins[i]) source_require(std::isfinite(v) && std::abs(v) < 1000, "Invalid native angular velocity.");
        }
        const bool ordinary = arcade_mode::AirMotion::ordinary_basis(bodies.up);
        if (a.owner != owner || a.generation != intent.generation ||
            (a.stepped_at && (now < a.stepped_at || now - a.stepped_at > 150))) { a.controller.reset(); a.ground_resistance.reset(); a.ground_yaw.reset(); a.ground_output={}; }
        const auto contacts = reader.value<std::uint32_t>(bodies.context, native::request_words + 4) & 0xc000u;
        const auto mode = reader.value<std::uint32_t>(bodies.rig_wrapper, native::crouch_mode);
        const auto requests = reader.value<std::uint32_t>(bodies.context, native::request_words);
        arcade_mode::NativeRequestSample sampled{};sampled.valid=true;sampled.state=physics;sampled.crouch_mode=mode;
        sampled.words[0]=requests;sampled.words[1]=contacts;
        arcade_mode::MotionCleanupProof cleanup{};
        const char* wall_frame_reason="frame_not_read";
        const auto native_frame = arcade_mode::native_air_context(now,
            {owner.base,owner.entity,owner.core,owner.context,owner.rig},sampled,&cleanup,
            wall_probe.input?&wall_frame_reason:nullptr);
        wall_writer(4,wall_frame_reason);
        // Angular writer observation begin. Reuse only the reads above.
        arcade_mode::AngularWriterObservation angular_observation{
            arcade_mode::begin_angular_writer_observation(now,{owner.base,owner.entity,owner.core,owner.context,owner.rig},native_frame,bodies.parts),
            arcade_mode::finish_angular_writer_observation};
        auto& angular_row=angular_observation.row;
        angular_row.input_ms=intent.sampled_at;angular_row.input_generation=intent.generation;
        angular_row.enabled=intent.enabled;angular_row.allowed=intent.angular_allowed;angular_row.published_rate=intent.yaw_rate;
        angular_row.proof_ms=intent.angular_proof.sampled_at;angular_row.proof_step=intent.angular_proof.physics_step;
        angular_row.proof_owner_generation=intent.angular_proof.owner_generation;
        angular_row.proof_episode=intent.angular_proof.episode;angular_row.proof_departure=intent.angular_proof.departure;
        angular_row.proof_valid=intent.angular_proof.valid;
        angular_row.proof_owner_matches=intent.angular_proof.owner==arcade_mode::NativeInputOwner{owner.base,owner.entity,owner.core,owner.context,owner.rig};
        angular_row.spin_mode=static_cast<unsigned>(intent.spin.mode);angular_row.facts=intent.spin.facts.encode();angular_row.armed=intent.spin.physical_armed;
        angular_row.spin_epoch=intent.spin.epoch;angular_row.spin_serial=intent.spin.serial;
        angular_row.bodies=static_cast<unsigned>(bodies.parts.size());angular_row.before=angular_row.staged=spins[0];
        angular_row.up=bodies.up;angular_row.forward=bodies.forward;
        // Angular writer observation end.
        // Ground writer observation begin. No new native reads or permissions.
        arcade_mode::GroundWriterObservation writer_observation{
            arcade_mode::begin_ground_writer_observation(now,{owner.base,owner.entity,owner.core,owner.context,owner.rig},native_frame,
                intent.enabled,intent.ground_controls.down,requests),arcade_mode::finish_ground_writer_observation};
        auto& writer_row=writer_observation.row;
        // This receipt describes scalar resistance controls, not yaw ownership.
        // Keep its existing wire bits while the internal carrier also owns LX.
        writer_row.input_ms=intent.sampled_at;writer_row.input_generation=intent.generation;writer_row.controls=intent.ground_controls.encode()&0x1ffu;
        writer_row.proof_ms=intent.angular_proof.sampled_at;writer_row.proof_step=intent.angular_proof.physics_step;
        writer_row.proof_owner_generation=intent.angular_proof.owner_generation;
        writer_row.proof_valid=intent.angular_proof.valid;writer_row.ordinary=ordinary;
        writer_row.proof_owner_matches=intent.angular_proof.owner==arcade_mode::NativeInputOwner{owner.base,owner.entity,owner.core,owner.context,owner.rig};
        writer_row.state=physics;writer_row.mode=mode;writer_row.contacts=contacts;writer_row.requests=requests;
        writer_row.bodies=static_cast<unsigned>(bodies.parts.size());writer_row.before=writer_row.staged=velocities[0];
        // Ground writer observation end.
        bool angular_release{};
        float resistance_change{};
        // Stage every owned change first. SourceReader keeps original bytes;
        // no native write may invalidate that snapshot before final validation.
        const auto commit_motion = [&](bool linear,bool angular) {
            angular_observation.staging(angular,spins[0]); // Angular writer observation.
            writer_observation.staging(linear,angular,velocities[0],resistance_change); // Ground writer observation.
            if (!linear && !angular) return;
            reader.verify(); NoBailSkater after{};
            source_require(no_bail_skater(after) && after==owner, "Arcade motion ownership changed before writing.");
            writer_observation.validated(); // Ground writer observation.
            angular_observation.validated(); // Angular writer observation.
            writer_observation.writing(); // Ground writer observation.
            angular_observation.writing(); // Angular writer observation.
            write_arcade_bodies(bodies.parts,velocities,spins,flags,linear,angular,[&] {
                writer_observation.body_completed(); // Ground writer observation.
                angular_observation.body_completed(); // Angular writer observation.
            });
            writer_observation.verified(); // Ground writer observation.
            angular_observation.verified(); // Angular writer observation.
            if (resistance_change<0) push_speed().arcade_drive.account_resistance(resistance_change);
        };
        const auto release_turn = [&](bool permitted,bool off=false) {
            bool released{};
            // AIR accepts a tilted native basis. Its exact write receipt, not
            // the upright-only ground policy, also governs withdrawal.
            if (off) {
                const bool ground=a.turn_write.release_cleanup(cleanup,bodies.parts,spins,bodies.up,now,ordinary && permitted);
                const bool air=a.air_write.release_cleanup(cleanup,bodies.parts,spins,bodies.up,now,permitted);
                released=ground || air;
            } else {
                const bool ground=a.turn_write.release(native_frame,bodies.parts,spins,bodies.up,now,ordinary && permitted);
                const bool air=a.air_write.release_cleanup(cleanup,bodies.parts,spins,bodies.up,now,permitted);
                released=ground || air;
            }
            if (!released) return;
            arcade_mode::AirMotionOutput neutral{}; neutral.ready=neutral.angular=true;
            source_require(arcade_mode::AirMotion::apply_bodies(neutral,velocities,spins,bodies.up), "Invalid angular release.");
            angular_release=true;
            angular_row.release=true; // Angular writer observation.
        };
        if (!intent.enabled) {
            wall_writer(8,"intent_disabled");
            a.ground_yaw.cancel(&ground_turn); a.ground_output={}; a.output={}; a.controller.reset();
            a.ground_resistance.reset(); a.ready_until.store(0,std::memory_order_release);
            release_turn(!manual.admitted,true); commit_motion(false,angular_release); return;
        }
        // OFF may have deferred withdrawal while the input mutex was busy.
        // A new command generation must retire that original physical receipt
        // before normal motion can consume or replace it. Cleanup retains its
        // original owner/contact/action/body checks and write deadline.
        if (a.generation != intent.generation && (a.turn_write.pending() || a.air_write.pending())) {
            if (!cleanup.inspected) {
                wall_writer(9,"cleanup_uninspected");
                a.output={}; a.ground_output={}; a.ready_until.store(0,std::memory_order_release); return;
            }
            release_turn(!manual.admitted,true);
        }
        // A wallplant can only enter through the recorded chooser handoff.
        // This ordinary motion path never bypasses its contact hold phase.
        (void)wallplant_handoff_active;
        wall_writer(6,"ordinary_motion_only");
        const auto output = a.controller.step(intent, {now, static_cast<std::uint64_t>(core), physics, bodies.seconds,
            bodies.root[1], velocities[0][1], spins[0][1], ordinary, velocities[0][0], velocities[0][2],true,
            arcade_mode::NativeAirSpinProof::from_frame(native_frame),
            {native_frame.flip_known,native_frame.flip_id,native_frame.selection_object},
            arcade_mode::wallplant_runtime::preserves_combo(native_frame,now)});
        // Angular writer observation: controller result, before native writes.
        angular_row.output_known=true;angular_row.output_ready=output.ready;angular_row.output_angular=output.angular;angular_row.output_rate=output.spin_y;
        NoBailSkater after{};
        reader.verify();
        source_require(no_bail_skater(after) && after == owner, "Air motion ownership changed before execution.");
        a.owner = owner; a.client = client; a.output = output; a.output_proof = arcade_mode::NativeAirSpinProof::from_frame(native_frame); a.stepped_at = now; a.generation = intent.generation;
        a.ready_version.fetch_add(1,std::memory_order_acq_rel);
        a.ready_proof.store(output.ready ? arcade_mode::NativeAirSpinProof::from_frame(native_frame) : arcade_mode::NativeAirSpinProof{});
        a.ready_until.store(output.ready && native_frame.valid ? native_frame.sampled_at + 150 : 0,std::memory_order_relaxed);
        a.ready_version.fetch_add(1,std::memory_order_release);
        // Ordinary ground and independently admitted Manual/Nose use the same
        // resistance adapter. Native collision/braking loss is never restored.
        // Fast arrivals retain the existing unknown-override protection.
        const bool braking = (requests & 4u) != 0 || ((requests & 2u) != 0 &&
            std::abs(reader.value<float>(bodies.context, 0x1880)) > .05f);
        const bool balancing = manual.matches(now,{owner.base,owner.entity,owner.core,owner.context,owner.rig},physics,mode,contacts) &&
            arcade_mode::native_manual_contact_current(manual);
        const bool controls_current = intent.angular_proof.same_owner_observation(native_frame,now) && intent.ground_controls.known;
        // A real Manual RS holder reserves vertical input before balance proof
        // catches up. Reservation only withdraws ordinary scalar assistance.
        const bool ground_eligible = controls_current && (balancing || !intent.ground_controls.manual_vertical_reserved) &&
            (physics == 100 || physics == 103) && ordinary &&
            (contacts == 0xc000u || balancing) && mode <= 2 && std::abs(velocities[0][1]) < .3f;
        const auto ground_step = a.ground_steps.consume(native_frame,now,ground_eligible);
        writer_row.balancing=balancing; // Ground writer observation.
        writer_observation.policy(controls_current,ground_eligible,ground_step,braking); // Ground writer observation.
        if (ground_step) {
            if (ground_step.baseline) a.ground_resistance.reset();
            const float speed = std::hypot(velocities[0][0], velocities[0][2]);
            const auto& controls = intent.ground_controls;
            const float total_speed = std::hypot(speed,velocities[0][1]);
            const float facing_dot = velocities[0][0]*bodies.forward[0] + velocities[0][1]*bodies.forward[1] + velocities[0][2]*bodies.forward[2];
            const bool selected_brake = controls_current && controls.known ? controls.trying_to_brake(total_speed,facing_dot) : braking;
            writer_observation.selected(selected_brake); // Ground writer observation.
            const arcade_mode::GroundFriction friction{!controls_current || !controls.known || controls.auto_kick,false};
            const float change = a.ground_resistance.step(ground_step.seconds, speed,
                physics == 103 && push_speed().arcade_crouched.load(std::memory_order_relaxed), selected_brake, balancing,friction);
            writer_row.requested_resistance=change; // Ground writer observation.
            if (change < 0 && speed > 0) {
                const float dx = velocities[0][0] / speed * change, dz = velocities[0][2] / speed * change;
                for (std::size_t i = 0; i < bodies.parts.size(); ++i) {
                    velocities[i][0] += dx; velocities[i][2] += dz;
                    for (float v : velocities[i])
                        source_require(std::isfinite(v) && std::abs(v) < 1000, "Invalid ground resistance result.");
                }
                resistance_change=change;
            }
        } else if (ground_step.status == arcade_mode::GroundStepStatus::unavailable) a.ground_resistance.reset();
        a.ground_output=a.ground_yaw.step(intent,native_frame,now,bodies.seconds,velocities[0],ordinary,manual.admitted,&ground_turn);
        auto applied=a.ground_output.ready ? arcade_mode::GroundYaw::body_output(a.ground_output) : output;
        angular_row.ground_selected=a.ground_output.ready; // Angular writer observation.
        // Neutral is a withdrawal of our previous ground rate, not permission
        // to overwrite a new native impulse with an unconditional zero.
        if (a.ground_output.ready && applied.angular && !a.ground_output.command.serial &&
            !a.ground_output.controls.left && !a.ground_output.controls.right && a.ground_output.rate==0) {
            a.ground_output.angular=false; applied.angular=false;
        }
        // A released AIR rate is conditional withdrawal of our exact write.
        // A new native angular impulse wins; it cannot be replaced by zero.
        if (!a.ground_output.ready && applied.angular && applied.spin_y==0 && !applied.tap_step) {
            a.output.angular=false; applied.angular=false;
        }
        if (!a.ground_output.command.serial)
            release_turn(!applied.angular && intent.angular_allowed && !manual.admitted);
        if (applied.vertical || applied.angular || applied.horizontal)
            source_require(arcade_mode::AirMotion::apply_bodies(applied, velocities, spins, bodies.up), "Arcade motion result is invalid.");
        commit_motion(resistance_change<0 || applied.vertical || applied.horizontal,applied.angular || angular_release);
        // Ordinary steering needs the same conditional withdrawal handoff as
        // a half-turn; neither may leave a still-owned rate after cancellation.
        if (a.ground_output.ready && applied.angular)
            a.turn_write.record(native_frame,bodies.parts,spins,bodies.up,cleanup);
        if (!a.ground_output.ready && applied.angular) {
            a.turn_write.clear();
            a.air_write.record(native_frame,bodies.parts,spins,bodies.up,cleanup);
        } else if (cleanup.inspected || !a.air_write.pending()) a.air_write.clear();
    } catch (...) {
        try {throw;} catch(const SourceGuard& issue) {wall_writer(7,issue.message);}
        catch(...) {wall_writer(7,"unknown_exception");}
        a.controller.reset(); a.ground_resistance.reset(); a.ground_yaw.reset(); a.ground_output={}; a.output = {}; a.turn_write.clear(); a.air_write.clear(); a.ready_until.store(0, std::memory_order_release);
    }
}
bool arcade_air_target(std::uintptr_t rig, std::uintptr_t context,
    const std::array<float, 16>* supplied, std::array<float, 16>& target) {
    auto& a = arcade_air();
    arcade_mode::AirMotionIntent intent{};
    arcade_mode::NativeManualContact current_manual{};
    arcade_mode::GroundTurnCommand ground_turn{};
    const auto now = GetTickCount64();
    if (!read_arcade_air(intent,&current_manual,&ground_turn) || !intent.enabled || intent.generation != a.generation ||
        (!a.output.ready && !a.ground_output.ready) || !a.owner.core || now < a.stepped_at ||
        now - a.stepped_at > 150 || now < intent.sampled_at || now - intent.sampled_at > 150 || multiplayer_session_active()) return false;
    const bool requested_angular = a.output.angular && intent.angular_allowed;
    const bool requested_ground=a.ground_output.ready && a.ground_output.angular && intent.angular_allowed;
    if (!a.output.vertical && !requested_angular && !requested_ground) return false;
    SourceReader reader;
    if (reader.pointer(a.owner.core, 0x438) != rig || reader.pointer(a.owner.core, 0x3c0) != context) return false;
    const auto bodies = debug_noclip_bodies(a.owner.base, a.client, a.owner.entity);
    NoBailSkater owner{};
    if (bodies.offboard || bodies.wipeout || !no_bail_skater(owner) || owner != a.owner) return false;
    namespace native = game::build::v20260929::arcade_native_probe;
    const auto state = reader.value<std::uint32_t>(context, native::selected_state);
    if (state!=100 && state!=103 && state!=200 && state!=201) return false;
    arcade_mode::NativeRequestSample sampled{};sampled.valid=true;sampled.state=state;
    sampled.crouch_mode=reader.value<std::uint32_t>(bodies.rig_wrapper,native::crouch_mode);
    sampled.words[0]=reader.value<std::uint32_t>(context,native::request_words);
    sampled.words[1]=reader.value<std::uint32_t>(context,native::request_words + 4);
    const auto current_frame=arcade_mode::native_air_context(now,
        {owner.base,owner.entity,owner.core,owner.context,owner.rig},sampled);
    if(state==100 || state==103){
        if(!arcade_mode::AirMotion::ordinary_basis(bodies.up))return false;
        if(!arcade_mode::GroundYaw::target_current(a.ground_output,intent,current_frame,now,current_manual.admitted,&ground_turn))return false;
        source_require(reader.value<std::uint8_t>(owner.entity,0x7e0)==0 &&
            reader.raw(reinterpret_cast<std::uintptr_t>(supplied),target.data(),sizeof(target)) && valid_flight_transform(target),
            "Invalid native ground motion target.");
        // The existing physical rig target is distinct from a display matrix.
        // No display field is assumed or overwritten by this physical adapter.
        for(unsigned column:{0u,4u,8u})for(unsigned axis=0;axis<3;++axis)target[column+axis]=bodies.matrix[column+axis];
        arcade_mode::AirMotion::rotate_target(target,a.ground_output.angle_delta);
        source_require(valid_flight_transform(target),"Ground motion target is out of bounds.");
        reader.verify();return no_bail_skater(owner) && owner==a.owner;
    }
    const auto current=arcade_mode::NativeAirSpinProof::from_frame(current_frame);
    if (a.output_proof.matches(current,now) && arcade_mode::AirMotion::other_native_action(
            {current_frame.flip_known,current_frame.flip_id,current_frame.selection_object},a.output.native_launch_id,
            arcade_mode::AirMotion::qualified_followup(intent,current,now))) {
        // This callback can see the actor before the next body update. Retire
        // the old trajectory now so a subsequent idle sample cannot revive it.
        a.controller.yield_ordinary_motion();
        a.output.vertical=a.output.horizontal=false;
    }
    const bool angular = arcade_mode::AirMotion::angular_target_current(a.output,intent,a.output_proof,current,now);
    const float velocity_y=a.output.vertical ? reader.value<std::array<float,3>>(bodies.parts[0],0x70)[1] : 0;
    const bool vertical=arcade_mode::AirMotion::ordinary_basis(bodies.up) && a.output.vertical && arcade_mode::AirMotion::vertical_target_current(a.output,intent,a.output_proof,current,now,
        velocity_y,bodies.seconds,{current_frame.flip_known,current_frame.flip_id,current_frame.selection_object});
    if (!vertical && !angular) return false;
    source_require(reader.value<std::uint8_t>(owner.entity, 0x7e0) == 0 &&
        reader.raw(reinterpret_cast<std::uintptr_t>(supplied), target.data(), sizeof(target)) && valid_flight_transform(target),
        "Invalid native air motion target.");
    if (vertical) {
        auto observed_output=a.output;observed_output.velocity_y=velocity_y;
        target[13] = arcade_mode::AirMotion::next_height(bodies.root[1],observed_output,bodies.seconds);
    }
    if (angular) {
        // Preserve the native actor's supplied orientation, including flip
        // pitch/roll. Compose only this qualified step's local-up yaw delta.
        arcade_mode::AirMotion::rotate_target(target, a.output.spin_y * bodies.seconds);
    }
    source_require(valid_flight_transform(target), "Air motion target is out of bounds.");
    reader.verify();
    return no_bail_skater(owner) && owner == a.owner;
}
// A plugin's push (skater_push.h), written by the next on-board physics step.
struct PushVelocity {
    std::atomic<bool> pending{};
    std::uintptr_t client{},entity{},core{};
    std::array<float,3> velocity{};
    std::uint64_t expires{};
};
PushVelocity& push_velocity(){static PushVelocity r;return r;}
void apply_push_velocity(std::uintptr_t core) noexcept {
    auto& request=push_velocity();auto& state=source_state();
    if(!request.pending.load(std::memory_order_acquire) || state.busy.test_and_set(std::memory_order_acquire))return;
    SourceBusyScope scope{state.busy};
    try {
        const auto wanted=skater_push::peek();
        // Held with no push: the board stays still; a push is written once it is asked for.
        const bool hold=skater_push::holding() && request.velocity==std::array<float,3>{};
        if(!competition_stock_physics() || GetTickCount64()>=request.expires || !wanted || *wanted!=request.velocity) {
            request.pending.store(false,std::memory_order_release);return;
        }
        if(request.core!=core)return;
        const auto bodies=debug_noclip_bodies(state.trial.base,request.client,request.entity);
        if(bodies.core!=core || bodies.offboard)return;
        SourceReader reader;if(reader.value<std::uint8_t>(request.entity,0x7e0)!=0)return;
        std::array<std::array<float,3>,32> velocities{};std::array<std::uint32_t,32> flags{};
        for(std::size_t i=0;i<bodies.parts.size();++i) {
            velocities[i]=reader.value<std::array<float,3>>(bodies.parts[i],0x70);
            flags[i]=reader.value<std::uint32_t>(bodies.parts[i],0x60);
            // A push must discard fall energy as well as set absolute horizontal speed.
            velocities[i]=request.velocity;
        }
        reader.verify();
        for(std::size_t i=0;i<bodies.parts.size();++i) {
            body_write(bodies.parts[i]+0x70,velocities[i]);
            if(hold)body_write(bodies.parts[i]+0x90,std::array<float,3>{});
            body_write(bodies.parts[i]+0x60,flags[i]|(hold?0x28u:8u));
        }
        // Hold the validated local board still while the plugin holds it (a
        // countdown, a pending return). Native teleport must not carry old fall
        // velocity away from its destination before the pushed speed is applied.
        if(hold)return;
        // A queued request is not delivery: bail/teleport physics may still be
        // busy or have replaced the body. Acknowledge only the actual write.
        skater_push::accepted(request.velocity);
        request.pending.store(false,std::memory_order_release);
    }catch(...){}
}
// A plugin's velocity (set_plugin_velocity): written, or added, by the next physics step that has the
// local skater on its board, on the ground or in the air, within 500 ms. It rides on the trainer's
// skater (set_push_speed), as cruise control does.
struct PluginVelocity {
    std::mutex mutex;
    std::array<float, 3> velocity{};
    bool add{};
    ULONGLONG expires{};
    std::atomic<bool> pending{};
};
PluginVelocity& plugin_velocity() { static auto* value = new PluginVelocity; return *value; }
void apply_plugin_velocity(std::uintptr_t core) noexcept {
    auto& v = plugin_velocity();
    if (!v.pending.load(std::memory_order_acquire)) return;
    std::unique_lock lock(v.mutex, std::try_to_lock);
    if (!lock) return;
    const ULONGLONG ticks = GetTickCount64();
    if (ticks >= v.expires) {
        v.pending.store(false, std::memory_order_release);
        return;
    }
    auto& p = push_speed();
    if (ticks >= p.expires.load(std::memory_order_acquire)) return;
    SourceLastError error;
    auto& state = source_state();
    if (!state.initialized.load(std::memory_order_acquire) || state.busy.test_and_set(std::memory_order_acquire)) return;
    SourceBusyScope scope{state.busy};
    try {
        const auto bodies = debug_noclip_bodies(state.trial.base, p.client, p.entity);
        if (bodies.core != core || bodies.offboard || bodies.parts.empty()) return;
        SourceReader reader;
        if (reader.value<std::uint8_t>(p.entity, 0x7e0) != 0) return; // teleporting
        std::array<std::array<float, 3>, 32> velocities{};
        std::array<std::uint32_t, 32> flags{};
        const auto count = std::min<std::size_t>(bodies.parts.size(), velocities.size());
        for (std::size_t i = 0; i < count; ++i) {
            velocities[i] = reader.value<std::array<float, 3>>(bodies.parts[i], 0x70);
            flags[i] = reader.value<std::uint32_t>(bodies.parts[i], 0x60);
        }
        reader.verify();
        for (std::size_t i = 0; i < count; ++i) {
            for (std::size_t a = 0; a < 3; ++a) velocities[i][a] = v.add ? velocities[i][a] + v.velocity[a] : v.velocity[a];
            body_write(bodies.parts[i] + 0x70, velocities[i]);
            body_write(bodies.parts[i] + 0x60, flags[i] | 8u);
        }
        v.pending.store(false, std::memory_order_release);
    } catch (...) {}
}
// ---- pump power --------------------------------------------------------------------------------
// What a pump gains, times a factor. The game's trick scripts decide when the skater is pumping
// (Bool.Intent.Pumping, read by the trainer) and the game's physics then speeds the skater up;
// which numbers size that has not been found. Measured on a 12 m ramp (the skater's energy, speed
// and height, after each physics step against the step before): a pump of the game's own gains
// about 0.3 to 1 m/s over at most 28 steps, and the game gives less the faster the skater already
// is, so a share of the game's gain fades to nothing at speed (x 100 felt like x 1). The trainer's
// share is therefore its own: for every physics step of a pump it adds pump_rate x (factor - 1)
// along the direction of travel, and under 1 takes that much away. The game takes about half of
// an addition back while the pump lasts (measured at x 10 and x 100; PumpMaxDeceleration is not
// what does it), so the rate is twice what one more of the game's pumps would need at x 2.
// Owner, 2026-10-07: x 100 is strong on a small quarter and on a 12 m half pipe.
constexpr float pump_gravity = 9.81f; // m/s2, as flights measure it
constexpr float pump_rate = 0.03f;    // m/s each physics step
constexpr float pump_step = 3.0f;     // m/s the trainer may add or take in one physics step
struct PumpPower {
    std::atomic<float> factor{1};
    std::atomic<bool> pumping{}, measure{};
    std::uintptr_t client{}, entity{};
    std::atomic<ULONGLONG> expires{};
    std::atomic<std::uint32_t> pumps{};
    std::atomic<float> gained{}, given{}, speed{};
    std::atomic<std::uint32_t> steps{};
    // Physics thread only.
    std::uint32_t count{};
    bool primed{}, running{};
    float energy{}, sum{}, credit{};
};
PumpPower& pump_power() { static auto* value = new PumpPower; return *value; }
void trainer_pump_power(std::uintptr_t core) noexcept {
    auto& p = pump_power();
    const float factor = p.factor.load(std::memory_order_relaxed);
    const auto stop = [&] { p.primed = p.running = false; };
    if ((factor == 1 && !p.measure.load(std::memory_order_relaxed)) || GetTickCount64() >= p.expires.load(std::memory_order_acquire)) return stop();
    const auto watch = watched_physics_state();
    if (!watch.valid || (watch.state != 100 && watch.state != 103)) return stop(); // riding, standing or crouched
    SourceLastError error;
    auto& state = source_state();
    if (!state.initialized.load(std::memory_order_acquire) || state.busy.test_and_set(std::memory_order_acquire)) return;
    SourceBusyScope scope{state.busy};
    try {
        const auto bodies = debug_noclip_bodies(state.trial.base, p.client, p.entity);
        if (bodies.core != core || bodies.offboard || bodies.parts.empty()) return stop();
        SourceReader reader;
        std::array<std::array<float, 3>, 32> velocities{};
        std::array<std::uint32_t, 32> flags{};
        const auto count = std::min<std::size_t>(bodies.parts.size(), velocities.size());
        for (std::size_t i = 0; i < count; ++i) {
            velocities[i] = reader.value<std::array<float, 3>>(bodies.parts[i], 0x70);
            flags[i] = reader.value<std::uint32_t>(bodies.parts[i], 0x60);
        }
        reader.verify();
        const auto& v = velocities[0];
        const float squared = v[0] * v[0] + v[1] * v[1] + v[2] * v[2], speed = std::sqrt(squared);
        const float height = bodies.root[1];
        if (!(speed >= 1.0f) || !(speed < 1000.0f) || !std::isfinite(height)) return stop();
        const float energy = 0.5f * squared + pump_gravity * height;
        if (!p.primed || !p.pumping.load(std::memory_order_relaxed)) { // coasting: only keep up
            p.primed = true;
            p.running = false;
            p.energy = energy;
            return;
        }
        if (!p.running) {
            p.running = true;
            p.sum = p.credit = 0;
            p.count = 0;
            p.pumps.fetch_add(1, std::memory_order_relaxed);
        }
        const float step = energy - p.energy;
        p.energy = energy;
        if (std::abs(step) > speed * 2.0f + 2.0f) return; // a teleport or a hit, not a pump
        p.sum += step;
        float change = 0;
        if (factor != 1) change = std::clamp((factor - 1) * pump_rate, -pump_step, pump_step);
        if (speed + change < 1.0f || speed + change > 300.0f) change = 0;
        ++p.count;
        if (std::abs(change) >= 0.002f) {
            const float scale = (speed + change) / speed;
            // Like the native velocity writers: XYZ at +70 and the dirty bit 8 at +60.
            for (std::size_t i = 0; i < count; ++i) {
                for (auto& axis : velocities[i]) axis *= scale;
                body_write(bodies.parts[i] + 0x70, velocities[i]);
                body_write(bodies.parts[i] + 0x60, flags[i] | 8u);
            }
            const float given = 0.5f * ((speed + change) * (speed + change) - squared);
            p.credit += given;
            p.energy += given; // the trainer's own share is not the game's gain
        }
        p.gained.store(p.sum, std::memory_order_relaxed);
        p.given.store(p.credit, std::memory_order_relaxed);
        p.speed.store(speed, std::memory_order_relaxed);
        p.steps.store(p.count, std::memory_order_relaxed);
    } catch (...) {}
}
// ---- gravity -----------------------------------------------------------------------------------
// In the air on the board the skater follows a record the game keeps in its trajectory object: a start point, a launch
// velocity, gravity (0, -9.8, 0) and a step, read as start + velocity t + gravity t^2 / 2 at every physics step. Measured
// with a scripted ollie (2026-10-12): the game's tuning, the gravity its jump planner reads and the bodies' own speeds
// all leave the fall at 9.8 m/s2; that record's gravity held at -4.9 gives a fall of 5.5 m/s2, an ollie of 0.70 m and
// 1.05 s (the game's: 0.36 m, 0.58 s), at -19.6 a fall of 20.4 m/s2. The game writes the record again at every launch,
// so the trainer writes its gravity after each physics step, and lets go by writing nothing.
constexpr std::uintptr_t flight_record_gravity = 0x18a0, trajectory_context = 0xb808;
constexpr float game_gravity = -9.8f;
struct AirGravity {
    std::atomic<float> factor{1};
    std::atomic<std::uint64_t> expires{};
    std::atomic<std::uintptr_t> client{}, entity{};
    std::atomic<std::uint32_t> steps{};
    // The trajectory objects the game has launched lately (the local skater's is the one that points at its context).
    std::array<std::atomic<std::uintptr_t>, 8> records{};
    std::atomic<std::uint32_t> noted{};
    std::uintptr_t held{}; // physics thread: the record whose gravity is the trainer's now
    std::uintptr_t searches{}; // physics thread: the landing searches' gravity (below) the trainer wrote, 0 when stock
    // The landing searches' gravity (0: factor), and how far the air animations keep the game's timing (0 .. 1).
    std::atomic<float> landing{}, timing{};
    // The rate that does it now, and the countdown seen last (physics thread).
    std::atomic<float> animation_rate{1};
    float countdown{-1};
    LONGLONG countdown_moved{};
};
AirGravity& air_gravity() {
    static auto* value = new AirGravity;
    return *value;
}
// The landing searches. Where a flight lands, and when, the game finds by walking trajectories through the world
// (reckoning__set_launch 0x4752aa0 every 7 steps in the air, the grind searches 0x47fb08f, 0x4803148); the gravity of each
// walk is copied from the trajectory query manager (the object at rva 0x7785da8, +0x30, (0, -9.8, 0)) into the query
// (0x4787820, +0xd0). The air state then follows the record it chose and lands at its step, and the animation counts
// down to it (TimeToLand). 0.8.1 set only that countdown again, for flat ground; on a real jump the record the skater
// followed was still planned at 9.8, so the landing stayed late (a player at x1.25, 2026-10-21). So the manager's gravity
// is the trainer's while it holds one, and goes back to the game's when it lets go: every search then plans with it.
constexpr std::uintptr_t query_manager = 0x7785da8, query_manager_gravity = 0x30;
// mov rcx, [rip + query_manager] in the reckoning's query launch (0x473e280): this build's manager.
constexpr std::uintptr_t query_manager_load = 0x473e312;
constexpr unsigned char query_manager_load_bytes[]{0x48, 0x8b, 0x0d, 0x8f, 0x7a, 0x04, 0x03};
void air_gravity_searches(AirGravity& g, std::uintptr_t base, float wanted) {
    static const bool known = std::memcmp(reinterpret_cast<const void*>(base + query_manager_load), query_manager_load_bytes,
                                          sizeof(query_manager_load_bytes)) == 0;
    if (!known) return;
    SourceReader reader;
    const auto manager = reader.pointer(base + query_manager);
    reader.verify();
    if (!manager) return;
    const auto now = reader.value<std::array<float, 3>>(manager, query_manager_gravity);
    reader.verify();
    if (now[0] != 0 || now[2] != 0 || !(now[1] < -0.1f) || !(now[1] > -250.0f)) return;
    if (now[1] != wanted) body_write(manager + query_manager_gravity + 4, wanted);
    g.searches = wanted == game_gravity ? 0 : manager;
}
// Air animations at the game's timing. The game fits a trick into the flight by the time to land (the animation's
// countdown, [[core+0x3b8]+0x10]+0xd8): the clips' time is warped toward it, but the catch at most to its own speed
// (animation-timelines.md, the type-3 windows: up to 1.5, then up to 1.0). A flight shorter than the game's gravity
// would make it (x1.31 on flat ground: a quarter shorter) so lands with the catch still playing: the stutter players
// see. With "timing" at 1 the skater's clips run faster by the time this flight would take at 9.8 over the time it
// takes, so the game sees the flight it planned for and its animations come out as they would at x1; between 0 and 1
// part of the way. The countdown is planned with the landing gravity, the flight falls with the trainer's.
constexpr std::uintptr_t core_animation_offset = 0x3b8, animation_output_offset = 0x10, output_landing_countdown = 0xd8;
constexpr std::uintptr_t flight_record_velocity = 0x1890;
void air_gravity_animation(AirGravity& g, std::uintptr_t core, std::uintptr_t record, float factor, float landing, float timing) {
    SourceReader reader;
    const auto animation = reader.pointer(core, core_animation_offset);
    reader.verify();
    const auto output = animation ? reader.pointer(animation, animation_output_offset) : 0;
    reader.verify();
    float countdown = output ? reader.value<float>(output, output_landing_countdown) : -1.0f;
    const float up = reader.value<float>(record, flight_record_velocity + 4);
    reader.verify();
    if (!std::isfinite(countdown) || countdown > 10.0f) countdown = -1.0f;
    LARGE_INTEGER clock{}, rate{};
    QueryPerformanceCounter(&clock);
    QueryPerformanceFrequency(&rate);
    if (countdown != g.countdown) g.countdown_moved = clock.QuadPart;
    // A new plan (the countdown set again, higher): the rate for this flight, from the drop it plans for.
    if (countdown > 0.02f && std::isfinite(up) && (g.countdown <= 0 || countdown > g.countdown + 0.05f)) {
        const float drop = up * countdown + 0.5f * game_gravity * landing * countdown * countdown;
        // The time to come down that far with a gravity of g x the game's.
        const auto flight = [&](float g) {
            const float root = up * up + 2.0f * game_gravity * g * drop;
            return root >= 0 ? (up + std::sqrt(root)) / (-game_gravity * g) : -1.0f;
        };
        const float stock = flight(1.0f), real = flight(factor);
        const float full = stock > 0 && real > 0 ? std::clamp(stock / real, 0.25f, 4.0f) : 1.0f;
        g.animation_rate.store(1.0f + (full - 1.0f) * timing, std::memory_order_relaxed);
    }
    g.countdown = countdown;
    // Landed, or the countdown has stood still (a bail, another state): the game's own rate.
    const bool still = clock.QuadPart - g.countdown_moved > rate.QuadPart / 10;
    if (!(countdown > 0) || still) g.animation_rate.store(1.0f, std::memory_order_relaxed);
}
void trainer_air_gravity(std::uintptr_t core) noexcept {
    auto& g = air_gravity();
    float factor = g.factor.load(std::memory_order_relaxed);
    float landing = g.landing.load(std::memory_order_relaxed);
    const float timing = g.timing.load(std::memory_order_relaxed);
    if (GetTickCount64() >= g.expires.load(std::memory_order_acquire)) factor = 1, landing = 1;
    if (!(landing > 0)) landing = factor;
    if (factor == 1 && landing == 1 && !g.held && !g.searches) {
        g.animation_rate.store(1.0f, std::memory_order_relaxed);
        return;
    }
    SourceLastError error;
    auto& state = source_state();
    if (!state.initialized.load(std::memory_order_acquire) || state.busy.test_and_set(std::memory_order_acquire)) return;
    SourceBusyScope scope{state.busy};
    try {
        const auto bodies = debug_noclip_bodies(state.trial.base, g.client.load(std::memory_order_relaxed), g.entity.load(std::memory_order_relaxed));
        if (bodies.core != core || !bodies.context) return;
        bool found = false;
        for (const auto& slot : g.records) {
            const auto record = slot.load(std::memory_order_relaxed);
            if (!record) continue;
            SourceReader reader;
            const auto context = reader.pointer(record, trajectory_context);
            const auto now = reader.value<std::array<float, 3>>(record, flight_record_gravity);
            reader.verify();
            // Only a record that reads as a gravity is written: straight down, and the game's or one of the trainer's.
            if (context != bodies.context || now[0] != 0 || now[2] != 0 || !(now[1] < -0.1f) || !(now[1] > -250.0f)) continue;
            const float wanted = game_gravity * factor;
            if (now[1] != wanted) body_write(record + flight_record_gravity + 4, wanted);
            g.held = factor == 1 ? 0 : record;
            if (factor != 1) g.steps.fetch_add(1, std::memory_order_relaxed);
            if (factor != 1 && timing > 0) air_gravity_animation(g, core, record, factor, landing, timing);
            else g.animation_rate.store(1.0f, std::memory_order_relaxed);
            found = true;
            break;
        }
        if (!found) {
            g.held = 0;
            g.animation_rate.store(1.0f, std::memory_order_relaxed);
        }
        // The searches plan with the landing gravity while one is set, in the air or not (a search starts at the launch).
        if (landing != 1 || g.searches) air_gravity_searches(g, state.trial.base, game_gravity * landing);
    } catch (...) {}
}
// ---- revert boost ------------------------------------------------------------------------------
// When the game auto reverts a landing the board is still well out of line with the body and the
// game swings it round. Earlier builds of the game gained speed there; this gives it back: a
// forward speed along the direction of travel on the step of that landing. The measuring, the
// thresholds and the size of the boost are AutoRevertBoost's (Sivaes, jaq and OVM,
// github.com/Sivaes/AutoRevertBoost, GPL-3.0), taken from their play logs: clean and short 180s
// land with the board 1-3 degrees off the body, auto reverts 71-130 degrees off.
// The thresholds and sizes are RevertTuning's (client_source_spawn.h), the player's to change.
// A second rule, off as shipped (min_slip 90): board and body together out of line with the
// direction of travel, which the game then swings round. 0.4.1 had it on at 12 degrees and it paid
// ordinary short or crooked 180s, flip trick 180s among them, whose landing the game merely
// straightens (AutoRevertBoost's authors, with 60 Hz logs: those land 0 to 3 degrees off the body,
// real auto reverts 70 to 82, board bends 85 to 90). A player may still turn it on; a landing that
// counts by it alone never gets more than an auto revert does.
constexpr ULONGLONG revert_max_age_ms = 400;
// How much, at strength 1, from how far the board turned relative to the body over the flight:
// a board bend (up to 140 degrees) gives +2 m/s at 40 degrees rising to +2.6 at 140; an auto
// revert (past 140) a flat +1.5 m/s, less than any bend.
float revert_boost_amount(const RevertTuning &t, float board_rotation) {
    if (board_rotation > 140.0f) return t.auto_boost;
    return t.bend_boost + (t.full_boost - t.bend_boost) * std::clamp((board_rotation - 40.0f) / 320.0f, 0.0f, 1.0f);
}
struct RevertBoost {
    std::atomic<float> strength{}; // 0: off
    std::uintptr_t client{}, entity{};
    std::atomic<ULONGLONG> expires{};
    SRWLOCK report_lock = SRWLOCK_INIT; // the report and the tuning
    RevertBoostReport report;
    RevertTuning tuning;
    // Physics thread only.
    bool primed{};          // the landing count has been read once since the boost was switched on
    std::uint64_t seen{};   // the last landing handled
    ULONGLONG cooldown_until{};
};
RevertBoost& revert_boost() { static auto* value = new RevertBoost; return *value; }
void trainer_revert_boost(std::uintptr_t core) noexcept {
    auto& r = revert_boost();
    const float strength = r.strength.load(std::memory_order_relaxed);
    const auto now = GetTickCount64();
    if (!(strength > 0) || now >= r.expires.load(std::memory_order_acquire)) {
        r.primed = false; // nothing is read while it is off
        return;
    }
    SourceLastError error;
    auto& state = source_state();
    if (!state.initialized.load(std::memory_order_acquire) || state.busy.test_and_set(std::memory_order_acquire)) return;
    SourceBusyScope scope{state.busy};
    try {
        const auto bodies = debug_noclip_bodies(state.trial.base, r.client, r.entity);
        if (bodies.core != core || bodies.parts.empty()) return;
        SourceReader reader;
        const auto selector = reader.pointer(bodies.core, 0x440);
        source_require(reader.pointer(selector, 8) == bodies.context, "Skater physics selector changed.");
        // The skater's world matrix, where debug_skater reads it.
        const auto collection = reader.pointer(r.entity, 0x70);
        source_require(reader.pointer(collection) == r.entity, "Skater transform owner changed.");
        const auto first = reader.value<std::uint8_t>(collection, 9), extra = reader.value<std::uint8_t>(collection, 10);
        source_require(reader.value<std::uint8_t>(collection, 8) <= 128 && first <= 128 && extra <= 32, "Skater transform layout changed.");
        std::array<std::array<float, 3>, 32> velocities{};
        std::array<std::uint32_t, 32> flags{};
        const auto count = std::min<std::size_t>(bodies.parts.size(), velocities.size());
        for (std::size_t i = 0; i < count; ++i) {
            velocities[i] = reader.value<std::array<float, 3>>(bodies.parts[i], 0x70);
            flags[i] = reader.value<std::uint32_t>(bodies.parts[i], 0x60);
        }
        reader.verify();
        watch_revert_spin(selector, collection + std::uintptr_t{0x10} + (std::uintptr_t{first} + 2 * std::uintptr_t{extra}) * 0x20, bodies.parts[0]);
        const auto landing = last_revert_landing();
        AcquireSRWLockShared(&r.report_lock);
        const auto t = r.tuning;
        ReleaseSRWLockShared(&r.report_lock);
        if (!r.primed) { // landings from before it was switched on are not ours
            r.primed = true;
            r.seen = landing.sequence;
            return;
        }
        if (landing.sequence == r.seen) return;
        r.seen = landing.sequence;
        float board_offset = std::fmod(std::abs(landing.board_offset_degrees), 180.0f);
        board_offset = std::min(board_offset, 180.0f - board_offset); // 0: lined up, forward or fakie
        const float spin = std::abs(landing.spin_degrees);
        const float board_rotation = landing.board_valid ? std::abs(landing.board_spin_degrees - landing.spin_degrees) : 0.0f;
        const float speed = std::hypot(velocities[0][0], velocities[0][2]);
        // Out of line with the travel, forward or fakie: 0 = lined up, 90 = sideways.
        float slip = 0.0f;
        if (std::isfinite(speed) && speed >= 1.0f) {
            slip = std::fmod(std::abs(landing.heading_degrees - std::atan2(velocities[0][0], velocities[0][2]) * 57.29578f), 180.0f);
            slip = std::min(slip, 180.0f - slip);
        }
        const bool twisted = landing.board_valid && board_offset >= t.min_twist;
        const bool slipped = t.min_slip < 90.0f && slip >= t.min_slip;
        float amount = revert_boost_amount(t, board_rotation);
        if (!twisted) amount = std::min(amount, t.auto_boost);
        const float added = std::isfinite(speed) ? std::min(amount * strength, t.max_speed - speed) : 0.0f;
        const char *outcome = spin < t.min_spin ? "no boost: not enough spin"
            : !twisted && !slipped ? "no boost: the board landed in line with the body (a clean landing)"
            : landing.to < 100 || landing.to >= 200 || bodies.offboard ? "no boost: not riding"
            : now < landing.landed_at || now - landing.landed_at > revert_max_age_ms ? "no boost: seen too late"
            : static_cast<float>(landing.air_ms) < t.min_air * 1000.0f ? "no boost: too short a flight"
            : now < r.cooldown_until ? "no boost: too soon after the last"
            : !std::isfinite(speed) || speed < 1.0f || !(added > 0.005f) ? "no boost: standing still or already at the speed limit"
            : "boost";
        const bool fire = std::string_view(outcome) == "boost";
        AcquireSRWLockExclusive(&r.report_lock);
        r.report = {r.report.sequence + 1, spin, board_offset, board_rotation, slip, speed, fire ? added : 0.0f, static_cast<std::uint32_t>(landing.air_ms),
                    landing.to, landing.board_valid, outcome};
        ReleaseSRWLockExclusive(&r.report_lock);
        if (!fire) return;
        const float scale = (speed + added) / speed;
        // Like the native velocity writers: XYZ at +70 and the dirty bit 8 at +60.
        for (std::size_t i = 0; i < count; ++i) {
            velocities[i][0] *= scale;
            velocities[i][2] *= scale;
            body_write(bodies.parts[i] + 0x70, velocities[i]);
            body_write(bodies.parts[i] + 0x60, flags[i] | 8u);
        }
        r.cooldown_until = now + static_cast<ULONGLONG>(std::clamp(t.cooldown, 0.0f, 60.0f) * 1000.0f);
    } catch (...) {}
}
// ---- transition assist -------------------------------------------------------------------------
// Coming off a coping grind, a stall or a handplant back into the ramp. The trainer's client tick says when the
// skater is on a coping (a grind that began on steep ground), which way the coping runs and which side the ramp is on;
// the rest happens here, on the physics steps after the grind ends:
// - carry: the speed the skater had along the coping is put back for a third of a second, and the speed across it
//   (into the ramp) is capped, so the skater rolls off at about the angle they came in at. Skate 3 never turned the
//   velocity at the end of a grind: it nudged the board sideways up to 1 m/s and left the rest alone.
// - unstick: Skate 3's hang-up rule. Slower than 1.1 m/s for more than ten steps after leaving the coping (or after
//   landing a flight off a transition) is a hang-up: the skater is pushed into the ramp and down until moving again.
// - plant: when a handplant ends, the skater leaves the lip at least 0.9 m/s into the ramp and without the downward
//   speed the plant left them with (Skate 3: 0.9 m/s outward, a ballistic arc from above the lip).
// Everything here is a first build on behaviour not yet measured in this game (2026-10-11): the report is how each
// is checked in play.
constexpr std::uint32_t transition_carry_steps = 20, transition_watch_steps = 90, transition_plant_steps = 12;
constexpr float transition_hang_speed = 1.1f; // m/s, Skate 3's
constexpr std::uint32_t transition_hang_steps = 10;
constexpr float transition_kick = 0.12f, transition_kick_down = 0.10f; // m/s a step at strength 1
constexpr float transition_plant_out = 0.9f; // m/s into the ramp, Skate 3's
struct TransitionRun {
    std::atomic<ULONGLONG> expires{};
    std::uintptr_t client{}, entity{};
    SRWLOCK lock = SRWLOCK_INIT;
    TransitionAssist assist;   // under lock
    TransitionReport report;   // under lock
    // Physics thread only.
    int phase{}; // 0 idle, 1 on a coping, 2 just left one, 3 handplanting, 4 just left a handplant, 5 in a flight off a transition, 6 just landed one
    std::uint32_t steps{}, slow{};
    std::array<float, 2> edge{}, ramp{};
    float along{}, top{}, low{};
    TransitionReport now;
};
TransitionRun& transition_run() { static auto* value = new TransitionRun; return *value; }
void trainer_transition(std::uintptr_t core) noexcept {
    auto& r = transition_run();
    if (GetTickCount64() >= r.expires.load(std::memory_order_acquire)) {
        r.phase = 0;
        return;
    }
    AcquireSRWLockShared(&r.lock);
    const auto a = r.assist;
    ReleaseSRWLockShared(&r.lock);
    const bool acting = a.carry > 0 || a.unstick > 0 || a.plant > 0;
    if (!acting && !a.measure) {
        r.phase = 0;
        return;
    }
    const auto watch = watched_physics_state();
    if (!watch.valid) return;
    const std::uint32_t state = watch.state;
    const bool grinding = (state >= 400 && state < 500) || state == 603; // grinds and slides; the lip trick
    const bool planting = state == 600;
    const bool flying = state >= 200 && state < 300;
    const bool gone = state == 300 || (state >= 500 && state < 600); // a wipeout, or off the board
    if (r.phase == 0 && !(a.coping && grinding) && !planting && !(a.transition && flying)) return;
    SourceLastError error;
    auto& source = source_state();
    if (!source.initialized.load(std::memory_order_acquire) || source.busy.test_and_set(std::memory_order_acquire)) return;
    SourceBusyScope scope{source.busy};
    try {
        const auto bodies = debug_noclip_bodies(source.trial.base, r.client, r.entity);
        if (bodies.core != core || bodies.parts.empty()) return;
        SourceReader reader;
        std::array<std::array<float, 3>, 32> velocities{};
        std::array<std::uint32_t, 32> flags{};
        const auto count = std::min<std::size_t>(bodies.parts.size(), velocities.size());
        for (std::size_t i = 0; i < count; ++i) {
            velocities[i] = reader.value<std::array<float, 3>>(bodies.parts[i], 0x70);
            flags[i] = reader.value<std::uint32_t>(bodies.parts[i], 0x60);
        }
        reader.verify();
        const auto v = velocities[0]; // as the game left it this step
        const float speed = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        if (!std::isfinite(speed) || speed > 300.0f) return;
        const auto add = [&](float x, float y, float z) {
            // Like the native velocity writers: XYZ at +70 and the dirty bit 8 at +60.
            for (std::size_t i = 0; i < count; ++i) {
                velocities[i][0] += x;
                velocities[i][1] += y;
                velocities[i][2] += z;
                body_write(bodies.parts[i] + 0x70, velocities[i]);
                body_write(bodies.parts[i] + 0x60, flags[i] | 8u);
            }
        };
        const auto publish = [&] {
            AcquireSRWLockExclusive(&r.lock);
            const auto sequence = r.report.sequence + 1;
            r.report = r.now;
            r.report.sequence = sequence;
            ReleaseSRWLockExclusive(&r.lock);
        };
        const float height = bodies.root[1];
        // A hang-up: slower than Skate 3's limit for longer than its ten frames. `x`, `z`: the way to push.
        const auto hang = [&](float x, float z) {
            if (speed < transition_hang_speed) {
                ++r.slow;
                ++r.now.hung;
            } else {
                r.slow = 0;
            }
            if (!(a.unstick > 0) || r.slow <= transition_hang_steps) return;
            add(x * transition_kick * a.unstick, -transition_kick_down * a.unstick, z * transition_kick * a.unstick);
            ++r.now.kicked;
        };
        if (a.coping && grinding && r.phase != 2) {
            // On the coping: remember how the skater is moving along and across it. Nothing is written.
            if (r.phase != 1) r.now = {};
            r.phase = 1;
            r.edge = a.edge;
            r.ramp = a.ramp;
            r.along = v[0] * r.edge[0] + v[2] * r.edge[1];
            r.now.kind = 1;
            r.now.from = state;
            r.now.in_along = r.along;
            r.now.in_across = v[0] * r.ramp[0] + v[2] * r.ramp[1];
            r.now.in_down = -v[1];
            r.top = height;
            return;
        }
        if (r.phase == 1) {
            // The grind ended. A pop is the player's own way off; a bail or a step off ends it too.
            r.now.to = state;
            if (state == 200 || gone || planting) {
                r.phase = 0;
                return;
            }
            r.phase = 2;
            r.steps = r.slow = 0;
        }
        if (r.phase == 2) {
            if (gone || state == 200 || planting || (grinding && r.steps > 2)) { // popped, bailed, or locked on again
                r.now.to_last = state;
                publish();
                r.phase = 0;
                return;
            }
            ++r.steps;
            const float along = v[0] * r.edge[0] + v[2] * r.edge[1], across = v[0] * r.ramp[0] + v[2] * r.ramp[1];
            if (a.carry > 0 && r.steps <= transition_carry_steps && std::abs(r.along) > 0.5f) {
                const float wanted = r.along * std::min(a.carry, 1.0f);
                float change_along = 0, change_across = 0;
                if (along * wanted < 0 || std::abs(along) < std::abs(wanted)) change_along = wanted - along;
                if (a.drop_speed > 0 && across > a.drop_speed) change_across = a.drop_speed - across;
                if (std::abs(change_along) > 0.01f || std::abs(change_across) > 0.01f) {
                    add(r.edge[0] * change_along + r.ramp[0] * change_across, 0.0f, r.edge[1] * change_along + r.ramp[1] * change_across);
                    ++r.now.carried;
                }
            }
            hang(r.ramp[0], r.ramp[1]);
            if (r.steps == transition_carry_steps) {
                r.now.out_along = velocities[0][0] * r.edge[0] + velocities[0][2] * r.edge[1];
                r.now.out_across = velocities[0][0] * r.ramp[0] + velocities[0][2] * r.ramp[1];
                r.now.out_down = -velocities[0][1];
                r.now.game_along = along;
                r.now.game_across = across;
                r.now.drop = r.top - height;
            }
            if (r.steps >= transition_watch_steps) {
                r.now.to_last = state;
                r.now.fall = r.top - height;
                publish();
                r.phase = 0;
            }
            return;
        }
        if (planting && r.phase != 4) {
            // On a handplant: how far the plant itself carries the skater down before it lets go.
            if (r.phase != 3) {
                r.now = {};
                r.now.kind = 2;
                r.now.from = state;
                r.ramp = a.ramp;
                r.edge = a.edge;
                r.top = height;
            }
            r.phase = 3;
            r.top = std::max(r.top, height);
            r.low = height;
            return;
        }
        if (r.phase == 3) {
            r.now.to = state;
            r.now.drop = r.top - r.low; // lost inside the plant
            r.now.in_across = v[0] * r.ramp[0] + v[2] * r.ramp[1];
            r.now.in_along = v[0] * r.edge[0] + v[2] * r.edge[1];
            r.now.in_down = -v[1];
            if (gone) {
                publish();
                r.phase = 0;
                return;
            }
            r.phase = 4;
            r.steps = r.slow = 0;
        }
        if (r.phase == 4) {
            if (gone || planting) {
                r.now.to_last = state;
                publish();
                r.phase = 0;
                return;
            }
            ++r.steps;
            const float across = v[0] * r.ramp[0] + v[2] * r.ramp[1];
            if (a.plant > 0 && r.steps <= transition_plant_steps) {
                const float blend = std::min(a.plant, 1.0f);
                const float change_across = across < transition_plant_out ? (transition_plant_out - across) * blend : 0.0f;
                const float change_up = r.steps == 1 && v[1] < 0 ? -v[1] * blend : 0.0f; // the fall starts from rest, as off the top of an arc
                if (std::abs(change_across) > 0.01f || change_up > 0.01f) {
                    add(r.ramp[0] * change_across, change_up, r.ramp[1] * change_across);
                    ++r.now.carried;
                }
            }
            hang(r.ramp[0], r.ramp[1]);
            if (r.steps == transition_plant_steps) {
                r.now.out_across = velocities[0][0] * r.ramp[0] + velocities[0][2] * r.ramp[1];
                r.now.out_along = velocities[0][0] * r.edge[0] + velocities[0][2] * r.edge[1];
                r.now.out_down = -velocities[0][1];
                r.now.game_across = across;
            }
            if (r.steps >= transition_watch_steps) {
                r.now.to_last = state;
                r.now.fall = r.top - height; // below the plant's highest point a second and a half later
                publish();
                r.phase = 0;
            }
            return;
        }
        if (flying && a.transition && r.phase != 6) {
            // A flight off a transition: which way it goes over the ground, for a hang-up where it comes down.
            if (r.phase != 5) {
                r.now = {};
                r.now.kind = 3;
                r.now.from = state;
            }
            r.phase = 5;
            const float over = std::hypot(v[0], v[2]);
            if (over > 0.3f) r.ramp = {v[0] / over, v[2] / over};
            r.now.in_along = over;
            r.now.in_down = -v[1];
            return;
        }
        if (r.phase == 5) {
            r.now.to = state;
            if (gone || planting || flying) {
                r.phase = 0;
                return;
            }
            r.phase = 6;
            r.steps = r.slow = 0;
        }
        if (r.phase == 6) {
            if (gone || flying || planting) {
                r.now.to_last = state;
                if (r.now.hung) publish();
                r.phase = 0;
                return;
            }
            ++r.steps;
            // Riding away is not a hang-up however slow (the top of a kick turn): only a skater held on an edge or in
            // one of the game's in-between states counts here.
            if (state >= 100 && state < 200) r.slow = 0;
            else hang(r.ramp[0], r.ramp[1]);
            if (r.steps >= transition_watch_steps / 2) {
                r.now.to_last = state;
                if (r.now.hung) publish(); // only a landing that hung is worth a line
                r.phase = 0;
            }
            return;
        }
        r.phase = 0;
    } catch (...) {}
}
void noclip_physics_update(std::uintptr_t core) {
    const auto retained_probe=arcade_mode::wallplant_runtime::retained_writer_probe();
    const auto retained_phase=[&](unsigned stage,const char* reason) {
        if(!retained_probe.contact || core!=retained_probe.native.owner.core)return;
        SourceLastError error;
        std::uint32_t selected{};SourceReader reader;
        const bool known=reader.raw(retained_probe.native.owner.context+
            game::build::v20260929::arcade_native_probe::selected_state,&selected,sizeof(selected));
        arcade_mode::wallplant_runtime::retained_writer_event(retained_probe,stage,reason,selected,known,core);
    };
    WallplantHandoff handoff{};
    auto& staged=wallplant_handoff();
    if(core==staged.execution.native.owner.core) {
        handoff=staged;handoff.ticket=staged.ticket.take();staged={};
    }
    const auto probe=arcade_mode::wallplant_runtime::writer_probe();
    const auto observe_phase=[&](unsigned stage,const char* reason) {
        if(!probe.input || core!=probe.native.owner.core)return;
        SourceLastError error;
        std::uint32_t selected{};
        SourceReader reader;
        const bool known=core==probe.native.owner.core &&
            reader.raw(probe.native.owner.context+game::build::v20260929::arcade_native_probe::selected_state,
                &selected,sizeof(selected));
        arcade_mode::wallplant_runtime::writer_event(probe,stage,reason,selected,known,core);
    };
    retained_phase(2,"before_original");
    observe_phase(0,"before_original");
    const auto original = source_state().velocity_update_original.load(std::memory_order_acquire);
    if (original) original(core);
    observe_phase(1,"after_original");
    retained_phase(3,"after_original");
    const bool holding=wallplant_hold_core.load(std::memory_order_acquire)==core;
    if(!holding) {
        noclip_apply_velocity(core);
        trainer_apply_jump_scale(core);
    }
    if (const auto callback = physics_step_callback.load(std::memory_order_acquire)) callback();
    if(!holding) {
        trainer_push_speed(core);
        arcade_air_apply(core,handoff.ticket.selected);
        apply_push_velocity(core);
        apply_plugin_velocity(core);
        trainer_pump_power(core);
        trainer_air_gravity(core);
        trainer_revert_boost(core);
        trainer_transition(core);
    }
    // Native100 entry 482c200 has reconstructed body velocities before this
    // existing +58 update. All ordinary writers finish before the single kick.
    arcade_wallplant_after_reconstruction(handoff,core);
    arcade_wallplant_hold_step(core);
    retained_phase(4,"after_reconstruction");
}
bool noclip_motion_target(std::uintptr_t rig, std::uintptr_t context,
    const std::array<float,16>* supplied, std::array<float,16>& target) noexcept {
    if(competition_stock_physics())return false;
    SourceLastError error;
    auto& state = source_state();
    std::uint64_t hold_now=GetTickCount64();
    arcade_mode::NativeInputFrame hold_frame{};
    if(wallplant_hold_core.load(std::memory_order_acquire)) {
        const auto held=arcade_mode::wallplant_runtime::held_command();
        if(held && held.native.owner.rig==rig && held.native.owner.context==context)
            hold_frame=arcade_mode::native_wallplant_hold_context(held.native.owner,hold_now);
    }
    if (!state.initialized.load(std::memory_order_acquire) || !state.velocity_guard_active.load(std::memory_order_acquire) ||
        state.busy.test_and_set(std::memory_order_acquire)) return false;
    SourceBusyScope scope{state.busy};
    if(arcade_wallplant_hold_target(rig,context,supplied,target,hold_frame,hold_now))return true;
    const auto& held_target=wallplant_physical_hold();
    if(held_target.command && rig==held_target.owner.rig && context==held_target.owner.context)return false;
    auto& debug = state.trial.debug;
    if (!debug.noclip && !debug.park_editor && !debug.offboard_up_velocity.valid) {
        try { if (arcade_air_target(rig, context, supplied, target)) return true; }
        catch (...) { arcade_air().ready_until.store(0, std::memory_order_release); }
    }
    auto& boost = debug.offboard_up_velocity;
    if (!debug.noclip && boost.valid && boost.applied) {
        try {
            SourceReader reader;
            if (reader.pointer(boost.core, 0x438) != rig || reader.pointer(boost.core, 0x3c0) != context)
                return false;
            if (GetTickCount64() >= boost.expires) return false;
            const auto bodies = debug_noclip_bodies(state.trial.base, boost.client, boost.entity);
            source_require(bodies.core == boost.core && bodies.rig_wrapper == rig && bodies.context == context,
                "Off-board boost skater changed.");
            source_require(!debug.park_editor && reader.value<std::uint8_t>(boost.entity, 0x7e0) == 0,
                "Off-board boost interrupted by editing or teleporting.");
            source_require(reader.raw(reinterpret_cast<std::uintptr_t>(supplied), target.data(), sizeof(target)) &&
                valid_flight_transform(target), "Invalid off-board motion target.");
            // Native off-board movement drives the skeleton toward this target.
            // Body velocity alone is undone by that drive on the same step.
            // Lead only Y for the launch; keep native X/Z, rotation and collision flags.
            target[13] = std::max(target[13], bodies.root[1] + boost.velocity[1] * bodies.seconds);
            source_require(valid_flight_transform(target), "Off-board boost target is out of bounds.");
            reader.verify();
            if (bodies.offboard)
                source_require(ensure_offboard_up_velocity(state.trial.base, bodies.core, context, rig, boost.velocity[1]),
                    "Off-board boost motion ownership changed.");
            return bodies.seconds > 0;
        } catch (const SourceGuard& issue) { debug.status = issue.message; }
          catch (...) { debug.status = "Off-board boost motion failed."; }
        boost.valid = false;
        state.offboard_boost_core.store(0, std::memory_order_release);
        return false;
    }
    const auto& request = debug.noclip_velocity;
    if (!debug.noclip || !request.valid) return false;
    try {
        SourceReader reader;
        // Foreign native calls forward without altering the local flight session.
        if (reader.pointer(request.core, 0x438) != rig || reader.pointer(request.core, 0x3c0) != context) return false;
        if (GetTickCount64() >= request.expires) { debug_stop_noclip(debug); debug.status = "Flight input timed out."; return false; }
        const auto bodies = debug_noclip_bodies(state.trial.base, request.client, request.entity);
        source_require(bodies.core == request.core && bodies.rig_wrapper == rig && bodies.context == context,
            "Skater motion changed; flight stopped.");
        if (!bodies.offboard || bodies.seconds == 0) return false;
        source_require(reader.value<std::uint8_t>(request.entity, 0x7e0) == 0, "A teleport started; flight stopped.");
        std::array<float,16> previous{};
        source_require(reader.raw(reinterpret_cast<std::uintptr_t>(supplied), target.data(), sizeof(target)) &&
            reader.raw(rig + 0x4720, previous.data(), sizeof(previous)) &&
            valid_flight_transform(target) && valid_flight_transform(previous), "Invalid skater motion target.");
        // The new build moved the simulation counter to +180. +170 now holds
        // state flags and can stay constant, suppressing every subsequent move.
        (void)reader.value<std::uint32_t>(request.core, 0x180);
        // Lead the skater's current root by one step instead of integrating the
        // previous target. The native drive is a spring toward this target: a
        // free-running target stretched it without bound at high speed (the
        // skater outran the camera) and the ragdoll snapped back on release.
        // Anchoring to the root bounds the stretch to one step and leaves no
        // stored energy when the input stops.
        for (std::size_t i = 0; i < 3; ++i) {
            source_require(std::isfinite(request.velocity[i]) && std::abs(request.velocity[i]) <= 6000, "Invalid flight input.");
            source_require(std::isfinite(bodies.root[i]) && std::abs(bodies.root[i]) <= 1000000, "Invalid skater root position.");
            target[12+i] = bodies.root[i] + request.velocity[i] * bodies.seconds;
        }
        source_require(valid_flight_transform(target), "Flight motion target is out of bounds.");
        reader.verify();
        source_require(sync_offboard_flight_velocity(state.trial.base, bodies.core, context, rig, request.velocity),
            "Off-board flight velocity ownership changed; flight stopped.");
        ++debug.noclip_motion_updates;
        return true;
    } catch (const SourceGuard& issue) { debug_stop_noclip(debug); debug.status = issue.message; }
      catch (...) { debug_stop_noclip(debug); debug.status = "Flight stopped after a motion error."; }
    return false;
}
void noclip_ground_target(std::uintptr_t rig,float seconds) {
    const auto original=source_state().ground_target_original.load(std::memory_order_acquire);
    if(!original)return;
    original(rig,seconds);
    SourceLastError error;
    if(!wallplant_hold_core.load(std::memory_order_acquire) || competition_stock_physics())return;
    const auto held=arcade_mode::wallplant_runtime::held_command();
    if(!held || held.native.owner.rig!=rig)return;
    std::uint64_t now=GetTickCount64();
    const auto frame=arcade_mode::native_wallplant_hold_context(held.native.owner,now);
    auto& state=source_state();
    if(!state.initialized.load(std::memory_order_acquire) ||
       !state.velocity_guard_active.load(std::memory_order_acquire) ||
       state.busy.test_and_set(std::memory_order_acquire))return;
    SourceBusyScope scope{state.busy};
    try {
        SourceReader reader;
        source_require(reader.pointer(rig)==held.native.owner.context,"ground_target_context");
        const auto selected=reader.value<std::uint32_t>(held.native.owner.context,
            game::build::v20260929::arcade_native_probe::selected_state);
        if(selected!=100)return;
        constexpr auto offset=spawn::rig_local_motion_target;
        static_assert(spawn::rig_world_motion_target==offset+64 &&
            spawn::rig_inverse_motion_target==offset+128);
        std::array<float,48> supplied{},replacement{},check{};
        source_require(reader.raw(rig+offset,supplied.data(),sizeof(supplied)),"ground_target_read");
        const auto origin=reader.value<std::array<float,3>>(rig,spawn::rig_motion_origin);
        std::array<float,16> world{},target{};
        std::copy_n(supplied.begin()+16,16,world.begin());
        if(!arcade_wallplant_hold_target(rig,held.native.owner.context,&world,target,frame,now))return;
        const auto orientation=wallplant_orientation_snapshot(held.native.owner,reader);
        float target_limit{},angular_travel{};
        source_require(game::orientation_target::motion_budget(orientation.limit,seconds,
            orientation.root_spin,orientation.reference_spin,target_limit,angular_travel),"ground_target_motion_budget");
        game::orientation_target::Result turn;
        source_require(wallplant_ground_orientation(supplied,origin,orientation.local_root,
            orientation.actual,orientation.limit,target,turn,target_limit),"ground_target_orientation");
        source_require(wallplant_ground_target_matrices(supplied,origin,target,replacement),"ground_target_pair");
        source_require(source_writable(rig+offset,sizeof(replacement)),"ground_target_writable");
        reader.verify();
        source_require(reader.raw(rig+offset,check.data(),sizeof(check)) && check==supplied,
            "ground_target_changed");
        now=GetTickCount64();
        auto& hold=wallplant_physical_hold();
        bool stale_query{};
        const bool current=arcade_mode::wallplant_runtime::hold_current(held,now,frame,&stale_query);
        if(stale_query)return;
        source_require(hold.command.input_sequence==held.input_sequence && now>=hold.started &&
            current,"ground_target_expired");
        // Authority lookup can yield to the physics publisher. Revalidate the
        // physical basis, native limit and target origin at the write boundary.
        reader.verify();
        // Native preparation and its subsequent drive each run once. Replace
        // only their coherent target triplet; contact, body and state data stay
        // owned by the native solver. A failed copy withdraws this hold.
        if(!copy_to(rig+offset,replacement.data(),sizeof(replacement)) ||
           !reader.raw(rig+offset,check.data(),sizeof(check)) || check!=replacement) {
            source_require(copy_to(rig+offset,supplied.data(),sizeof(supplied)) &&
                reader.raw(rig+offset,check.data(),sizeof(check)) && check==supplied,
                "ground_target_restore_failed");
            source_require(false,"ground_target_write");
        }
        hold.orientation=turn;hold.orientation_limit=orientation.limit;
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE WALLPLANT ORIENTATION: occurrence={} ms={} native_limit={} requested_error={} applied_error={} fraction={} target_forward={},{},{} physical_forward={},{},{} target_limit={} angular_travel={} physics_seconds={} root_spin={},{},{} reference_spin={},{},{}.",
            held.input_sequence,now,orientation.limit,turn.requested_error,turn.applied_error,turn.fraction,
            target[8],target[9],target[10],orientation.actual[8],orientation.actual[9],orientation.actual[10],
            target_limit,angular_travel,seconds,orientation.root_spin[0],orientation.root_spin[1],orientation.root_spin[2],
            orientation.reference_spin[0],orientation.reference_spin[1],orientation.reference_spin[2]);
    } catch(const SourceGuard& issue) {wallplant_hold_cancel(issue.message,now);}
      catch(...) {wallplant_hold_cancel("ground_target_exception",now);}
}
void wallplant_prepare_board_drive(std::uintptr_t rig,std::uintptr_t caller) noexcept {
    SourceLastError error;
    if(!wallplant_hold_core.load(std::memory_order_acquire) || competition_stock_physics())return;
    const auto held=arcade_mode::wallplant_runtime::held_command();
    if(!held || held.native.owner.rig!=rig ||
       caller!=held.native.owner.base+spawn::ground_drive_return)return;
    auto now=GetTickCount64();
    const auto frame=arcade_mode::native_wallplant_hold_context(held.native.owner,now);
    auto& state=source_state();
    if(!state.initialized.load(std::memory_order_acquire) ||
       !state.velocity_guard_active.load(std::memory_order_acquire) ||
       state.busy.test_and_set(std::memory_order_acquire))return;
    SourceBusyScope scope{state.busy};
    try {
        const auto& hold=wallplant_physical_hold();
        source_require(hold.command.input_sequence==held.input_sequence && hold.targets &&
            held.anchor_verified,"board_drive_hold");
        SourceReader reader;
        source_require(reader.pointer(rig)==held.native.owner.context,"board_drive_context");
        source_require(reader.value<std::uint32_t>(held.native.owner.context,
            game::build::v20260929::arcade_native_probe::selected_state)==100,"board_drive_state");
        const auto orientation=wallplant_orientation_snapshot(held.native.owner,reader);
        const auto matrix=[&](std::uintptr_t base,std::uintptr_t offset) {
            std::array<float,16> value{};
            for(unsigned row=0;row<4;++row) {
                const auto lanes=reader.value<std::array<float,4>>(base,offset+row*16);
                std::copy(lanes.begin(),lanes.end(),value.begin()+row*4);
            }
            return value;
        };
        const auto local=matrix(rig,spawn::rig_local_motion_target);
        const auto origin=reader.value<std::array<float,3>>(rig,spawn::rig_motion_origin);
        const auto world=matrix(rig,spawn::rig_world_motion_target);
        const auto reference=matrix(held.native.owner.context,spawn::context_motion_reference);
        const auto supplied=matrix(rig,spawn::rig_board_drive_target);
        for(unsigned axis=0;axis<3;++axis)
            source_require(std::abs(world[12+axis]-held.anchor[axis])<=.001f,"board_drive_anchor");
        std::array<float,16> replacement{},check{};
        source_require(wallplant_ground_drive_target(orientation.local_root,local,origin,
            orientation.actual,reference,supplied,orientation.limit,replacement),"board_drive_preparation");
        arcade_mode::AirMotionIntent intent{};
        wallplant_read_current_intent(intent,now,hold.intent_generation);
        bool stale_query{};
        if(!arcade_mode::wallplant_runtime::hold_current(held,now,frame,&stale_query)) {
            if(stale_query)return;
            source_require(false,"board_drive_expired");
        }
        source_require(source_writable(rig+spawn::rig_board_drive_target,sizeof(replacement)),"board_drive_writable");
        reader.verify();
        if(!copy_to(rig+spawn::rig_board_drive_target,replacement.data(),sizeof(replacement)) ||
           !reader.raw(rig+spawn::rig_board_drive_target,check.data(),sizeof(check)) || check!=replacement) {
            source_require(copy_to(rig+spawn::rig_board_drive_target,supplied.data(),sizeof(supplied)) &&
                reader.raw(rig+spawn::rig_board_drive_target,check.data(),sizeof(check)) && check==supplied,
                "board_drive_restore_failed");
            source_require(false,"board_drive_write");
        }
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE WALLPLANT BOARD TARGET: occurrence={} contact={} ms={} state=100 from={},{},{} target={},{},{} forward={},{},{} physical_pose_write=false.",
            held.input_sequence,held.contact,now,supplied[12],supplied[13],supplied[14],
            replacement[12],replacement[13],replacement[14],replacement[8],replacement[9],replacement[10]);
    } catch(const SourceGuard& issue) {wallplant_hold_cancel(issue.message,now);}
      catch(...) {wallplant_hold_cancel("board_drive_exception",now);}
}
bool noclip_rig_drive(std::uintptr_t rig) {
    const auto original=source_state().rig_drive_original.load(std::memory_order_acquire);
#if defined(_MSC_VER)
    const auto caller=reinterpret_cast<std::uintptr_t>(_ReturnAddress());
#else
    const auto caller=reinterpret_cast<std::uintptr_t>(__builtin_return_address(0));
#endif
    wallplant_prepare_board_drive(rig,caller);
    return original?original(rig):false;
}
void noclip_skater_motion(std::uintptr_t rig, std::uintptr_t context,
    const std::array<float,16>* supplied, std::uint8_t flags) {
    const auto original = source_state().motion_original.load(std::memory_order_acquire);
    alignas(16) std::array<float,16> target{};
    const auto* motion = noclip_motion_target(rig, context, supplied, target) ? &target : supplied;
    if (original) original(rig, context, motion, flags);
    if (source_state().offboard_boost_core.load(std::memory_order_acquire)) {
        std::uintptr_t core{};
        if (memory::peek(rig + 0x4630, core) &&
            core == source_state().offboard_boost_core.load(std::memory_order_acquire))
            noclip_apply_velocity(core);
    }
}
}
}

namespace dingosdk {
using namespace client_source::detail;
void observe_arcade_wallplant_native_response(std::uintptr_t rig,bool wipeout,
        std::uint64_t callback,unsigned stage) noexcept {
    if(stage>2)return;
    const auto phase=stage==0?"before_listeners":stage==1?"before_skeleton":"after_skeleton";
    if(!wallplant_hold_core.load(std::memory_order_acquire) &&
       !wallplant_observed_release_core.load(std::memory_order_acquire))return;
    SourceLastError error;
    auto& state=source_state();
    if(state.busy.test_and_set(std::memory_order_acquire))return;
    SourceBusyScope scope{state.busy};
    auto& observation=wallplant_release_observation();
    const bool released=!wallplant_physical_hold().command;
    const auto& hold=released?observation.release:wallplant_physical_hold();
    if(!hold.command || hold.owner.rig!=rig)return;
    const auto now=GetTickCount64();
    const auto grace=static_cast<std::uint64_t>(std::ceil(hold.command.native.physics_seconds*1000.0));
    if(released) {
        if(!observation.remaining || now<observation.at ||
           now-observation.at>arcade_mode::WallplantHoldContinuity::duration_ms) {
            observation={};wallplant_observed_release_core.store(0,std::memory_order_release);return;
        }
        if(stage==2)--observation.remaining;
    } else if(now<hold.started || now-hold.started>arcade_mode::WallplantHoldContinuity::duration_ms+grace)return;
    try {
        NoBailSkater owner{};
        if(!no_bail_skater(owner) || owner!=hold.owner)return;
        SourceReader reader;
        const arcade_mode::NativeInputOwner input_owner{owner.base,owner.entity,owner.core,owner.context,owner.rig};
        const auto orientation=wallplant_orientation_snapshot(input_owner,reader);
        std::array<float,16> reference{};
        for(unsigned row=0;row<4;++row) {
            const auto lanes=reader.value<std::array<float,4>>(owner.context,spawn::context_motion_reference+row*16);
            std::copy(lanes.begin(),lanes.end(),reference.begin()+row*4);
        }
        namespace native=addr::skater_state;
        source_require(reader.pointer(orientation.root_body,native::body_owner_offset)==
            reader.pointer(orientation.reference_body,native::body_owner_offset),"hold_observer_body_owner");
        const auto velocity=reader.value<std::array<float,3>>(orientation.reference_body,native::body_velocity_offset);
        const auto spin0=reader.value<std::array<float,3>>(orientation.root_body,native::body_spin_offset);
        const auto spin8=reader.value<std::array<float,3>>(orientation.reference_body,native::body_spin_offset);
        const auto flags=reader.value<std::uint32_t>(orientation.reference_body,native::body_pending_flags_offset);
        const auto selected=reader.value<std::uint32_t>(owner.context,native::physics_state_offset);
        const auto root_velocity=reader.value<std::array<float,3>>(orientation.root_body,native::body_velocity_offset);
        const auto root_position=reader.value<std::array<float,3>>(orientation.root_body,addr::skater_body::body_transform_offset+48);
        const auto board_position=reader.value<std::array<float,3>>(orientation.reference_body,addr::skater_body::body_transform_offset+48);
        const auto target_position=reader.value<std::array<float,3>>(owner.rig,spawn::rig_world_motion_target+48);
        const auto reference_position=reader.value<std::array<float,3>>(owner.context,spawn::context_motion_reference+48);
        // The prefix was checked before installing the drive trampoline.
        source_require(state.rig_drive_original.load(std::memory_order_acquire)!=nullptr,
            "hold_observer_drive_contract");
        // Read XYZ separately. SIMD padding and an uninitialised drive record
        // must not discard the independently valid body/reference observation.
        const auto drive_velocity=reader.value<std::array<float,3>>(owner.rig,spawn::rig_target_velocity);
        const auto previous_target=reader.value<std::array<float,3>>(owner.rig,spawn::rig_previous_target_position);
        const auto finite_vector=[](const auto& vector) {
            return std::all_of(vector.begin(),vector.end(),[](float value){return std::isfinite(value);});
        };
        const bool drive_valid=finite_vector(drive_velocity) && finite_vector(previous_target);
        source_require(game::orientation_target::basis_valid(reference) &&
            game::orientation_target::basis_valid(orientation.actual),"hold_observer_basis");
        for(const auto& vector:{velocity,spin0,spin8})for(float v:vector)
            source_require(std::isfinite(v),"hold_observer_motion");
        reader.verify();NoBailSkater after{};
        if(!no_bail_skater(after) || after!=owner)return;
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE WALLPLANT DRIVE: occurrence={} contact={} ms={} phase={} stage={} callback={} state={} root={},{},{} board={},{},{} velocity0={},{},{} velocity8={},{},{} target={},{},{} reference={},{},{} derived_valid={} derived_velocity={},{},{} previous_target={},{},{} read_only=true atomic_snapshot=false.",
            hold.command.input_sequence,hold.command.contact,now,released?"released":"held",phase,callback,selected,
            root_position[0],root_position[1],root_position[2],board_position[0],board_position[1],board_position[2],
            root_velocity[0],root_velocity[1],root_velocity[2],velocity[0],velocity[1],velocity[2],
            target_position[0],target_position[1],target_position[2],reference_position[0],reference_position[1],reference_position[2],
            drive_valid,drive_velocity[0],drive_velocity[1],drive_velocity[2],previous_target[0],previous_target[1],previous_target[2]);
        const auto& a=orientation.actual;const auto& r=reference;
        // This is an original native callback observation, not the earlier
        // pending-write readback and not a hook of the bail predicate itself.
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE WALLPLANT NATIVE: occurrence={} contact={} ms={} elapsed={} phase={} callback={} state={} core={} rig={} context={} original_wipeout={} limit={} observed_error={} last_target_error={} flags={} velocity8={},{},{} spin0={},{},{} spin8={},{},{} actual={},{},{},{},{},{},{},{},{} reference={},{},{},{},{},{},{},{},{}.",
            hold.command.input_sequence,hold.command.contact,now,now-hold.started,phase,callback,selected,owner.core,owner.rig,owner.context,
            wipeout,orientation.limit,game::orientation_target::error(a,r),hold.orientation.applied_error,flags,
            velocity[0],velocity[1],velocity[2],spin0[0],spin0[1],spin0[2],spin8[0],spin8[1],spin8[2],
            a[0],a[1],a[2],a[4],a[5],a[6],a[8],a[9],a[10],r[0],r[1],r[2],r[4],r[5],r[6],r[8],r[9],r[10]);
    } catch(...) {}
}
std::uint64_t apply_arcade_wallplant_before_selection(std::uintptr_t selector,std::uint32_t current) noexcept {
    return arcade_wallplant_before_selection(selector,current);
}
void finish_arcade_wallplant_selection(std::uint64_t ticket,std::uintptr_t selector,
        std::uint32_t current,std::uint32_t chosen) noexcept {
    wallplant_selection_phase(selector,chosen,1,"after_selection");
    if(ticket)wallplant_handoff().ticket.finish(ticket,selector,current,chosen);
}

bool queue_skater_push_velocity(std::uintptr_t client,std::uintptr_t entity,const std::array<float,3>& velocity) noexcept {
    try {
        auto& state=source_state();auto& request=push_velocity();
        if(!competition_stock_physics() || !start_client_noclip_velocity(state.trial.base) ||
            !skater_push::valid(velocity))return false;
        if(state.busy.test_and_set(std::memory_order_acquire))return false;
        SourceBusyScope scope{state.busy};
        if(request.pending.load(std::memory_order_acquire)) {
            if(skater_push::holding() && velocity==std::array<float,3>{} &&
               request.velocity==velocity && request.client==client && request.entity==entity) {
                request.expires=GetTickCount64()+500;return true;
            }
            if(GetTickCount64()<request.expires)return false;
            request.pending.store(false,std::memory_order_release);
        }
        const auto bodies=debug_noclip_bodies(state.trial.base,client,entity);if(bodies.offboard)return false;
        request.client=client;request.entity=entity;request.core=bodies.core;request.velocity=velocity;request.expires=GetTickCount64()+500;
        request.pending.store(true,std::memory_order_release);return true;
    }catch(...){return false;}
}
void set_physics_step_callback(void (*callback)() noexcept) noexcept { physics_step_callback.store(callback, std::memory_order_release); }
bool queue_jump_scale(std::uintptr_t client, std::uintptr_t entity, float factor) noexcept {
    SourceLastError error;
    try {
        auto& state = source_state();
        auto& j = jump_scale();
        if (!state.initialized.load(std::memory_order_acquire) || !state.velocity_guard_active.load(std::memory_order_acquire) ||
            !std::isfinite(factor) || factor <= 0 || factor > 20 || j.pending.load(std::memory_order_acquire)) return false;
        const auto bodies = debug_noclip_bodies(state.trial.base, client, entity);
        j.client = client;
        j.entity = entity;
        j.core = bodies.core;
        j.factor = factor;
        j.expires = GetTickCount64() + 150;
        j.outcome.store(0);
        j.pending.store(true, std::memory_order_release);
        return true;
    } catch (...) { return false; }
}
void set_plugin_velocity(const std::array<float, 3>& velocity, bool add) noexcept {
    auto& v = client_source::detail::plugin_velocity();
    std::lock_guard lock(v.mutex);
    v.velocity = velocity;
    v.add = add;
    v.expires = GetTickCount64() + 500;
    v.pending.store(true, std::memory_order_release);
}
void set_plugin_cruise(float speed, bool may_slow) noexcept {
    auto& p = push_speed();
    p.plugin.store(speed > 0 && speed <= 30 ? speed : 0.0f, std::memory_order_relaxed);
    p.plugin_slow.store(may_slow, std::memory_order_relaxed);
    p.plugin_expires.store(speed > 0 ? GetTickCount64() + 300 : 0, std::memory_order_release);
}
void set_push_speed(std::uintptr_t client, std::uintptr_t entity, float factor, float stock, float cruise) noexcept {
    auto& p = push_speed();
    p.client = client;
    p.entity = entity;
    p.cruise.store(cruise > 0 && cruise < 100 ? cruise : 0.0f, std::memory_order_relaxed);
    p.stock.store(stock > 1 && stock < 100 ? stock : 9.25f, std::memory_order_relaxed);
    p.factor.store(factor > 0.02f && factor <= 1.0e6f ? factor : 1.0f, std::memory_order_relaxed);
    p.expires.store(GetTickCount64() + 500, std::memory_order_release);
}
void set_arcade_push_intent(bool active, bool crouched) noexcept {
    auto& p = push_speed();
    const auto now = GetTickCount64(), previous = p.arcade_expires.load(std::memory_order_relaxed);
    if ((!active && previous) || (active && previous < now)) p.arcade_epoch.fetch_add(1, std::memory_order_relaxed);
    p.arcade_crouched.store(crouched, std::memory_order_relaxed);
    p.arcade_expires.store(active ? now + 150 : 0, std::memory_order_release);
}
void set_arcade_air_intent(bool active, std::uint64_t release_sequence, std::uint64_t release_at,
    unsigned charge_ms, float yaw_rate, bool angular_allowed,
    const arcade_mode::NativeManualContact* manual, const arcade_mode::NativeInputFrame* angular_proof,
    const arcade_mode::GroundControl* ground_controls, const arcade_mode::SpinCommand* spin_command,
    const arcade_mode::LaunchDescriptor* launch, const arcade_mode::GroundTurnCommand* ground_turn,
    std::int32_t native_launch_id, std::int32_t native_followup_id) noexcept {
    auto& a = arcade_air();
    if (!active) a.ready_until.store(0, std::memory_order_release);
    a.version.fetch_add(1, std::memory_order_acq_rel);
    if (a.enabled.load(std::memory_order_relaxed) != active) a.epoch.fetch_add(1, std::memory_order_relaxed);
    a.enabled.store(active, std::memory_order_relaxed);
    a.sampled.store(GetTickCount64(), std::memory_order_relaxed);
    a.release_sequence.store(release_sequence, std::memory_order_relaxed);
    a.release_at.store(release_at, std::memory_order_relaxed);
    a.charge.store(std::min(charge_ms, 200u), std::memory_order_relaxed);
    a.native_launch_id.store(active && release_sequence && (native_launch_id==1 || native_launch_id==16) ? native_launch_id : 0,std::memory_order_relaxed);
    a.native_followup_id.store(active && arcade_mode::ordinary_ollie_followup(native_followup_id) ? native_followup_id : 0,std::memory_order_relaxed);
    a.launch.store(active && launch ? *launch : arcade_mode::LaunchDescriptor{});
    const auto turn=active&&angular_allowed&&ground_turn ? *ground_turn : arcade_mode::GroundTurnCommand{};
    a.turn_serial.store(turn.serial,std::memory_order_relaxed);a.turn_at.store(turn.requested_at,std::memory_order_relaxed);
    a.turn_step.store(turn.physics_step,std::memory_order_relaxed);a.turn_sampled.store(turn.sampled_at,std::memory_order_relaxed);
    a.turn_generations[0].store(turn.generations.owner,std::memory_order_relaxed);
    a.turn_generations[1].store(turn.generations.contact,std::memory_order_relaxed);
    a.turn_generations[2].store(turn.generations.action,std::memory_order_relaxed);
    a.yaw_rate.store(yaw_rate, std::memory_order_relaxed);
    a.angular_allowed.store(angular_allowed, std::memory_order_relaxed);
    a.ground_controls.store(active && ground_controls ? ground_controls->encode() : 0,std::memory_order_relaxed);
    const auto command=active && spin_command ? *spin_command : arcade_mode::SpinCommand{};
    a.spin_mode.store(static_cast<unsigned>(command.mode),std::memory_order_relaxed);
    a.spin_epoch.store(command.epoch,std::memory_order_relaxed);a.spin_serial.store(command.serial,std::memory_order_relaxed);
    a.spin_balance.store(command.balance,std::memory_order_relaxed);a.spin_facts.store(command.facts.encode()|(command.physical_armed?0x100u:0u),std::memory_order_relaxed);
    a.left_pressed_at.store(command.left_pressed_at,std::memory_order_relaxed);a.right_pressed_at.store(command.right_pressed_at,std::memory_order_relaxed);
    auto spin = angular_proof ? arcade_mode::NativeAirSpinProof::from_frame(*angular_proof) : arcade_mode::NativeAirSpinProof{};
    spin.valid = active && spin.valid;
    a.spin_proof.store(spin);
    for (std::size_t i=0;i<a.manual_owner.size();++i)
        a.manual_owner[i].store(manual?manual->owner[i]:0,std::memory_order_relaxed);
    a.manual_sampled.store(manual?manual->sampled_at:0,std::memory_order_relaxed);
    a.manual_contacts.store(manual?manual->contacts:0,std::memory_order_relaxed);
    a.manual_admitted.store(active&&manual&&manual->admitted,std::memory_order_relaxed);
    a.manual_generations[0].store(manual?manual->owner_generation:0,std::memory_order_relaxed);
    a.manual_generations[1].store(manual?manual->contact_generation:0,std::memory_order_relaxed);
    a.manual_generations[2].store(manual?manual->action_generation:0,std::memory_order_relaxed);
    a.version.fetch_add(1, std::memory_order_release);
}
bool arcade_air_motion_ready(const arcade_mode::NativeInputFrame* current) noexcept {
    if (!current) return false;
    auto& a = arcade_air();const auto before=a.ready_version.load(std::memory_order_acquire);
    if(before & 1u)return false;
    const auto now=GetTickCount64(),until=a.ready_until.load(std::memory_order_acquire);
    const auto proof=a.ready_proof.load();
    return before==a.ready_version.load(std::memory_order_acquire) && until && until>=now && until-now<=150 &&
        proof.matches(arcade_mode::NativeAirSpinProof::from_frame(*current),now);
}
void set_air_gravity(std::uintptr_t client, std::uintptr_t entity, float factor, float landing, float timing) noexcept {
    auto& g = air_gravity();
    g.landing.store(std::isfinite(landing) && landing >= 0.02f && landing <= 20.0f ? landing : 0.0f, std::memory_order_relaxed);
    g.timing.store(std::isfinite(timing) ? std::clamp(timing, 0.0f, 1.0f) : 0.0f, std::memory_order_relaxed);
    g.client.store(client, std::memory_order_relaxed);
    g.entity.store(entity, std::memory_order_relaxed);
    g.factor.store(std::isfinite(factor) && factor >= 0.02f && factor <= 20.0f ? factor : 1.0f, std::memory_order_relaxed);
    g.expires.store(GetTickCount64() + 500, std::memory_order_release);
}
float air_animation_rate() noexcept {
    auto& g = air_gravity();
    if (!(g.timing.load(std::memory_order_relaxed) > 0) || GetTickCount64() >= g.expires.load(std::memory_order_acquire)) return 1.0f;
    return g.animation_rate.load(std::memory_order_relaxed);
}
std::uint32_t air_gravity_steps() noexcept { return air_gravity().steps.load(std::memory_order_relaxed); }
void note_flight_record(std::uintptr_t trajectory) noexcept {
    auto& g = air_gravity();
    for (const auto& slot : g.records)
        if (slot.load(std::memory_order_relaxed) == trajectory) return;
    g.records[g.noted.fetch_add(1, std::memory_order_relaxed) % g.records.size()].store(trajectory, std::memory_order_relaxed);
}
void set_pump_power(std::uintptr_t client, std::uintptr_t entity, float factor, bool pumping, bool measure) noexcept {
    auto& p = pump_power();
    p.client = client;
    p.entity = entity;
    p.factor.store(factor >= 0 && factor <= 1.0e6f ? factor : 1.0f, std::memory_order_relaxed);
    p.pumping.store(pumping, std::memory_order_relaxed);
    p.measure.store(measure, std::memory_order_relaxed);
    p.expires.store(GetTickCount64() + 500, std::memory_order_release);
}
PumpPowerReport pump_power_report() noexcept {
    auto& p = pump_power();
    return {p.pumps.load(std::memory_order_relaxed), p.gained.load(std::memory_order_relaxed), p.given.load(std::memory_order_relaxed),
            p.speed.load(std::memory_order_relaxed), p.steps.load(std::memory_order_relaxed)};
}
void set_revert_boost(std::uintptr_t client, std::uintptr_t entity, float strength, const RevertTuning &tuning) noexcept {
    auto& r = revert_boost();
    AcquireSRWLockExclusive(&r.report_lock);
    r.tuning = tuning;
    ReleaseSRWLockExclusive(&r.report_lock);
    r.client = client;
    r.entity = entity;
    r.strength.store(std::isfinite(strength) && strength > 0 ? std::min(strength, 1000.0f) : 0.0f, std::memory_order_relaxed);
    r.expires.store(GetTickCount64() + 500, std::memory_order_release);
}
void set_transition_assist(std::uintptr_t client, std::uintptr_t entity, const TransitionAssist &assist) noexcept {
    auto& r = transition_run();
    r.client = client;
    r.entity = entity;
    AcquireSRWLockExclusive(&r.lock);
    r.assist = assist;
    ReleaseSRWLockExclusive(&r.lock);
    r.expires.store(GetTickCount64() + 500, std::memory_order_release);
}
TransitionReport transition_report() noexcept {
    auto& r = transition_run();
    AcquireSRWLockShared(&r.lock);
    const auto report = r.report;
    ReleaseSRWLockShared(&r.lock);
    return report;
}
RevertBoostReport revert_boost_report() noexcept {
    auto& r = revert_boost();
    AcquireSRWLockShared(&r.report_lock);
    const auto report = r.report;
    ReleaseSRWLockShared(&r.report_lock);
    return report;
}
JumpScaleResult take_jump_scale_result() noexcept {
    auto& j = jump_scale();
    if (j.pending.load(std::memory_order_acquire)) return {};
    return {j.outcome.exchange(0), j.up_speed.load()};
}

bool start_client_noclip_velocity(std::uintptr_t base) noexcept {
    SourceLastError error;
    try {
        auto& state = source_state();
        std::lock_guard lock(state.initialization_mutex);
        if (!state.initialized.load() || state.trial.base != base) return false;
        if (state.velocity_guard_attempted) return state.velocity_guard_active.load();
        state.velocity_guard_attempted = true;
        SourceReader reader;
        std::array<unsigned char, 32> bytes{};
        if (!reader.raw(base + spawn::physics_update, bytes.data(), bytes.size()) || bytes != spawn::physics_update_prefix ||
            reader.pointer(base + addr::no_bail::bail_core_vtable, 0x58) != base + spawn::physics_update ||
            !reader.raw(base + spawn::skater_motion, bytes.data(), bytes.size()) || bytes != spawn::skater_motion_prefix ||
            !reader.raw(base + spawn::ground_motion_target, bytes.data(), bytes.size()) || bytes != spawn::ground_motion_target_prefix ||
            !reader.raw(base + spawn::rig_drive_contract.rva, bytes.data(), bytes.size()) || bytes != spawn::rig_drive_contract.bytes ||
            !offboard_flight_compatible(base)) return false;
        for(const auto& contract:spawn::board_drive_contracts)
            if(!reader.raw(base+contract.rva,bytes.data(),bytes.size()) || bytes!=contract.bytes)return false;
        reader.verify();
        auto* target = reinterpret_cast<void*>(base + spawn::physics_update);
        auto* motion_target = reinterpret_cast<void*>(base + spawn::skater_motion);
        auto* ground_target = reinterpret_cast<void*>(base + spawn::ground_motion_target);
        auto* drive_target = reinterpret_cast<void*>(base + spawn::rig_drive_contract.rva);
        HookBatch batch;
        void* original{};
        if (hook_prepare(target, reinterpret_cast<void*>(&noclip_physics_update), &original) != HookOk) return false;
        if (!original) { (void)hook_remove(target); return false; }
        state.velocity_update_original.store(reinterpret_cast<SourcePhysicsUpdate>(original), std::memory_order_release);
        original = nullptr;
        if (hook_prepare(motion_target, reinterpret_cast<void*>(&noclip_skater_motion), &original) != HookOk) {
            (void)hook_remove(target); state.velocity_update_original.store(nullptr); return false;
        }
        if (!original) {
            (void)hook_remove(motion_target); (void)hook_remove(target);
            state.velocity_update_original.store(nullptr); return false;
        }
        state.motion_original.store(reinterpret_cast<SourceSkaterMotion>(original), std::memory_order_release);
        original=nullptr;
        if(hook_prepare(ground_target,reinterpret_cast<void*>(&noclip_ground_target),&original)!=HookOk || !original) {
            (void)hook_remove(ground_target);(void)hook_remove(motion_target);(void)hook_remove(target);
            state.motion_original.store(nullptr);state.velocity_update_original.store(nullptr);return false;
        }
        state.ground_target_original.store(reinterpret_cast<SourceGroundTarget>(original),std::memory_order_release);
        original=nullptr;
        if(hook_prepare(drive_target,reinterpret_cast<void*>(&noclip_rig_drive),&original)!=HookOk || !original) {
            (void)hook_remove(drive_target);(void)hook_remove(ground_target);
            (void)hook_remove(motion_target);(void)hook_remove(target);
            state.ground_target_original.store(nullptr);state.motion_original.store(nullptr);
            state.velocity_update_original.store(nullptr);return false;
        }
        state.rig_drive_original.store(reinterpret_cast<SourceRigDrive>(original),std::memory_order_release);
        // Keep trampolines on uncertain enable results; handlers still forward.
        if (hook_queue_enable(target) != HookOk || hook_queue_enable(motion_target) != HookOk ||
            hook_queue_enable(ground_target)!=HookOk || hook_queue_enable(drive_target)!=HookOk ||
            hook_apply_queued()!=HookOk) return false;
        state.velocity_guard_active.store(true, std::memory_order_release);
        trainer::set_native_world_trajectory_bridge(&wallplant_native_departure);
        return true;
    } catch (...) { return false; }
}
}

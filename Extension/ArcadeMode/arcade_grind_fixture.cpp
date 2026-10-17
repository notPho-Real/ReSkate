#include "Extension/Console/commands.h"
#include "Extension/ArcadeMode/arcade_mode.h"
#include "Extension/ArcadeMode/arcade_fixture_geometry.h"
#include "Extension/ArcadeMode/arcade_input_telemetry.h"
#include "Extension/Skater/client_source_spawn.h"
#include "Extension/Skater/no_bail.h"
#include "Extension/Objects/ParkEditor/park_editor_runtime.h"
#include "Extension/Profile/local_profile_runtime.h"
#include "Extension/Trainer/trainer.h"
#include "Extension/Multiplayer/Hud/game_ui_state.h"
#include "Extension/HallOfMeat/hall_of_meat_skater.h"
#include "Extension/Settings/named_settings.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Game/Build/20260929/arcade_native_probe.h"
#include "Engine/Game/Multiplayer/session_tools.h"
#include "Engine/Game/Settings/simulation_time.h"
#include <algorithm>
#include <cmath>
#include <format>
#include <Windows.h>

namespace dingosdk::console {
namespace {
// Standard round steel rail from the pinned 25414733 authored catalog.
// The shorter key in the category unit-test fixture is not a live asset.
constexpr auto rail_item = "own_bkrails_generic_flatrailround_00002";
// One added object, retained by the existing Park Editor document and save path.
// No layout clearing, physics tuning, collision overrides or direct skater writes.
// Explicit rate operations separately reuse the existing clock/cached-step APIs.
struct Fixture {
    bool configured{};
    std::string map, saved_project;
    profile::PlacedObject rail;
    std::array<float, 3> start{};
} fixture;
bool same_rail(const EditorObject& object) {
    return object.item == fixture.rail.item && object.position == fixture.rail.position &&
        object.rotation == fixture.rail.rotation && object.scale == fixture.rail.scale;
}
bool flat_start(const std::array<float, 3>& at, float& ground, bool entire_lane) {
    const auto center = local_ground_height(at[0], at[2], at[1] + .25f, at[1] - 3);
    if (!center) return false;
    ground = *center;
    // Near-ground rays intentionally exclude roofs/overpasses. cat pouch
    for (const float x : {-1.5f, 0.0f, 1.5f}) {
        for (const float z : {-1.0f, 3.0f, 8.0f, 13.0f}) {
            if (!entire_lane && z != -1.0f) continue;
            const auto hit = local_ground_height(at[0] + x, at[2] + z, *center + .25f, *center - .25f);
            if (!hit || std::abs(*hit - *center) > .08f) return false;
        }
    }
    return true;
}
arcade_mode::FixtureGeometry geometry;
arcade_mode::FixtureGeometryContext geometry_captured{};
profile::PlacedObject geometry_rail;
std::array<float, 3> geometry_skater{};
std::uint64_t geometry_tick{}, geometry_session{}, geometry_started{};
bool geometry_gameplay{};
arcade_mode::FixtureTrajectory trajectory;
std::uint64_t trajectory_gameplay_seen{};
arcade_mode::TrajectoryContext trajectory_context(const ParkEditorModel& editor, const trainer::Telemetry& skater) {
    NoBailSkater owner{};
    const auto watch = watched_physics_state();
    const auto found = std::find_if(editor.objects.begin(), editor.objects.end(), same_rail);
    arcade_mode::TrajectoryContext c;
    c.valid = fixture.configured && !fixture.saved_project.empty() && fixture.map == editor.map &&
        editor.available && !editor.busy && !editor.failed && !editor.preview_token &&
        std::count_if(editor.objects.begin(), editor.objects.end(), same_rail) == 1 &&
        found != editor.objects.end() && found->spawned && !client_free_camera_active() &&
        // Trainer publishes later in the client tick. Its retained state and
        // position carry their own timestamp; a new selector state must not
        // relabel that preceding sample or manufacture a discontinuity.
        arcade_mode::enabled() && skater.skater && watch.valid &&
        no_bail_skater(owner) && owner.entity == skater.sampled_entity;
    c.owner = {owner.base, owner.entity, owner.core, owner.context, owner.rig};
    if (c.valid) {
        const auto ui = multiplayer::sample_game_ui_state(owner.base);
        c.valid = ui.available && !ui.in_menu;
    }
    c.generation = editor.generation; c.revision = editor.revision;
    c.object = found == editor.objects.end() ? 0 : found->id;
    return c;
}
arcade_mode::TrajectorySample trajectory_sample(const trainer::Telemetry& skater) {
    return {skater.sampled_ms, skater.position, skater.speed, skater.heading, skater.physics_state,
        skater.skater && skater.sampled_entity != 0};
}
void trajectory_emit(const std::optional<arcade_mode::FixtureTrajectoryEvidence>& row) {
    if (row && !arcade_mode::enqueue_client_telemetry(*row)) trajectory.delivery_failed();
}
void trajectory_update(std::uint64_t now, bool gameplay) {
    if (gameplay) trajectory_gameplay_seen = now;
    if (!trajectory.active()) return;
    const auto skater = trainer::telemetry();
    trajectory_emit(trajectory.tick(trajectory_context(local_park_editor(), skater), trajectory_sample(skater), now, gameplay));
}
arcade_mode::FixtureGeometryContext geometry_context(const ParkEditorModel& editor, bool require_gameplay = true) {
    arcade_mode::FixtureGeometryContext context{};
    const auto skater = trainer::telemetry();
    const auto watch = watched_physics_state();
    NoBailSkater owner{};
    const auto found = std::find_if(editor.objects.begin(), editor.objects.end(), same_rail);
    context.valid = (!require_gameplay || geometry_gameplay) && !client_free_camera_active() && fixture.configured &&
        fixture.map == editor.map && editor.available && !editor.busy && !editor.failed && !editor.preview_token &&
        std::count_if(editor.objects.begin(), editor.objects.end(), same_rail) == 1 &&
        found != editor.objects.end() && found->spawned && skater.skater &&
        watch.valid && watch.state == 100 && skater.physics_state == 100 &&
        std::isfinite(skater.speed) && skater.speed <= .1f && no_bail_skater(owner);
    float distance{};
    for (unsigned i = 0; i < 3; ++i) {
        if (!std::isfinite(skater.position[i])) context.valid = false;
        distance += std::pow(skater.position[i] - geometry_skater[i], 2.f);
    }
    context.valid = context.valid && distance <= .01f;
    context.owner = {owner.base, owner.entity, owner.core, owner.context, owner.rig};
    context.generation = editor.generation; context.revision = editor.revision;
    context.object = found == editor.objects.end() ? 0 : found->id;
    context.physics_changes = watch.changes;
    return context;
}
void geometry_update(std::uint64_t now, bool gameplay) {
    const bool fresh = !geometry_tick || (now >= geometry_tick && now - geometry_tick <= 150);
    geometry_tick = now; geometry_gameplay = gameplay;
    if (geometry.status != arcade_mode::GeometryStatus::sampling && geometry.status != arcade_mode::GeometryStatus::armed) return;
    if (!fresh) { geometry.cancel(); return; }
    const auto editor = local_park_editor();
    if (geometry.status == arcade_mode::GeometryStatus::armed) {
        geometry.activate(geometry_context(editor, false), now, gameplay);
        if (geometry.status != arcade_mode::GeometryStatus::sampling) return;
    }
    const auto context = geometry_context(editor);
    if (!geometry.current(context, now)) return;
    if (const auto ray = geometry.next(context, now)) {
        const auto request_id = 0x4152430000000000ULL + geometry_session * 256 + ray->index;
        EditorSurfaceRequest request{context.generation, request_id,
            {fixture.rail.position[0] + ray->x, fixture.rail.position[1] + 2.f, fixture.rail.position[2] + ray->z},
            {0.f, -1.f, 0.f}, 0.f, true};
        const auto hit = read_local_park_surface(request, context.revision);
        const auto after = GetTickCount64();
        if (after < now || after - now > 150) { geometry.cancel(); return; }
        if (!geometry.current(geometry_context(local_park_editor()), after)) return;
        if (!hit || hit->id != request_id || hit->generation != context.generation) { geometry.unavailable(); return; }
        const float height = hit->position[1] - fixture.rail.position[1];
        // Only exact object hits in the near-rail height range can count.
        const bool bounded = !hit->hit || (std::isfinite(height) && height >= -.1f && height <= 2.f);
        geometry.accept(ray->index, hit->available && bounded, hit->hit, hit->object, height);
    }
}
Json geometry_status() {
    const auto now = GetTickCount64();
    Json result{{"status", static_cast<unsigned>(geometry.status)}, {"samples", geometry.count},
        {"budget", arcade_mode::FixtureGeometry::samples}, {"step", arcade_mode::FixtureGeometry::step},
        {"radius", arcade_mode::FixtureGeometry::radius}, {"contact_verified", false},
        {"session", geometry_session}, {"started_ms", geometry_started},
        {"generation", geometry_captured.generation}, {"revision", geometry_captured.revision},
        {"object", geometry_captured.object},
        {"context_current", geometry_tick && now >= geometry_tick && now - geometry_tick <= 150 &&
            geometry.result_current(geometry_context(local_park_editor(), false))}};
    if (geometry_session) {
        result["rail_position"] = geometry_rail.position;
        result["rail_rotation"] = geometry_rail.rotation; result["rail_scale"] = geometry_rail.scale;
    }
    if (geometry.status == arcade_mode::GeometryStatus::complete) {
        result["sampled_low"] = geometry.low; result["sampled_high"] = geometry.high;
        result["height_min"] = geometry.height_min; result["height_max"] = geometry.height_max;
        result["edge_uncertainty"] = arcade_mode::FixtureGeometry::step;
    }
    return result;
}
std::string fixture_rate(unsigned rate, const trainer::Telemetry& skater, bool editing) {
    namespace native = game::build::v20260929::arcade_native_probe;
    NoBailSkater owner{}, after{};
    std::array<std::uint32_t, 8> words{};
    std::uint32_t state{}, mode{};
    const auto now = GetTickCount64();
    if ((rate != 30 && rate != 60) || editing || multiplayer_session_active() ||
        !hall_of_meat::skater_available() || !no_bail_skater(owner) || !skater.skater ||
        skater.sampled_entity != owner.entity || !skater.sampled_ms || now < skater.sampled_ms ||
        now - skater.sampled_ms > 150 || !std::isfinite(skater.speed) || skater.speed > .1f ||
        !std::isfinite(skater.vertical) || std::abs(skater.vertical) > .1f ||
        !memory::peek(owner.context + native::selected_state, state) || state != 100 ||
        !memory::peek(owner.rig + native::crouch_mode, mode) || mode != 1 ||
        !memory::peek(owner.context + native::request_words, words) ||
        (words[1] & 0xc000u) != 0xc000u || (words[0] & 0x40u) ||
        !no_bail_skater(after) || owner != after)
        return "error: Rate setup requires the stopped local two-wheel skater, closed editor and no multiplayer session.";
    constexpr std::array names{"SimulationTime.ForceSimRate", "SimulationTime.MaxSimFps"};
    const std::array previous{named_setting_value(names[0]), named_setting_value(names[1])};
    const float old_step = hall_of_meat::step_length(), step = simulation_time::step_seconds(rate);
    if (!previous[0] || !previous[1] || !std::isfinite(old_step) || old_step <= 0 || old_step > .1f)
        return "error: Native rate ownership is unavailable.";
    const auto value = std::to_string(rate);
    auto restore = [&] {
        for (unsigned i = 0; i < names.size(); ++i)
            if (named_setting_value(names[i]) == value) (void)change_named_setting(names[i], *previous[i], false);
        if (no_bail_skater(after) && owner == after && hall_of_meat::step_length() == step)
            (void)hall_of_meat::set_step_length(old_step);
    };
    for (const auto name : names)
        if (change_named_setting(name, value, false).starts_with("error:")) {
            restore(); return "error: Native clock rate setup failed; owned values restored.";
        }
    if (!no_bail_skater(after) || owner != after || !hall_of_meat::set_step_length(step) ||
        hall_of_meat::step_length() != step || named_setting_value(names[0]) != value || named_setting_value(names[1]) != value) {
        restore(); return "error: Native skater rate setup failed; owned values restored.";
    }
    return std::format("ARCADE FIXTURE RATE: requested={} cached_rate={} cached_step_seconds={}.",
        rate, simulation_time::rate_of(hall_of_meat::step_length()), hall_of_meat::step_length());
}
}
void register_arcade_fixture_commands(Commands& registry) {
    auto operation = argument("status|place|relocate|turn|reset|save|adopt|geometry|geometry-status|geometry-cancel|trajectory|trajectory-status|trajectory-cancel|rate-30|rate-60", Type::text);
    operation.choices = {"status", "place", "relocate", "turn", "reset", "save", "adopt",
        "geometry", "geometry-status", "geometry-cancel", "trajectory", "trajectory-status", "trajectory-cancel", "rate-30", "rate-60"};
    auto entry = action("arcadefixture", "Single native flat-rail practice fixture", Group::objects, {operation});
    entry.run = [](const Model& model, const Values& values, const Output& out) {
        const auto& op = std::get<std::string>(values[0]);
        const auto editor = local_park_editor();
        const auto skater = trainer::telemetry();
        if (op == "rate-30" || op == "rate-60") {
            out(fixture_rate(op == "rate-30" ? 30u : 60u, skater, model.debug.park_editor)); return;
        }
        const auto count = std::count_if(editor.objects.begin(), editor.objects.end(), same_rail);
        const auto found = std::find_if(editor.objects.begin(), editor.objects.end(), same_rail);
        const bool present = fixture.configured && fixture.map == editor.map && count == 1 &&
            found != editor.objects.end() && found->spawned;
        const bool asset = editor.assets && std::any_of(editor.assets->begin(), editor.assets->end(),
            [](const auto& item) { return item.key == rail_item; });
        if (op == "trajectory" || op == "trajectory-status" || op == "trajectory-cancel") {
            const auto now = GetTickCount64();
            if (op == "trajectory") {
                if (model.debug.park_editor || !present || !asset || !trajectory_gameplay_seen ||
                    now < trajectory_gameplay_seen || now - trajectory_gameplay_seen > 5000 ||
                    geometry.status == arcade_mode::GeometryStatus::sampling || geometry.status == arcade_mode::GeometryStatus::armed ||
                    !trajectory.arm(trajectory_context(editor, skater), trajectory_sample(skater), fixture.start, now)) {
                    out("error: Trajectory requires the closed editor and a fresh stopped saved fixture baseline."); return;
                }
                out("Read-only trajectory armed; close console within five seconds. Capture ends after twenty-four seconds.");
            } else if (op == "trajectory-cancel") trajectory_emit(trajectory.cancel(now));
            out("ARCADE FIXTURE TRAJECTORY STATUS: " + Json{{"session", trajectory.session},
                {"status", static_cast<unsigned>(trajectory.status)}, {"samples", trajectory.count},
                {"dropped", trajectory.dropped}, {"duration_ms", arcade_mode::FixtureTrajectory::duration_ms},
                {"sample_limit", arcade_mode::FixtureTrajectory::sample_limit}}.dump());
            return;
        }
        if (op == "geometry-status" || op == "geometry-cancel") {
            if (op == "geometry-cancel") geometry.cancel();
            out("ARCADE FIXTURE GEOMETRY: " + geometry_status().dump()); return;
        }
        if (op == "geometry") {
            const auto now = GetTickCount64();
            float baseline_distance{};
            for (unsigned i = 0; i < 3; ++i) baseline_distance += std::pow(skater.position[i] - fixture.start[i], 2.f);
            if (trajectory.active() || model.debug.park_editor || !present || !geometry_tick || now < geometry_tick ||
                now - geometry_tick > 150 || geometry_session >= 0xffffff ||
                geometry.status == arcade_mode::GeometryStatus::sampling || geometry.status == arcade_mode::GeometryStatus::armed ||
                !std::isfinite(baseline_distance) ||
                baseline_distance > .25f || !std::isfinite(skater.heading) ||
                std::abs(std::remainder(skater.heading, 360.f)) > 3.f) {
                out("error: Geometry requires the closed editor and a fresh stopped fixture baseline."); return;
            }
            geometry_skater = skater.position;
            const auto context = geometry_context(editor, false);
            if (!geometry.arm(context, now)) {
                out("error: Geometry requires unchanged native riding and fixture ownership."); return;
            }
            geometry_captured = context; geometry_rail = fixture.rail; geometry_started = now;
            ++geometry_session;
            out("Read-only fixture measurement armed; close console within five seconds, then inspect geometry-status."); return;
        }
        if (op != "status") { geometry.cancel(); trajectory_emit(trajectory.cancel(GetTickCount64())); }
        if (op == "status") {
            NoBailSkater menu_owner{};
            const auto ui = no_bail_skater(menu_owner) ? multiplayer::sample_game_ui_state(menu_owner.base) :
                multiplayer::GameUiState{};
            Json status{{"configured", fixture.configured}, {"available", editor.available}, {"busy", editor.busy},
                {"failed", editor.failed}, {"editor_open", model.debug.park_editor}, {"asset_available", asset},
                {"present", present}, {"arcade", arcade_mode::enabled()}, {"generation", editor.generation}, {"revision", editor.revision},
                {"map", editor.map}, {"position", skater.position}, {"heading", skater.heading},
                {"speed", skater.speed}, {"state", skater.physics_state}, {"skater", skater.skater},
                {"native_ui_available", ui.available}, {"native_in_menu", ui.available ? Json(ui.in_menu) : Json(nullptr)}};
            if (fixture.configured) {
                status["start"] = fixture.start; status["rail_position"] = fixture.rail.position;
                status["rail_rotation"] = fixture.rail.rotation; status["rail_scale"] = fixture.rail.scale;
                status["item"] = rail_item;
                float distance{};
                for (unsigned i = 0; i < 3; ++i) distance += std::pow(skater.position[i] - fixture.start[i], 2.0f);
                status["reset_ready"] = present && !editor.busy && !model.debug.park_editor && skater.skater &&
                    skater.physics_state == 100 && skater.speed <= .1f && distance <= .25f &&
                    std::abs(std::remainder(skater.heading, 360.0f)) <= 3.0f;
            }
            out("ARCADE FIXTURE: " + status.dump()); return;
        }
        if (!editor.available || editor.busy || editor.failed || !skater.skater) {
            out("error: Fixture requires a ready native editor and local skater."); return;
        }
        if (op == "adopt") {
            const auto rails = std::count_if(editor.objects.begin(), editor.objects.end(),
                [](const auto& object) { return object.item == rail_item; });
            if (fixture.configured || !editor.project.starts_with("Arcade Rail Fixture") || rails != 1) {
                out("error: Load the saved fixture park containing exactly one flat rail before adoption."); return;
            }
            const auto rail = std::find_if(editor.objects.begin(), editor.objects.end(),
                [](const auto& object) { return object.item == rail_item; });
            if (!rail->spawned || rail->scale != 1) { out("error: Saved fixture rail is not ready at its original scale."); return; }
            fixture.configured = true; fixture.map = editor.map; fixture.saved_project = editor.project;
            fixture.rail = {rail->id, rail->item, rail->position, rail->rotation, rail->scale};
            // Native riding telemetry settles about one metre above the floor.
            fixture.start = {rail->position[0], rail->position[1] + 1.0f, rail->position[2] - 8};
            out("Saved fixture adopted; verify status and native reset before trials."); return;
        }
        if (op == "place" || op == "relocate") {
            const bool relocate = op == "relocate";
            if ((relocate ? !present : fixture.configured) || !model.debug.park_editor || !asset ||
                skater.speed > .1f || skater.physics_state != 100) {
                out("error: Open editor at a stopped riding baseline; place creates one fixture and relocate moves that same rail."); return;
            }
            float ground{};
            if (!flat_start(skater.position, ground, true)) {
                out("error: Fixture lane lacks a continuous flat native collision surface."); return;
            }
            Fixture candidate = relocate ? fixture : Fixture{};
            candidate.configured = true; candidate.map = editor.map;
            candidate.start = skater.position;
            candidate.rail.item = rail_item;
            candidate.rail.position = {candidate.start[0], ground, candidate.start[2] + 8};
            if (relocate) candidate.rail.id = found->id;
            const auto answer = edit_local_park(relocate ? "move" : "place", editor.map,
                editor.generation, editor.revision, {}, candidate.rail);
            if (!answer.starts_with("error:")) fixture = candidate;
            out(answer); return;
        }
        if (!present) { out("error: Fixture object is absent, changed or not natively spawned."); return; }
        if (op == "turn") {
            if (!model.debug.park_editor) { out("error: Open editor before rotating the fixture rail."); return; }
            auto rail = fixture.rail; rail.id = found->id;
            rail.rotation = rail.rotation[1] == 0 ? std::array<float, 4>{0, .70710678f, 0, .70710678f} :
                std::array<float, 4>{0, 0, 0, 1};
            const auto answer = edit_local_park("move", editor.map, editor.generation, editor.revision, {}, rail);
            if (!answer.starts_with("error:")) fixture.rail = rail;
            out(answer); return;
        }
        if (op == "reset") {
            float ground{};
            if (model.debug.park_editor || !flat_start(fixture.start, ground, false) ||
                std::abs(ground - fixture.rail.position[1]) > .08f) {
                out("error: Close editor and retain the verified fixture start surface before reset."); return;
            }
            // Trainer routes this through the existing native teleport manager:
            // upright identity heading, native streaming/reset, no raw transform write.
            out(trainer::command("tp", {std::format("{:.6f}", fixture.start[0]),
                std::format("{:.6f}", fixture.start[1]), std::format("{:.6f}", fixture.start[2])}));
            return;
        }
        if (op == "save") {
            const bool create = fixture.saved_project.empty();
            const auto answer = edit_local_park(create ? "mod-create" : "mod-save", editor.map,
                editor.generation, editor.revision, fixture.saved_project, {},
                {"Arcade Rail Fixture", "", "1", "Native flat rail and unchanged course objects for controlled grind trials."});
            if (!answer.starts_with("error:")) fixture.saved_project = local_park_editor().project;
            out(answer);
        }
    };
    registry.add(std::move(entry));
}
}
namespace dingosdk::arcade_mode {
void tick_arcade_fixture_geometry(std::uint64_t now, bool gameplay) noexcept {
    try { console::geometry_update(now, gameplay); console::trajectory_update(now, gameplay); }
    catch (...) { console::geometry.cancel(); console::trajectory_emit(console::trajectory.cancel(now)); }
}
}

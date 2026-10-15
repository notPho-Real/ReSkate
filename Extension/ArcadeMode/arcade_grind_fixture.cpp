#include "Extension/Console/commands.h"
#include "Extension/Objects/ParkEditor/park_editor_runtime.h"
#include "Extension/Profile/local_profile_runtime.h"
#include "Extension/Trainer/trainer.h"
#include <algorithm>
#include <cmath>
#include <format>

namespace dingosdk::console {
namespace {
constexpr auto rail_item = "own_bkrails_flat_00001";
// One added object, retained by the existing Park Editor document and save path.
// No layout clearing, physics tuning, collision overrides or direct skater writes.
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
}
void register_arcade_fixture_commands(Commands& registry) {
    auto operation = argument("status|place|turn|reset|save|adopt", Type::text);
    operation.choices = {"status", "place", "turn", "reset", "save", "adopt"};
    auto entry = action("arcadefixture", "Single native flat-rail practice fixture", Group::objects, {operation});
    entry.run = [](const Model& model, const Values& values, const Output& out) {
        const auto& op = std::get<std::string>(values[0]);
        const auto editor = local_park_editor();
        const auto skater = trainer::telemetry();
        const auto count = std::count_if(editor.objects.begin(), editor.objects.end(), same_rail);
        const auto found = std::find_if(editor.objects.begin(), editor.objects.end(), same_rail);
        const bool present = fixture.configured && fixture.map == editor.map && count == 1 &&
            found != editor.objects.end() && found->spawned;
        const bool asset = editor.assets && std::any_of(editor.assets->begin(), editor.assets->end(),
            [](const auto& item) { return item.key == rail_item; });
        if (op == "status") {
            Json status{{"configured", fixture.configured}, {"available", editor.available}, {"busy", editor.busy},
                {"failed", editor.failed}, {"editor_open", model.debug.park_editor}, {"asset_available", asset},
                {"present", present}, {"generation", editor.generation}, {"revision", editor.revision},
                {"map", editor.map}, {"position", skater.position}, {"heading", skater.heading},
                {"speed", skater.speed}, {"state", skater.physics_state}, {"skater", skater.skater}};
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
            fixture.start = {rail->position[0], rail->position[1] + .5f, rail->position[2] - 8};
            out("Saved fixture adopted; verify status and native reset before trials."); return;
        }
        if (op == "place") {
            if (fixture.configured || !model.debug.park_editor || !asset || skater.speed > .1f) {
                out("error: Open editor at a stopped flat baseline with the exact flat rail available; only one fixture is allowed."); return;
            }
            float ground{};
            if (!flat_start(skater.position, ground, true)) {
                out("error: Fixture lane lacks a continuous flat native collision surface."); return;
            }
            Fixture candidate;
            candidate.configured = true; candidate.map = editor.map;
            candidate.start = {skater.position[0], ground + .5f, skater.position[2]};
            candidate.rail.item = rail_item;
            candidate.rail.position = {candidate.start[0], ground, candidate.start[2] + 8};
            const auto answer = edit_local_park("place", editor.map, editor.generation, editor.revision, {}, candidate.rail);
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
                std::abs(ground + .5f - fixture.start[1]) > .08f) {
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

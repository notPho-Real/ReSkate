// Adapted from worthlessnorms/ReSkate Style Editor, commit
// f6d7e5a929f9e2b046ea6b1192b1fac2487079c7, Extension/Style/trick_gestures.cpp.
// Covered by the repository's GNU GPL v3 license. Context variants are retained.
#include "arcade_authored_gestures.h"
#include "Engine/Game/Build/20260929/skater_skeleton.h"
#include "Engine/Resource/ebx_document.h"
#include "Engine/Vfs/game_bundles.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace dingosdk::arcade_mode {
namespace {
namespace ebx = frostbite::ebx;
template<class T> const T& field(const ebx::Object& object, std::string_view name) {
    const auto* found = object.find(name);
    const auto* value = found ? std::get_if<T>(&found->value.data) : nullptr;
    if (!value) throw std::runtime_error("Authored gesture field missing or mistyped");
    return *value;
}
const ebx::Object& object_of(const ebx::Value& value) {
    const auto* object = std::get_if<std::shared_ptr<ebx::Object>>(&value.data);
    if (!object || !*object) throw std::runtime_error("Authored gesture object missing");
    return **object;
}
float number(const ebx::Object& object, std::string_view name, float minimum, float maximum) {
    const auto value = field<double>(object, name);
    if (!std::isfinite(value) || value < minimum || value > maximum)
        throw std::runtime_error("Authored gesture number outside bounds");
    return static_cast<float>(value);
}
}
std::vector<AuthoredGesture> read_authored_gestures(const std::filesystem::path& game_root) {
    namespace build = game::build::v20260929::skater_skeleton;
    const vfs::GameData data(game_root);
    const auto bundle = data.read_bundle(data.read_toc(build::skeleton_toc), build::skeleton_bundle);
    if (!bundle) throw std::runtime_error("Authored gesture bundle missing");
    std::vector<AuthoredGesture> result;
    for (std::size_t i = 0; i < bundle->manifest.ebx.size(); ++i) {
        auto name = bundle->manifest.ebx[i].name;
        for (auto& c : name) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
        if (!name.starts_with("gameplay/input/gestures/fliptricks/gesture_fliptrick_")) continue;
        const auto* payload = bundle->payload(frostbite::AssetKind::ebx, i);
        if (!payload) continue;
        const auto document = ebx::read_document(data.read(*payload));
        const auto* root = document.root();
        if (!root || !root->object || document.rootType != "DelMarGameplayGesture") continue;
        const auto& object = *root->object;
        const auto& pointer = field<ebx::PointerReference>(object, "AnimationEnum");
        if (pointer.kind != ebx::PointerKind::internal || pointer.index < 0 ||
            static_cast<std::size_t>(pointer.index) >= document.instances.size()) continue;
        const auto& instance = document.instances[static_cast<std::size_t>(pointer.index)];
        if (!instance.object) continue;
        const auto id = field<std::int64_t>(*instance.object, "Value");
        if (id < 1 || id > 63) continue;
        AuthoredGesture base{};
        base.native_id = static_cast<std::uint8_t>(id);
        base.nollie = field<bool>(object, "IsNollie");
        base.context = name.ends_with("_fromgrind") ? GestureContext::from_grind :
            name.ends_with("_underflip") ? GestureContext::underflip : GestureContext::ground;
        base.hold_start = number(object, "HoldPatternMinAngleDeg", -3600, 3600);
        base.hold_end = number(object, "HoldPatternMaxAngleDeg", -3600, 3600);
        base.hold_minimum = number(object, "HoldPatternMinMagnitude", 0, 1);
        const auto& patterns = field<ebx::Value::Array>(object, "Patterns");
        if (patterns.size() > 32) throw std::runtime_error("Too many authored gesture patterns");
        for (const auto& value : patterns) {
            const auto& pattern = object_of(value);
            if (field<std::int64_t>(pattern, "Mode") != 1) continue;
            const auto& arcs = field<ebx::Value::Array>(pattern, "Arcs");
            if (arcs.empty()) continue;
            if (arcs.size() > 32 || result.size() >= 256)
                throw std::runtime_error("Authored gesture catalog exceeds bounds");
            auto gesture = base;
            for (const auto& value : arcs) {
                const auto& arc = object_of(value);
                AuthoredArc item{number(arc, "StartAngle", -3600, 3600),
                    number(arc, "EndAngle", -3600, 3600), number(arc, "MinMagnitude", 0, 1),
                    number(arc, "MaxMagnitude", 0, 2), number(arc, "OverrideMaxTimeWaitingForArc", -1, 10)};
                if (item.minimum > item.maximum) throw std::runtime_error("Authored gesture magnitude order");
                gesture.arcs.push_back(item);
            }
            result.push_back(std::move(gesture));
        }
    }
    if (result.empty()) throw std::runtime_error("No authored flip gestures found");
    // Keep alternative patterns in their authored order. cat pouch
    std::stable_sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
        return a.native_id < b.native_id;
    });
    return result;
}
}

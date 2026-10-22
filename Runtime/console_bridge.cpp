#include "runtime_internal.h"
#include "Extension/Console/commands.h"
#include "Extension/ArcadeMode/arcade_mode.h"
#include "Extension/Progression/mission_progression_override.h"
#include "Extension/Settings/gameplay_settings_override.h"
#include "Extension/Settings/named_settings.h"
#include "Extension/Skater/skater_slot_override.h"
#include <array>

namespace dingosdk::runtime::detail {
namespace {
// This scope exists only for one scheduled console invocation on its engine
// thread. It cannot be extended or supplied by console text.
thread_local std::optional<std::chrono::steady_clock::time_point> console_reset_queued_at;
struct ConsoleResetScope {
    explicit ConsoleResetScope(std::chrono::steady_clock::time_point queued_at) noexcept {
        console_reset_queued_at = queued_at;
    }
    ~ConsoleResetScope() { console_reset_queued_at.reset(); }
};
std::string apply_console_arcade_runtime_reset() {
    auto& r = runtime();
    {
        std::lock_guard lock(r.mutex);
        if (r.engine_thread.load() != GetCurrentThreadId() || !r.native_tick_ready || r.observer_failed ||
            r.requests.loading() || !r.requests.executing_console(GetCurrentThreadId()))
            return "error: Runtime reset requires a healthy scheduled command on the game update thread.";
    }
    const auto reset_now = std::chrono::steady_clock::now();
    if (!console_reset_queued_at || reset_now < *console_reset_queued_at ||
        reset_now - *console_reset_queued_at > std::chrono::milliseconds(1000))
        return "error: Runtime reset request expired or has no dispatch receipt.";
    if (!dingosdk::arcade_mode::reset_runtime_transient())
        return "error: Runtime reset unavailable until initial preferences are applied.";
    return dingosdk::arcade_mode::enabled()
        ? "Runtime reset complete: mode ON unchanged; preferences untouched."
        : "Runtime reset complete: mode OFF unchanged; preferences untouched.";
}
dingosdk::overlay::Model console_model_snapshot() {
    auto& r = runtime();
    // Runs on the game thread before a command: the settings it inspects are current.
    publish_named_settings(r);
    dingosdk::overlay::Model model;
    std::shared_ptr<const NamedSettings> settings;
    {
        std::lock_guard lock(r.mutex);
        model = r.model;
        model.debug = r.debug_model;
        model.offline = r.offline_model;
        settings = r.named_settings;
    }
    if (settings) model.engine_settings = *settings;
    return model;
}

void console_callback_result(bool queued, const std::array<char, 512>& result,
                             std::string_view fallback) {
    std::string line = queued ? "ok: " : "error: ";
    line += result[0] ? result.data() : fallback;
    console_line(line);
}

bool queue_console_debug(dingosdk::overlay::DebugAction action, bool enabled = false,
                         float value = 0.0f) {
    std::array<char, 512> result{};
    const dingosdk::overlay::DebugRequest request{action, enabled, value};
    const auto queued = queue_debug(nullptr, request, result.data(), result.size());
    result.back() = '\0';
    console_callback_result(queued, result, "Debug request rejected.");
    return queued;
}

bool queue_console_feature(dingosdk::overlay::OfflineFeatureGroup group, bool enabled) {
    std::array<char, 512> result{};
    const dingosdk::overlay::OfflineFeatureRequest request{group, enabled};
    const auto queued = queue_offline_feature(nullptr, request, result.data(), result.size());
    result.back() = '\0';
    console_callback_result(queued, result, "Gameplay setting request rejected.");
    return queued;
}

void apply_console_engine_variable(std::string_view name,
                                   dingosdk::EngineVariableAction action, bool value) {
    auto& r = runtime();
    {
        std::lock_guard lock(r.mutex);
        if (!r.requests.executing_console(GetCurrentThreadId()) || r.requests.loading()) {
            console_line("error: Engine variables require a scheduled command in an idle local session.");
            return;
        }
    }
    const dingosdk::EngineVariableRequest request{std::string(name), action, value};
    auto observation = dingosdk::update_gameplay_engine_variable(request);
    merge_skater_slot_observation(
        observation.model, dingosdk::skater_slot_override_observation());
    merge_main_mission_observation(
        observation.model, dingosdk::main_mission_override_observation());
    if (observation.json != r.last_gameplay_settings_override_observation) {
        record(observation.json);
        r.last_gameplay_settings_override_observation = observation.json;
    }
    const auto status = observation.model.status;
    {
        std::lock_guard lock(r.mutex);
        r.offline_model = std::move(observation.model);
        ++r.offline_revision;
    }
    console_line(status.empty() ? "Engine variable request completed." : status);
}

std::string apply_console_named_setting(std::string_view name, std::string_view value, bool restore, bool all) {
    auto& r = runtime();
    {
        std::lock_guard lock(r.mutex);
        if (r.engine_thread.load() != GetCurrentThreadId() || !r.named_context_ready || r.observer_failed ||
            r.requests.loading() || !r.requests.executing_console(GetCurrentThreadId()))
            return "error: Native settings require an idle local session on the game update thread.";
    }
    auto result = all ? dingosdk::restore_named_settings() : dingosdk::player_change_named_setting(name, value, restore);
    publish_named_settings(r);
    return result;
}
}
void execute_console_command(const std::string& command, std::chrono::steady_clock::time_point queued_at) {
    const ConsoleResetScope reset_scope{queued_at};
    try {
        const dingosdk::console::Output output{[](const std::string& line) { console_line(line); }, {}, {}};
        dingosdk::console::game_commands().execute(command, console_model_snapshot(), output);
    } catch (const std::exception& error) {
        console_line(std::string("error: ") + error.what());
    } catch (...) {
        console_line("error: console command raised an internal exception.");
    }
}
}

namespace dingosdk::console {
void request_debug(overlay::DebugAction action, bool enabled, float value) {
    dingosdk::runtime::detail::queue_console_debug(action, enabled, value);
}
void request_feature(overlay::OfflineFeatureGroup group, bool enabled) {
    dingosdk::runtime::detail::queue_console_feature(group, enabled);
}
void request_engine_variable(std::string_view name, bool restore, bool value) {
    dingosdk::runtime::detail::apply_console_engine_variable(name, restore ? EngineVariableAction::restore : EngineVariableAction::set, value);
}
std::string request_named_setting(std::string_view name, std::string_view value, bool restore) {
    return dingosdk::runtime::detail::apply_console_named_setting(name, value, restore, false);
}
std::string request_arcade_runtime_reset() { return dingosdk::runtime::detail::apply_console_arcade_runtime_reset(); }
std::string request_reset_named_settings() { return dingosdk::runtime::detail::apply_console_named_setting({}, {}, true, true); }
void request_level(const std::string& level, const std::string& start,
    const std::string& sublevel, const std::string& substart) {
    std::array<char, 512> result{};
    const auto queued = dingosdk::runtime::detail::queue_load(nullptr, level.c_str(), start.c_str(), sublevel.c_str(), substart.c_str(),
        result.data(), result.size());
    result.back() = '\0';
    dingosdk::runtime::detail::console_callback_result(queued, result, "Load request rejected.");
}
}

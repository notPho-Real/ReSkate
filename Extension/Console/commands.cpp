#include "commands.h"
#include "Extension/ArcadeMode/arcade_mode.h"
#include "Engine/Core/Log/logging.h"
#include "Engine/Core/Platform/path_text.h"
#include <memory>

namespace dingosdk::console {
Argument argument(std::string name, Type type, bool optional) {
    Argument result;
    result.name = std::move(name);
    result.type = type;
    result.optional = optional;
    return result;
}
Entry action(std::string name, std::string description, Group group, std::vector<Argument> args) {
    Entry entry;
    entry.name = std::move(name);
    entry.description = std::move(description);
    entry.group = group;
    entry.arguments = std::move(args);
    return entry;
}
Entry variable(std::string name, std::string description, Group group, Argument arg) {
    auto entry = action(std::move(name), std::move(description), group, {std::move(arg)});
    entry.kind = Kind::variable;
    return entry;
}
State boolean_state(bool available, bool value, std::string reason, std::string detail) {
    return {available, available ? std::optional<std::string>(value ? "1" : "0") : std::nullopt, std::move(reason),
            std::move(detail), false};
}
namespace {
void register_console_commands(Commands &registry) {
    auto arcade = action("arcademode", "Read or change Arcade Mode immediately (saved)", Group::gameplay,
                         {argument("on|off", Type::boolean, true)});
    arcade.run = [](const Model&, const Values& values, const Output& out) {
        if (!values.empty()) arcade_mode::set_enabled(std::get<bool>(values[0]));
        out(arcade_mode::enabled() ? "Arcade Mode ON" : "Arcade Mode OFF");
    };
    registry.add(std::move(arcade));
    auto runtime_reset = action("arcaderuntimereset", "Reset transient Arcade state without changing runtime mode or saved preferences", Group::gameplay);
    runtime_reset.run = [](const Model&, const Values&, const Output& out) {
        out(request_arcade_runtime_reset());
    };
    registry.add(std::move(runtime_reset));
    auto controller=action("arcadecontroller","Unsaved source binding: slot0..3 and lease1000..300000ms; clear revokes (OFF restores automatic)",Group::gameplay,
        {argument("slot|clear",Type::text,false),argument("milliseconds",Type::unsigned_integer,true)});
    controller.run=[](const Model&,const Values& values,const Output& out){
        const auto& name=std::get<std::string>(values[0]);
        if(name=="clear" && values.size()==1){arcade_mode::clear_controller_source();out("Controller binding cleared; sources blocked until OFF or a new explicit binding.");return;}
        if(name.size()!=1 || name[0]<'0' || name[0]>'3' || values.size()!=2){out("Use arcadecontroller <0..3> <1000..300000ms>, or arcadecontroller clear.");return;}
        const auto duration=std::get<std::uint64_t>(values[1]);
        if(duration>300000 || !arcade_mode::set_controller_source(static_cast<unsigned>(name[0]-'0'),static_cast<unsigned>(duration))){out("Binding rejected: require Arcade ON, ready input and lease1000..300000ms.");return;}
        out("Binding requested; close input capture within5s and neutralize the selected pad. Admission requires fresh native owner proof; loss revokes without fallback.");
    };
    registry.add(std::move(controller));
    auto auto_kick = action("arcadeautokick", "Read or change automatic ground kicking (saved)", Group::gameplay,
        {argument("on|off", Type::boolean, true)});
    auto_kick.run = [](const Model&, const Values& values, const Output& out) {
        if (!values.empty()) arcade_mode::set_auto_kick(std::get<bool>(values[0]));
        out(arcade_mode::auto_kick_enabled() ? "Automatic kicking ON" : "Automatic kicking OFF");
    };
    registry.add(std::move(auto_kick));
    auto tap_spin = action("arcadetapspin", "Read or change tap shoulder spin; OFF selects held spin (saved)", Group::gameplay,
        {argument("on|off", Type::boolean, true)});
    tap_spin.run = [](const Model&, const Values& values, const Output& out) {
        if (!values.empty()) arcade_mode::set_spin_mode(std::get<bool>(values[0]) ? arcade_mode::SpinMode::tap : arcade_mode::SpinMode::held);
        out(arcade_mode::spin_mode()==arcade_mode::SpinMode::tap ? "Tap spin ON" : "Held spin ON");
    };
    registry.add(std::move(tap_spin));
    auto capture=action("arcadeinputcapture","Finite input/native correlation capture; duration0 stops (milliseconds, source mask1..127 or bounded semantic255; optional angular-on requires mask39)",Group::gameplay,
        {argument("milliseconds",Type::unsigned_integer,false),argument("source-mask",Type::unsigned_integer,true),argument("angular-on|off",Type::boolean,true)});
    capture.run=[](const Model&,const Values& values,const Output& out){
        const auto duration=std::get<std::uint64_t>(values[0]);const auto scope=values.size()>1?std::get<std::uint64_t>(values[1]):96;
        const bool angular=values.size()>2&&std::get<bool>(values[2]);
        if(angular&&(!duration||duration>5000||scope!=39)){out("Angular observation requires duration1..5000 and base source mask39.");return;}
        if(duration>5000||!scope||(scope>127&&scope!=255)||(scope==255&&duration>1000)||!arcade_mode::set_input_capture(static_cast<unsigned>(duration),static_cast<unsigned>(scope),angular))
            out("Capture unavailable: require active telemetry, duration1..5000 and source mask1..127, or semantic255 for at most1000ms; legacy sources1..31 also require Arcade ON and a fresh native owner; duration0 stops an active capture.");
        else if(duration&&scope==255)out("Input capture capability=manual_capture_schema4_v2 required_semantics=255 active_only=true; completeness and native acceptance remain unverified.");
        else out(duration?"Input correlation capture started; completeness and native acceptance remain unverified.":"Input correlation capture stopped.");
    };
    registry.add(std::move(capture));
    auto camera_capture=action("arcadecameracapture","Finite observational camera capture; milliseconds1..3000, zero stops; no camera/input changes",Group::gameplay,
        {argument("milliseconds",Type::unsigned_integer,false)});
    camera_capture.run=[](const Model&,const Values& values,const Output& out){
        const auto duration=std::get<std::uint64_t>(values[0]);
        if(duration>3000||!arcade_mode::set_camera_capture(static_cast<unsigned>(duration)))
            out("Camera capture unavailable: require telemetry, duration1..3000, no active camera capture; zero stops.");
        else out(duration?"Camera observation capture started; completeness and final presented view remain unverified.":"Camera observation stop requested; require its end record.");
    };
    registry.add(std::move(camera_capture));
    auto playback_mode = argument("delivery|timeline|reference|off", Type::text, true);
    playback_mode.choices = {"delivery", "timeline", "reference", "off"};
    auto playback = action("arcadeplayback", "Unsaved playback A/B diagnostic; change with Arcade Mode OFF",
                           Group::gameplay, {playback_mode});
    playback.run = [](const Model&, const Values& values, const Output& out) {
        using arcade_mode::PlaybackExperiment;
        if (!values.empty()) {
            const auto& name = std::get<std::string>(values[0]);
            const auto mode = name == "reference" ? PlaybackExperiment::reference :
                name == "timeline" ? PlaybackExperiment::timeline : PlaybackExperiment::delivery;
            if (!arcade_mode::set_playback_experiment(mode, name != "off")) {
                out("Turn Arcade Mode OFF before changing playback diagnostics.");
                return;
            }
        }
        const auto mode = arcade_mode::playback_experiment();
        out(mode == PlaybackExperiment::reference ? "Playback: reference (experimental)" :
            mode == PlaybackExperiment::timeline ? "Playback: timeline (experimental)" : "Playback: delivery");
    };
    registry.add(std::move(playback));
    auto action_mode = argument("classic|off|observe|grabs|revert-fs|revert-bs", Type::text, true);
    action_mode.choices = {"classic", "off", "observe", "grabs", "revert-fs", "revert-bs"};
    auto actions = action("arcadeactions", "Unsaved native action selection; change with Arcade Mode OFF",
                          Group::gameplay, {action_mode});
    actions.run = [](const Model&, const Values& values, const Output& out) {
        using arcade_mode::ActionExperiment;
        if (!values.empty()) {
            const auto& name = std::get<std::string>(values[0]);
            const auto mode = name == "classic" ? ActionExperiment::classic : name == "observe" ? ActionExperiment::observe : name == "grabs" ? ActionExperiment::grabs :
                name == "revert-fs" ? ActionExperiment::revert_frontside : name == "revert-bs" ? ActionExperiment::revert_backside : ActionExperiment::off;
            if (!arcade_mode::set_action_experiment(mode)) { out("Turn Arcade Mode OFF before changing action experiments."); return; }
        }
        const auto mode = arcade_mode::action_experiment();
        out(mode == ActionExperiment::classic ? "Actions: airborne grabs and RT revert" : mode == ActionExperiment::off ? "Action experiments OFF" : mode == ActionExperiment::observe ? "Actions: observe only" :
            mode == ActionExperiment::grabs ? "Actions: B grab candidates (unverified)" :
            mode == ActionExperiment::revert_frontside ? "Actions: RT frontside candidate (unverified)" : "Actions: RT backside candidate (unverified)");
    };
    registry.add(std::move(actions));
    auto topic = argument("command", Type::text, true);
    topic.rest = true;
    topic.complete = [&registry](const Model &, auto) {
        std::vector<std::string> names;
        for (const auto &e : registry.entries())
            names.push_back(e.name);
        return names;
    };
    auto help = action("help", "Show command syntax, values, restrictions, and availability", Group::console, {topic});
    help.execution = Execution::local;
    help.run = [&registry](const Model &model, const Values &args, const Output &out) {
        if (args.empty()) {
            out("Enter a variable by itself to read it, or follow it with a value: "
                "noclip 1, nobail 0, flyspeed 60.");
            out("Use commands [prefix] to list entries, help <command> for details, "
                "and Tab to complete names and "
                "arguments.");
            out("Groups: Movement, Gameplay, World, Graphics, Progression, Objects, "
                "Engine, Console.");
            return;
        }
        const auto &name = std::get<std::string>(args[0]);
        const auto *entry = registry.find(name);
        if (!entry) {
            bool found = false;
            for (const auto &item : registry.entries())
                if (ascii_case_insensitive_starts_with(item.name, name + " ")) {
                    out(Commands::usage(item) + (item.description.empty() ? "" : " - " + item.description));
                    found = true;
                }
            if (!found)
                out("error: Unknown help topic.");
            return;
        }
        out(Commands::usage(*entry));
        out(registry.describe(*entry, model));
        out(std::string("Group: ") + group_name(entry->group));
        for (const auto &arg : entry->arguments) {
            if (!arg.choices.empty()) {
                std::string choices = arg.name + ": ";
                for (const auto &choice : arg.choices) {
                    if (choices.back() != ' ')
                        choices += ", ";
                    choices += choice;
                }
                out(choices);
            }
            if (arg.minimum || arg.maximum)
                out(arg.name + " range: " + (arg.minimum ? value_text(*arg.minimum) : "unbounded") + " to " +
                    (arg.maximum ? value_text(*arg.maximum) : "unbounded"));
        }
        if (entry->reset)
            out("Restore: reset " + entry->name);
    };
    registry.add(std::move(help));
    auto list = action("commands", "List commands and variables, optionally filtered by prefix", Group::console,
                       {argument("prefix", Type::text, true)});
    list.execution = Execution::local;
    list.run = [&registry](const Model &model, const Values &args, const Output &out) {
        const auto prefix = args.empty() ? std::string{} : std::get<std::string>(args[0]);
        std::vector<const Entry *> rows;
        for (const auto &e : registry.entries())
            if (ascii_case_insensitive_starts_with(e.name, prefix))
                rows.push_back(&e);
        std::sort(rows.begin(), rows.end(), [](auto *a, auto *b) { return lower(a->name) < lower(b->name); });
        for (const auto *e : rows)
            out(registry.describe(*e, model));
        out(std::to_string(rows.size()) + " matching entries.");
    };
    registry.add(std::move(list));
    for (const auto &name : {"clear", "history"}) {
        auto entry =
            action(name, equal(name, "clear") ? "Clear console scrollback" : "Show recent commands", Group::console);
        entry.execution = Execution::local;
        entry.echo_input = !equal(name, "clear");
        entry.run = [clear = equal(name, "clear")](const Model &, const Values &, const Output &out) {
            const auto &callback = clear ? out.clear : out.history;
            if (callback)
                callback();
            else
                out("This command belongs to the overlay console.");
        };
        registry.add(std::move(entry));
    }
    auto text = argument("text", Type::text, true);
    text.rest = true;
    auto echo = action("echo", "Print text to the console", Group::console, {text});
    echo.execution = Execution::local;
    echo.run = [](const Model &, const Values &args, const Output &out) {
        out(args.empty() ? "" : std::get<std::string>(args[0]));
    };
    registry.add(std::move(echo));
    auto status = action("status", "Show current game state", Group::console);
    status.execution = Execution::local;
    status.run = [](const Model &model, const Values &, const Output &out) {
        out("state: " + model.state);
        out("freecam=" + value_text(model.debug.free_camera) + " noclip=" + value_text(model.debug.noclip) +
            " nobail=" + value_text(model.debug.no_bail) +
            " flyspeed=" + value_text(static_cast<double>(model.debug.camera_speed)));
        if (!model.offline.status.empty())
            out(model.offline.status);
    };
    registry.add(std::move(status));
    auto reset_arg = argument("command");
    reset_arg.rest = true;
    reset_arg.complete = [&registry](const Model &, auto) {
        std::vector<std::string> names{"all", "movement", "features", "engine"};
        for (const auto &e : registry.entries())
            if (e.reset)
                names.push_back(e.name);
        return names;
    };
    auto reset = action("reset", "Restore a variable or a group of session overrides", Group::console, {reset_arg});
    reset.run = [&registry](const Model &model, const Values &args, const Output &out) {
        const auto &name = std::get<std::string>(args[0]);
        const auto *entry = registry.find(name);
        if (!entry || !entry->reset) {
            out("error: This entry has no restore operation. Use help reset.");
            return;
        }
        entry->reset(model, out);
    };
    registry.add(std::move(reset));
    for (const auto &name : {"movement", "features", "engine", "all"}) {
        auto entry = action(std::string("reset ") + name, "Restore the selected runtime overrides", Group::console);
        entry.run = [scope = std::string(name)](const Model &model, const Values &, const Output &out) {
            // Restore native fields before queuing other changes; the runtime
            // reservation intentionally rejects competing setting transactions.
            if (scope == "engine" || scope == "all") {
                out(request_reset_named_settings());
                if (scope == "engine")
                    for (const auto &v : model.offline.variables)
                        if (v.override_active)
                            request_engine_variable(v.name, true, false);
            }
            if (scope == "movement" || scope == "all")
                request_debug(overlay::DebugAction::restore_debug);
            if (scope == "features" || scope == "all")
                request_feature(overlay::OfflineFeatureGroup::restore_all, false);
        };
        if (equal(name, "movement"))
            entry.aliases = {"reset debug"};
        registry.add(std::move(entry));
    }
    auto log = action("log", "Show logging level and output path", Group::console);
    log.execution = Execution::local;
    log.aliases = {"log status"};
    log.run = [](const Model &, const Values &, const Output &out) {
        const auto s = logging::status();
        out(std::string("Logging level: ") + std::string(logging::name(s.level)));
        out(path_utf8(s.directory / L"ReSkate.log"));
    };
    registry.add(std::move(log));
    auto level = argument("level");
    level.choices = {"trace", "debug", "info", "warning", "error", "critical", "off"};
    auto loglevel = variable("loglevel", "Set the shared logging level", Group::console, level);
    loglevel.execution = Execution::local;
    loglevel.aliases = {"log level"};
    loglevel.inspect = [](const Model &) {
        return State{true, std::string(logging::name(logging::status().level)), {}, {}, false};
    };
    loglevel.run = [](const Model &, const Values &args, const Output &out) {
        const auto &text = std::get<std::string>(args[0]);
        const auto parsed = logging::parse_level(text);
        if (parsed) {
            logging::set_level(*parsed);
            out("Logging level: " + text);
        }
    };
    registry.add(std::move(loglevel));
    auto flush = action("log flush", "Flush the log file", Group::console);
    flush.execution = Execution::local;
    flush.run = [](const Model &, const Values &, const Output &out) {
        logging::flush();
        out("Log flushed.");
    };
    registry.add(std::move(flush));
}
} // namespace
const Commands &game_commands() {
    static const auto registry = [] {
        auto result = std::make_unique<Commands>();
        register_console_commands(*result);
        register_movement_commands(*result);
        register_ai_commands(*result);
        register_settings_commands(*result);
        register_world_commands(*result);
        register_graphics_commands(*result);
        register_progression_commands(*result);
        register_object_commands(*result);
        register_park_editor_commands(*result);
        register_arcade_fixture_commands(*result);
        register_multiplayer_commands(*result);
        register_perf_commands(*result);
        register_item_commands(*result);
        register_trainer_commands(*result);
        register_hall_of_meat_commands(*result);
        register_road_rash_commands(*result);
        register_board_break_commands(*result);
        register_grind_exit_commands(*result);
        return result;
    }();
    return *registry;
}
} // namespace dingosdk::console

#include "arcade_score_probe.h"
#include "arcade_score_labels.h"
#include "Engine/Core/Log/logging.h"
#include "Engine/Game/Build/20260929/engine.h"
#include "Engine/Game/Build/20260929/hud_corner.h"
#include "Extension/UI/NativeMenu/native_menu_data.h"
#include <format>
#include <map>
#include <set>
#include <string_view>

namespace dingosdk::arcade_mode {
namespace {
namespace data = multiplayer::menu_data;
namespace build = game::build::v20260929;
std::map<std::string, std::uint32_t> previous;
std::set<std::string> described;
std::uint64_t next_poll{};
std::uint64_t next_description{};
ScoreActivity activity{};
ScoreLabelEdges label_edges{};
std::uint64_t last_poll{};
bool polled{};
unsigned emitted{};
std::uintptr_t previous_manager{};
data::Value score_model{};
std::uint32_t read_label_presence(const data::Context& context) {
    // These twelve string slots were observed in score-schema-823c3ad.json.
    // Their individual presentation roles remain unverified; use their union
    // so moving one label between slots cannot manufacture a second occurrence.
    constexpr std::array<std::uint32_t, 5> paired{
        0xb3e9db3c, 0x7d1e237c, 0x9acb7cf0, 0xe8264626, 0xf9dc9463};
    constexpr std::array<std::uint32_t, 2> text_fields{0x4d8e01b9, 0x4db7e22e};
    constexpr std::array<std::uint32_t, 2> nested{0x55c071ca, 0xe1b69fc5};
    std::uint32_t labels{};
    auto read_text = [&](data::Value value) {
        data::require(data::kind(value.type) == 7 && data::size(value.type) == 8,
                      "native score label schema differs");
        labels |= score_label_bit(score_label_id(context.text(value, 96)));
    };
    for (const auto parent : paired) {
        const auto group = context.field(score_model, parent);
        for (const auto field : text_fields) read_text(context.field(group, field));
    }
    for (const auto parent : nested)
        read_text(context.field(context.field(context.field(score_model, parent), 0x2358a817), 0x187bbd85));
    return labels;
}
void inspect(const data::Context& context, data::Value value, const std::string& path,
             unsigned depth, unsigned& budget, std::uint64_t now) {
    if (!budget || depth > 3 || emitted >= 4096) return;
    --budget;
    const auto kind = data::kind(value.type), size = data::size(value.type);
    if (!described.contains(path)) {
        if (described.size() >= 256) return;
        described.insert(path);
        logging::log(logging::Level::info, logging::Channel::skater,
            "ARCADE SCORE NODE: path={} kind={} size={}.", path, kind, size);
    }
    const auto meta = data::read<std::uintptr_t>(value.type);
    if (kind == 2) {
        const auto count = data::read<std::uint16_t>(meta + 0x2a);
        data::require(count <= 128, "score structure exceeds probe bound");
        const auto fields = data::read<std::uintptr_t>(meta + 0x60);
        for (unsigned i = 0; i < count && budget; ++i) {
            const auto hash = data::read<std::uint32_t>(fields + i * 24ULL);
            try { inspect(context, context.field(value, hash), path + std::format(".{:08x}", hash), depth + 1, budget, now); }
            catch (...) { /* one unavailable optional field must not hide the others */ }
        }
        return;
    }
    if (kind == 4) {
        unsigned count{}, stride{};
        (void)context.array(value, 8, count, stride);
        for (unsigned i = 0; i < count && budget; ++i)
            inspect(context, context.element(value, i), path + std::format(".{:08x}", i), depth + 1, budget, now);
        return;
    }
    std::uint32_t current{};
    if (kind == 7) current = score_label_id(context.text(value, 96));
    else if (kind == 10 && size == 1) {
        current = data::read<std::uint8_t>(context.address(value));
        if (current > 1) return;
    } else if ((kind == 8 || kind == 15) && size == 4)
        current = data::read<std::uint32_t>(context.address(value));
    else return; // no references, pointers, floats, arbitrary strings or objects
    if (const auto found = previous.find(path); found != previous.end() && found->second == current) return;
    if (previous.size() >= 256 && !previous.contains(path)) return;
    previous[path] = current;
    ++emitted;
    if (kind == 7 && current)
        logging::log(logging::Level::info, logging::Channel::skater,
            "ARCADE SCORE LABEL: ms={} path={} id={}.", now, path, current);
    logging::log(logging::Level::info, logging::Channel::skater,
        "ARCADE SCORE VALUE: ms={} path={} value={}.", now, path, current);
    if (emitted == 4096) logging::log(logging::Level::info, logging::Channel::skater,
        "ARCADE SCORE LIMIT: reached=true.");
}
}
ScoreActivity score_activity() noexcept { return activity; }
ScoreLabelOccurrence take_score_label_occurrence(std::uint64_t now) noexcept { return label_edges.take(now); }
void reset_score_label_occurrences() noexcept { label_edges.invalidate(); }
void observe_score_hud(std::uintptr_t base, std::uint64_t now) noexcept {
    if (!base) { activity = {}; label_edges.invalidate(); return; }
    if (polled && now < last_poll) {
        next_poll = next_description = 0;
        label_edges.invalidate(ScoreLabelUncertainty::clock_reset);
    }
    if (now < next_poll) return;
    polled = true; last_poll = now;
    next_poll = now + 50;
    activity = {};
    try {
        const auto& functions = game::native_data().models;
        if (!functions.lock || !functions.unlock || !functions.value || !functions.field) {
            label_edges.invalidate(); return;
        }
        const auto ui = data::read<std::uintptr_t>(base + build::engine::ui_manager);
        const auto manager = ui ? data::read<std::uintptr_t>(ui + 0x140) : 0;
        if (!manager) { label_edges.invalidate(); return; }
        game::ModelWriteLock lock(manager);
        const data::Context context(base, manager);
        if (manager != previous_manager) {
            score_model = {}; previous_manager = manager; previous.clear(); label_edges.invalidate();
        }
        if (!score_model.handle || context.type_of(score_model.handle) != score_model.type) {
            label_edges.invalidate();
            const auto roots = context.roots({build::hud_corner::score_view_schema});
            if (roots.empty()) return;
            score_model = roots.front().model;
        }
        const auto flag = context.field(score_model, 0x7aab675c);
        data::require(data::kind(flag.type) == 10 && data::size(flag.type) == 1,
                      "native scoring activity schema differs");
        const auto active = data::read<std::uint8_t>(context.address(flag));
        data::require(active <= 1, "native scoring activity is not boolean");
        activity = {true, active != 0, now};
        // Continuous bounded observation is independent of the discovery log cap.
        try { label_edges.observe(read_label_presence(context), true, now); }
        catch (...) { label_edges.invalidate(); }
        if (now >= next_description && emitted < 4096) {
            next_description = now + 250;
            unsigned budget = 256;
            inspect(context, score_model, "9b74fc7c", 0, budget, now);
        }
    } catch (...) { label_edges.invalidate(); /* unavailable schema leaves all gameplay untouched */ }
}
}

#include "arcade_score_probe.h"
#include "arcade_score_labels.h"
#include "arcade_score_numbers.h"
#include "arcade_input_telemetry.h"
#include "Engine/Game/Build/20260929/engine.h"
#include "Engine/Game/Build/20260929/hud_corner.h"
#include "Extension/UI/NativeMenu/native_menu_data.h"
#include <map>
#include <set>
#include <string_view>

namespace dingosdk::arcade_mode {
namespace {
namespace data = multiplayer::menu_data;
namespace build = game::build::v20260929;
std::map<ScoreProbePath, std::uint32_t> previous;
std::set<ScoreProbePath> described;
std::uint64_t next_poll{};
std::uint64_t next_description{};
ScoreActivity activity{};
ScoreHudNumbers numbers{};
ScoreLabelEdges label_edges{};
ManualHistory manual_history;
void invalidate_label_observation(ScoreLabelUncertainty why = ScoreLabelUncertainty::unavailable) noexcept {
    label_edges.invalidate(why);
    manual_history.reset();
}
void report_manual_history(std::uint64_t now, const ManualHistoryDiagnostic& diagnostic) noexcept {
    static ManualHistoryDiagnostic previous{};
    static std::uint64_t report_at{}, sequence{};
    if (diagnostic == previous && now < report_at) return;
    previous = diagnostic; report_at = now + 1000;
    enqueue_client_telemetry(ManualHistoryEvidence{++sequence, now, diagnostic});
}
std::uint64_t last_poll{};
bool polled{};
unsigned emitted{};
std::uintptr_t previous_manager{};
data::Value score_model{};
unsigned label_source{};
struct LabelDiagnostics { unsigned rows{}, stage{}; std::uint32_t parts{}; };
void observe_label_part(std::string_view text, LabelDiagnostics& diagnostics) noexcept {
    // Diagnostic components only: a split label does not establish FS/BS identity.
    constexpr std::array<std::string_view, 5> parts{"Revert", "FS", "BS", "50-50 Grind", "Axle Stall"};
    for (unsigned i = 0; i < parts.size(); ++i) if (text == parts[i]) diagnostics.parts |= 1u << i;
}
void report_labels(std::uint64_t now, unsigned source, const LabelDiagnostics& diagnostic,
                   std::uint32_t labels) {
    static std::uint64_t report_at{};
    static std::array<unsigned, 5> previous_diagnostic{};
    const std::array<unsigned, 5> current{source, diagnostic.rows, diagnostic.stage, diagnostic.parts, labels};
    if (current == previous_diagnostic && now < report_at) return;
    previous_diagnostic = current; report_at = now + 1000;
    enqueue_client_telemetry(ScorePresenceEvidence{now, source, diagnostic.rows, diagnostic.stage, diagnostic.parts, labels});
}
// Field mapping from indicouchcoding/ReSkate-UI-2.0 (GPL-3.0), revision
// d5b3d978763c1a279e21ac47fc2b2003746a0a0a:
// Engine/Game/Build/20260929/hud_corner.h and Extension/Scoring/scoring_hud.cpp.
// Only detached numeric observations are retained; this never replaces the HUD.
ScoreDisplayNumber read_display_number(const data::Context& context, std::uint32_t field, bool multiplier) {
    const auto text = context.field(context.field(score_model, field), 0x4d8e01b9);
    data::require(data::kind(text.type) == 7 && data::size(text.type) == 8,
                  "native numeric score text schema differs");
    return parse_score_display_number(context.text(text, 32), multiplier);
}
std::uint32_t read_legacy_label_presence(const data::Context& context, LabelDiagnostics& diagnostic) {
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
        const auto text = context.text(value, 96);
        observe_label_part(text, diagnostic);
        labels |= score_label_bit(score_label_id(text));
    };
    for (const auto parent : paired) {
        const auto group = context.field(score_model, parent);
        for (const auto field : text_fields) read_text(context.field(group, field));
    }
    for (const auto parent : nested)
        read_text(context.field(context.field(context.field(score_model, parent), 0x2358a817), 0x187bbd85));
    return labels;
}
std::uint32_t read_history_label_presence(const data::Context& context, LabelDiagnostics& diagnostic,
    std::array<ManualHistoryRow, ManualHistory::capacity>& history, bool& complete) {
    // Same pinned GPL-3.0 reader as above: history items contain presenters,
    // not embedded text fields. Keep only exact allowlisted labels. cat pouch
    namespace corner = build::hud_corner;
    constexpr std::uint32_t trick_history = 0x84db5623, trick_name = 0x53bac52d;
    diagnostic.stage = 1;
    const auto items = context.field(context.field(score_model, trick_history), corner::stack_items);
    unsigned count{}, stride{};
    diagnostic.stage = 2;
    (void)context.array(items, 16, count, stride);
    diagnostic.rows = count;
    complete = true;
    std::uint32_t labels{};
    for (unsigned i = 0; i < count; ++i) {
        diagnostic.stage = 3;
        const auto presenter = context.field(context.element(items, i), corner::item_content);
        const auto widget = data::read<data::Widget>(context.address(presenter));
        if (!widget.data.handle) { complete = false; continue; }
        diagnostic.stage = 4;
        const auto type = context.type_of(widget.data.handle);
        data::require(type != 0, "native trick presenter type unavailable");
        const data::Value row{widget.data.handle, type};
        diagnostic.stage = 5;
        const auto text = context.field(context.field(row, trick_name), 0x4d8e01b9);
        data::require(data::kind(text.type) == 7 && data::size(text.type) == 8,
                      "native trick history text schema differs");
        const auto value = context.text(text, 96);
        history[i] = {widget.data.handle, type, score_label_id(value)};
        observe_label_part(value, diagnostic);
        labels |= score_label_bit(score_label_id(value));
    }
    diagnostic.stage = 0;
    return labels;
}
void inspect(const data::Context& context, data::Value value, const ScoreProbePath& path,
             unsigned depth, unsigned& budget, std::uint64_t now) {
    if (!budget || depth > 3 || emitted >= 4096) return;
    --budget;
    const auto kind = data::kind(value.type), size = data::size(value.type);
    if (!described.contains(path)) {
        if (described.size() >= 256) return;
        described.insert(path);
        enqueue_client_telemetry(ScoreNodeEvidence{path, kind, size});
    }
    const auto meta = data::read<std::uintptr_t>(value.type);
    if (kind == 2) {
        const auto count = data::read<std::uint16_t>(meta + 0x2a);
        data::require(count <= 128, "score structure exceeds probe bound");
        const auto fields = data::read<std::uintptr_t>(meta + 0x60);
        for (unsigned i = 0; i < count && budget; ++i) {
            const auto hash = data::read<std::uint32_t>(fields + i * 24ULL);
            auto child = path;
            if (!child.append(hash)) continue;
            try { inspect(context, context.field(value, hash), child, depth + 1, budget, now); }
            catch (...) { /* one unavailable optional field must not hide the others */ }
        }
        return;
    }
    if (kind == 4) {
        unsigned count{}, stride{};
        (void)context.array(value, 8, count, stride);
        for (unsigned i = 0; i < count && budget; ++i) {
            auto child = path;
            if (child.append(i)) inspect(context, context.element(value, i), child, depth + 1, budget, now);
        }
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
    enqueue_client_telemetry(ScoreValueEvidence{now, path, current, kind == 7 && current != 0, emitted == 4096});
}
}
ScoreActivity score_activity() noexcept { return activity; }
ScoreHudNumbers score_hud_numbers() noexcept { return numbers; }
ScoreLabelOccurrence take_score_label_occurrence(std::uint64_t now) noexcept { return label_edges.take(now); }
void reset_score_label_occurrences() noexcept { invalidate_label_observation(); }
void observe_score_hud(std::uintptr_t base, std::uint64_t now) noexcept {
    const auto invalidate = [now](ScoreLabelUncertainty why = ScoreLabelUncertainty::unavailable) {
        invalidate_label_observation(why);
        report_manual_history(now, ManualHistoryDiagnostic{});
    };
    if (!base) { activity = {}; numbers = {}; invalidate(); return; }
    if (polled && now < last_poll) {
        next_poll = next_description = 0;
        invalidate(ScoreLabelUncertainty::clock_reset);
    }
    if (now < next_poll) return;
    polled = true; last_poll = now;
    next_poll = now + 50;
    activity = {};
    numbers = {};
    try {
        const auto& functions = game::native_data().models;
        if (!functions.lock || !functions.unlock || !functions.value || !functions.field) {
            invalidate(); return;
        }
        const auto ui = data::read<std::uintptr_t>(base + build::engine::ui_manager);
        const auto manager = ui ? data::read<std::uintptr_t>(ui + 0x140) : 0;
        if (!manager) { invalidate(); return; }
        game::ModelWriteLock lock(manager);
        const data::Context context(base, manager);
        if (manager != previous_manager) {
            score_model = {}; previous_manager = manager; previous.clear(); invalidate();
        }
        if (!score_model.handle || context.type_of(score_model.handle) != score_model.type) {
            invalidate();
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
        // Optional presentation fields cannot invalidate native activity or labels.
        numbers.sampled_at = now;
        try {
            const auto score = read_display_number(context, 0x7d1e237c, false);
            numbers.score_valid = score.valid;
            numbers.score = score.value;
        } catch (...) { /* a missing or unsupported score remains unavailable */ }
        try {
            const auto multiplier = read_display_number(context, 0x9acb7cf0, true);
            numbers.multiplier_valid = multiplier.valid;
            numbers.multiplier = multiplier.value;
        } catch (...) { /* the other fields remain independently observable */ }
        // Continuous bounded observation is independent of the discovery log cap.
        LabelDiagnostics diagnostic;
        try {
            std::uint32_t labels{};
            unsigned source = 1;
            std::array<ManualHistoryRow, ManualHistory::capacity> history{};
            bool history_complete{};
            try { labels = read_history_label_presence(context, diagnostic, history, history_complete); }
            catch (...) { labels = read_legacy_label_presence(context, diagnostic); source = 2; }
            // Switching presentation paths cannot manufacture an occurrence.
            if (source != label_source) invalidate();
            label_source = source;
            report_manual_history(now, manual_history.observe(
                std::span<const ManualHistoryRow>(history).first(source == 1 ? diagnostic.rows : 0),
                {now, manager, score_model.handle, score_model.type, source == 1, history_complete}));
            label_edges.observe(labels, true, now);
            report_labels(now, source, diagnostic, labels);
        } catch (...) { invalidate(); label_source = 0; report_labels(now, 0, diagnostic, 0); }
        if (now >= next_description && emitted < 4096) {
            next_description = now + 250;
            unsigned budget = 256;
            inspect(context, score_model, {{0x9b74fc7c}, 1}, 0, budget, now);
        }
    } catch (...) { invalidate(); /* unavailable schema leaves all gameplay untouched */ }
}
}

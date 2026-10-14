#include "arcade_score_probe.h"
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
unsigned emitted{};
std::uintptr_t previous_manager{};
data::Value score_model{};
// Exact allowlist: unrelated text is never logged or hashed into the report.
std::uint32_t label(std::string_view text) noexcept {
    if (text == "Manual") return 1;
    if (text == "Nose Manual") return 2;
    if (text == "FS Revert") return 3;
    if (text == "BS Revert") return 4;
    return 0;
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
    if (kind == 7) current = label(context.text(value, 96));
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
    logging::log(logging::Level::info, logging::Channel::skater,
        "ARCADE SCORE VALUE: ms={} path={} value={}.", now, path, current);
    if (emitted == 4096) logging::log(logging::Level::info, logging::Channel::skater,
        "ARCADE SCORE LIMIT: reached=true.");
}
}
void observe_score_hud(std::uintptr_t base, std::uint64_t now) noexcept {
    if (!base || now < next_poll || emitted >= 4096) return;
    next_poll = now + 250;
    try {
        const auto& functions = game::native_data().models;
        if (!functions.lock || !functions.unlock || !functions.value || !functions.field) return;
        const auto ui = data::read<std::uintptr_t>(base + build::engine::ui_manager);
        const auto manager = ui ? data::read<std::uintptr_t>(ui + 0x140) : 0;
        if (!manager) return;
        game::ModelWriteLock lock(manager);
        const data::Context context(base, manager);
        if (manager != previous_manager) { score_model = {}; previous_manager = manager; previous.clear(); }
        if (!score_model.handle || context.type_of(score_model.handle) != score_model.type) {
            const auto roots = context.roots({build::hud_corner::score_view_schema});
            if (roots.empty()) return;
            score_model = roots.front().model;
        }
        unsigned budget = 256;
        inspect(context, score_model, "9b74fc7c", 0, budget, now);
    } catch (...) { /* unavailable schema leaves all gameplay untouched */ }
}
}

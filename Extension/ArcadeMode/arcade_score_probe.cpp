#include "arcade_score_probe.h"
#include "Engine/Core/Log/logging.h"
#include "Engine/Game/Build/20260929/engine.h"
#include "Engine/Game/Build/20260929/hud_corner.h"
#include "Extension/UI/NativeMenu/native_menu_data.h"
#include <array>
#include <string_view>

namespace dingosdk::arcade_mode {
namespace {
namespace data = multiplayer::menu_data;
namespace build = game::build::v20260929;
struct FieldSample {
    std::uint32_t hash{}, value{};
    unsigned kind{}, size{};
    bool operator==(const FieldSample&) const = default;
};
std::array<FieldSample, 128> previous{};
std::uint64_t next_poll{};
unsigned emitted{};
bool schema_logged{};
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
}
void observe_score_hud(std::uintptr_t base, std::uint64_t now) noexcept {
    if (!base || now < next_poll || emitted >= 1024) return;
    next_poll = now + 250;
    try {
        const auto& functions = game::native_data().models;
        if (!functions.lock || !functions.unlock || !functions.value || !functions.field) return;
        const auto ui = data::read<std::uintptr_t>(base + build::engine::ui_manager);
        const auto manager = ui ? data::read<std::uintptr_t>(ui + 0x140) : 0;
        if (!manager) return;
        game::ModelWriteLock lock(manager);
        const data::Context context(base, manager);
        if (manager != previous_manager) { score_model = {}; previous_manager = manager; }
        if (!score_model.handle || context.type_of(score_model.handle) != score_model.type) {
            const auto roots = context.roots({build::hud_corner::score_view_schema});
            if (roots.empty()) return;
            score_model = roots.front().model;
        }
        const auto model = score_model;
        data::require(data::kind(model.type) == 2, "score schema is not a structure");
        const auto meta = data::read<std::uintptr_t>(model.type);
        const auto count = data::read<std::uint16_t>(meta + 0x2a);
        data::require(count <= previous.size(), "score schema exceeds bounded probe");
        const auto fields = data::read<std::uintptr_t>(meta + 0x60);
        const bool describe = !schema_logged;
        schema_logged = true; // Even a later field read failure must not spam schemas.
        for (unsigned i = 0; i < count && emitted < 1024; ++i) {
            const auto hash = data::read<std::uint32_t>(fields + i * 24ULL);
            const auto member = context.member(model.type, hash);
            FieldSample sample{hash, 0, data::kind(member.type), data::size(member.type)};
            // Metadata only for references/arrays/structures. Never dereference
            // arbitrary nested objects in this first focused discovery pass.
            if (describe) logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE SCORE FIELD: hash={:08x} kind={} size={}.", hash, sample.kind, sample.size);
            const auto field = context.field(model, hash);
            if (sample.kind == 7) {
                try { sample.value = label(context.text(field, 96)); }
                catch (...) { continue; }
            }
            else if (sample.kind == 0 && sample.size == 1)
                sample.value = data::read<std::uint8_t>(context.address(field));
            else continue; // no unknown numeric IDs, pointers or arbitrary text
            if (sample.kind != 7 && sample.value > 1) continue;
            if (sample == previous[i]) continue;
            previous[i] = sample;
            ++emitted;
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE SCORE SAMPLE: ms={} hash={:08x} value={}.", now, hash, sample.value);
        }
        schema_logged = true;
    } catch (...) { /* unavailable schema leaves all gameplay untouched */ }
}
}

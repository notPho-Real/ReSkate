#include "score_multiplier_asset.h"
#include "Engine/Vfs/builtin_packs.h"
#include "Engine/Vfs/local_asset_pack.h"
#include <array>

namespace dingosdk::score_line {
namespace {
void prepare(const std::filesystem::path& game_root, const std::filesystem::path& output) {
    constexpr std::array<std::string_view, 4> sources{
        "ui/features/hud/scoring/scoringmultiplier_widget",
        "ui/features/hud/scoring/scoringhud_default_widget",
        "ui/features/hud/scoring/scoringhudviewmodel",
        "ui/foundations/components/text/label/widget/label_unconstrained_widget"};
    mods::make_local_ebx_pack(game_root, output,
        "Win32/levels/game/bam_levelroot/bam_levelroot.toc",
        "win32/levels/game/bam_levelroot/bam_coregameassets", sources,
        [](std::span<const frostbite::ebx::Document> documents) {
            return build_multiplier_label(documents[0], documents[1], documents[2], documents[3]);
        });
}
}
void register_multiplier_label_pack() {
    mods::register_local_pack("score-multiplier-text-v1", prepare);
}
}

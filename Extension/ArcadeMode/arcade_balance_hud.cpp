#include "arcade_balance_hud.h"
#include "arcade_balance_input.h"
#include "arcade_balance_layout.h"
#include "arcade_balance_hud_lease.h"
#if !defined(DINGOSDK_BALANCE_HUD_TEST_CLOCK)
#include <Windows.h>
#endif
#include <imgui.h>
#include <mutex>

namespace dingosdk::arcade_mode {
#if defined(DINGOSDK_BALANCE_HUD_TEST_CLOCK)
std::uint64_t balance_hud_test_now() noexcept;
#endif
namespace {
std::uint64_t hud_now() noexcept {
#if defined(DINGOSDK_BALANCE_HUD_TEST_CLOCK)
    return balance_hud_test_now();
#else
    return GetTickCount64();
#endif
}
std::mutex hud_mutex;
BalanceMeter meter;
unsigned danger_level{};
BalanceHudSource observed;
}
void publish_balance_hud(const State& state, unsigned danger, BalanceHudSource source) noexcept {
    std::lock_guard lock(hud_mutex);
    const bool finite = std::isfinite(state.balance) && std::isfinite(state.balance_velocity);
    const bool kind = state.balance_kind == BalanceKind::manual || state.balance_kind == BalanceKind::grind;
    meter = finite && kind && BalanceHudLease::fresh(source, hud_now()) ? balance_meter(state) : BalanceMeter{};
    danger_level = danger;
    observed = source;
}
bool balance_hud_pending() noexcept {
    std::lock_guard lock(hud_mutex);
    return meter.visible && BalanceHudLease::fresh(observed, hud_now());
}
void draw_balance_hud() noexcept {
    BalanceMeter view;
    unsigned danger{};
    {
        std::lock_guard lock(hud_mutex);
        if (!BalanceHudLease::fresh(observed, hud_now())) return;
        view = meter;
        danger = danger_level;
    }
    if (!view.visible) return;
    const auto screen = ImGui::GetIO().DisplaySize;
    const auto layout = balance_hud_layout(screen.x, screen.y, view.vertical);
    if (layout.scale <= 0) return;
    const float scale = layout.scale;
    const auto point = [&](float offset, float across) {
        const auto p = layout.point(offset, across);
        return ImVec2(p.x, p.y);
    };
    auto* draw = ImGui::GetBackgroundDrawList();
    const ImVec2 panel_min(layout.panel_min.x, layout.panel_min.y);
    const ImVec2 panel_max(layout.panel_max.x, layout.panel_max.y);
    const ImU32 color = danger >= 2 ? IM_COL32(255, 65, 65, 255) :
        danger == 1 ? IM_COL32(255, 190, 65, 255) : IM_COL32(90, 225, 230, 255);
    draw->AddRectFilled(panel_min, panel_max, IM_COL32(8, 12, 18, 220), 10 * scale);
    draw->AddRect(panel_min, panel_max, danger >= 2 ? color : IM_COL32(150, 165, 180, 210),
                  10 * scale, 0, (danger >= 2 ? 4 : 2) * scale);
    const char* title = danger >= 2 ? "DANGER" : view.vertical ? "MANUAL" : "GRIND";
    const float font_size = 20 * scale;
    const auto text_size = ImGui::GetFont()->CalcTextSizeA(font_size, screen.x, 0, title);
    draw->AddText(ImGui::GetFont(), font_size,
        ImVec2(layout.center.x - text_size.x / 2, panel_min.y + 10 * scale),
        danger >= 2 ? color : IM_COL32(245, 248, 250, 255), title);

    const float track = layout.track_half_width;
    draw->AddLine(point(-1, 0), point(1, 0), IM_COL32(2, 4, 8, 255), (track + 4 * scale) * 2);
    draw->AddLine(point(-1, 0), point(1, 0), IM_COL32(130, 155, 165, 255), track * 2);
    // These zone edges match the model's default tuning; the cursor is always the published model position.
    constexpr Tuning tuning{};
    for (const float sign : {-1.0f, 1.0f}) {
        draw->AddLine(point(sign * tuning.danger_edge, 0), point(sign * tuning.severe_edge, 0),
                      IM_COL32(255, 185, 45, 255), track * 2);
        draw->AddLine(point(sign * tuning.severe_edge, 0), point(sign, 0),
                      IM_COL32(235, 60, 55, 255), track * 2);
        draw->AddLine(point(sign, -18 * scale), point(sign, 18 * scale),
                      IM_COL32(255, 245, 240, 255), 4 * scale);
    }
    draw->AddLine(point(0, -19 * scale), point(0, 19 * scale), IM_COL32(0, 0, 0, 255), 7 * scale);
    draw->AddLine(point(0, -19 * scale), point(0, 19 * scale), IM_COL32(255, 255, 255, 255), 3 * scale);
    const auto cursor = point(view.vertical ? view.cursor_y : view.cursor_x, 0);
    draw->AddCircleFilled(cursor, layout.cursor_radius + 4 * scale, IM_COL32(0, 0, 0, 255));
    draw->AddCircleFilled(cursor, layout.cursor_radius + 2 * scale, IM_COL32(255, 255, 255, 255));
    draw->AddCircleFilled(cursor, layout.cursor_radius, color);
}
}

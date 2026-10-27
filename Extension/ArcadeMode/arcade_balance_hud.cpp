#include "arcade_balance_hud.h"
#include "arcade_balance_input.h"
#include "arcade_balance_layout.h"
#include "arcade_balance_hud_lease.h"
#if !defined(DINGOSDK_BALANCE_HUD_TEST_CLOCK)
#include <Windows.h>
#endif
#include <imgui.h>
#include <atomic>
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
std::atomic<BalanceHudRenderAck> render_ack{};
}
void set_balance_hud_render_ack(BalanceHudRenderAck callback) noexcept {
    render_ack.store(callback,std::memory_order_release);
}
void publish_balance_hud(const State& state, unsigned danger, BalanceHudSource source,
        BalanceEndpoints endpoints, float limit) noexcept {
    std::lock_guard lock(hud_mutex);
    const bool finite = std::isfinite(state.balance) && std::isfinite(state.balance_velocity);
    const bool kind = state.balance_kind == BalanceKind::manual || state.balance_kind == BalanceKind::grind;
    meter = finite && kind && BalanceHudLease::fresh(source, hud_now()) ? balance_meter(state,endpoints,limit) : BalanceMeter{};
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
    BalanceHudSource source;
    {
        std::lock_guard lock(hud_mutex);
        if (!BalanceHudLease::fresh(observed, hud_now())) return;
        view = meter;
        danger = danger_level;
        source = observed;
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
    const bool safe_side = view.vertical && !view.bail_side(view.cursor_y) && view.cursor_y != 0;
    const ImU32 color = safe_side ? IM_COL32(90, 225, 150, 255) : danger >= 2 ? IM_COL32(255, 65, 65, 255) :
        danger == 1 ? IM_COL32(255, 190, 65, 255) : IM_COL32(90, 225, 230, 255);
    draw->AddRectFilled(panel_min, panel_max, IM_COL32(8, 12, 18, 220), 10 * scale);
    draw->AddRect(panel_min, panel_max, danger >= 2 ? color : IM_COL32(150, 165, 180, 210),
                  10 * scale, 0, (danger >= 2 ? 4 : 2) * scale);
    const char* title = view.title();
    const float font_size = (view.vertical && view.endpoints == BalanceEndpoints::negative_bail ? 14 : 20) * scale;
    const auto text_size = ImGui::GetFont()->CalcTextSizeA(font_size, screen.x, 0, title);
    draw->AddText(ImGui::GetFont(), font_size,
        ImVec2(layout.center.x - text_size.x / 2, panel_min.y + 10 * scale),
        danger >= 2 ? color : IM_COL32(245, 248, 250, 255), title);

    const float track = layout.track_half_width;
    draw->AddLine(point(-1, 0), point(1, 0), IM_COL32(2, 4, 8, 255), (track + 4 * scale) * 2);
    draw->AddLine(point(-1, 0), point(1, 0), IM_COL32(130, 155, 165, 255), track * 2);
    // The physical owner supplies the terminal limit and type. Safe putdown is
    // distinct from failure; the cursor remains the unmodified governing lean.
    constexpr Tuning tuning{};
    for (const float sign : {-1.0f, 1.0f}) {
        const bool bail = view.bail_side(sign);
        draw->AddLine(point(sign * tuning.danger_edge, 0), point(sign * tuning.severe_edge, 0),
                      bail ? IM_COL32(255, 185, 45, 255) : IM_COL32(80, 180, 140, 255), track * 2);
        draw->AddLine(point(sign * tuning.severe_edge, 0), point(sign * view.limit, 0),
                      bail ? IM_COL32(235, 60, 55, 255) : IM_COL32(90, 225, 150, 255), track * 2);
        draw->AddLine(point(sign * view.limit, -18 * scale), point(sign * view.limit, 18 * scale),
                      IM_COL32(255, 245, 240, 255), 4 * scale);
        if (view.vertical && view.endpoints != BalanceEndpoints::bail_both) {
            const char* label = bail ? "BAIL" : "PUT DOWN";
            const float size = 12 * scale;
            const auto width = ImGui::GetFont()->CalcTextSizeA(size,screen.x,0,label).x;
            const auto end = point(sign * view.limit,0);
            draw->AddText(ImGui::GetFont(),size,ImVec2(end.x-width/2,end.y+(sign<0?-19:8)*scale),
                bail ? IM_COL32(255, 130, 120, 255) : IM_COL32(130, 245, 180, 255),label);
        }
    }
    draw->AddLine(point(0, -19 * scale), point(0, 19 * scale), IM_COL32(0, 0, 0, 255), 7 * scale);
    draw->AddLine(point(0, -19 * scale), point(0, 19 * scale), IM_COL32(255, 255, 255, 255), 3 * scale);
    const auto cursor = point(view.vertical ? view.cursor_y : view.cursor_x, 0);
    draw->AddCircleFilled(cursor, layout.cursor_radius + 4 * scale, IM_COL32(0, 0, 0, 255));
    draw->AddCircleFilled(cursor, layout.cursor_radius + 2 * scale, IM_COL32(255, 255, 255, 255));
    draw->AddCircleFilled(cursor, layout.cursor_radius, color);
    if(source.capture&&source.request)
        if(const auto callback=render_ack.load(std::memory_order_acquire))
            callback(source,static_cast<std::uint64_t>(ImGui::GetFrameCount()),hud_now());
}
}

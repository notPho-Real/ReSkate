#include "arcade_balance_hud.h"
#include "arcade_balance_input.h"
#include <imgui.h>
#include <mutex>
#include <chrono>

namespace dingosdk::arcade_mode {
namespace {
std::mutex hud_mutex;
BalanceMeter meter;
unsigned danger_level{};
std::chrono::steady_clock::time_point updated;
}
void publish_balance_hud(const State& state, unsigned danger) noexcept {
    std::lock_guard lock(hud_mutex);
    meter = balance_meter(state);
    danger_level = danger;
    updated = std::chrono::steady_clock::now();
}
bool balance_hud_pending() noexcept {
    std::lock_guard lock(hud_mutex);
    return meter.visible && std::chrono::steady_clock::now() - updated <= std::chrono::milliseconds(250);
}
void draw_balance_hud() noexcept {
    BalanceMeter view;
    unsigned danger{};
    {
        std::lock_guard lock(hud_mutex);
        if (std::chrono::steady_clock::now() - updated > std::chrono::milliseconds(250)) return;
        view = meter;
        danger = danger_level;
    }
    if (!view.visible) return;
    const auto screen = ImGui::GetIO().DisplaySize;
    const float scale = screen.y / 800.0f;
    const ImVec2 center(view.vertical ? screen.x * 0.73f : screen.x * 0.5f,
                        view.vertical ? screen.y * 0.5f : screen.y * 0.76f);
    const float half = 95.0f * scale, width = 7.0f * scale;
    const auto point = [&](float offset, float across) {
        return view.vertical ? ImVec2(center.x + across, center.y + offset)
                             : ImVec2(center.x + offset, center.y + across);
    };
    auto* draw = ImGui::GetBackgroundDrawList();
    draw->AddLine(point(-half, 0), point(half, 0), IM_COL32(20, 24, 30, 220), width * 2);
    draw->AddLine(point(-half, 0), point(half, 0), IM_COL32(180, 190, 200, 220), width);
    for (const float end : {-half, half})
        draw->AddLine(point(end, -width), point(end, width), IM_COL32(245, 90, 75, 255), 3 * scale);
    draw->AddLine(point(0, -width), point(0, width), IM_COL32(255, 255, 255, 255), 2 * scale);
    const float position = view.vertical ? view.cursor_y : view.cursor_x;
    const ImU32 color = danger >= 2 ? IM_COL32(255, 65, 65, 255) :
        danger == 1 ? IM_COL32(255, 190, 65, 255) : IM_COL32(90, 225, 230, 255);
    draw->AddCircleFilled(point(position * half, 0), width, color);
}
}

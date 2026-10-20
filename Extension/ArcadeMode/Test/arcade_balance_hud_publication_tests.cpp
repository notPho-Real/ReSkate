#include "Extension/ArcadeMode/arcade_combo_session.h"
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <latch>
#include <limits>
#include <thread>

// This focused target supplies the platform clock and compiles the actual
// publisher/pending/draw implementation, including its mutex and ImGui calls.
namespace {
std::atomic<std::uint64_t> clock_ms{};
unsigned checks{};
}
namespace dingosdk::arcade_mode {
std::uint64_t balance_hud_test_now() noexcept { return clock_ms.load(std::memory_order_acquire); }
}
#define DINGOSDK_BALANCE_HUD_TEST_CLOCK
#include "../arcade_balance_hud.cpp"

namespace {
using namespace dingosdk::arcade_mode;
void require(bool condition, const char* label) {
    ++checks;
    if (!condition) {
        std::cerr << label << '\n';
        std::exit(1);
    }
}
State active() {
    State state;
    state.combo_active = true;
    state.balance_kind = BalanceKind::manual;
    state.balance = .25f;
    return state;
}
void drawn(bool expected, const char* label) {
    ImGui::NewFrame();
    draw_balance_hud();
    ImGui::Render();
    require((ImGui::GetDrawData()->TotalVtxCount > 0) == expected, label);
}
void displayed(bool expected, const char* label) {
    require(balance_hud_pending() == expected, label);
    drawn(expected, label);
}
// Inspect the actual final cursor primitive, including its warning color. A
// new ImGui frame for every observation also detects stale submitted geometry.
void rendered_cursor(BalanceKind kind, float balance, unsigned danger) {
    require(balance_hud_pending(), "fresh admitted balance schedules an overlay frame");
    ImGui::NewFrame();
    draw_balance_hud();
    const auto& vertices = ImGui::GetBackgroundDrawList()->VtxBuffer;
    require(vertices.Size > 0, "fresh admitted balance submits geometry this frame");
    const ImU32 color = danger >= 2 ? IM_COL32(255, 65, 65, 255) :
        danger == 1 ? IM_COL32(255, 190, 65, 255) : IM_COL32(90, 225, 230, 255);
    const ImU32 rgb_mask = ~IM_COL32_A_MASK;
    float left = std::numeric_limits<float>::max(), top = left;
    float right = std::numeric_limits<float>::lowest(), bottom = right;
    unsigned cursor_vertices{};
    for (int i = vertices.Size - 1; i >= 0 && (vertices[i].col & rgb_mask) == (color & rgb_mask); --i) {
        if (!(vertices[i].col & IM_COL32_A_MASK)) continue;
        ++cursor_vertices;
        left = std::min(left, vertices[i].pos.x); right = std::max(right, vertices[i].pos.x);
        top = std::min(top, vertices[i].pos.y); bottom = std::max(bottom, vertices[i].pos.y);
    }
    require(cursor_vertices >= 3, "current danger selects the actual cursor color");
    const auto screen = ImGui::GetIO().DisplaySize;
    const float clamped = std::clamp(balance, -1.0f, 1.0f);
    const float x = kind == BalanceKind::manual ? screen.x * .80f : screen.x * (.5f + .15f * clamped);
    const float y = kind == BalanceKind::manual ? screen.y * (.5f - .15f * clamped) : screen.y * .78f;
    require(std::abs((left + right) / 2 - x) < .75f && std::abs((top + bottom) / 2 - y) < .75f,
            "rendered cursor uses the current kind and signed snapshot position");
    require(std::all_of(vertices.begin(), vertices.end(), [](const ImDrawVert& vertex) {
        return std::isfinite(vertex.pos.x) && std::isfinite(vertex.pos.y);
    }), "rendered geometry stays finite");
    ImGui::Render();
    require(ImGui::GetDrawData()->TotalVtxCount > 0, "balance geometry reaches final ImGui draw data");
}
State snapshot(BalanceKind kind, float balance) {
    auto state = active();
    state.balance_kind = kind;
    state.balance = balance;
    return state;
}
void lifecycle_frames(unsigned hz) {
    ImGui::GetIO().DeltaTime = 1.0f / hz;
    const std::uint64_t base = 30000 + hz * 1000;
    const auto publish = [&](unsigned frame, const State& state, unsigned danger = 0, bool valid = true) {
        const auto now = base + std::uint64_t{frame} * 1000 / hz;
        clock_ms.store(now);
        publish_balance_hud(state, danger, {now, valid});
    };
    unsigned frame{};
    // These are synthetic authoritative display snapshots, not native admission
    // evidence. An Ollie/active combo alone must never invent a balance meter.
    auto air = snapshot(BalanceKind::none, 0);
    for (; frame < hz; ++frame) {
        publish(frame, air);
        displayed(false, "air/combo activity without balance remains hidden");
    }
    for (; frame < 2 * hz; ++frame) {
        publish(frame, snapshot(BalanceKind::manual, .25f));
        rendered_cursor(BalanceKind::manual, .25f, 0);
    }
    // Nose Manual shares the vertical meter; its signed position still changes.
    for (; frame < 3 * hz; ++frame) {
        publish(frame, snapshot(BalanceKind::manual, -.5f), 1);
        rendered_cursor(BalanceKind::manual, -.5f, 1);
    }
    publish(frame++, air);
    displayed(false, "observed manual takeoff removes the meter immediately");
    publish(frame++, snapshot(BalanceKind::grind, -.75f), 2);
    rendered_cursor(BalanceKind::grind, -.75f, 2);
    publish(frame++, snapshot(BalanceKind::manual, .5f));
    rendered_cursor(BalanceKind::manual, .5f, 0);
    publish(frame++, snapshot(BalanceKind::grind, .4f), 1);
    rendered_cursor(BalanceKind::grind, .4f, 1);
    publish(frame++, snapshot(BalanceKind::manual, -.25f), 0, false);
    displayed(false, "invalid source clears a previously visible grind");
    publish(frame++, snapshot(BalanceKind::manual, -.25f));
    rendered_cursor(BalanceKind::manual, -.25f, 0);
    auto invalid = snapshot(BalanceKind::manual, std::numeric_limits<float>::quiet_NaN());
    publish(frame++, invalid);
    displayed(false, "invalid position clears a previously visible manual");
    publish(frame++, snapshot(BalanceKind::grind, .25f));
    rendered_cursor(BalanceKind::grind, .25f, 0);
    invalid = snapshot(BalanceKind::grind, .25f);
    invalid.balance_velocity = std::numeric_limits<float>::infinity();
    publish(frame++, invalid);
    displayed(false, "invalid velocity clears a previously visible grind");
    publish(frame++, snapshot(BalanceKind::manual, -.25f));
    rendered_cursor(BalanceKind::manual, -.25f, 0);
    auto inactive = snapshot(BalanceKind::manual, -.25f);
    inactive.combo_active = false;
    publish(frame++, inactive);
    displayed(false, "bail or completed combo clears the prior manual");
    publish(frame++, snapshot(BalanceKind::manual, .25f), 2);
    rendered_cursor(BalanceKind::manual, .25f, 2);
    publish_balance_hud({}, 0, {});
    displayed(false, "disable/context exit/reset clear publication hides on the same render frame");
    for (unsigned clear = 0; clear < 3; ++clear) {
        clock_ms.store(base + std::uint64_t{frame++} * 1000 / hz);
        displayed(false, "clear cannot leave prior-frame geometry behind");
    }
    // An authoritative continuing section must stay visible. A renderer-only
    // timeout would conceal an upstream native lifecycle failure.
    const auto sustained_end = frame + 10 * hz;
    for (; frame < sustained_end; ++frame) {
        publish(frame, snapshot(BalanceKind::manual, .25f));
        rendered_cursor(BalanceKind::manual, .25f, 0);
    }
    const auto source = clock_ms.load();
    for (unsigned age : {0u, 149u, 150u, 151u, 200u}) {
        clock_ms.store(source + age);
        publish_balance_hud(snapshot(BalanceKind::manual, .25f), 0, {source, true});
        displayed(age <= 150, "source stall expires despite repeated presentation updates");
    }
    frame += hz; // Keep later snapshots beyond the deliberate source stall.
    publish(frame++, snapshot(BalanceKind::manual, .25f));
    require(balance_hud_pending(), "fresh snapshot passes the overlay scheduling gate");
    clock_ms.store(clock_ms.load() + 151);
    drawn(false, "evidence expiring between scheduling and draw cannot submit a stale meter");
    frame += hz;
    // Use the existing session's failed display contract: retain the red edge
    // only while physical exit/bail is pending, then clear on actual exit.
    for (const auto pose : {SkatePose::manual, SkatePose::nose_manual}) {
        ComboSession session;
        session.step(SkatePose::air, 0, 0);
        publish(frame++, session.balance_display_state());
        displayed(false, "observed air session alone cannot activate the renderer");
        const PhysicalBalanceSample balanced{.valid = true, .active = true, .position = .25f};
        session.step(pose, 1.0f / hz, 0, false, &balanced);
        publish(frame++, session.balance_display_state());
        rendered_cursor(BalanceKind::manual, .25f, 0);
        const PhysicalBalanceSample failed{.valid = true,
            .position = pose == SkatePose::manual ? 1.0f : -1.0f, .exit = BalanceExit::bail};
        require(session.step(pose, 1.0f / hz, 0, false, &failed), "session supplies a failed manual snapshot");
        publish(frame++, session.balance_display_state(), 2);
        rendered_cursor(BalanceKind::manual, failed.position, 2);
        session.step(SkatePose::riding, 1.0f / hz, 0);
        publish(frame++, session.balance_display_state());
        displayed(false, "actual native section exit clears the retained failed meter");
        session.invalidate();
        publish(frame++, session.balance_display_state());
        displayed(false, "invalidated session supplies no retained failed display");
    }
}
void delayed_publication(std::uint64_t age, std::uint64_t wait, bool expected) {
    constexpr std::uint64_t sample = 1000;
    clock_ms.store(sample + age);
    std::unique_lock lock(hud_mutex);
    std::latch calling(1);
    std::thread publisher([&] {
        calling.count_down();
        publish_balance_hud(active(), 0, {sample, true});
    });
    calling.wait();
    clock_ms.store(sample + age + wait);
    lock.unlock();
    publisher.join();
    displayed(expected, "mutex delay must spend the original source lease");
}
}
int main() {
    using namespace dingosdk::arcade_mode;
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.DisplaySize = ImVec2(1280, 800);
    io.DeltaTime = 1.0f / 60.0f;
    unsigned char* pixels{};
    int width{}, height{};
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    delayed_publication(149, 2, false);
    delayed_publication(150, 1, false);
    delayed_publication(0, 151, false);
    delayed_publication(149, 1, true);
    delayed_publication(0, 150, true);

    // Pending and draw independently age evidence even without another publish.
    clock_ms.store(2000);
    publish_balance_hud(active(), 0, {2000, true});
    for (const std::uint64_t age : {0u, 149u, 150u, 151u, 300u}) {
        clock_ms.store(2000 + age);
        displayed(age <= 150, "draw and pending use the same immutable source");
    }
    // Repeated client publication cannot renew unchanged physical evidence.
    for (const std::uint64_t age : {149u, 150u, 151u, 160u, 300u, 10000u}) {
        clock_ms.store(3000 + age);
        publish_balance_hud(active(), 2, {3000, true});
        displayed(age <= 150, "unchanged evidence cannot renew presentation");
    }
    clock_ms.store(14000);
    publish_balance_hud(active(), 1, {14000, true});
    displayed(true, "fresh native evidence can establish a new lease");
    clock_ms.store(14151);
    displayed(false, "new lease still expires from its source");
    publish_balance_hud(active(), 0, {14151, true});
    displayed(true, "another physical sample is independently fresh");

    clock_ms.store(20000);
    for (const auto source : {BalanceHudSource{}, BalanceHudSource{20001, true},
                              BalanceHudSource{UINT64_MAX, true}, BalanceHudSource{20000, false}}) {
        publish_balance_hud(active(), 0, source);
        displayed(false, "invalid and future sources hide");
    }
    for (const float value : {std::numeric_limits<float>::quiet_NaN(),
                              std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()}) {
        auto state = active();
        state.balance = value;
        publish_balance_hud(state, 0, {20000, true});
        displayed(false, "nonfinite balance hides");
        state = active();
        state.balance_velocity = value;
        publish_balance_hud(state, 0, {20000, true});
        displayed(false, "nonfinite velocity hides");
    }
    auto state = active();
    state.combo_active = false;
    publish_balance_hud(state, 0, {20000, true});
    displayed(false, "inactive balance hides");
    state = active();
    state.balance_kind = BalanceKind::none;
    publish_balance_hud(state, 0, {20000, true});
    displayed(false, "missing balance kind hides");
    state.balance_kind = static_cast<BalanceKind>(255);
    publish_balance_hud(state, 0, {20000, true});
    displayed(false, "invalid balance kind hides");
    state = active();
    state.balance_kind = BalanceKind::grind;
    publish_balance_hud(state, 0, {20000, true});
    displayed(true, "grind presentation uses the same source lease");
    publish_balance_hud({}, 0, {});
    displayed(false, "explicit clearing hides immediately");

    // Compare before subtracting: no wrap, narrowing or sentinel-age aliasing.
    clock_ms.store(UINT64_MAX);
    publish_balance_hud(active(), 0, {UINT64_MAX - 150, true});
    displayed(true, "large timestamp at inclusive endpoint");
    publish_balance_hud(active(), 0, {UINT64_MAX - 151, true});
    displayed(false, "large timestamp after endpoint");
    clock_ms.store(150);
    publish_balance_hud(active(), 0, {0, true});
    displayed(true, "zero clock origin is valid with explicit validity");
    clock_ms.store(151);
    displayed(false, "zero origin expires normally");
    require(BalanceHudLease::fresh(std::chrono::milliseconds(150)), "existing inclusive lease helper");
    require(!BalanceHudLease::fresh(std::chrono::milliseconds(-1)), "existing negative lease rejection");
    require(!BalanceHudLease::fresh(std::chrono::milliseconds(151)), "existing stale lease rejection");

    lifecycle_frames(30);
    lifecycle_frames(60);
    ImGui::DestroyContext();
    std::cout << checks << " HUD publication, pending and actual draw checks passed\n";
}

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
void displayed(bool expected, const char* label) {
    require(balance_hud_pending() == expected, label);
    const auto before = ImGui::GetBackgroundDrawList()->VtxBuffer.Size;
    draw_balance_hud();
    require((ImGui::GetBackgroundDrawList()->VtxBuffer.Size > before) == expected, label);
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
    ImGui::NewFrame();

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

    ImGui::EndFrame();
    ImGui::DestroyContext();
    std::cout << checks << " HUD publication, pending and actual draw checks passed\n";
}

#pragma once
#include <array>
#include <cstdint>

namespace dingosdk::arcade_mode {
// Raw diagnostics only: these words are not a command API or gameplay events.
struct NativeRequestSample {
    bool valid{};
    std::uint32_t state{}, crouch_mode{};
    std::array<std::uint32_t, 8> words{};
    bool operator==(const NativeRequestSample&) const = default;
};

// One short, bounded capture per explicit A press. No unbounded per-frame log.
class NativeRequestCapture {
public:
    static constexpr std::uint64_t duration_ms = 5000;
    static constexpr unsigned limit = 128;
    void arm(std::uint64_t now) noexcept {
        started_ = now; previous_ = {}; have_previous_ = false; count_ = 0; active_ = true;
    }
    bool sample(std::uint64_t now, NativeRequestSample value) noexcept {
        if (!active_) return false;
        if (now < started_ || now - started_ >= duration_ms || count_ >= limit) {
            active_ = false; return false;
        }
        if (!value.valid) value = {}; // Never retain fields after ownership loss.
        if (have_previous_ && value == previous_) return false;
        previous_ = value; have_previous_ = true; ++count_;
        return true;
    }
    const NativeRequestSample& last() const noexcept { return previous_; }
    unsigned count() const noexcept { return count_; }
    bool active() const noexcept { return active_; }
private:
    std::uint64_t started_{};
    NativeRequestSample previous_{};
    unsigned count_{};
    bool have_previous_{}, active_{};
};
}

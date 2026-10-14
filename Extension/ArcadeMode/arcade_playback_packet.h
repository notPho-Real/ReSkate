#pragma once
#include <cstdint>

namespace dingosdk::arcade_mode {
// Reference playback publishes a new packet on every game-facing poll, even
// when several reads share one simulation tick. No physical packet is mutated.
class PlaybackPacket {
public:
    std::uint32_t sample(std::uint32_t physical, std::uint32_t normal, bool reference) noexcept {
        if (reference) {
            if (!owned_) last_ = physical;
            owned_ = true;
            return ++last_;
        }
        if (owned_) {
            owned_ = false;
            // Make cancellation visible even when the physical counter caught up.
            if (normal == last_) return normal + 1;
        }
        return normal;
    }
private:
    std::uint32_t last_{};
    bool owned_{};
};
}

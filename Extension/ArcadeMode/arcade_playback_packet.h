#pragma once
#include <cstdint>

namespace dingosdk::arcade_mode {
// Reference playback publishes a new packet on every game-facing poll, even
// when several reads share one simulation tick. No physical packet is mutated.
class PlaybackPacket {
public:
    // Retain one cancellation identity across owner/mode reset. This carries
    // no input authority; the next actual delivery consumes it exactly once.
    bool owned()const noexcept{return owned_;}
    bool retiring()const noexcept{return retiring_;}
    void retire() noexcept {
        if (owned_) { retired_last_ = last_; retiring_ = true; owned_ = false; }
    }
    std::uint32_t sample(std::uint32_t physical, std::uint32_t normal, bool reference) noexcept {
        auto output = normal;
        if (reference) {
            if (!owned_) last_ = physical;
            owned_ = true;
            output = ++last_;
        } else if (owned_) {
            owned_ = false;
            // Make cancellation visible even when the physical counter caught up.
            if (output == last_) ++output;
        }
        if (retiring_) {
            retiring_ = false;
            if (output == retired_last_) {
                ++output;
                if (reference) last_ = output;
            }
        }
        return output;
    }
private:
    std::uint32_t last_{}, retired_last_{};
    bool owned_{}, retiring_{};
};
}

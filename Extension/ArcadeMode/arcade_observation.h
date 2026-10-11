#pragma once
#include <cstdint>

namespace dingosdk::arcade_mode {
// Diagnostic samples, not gameplay events: polling can skip transitions and
// the runtime reader does not promise a transactional snapshot of these fields.
struct Observation {
    bool valid{};
    std::uint32_t state{};
    std::uint64_t changes{}, wipeouts{};
    bool operator==(const Observation&) const = default;
};
class ObservationTracker {
public:
    bool update(Observation sample) noexcept {
        if (!sample.valid || sample.state == 0) sample = {};
        const bool changed = sample != last_;
        last_ = sample;
        return changed;
    }
    const Observation& state() const noexcept { return last_; }
private:
    Observation last_{};
};
}

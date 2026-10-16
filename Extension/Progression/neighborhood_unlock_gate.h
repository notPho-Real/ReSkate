#pragma once
#include <atomic>

namespace dingosdk {
// Hook availability is separate from authority to change progression. Only the
// existing provider preparation path may grant that authority; observers cannot.
class NeighborhoodUnlockGate {
    std::atomic<bool> provider_prepared_{}, enabled_{};
public:
    void provider_prepared(bool hook_prepared) noexcept {
        if (hook_prepared) provider_prepared_.store(true, std::memory_order_release);
    }
    bool prepared(bool hook_prepared) const noexcept {
        return hook_prepared && provider_prepared_.load(std::memory_order_acquire);
    }
    bool request(bool requested, bool hook_prepared) noexcept {
        if (!prepared(hook_prepared)) return false;
        enabled_.store(requested, std::memory_order_release);
        return true;
    }
    bool enabled() const noexcept { return enabled_.load(std::memory_order_acquire); }
};
}

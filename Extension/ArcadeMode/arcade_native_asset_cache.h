#pragma once
#include <span>

namespace dingosdk::arcade_mode {
// A loaded map can release an asset and reuse its address for another GUID.
// A successful lookup is therefore not a permanent readiness latch. Keep
// validation separate from the throttled lookup so stale caches fail closed
// immediately, including while the next lookup is waiting for its deadline.
template<class Asset, std::size_t Size, class Matches>
bool native_assets_current(std::span<Asset,Size> assets, Matches matches) noexcept {
    for (const auto& asset : assets)
        if (!asset.object || !matches(asset.object,asset.guid)) return false;
    return true;
}
}

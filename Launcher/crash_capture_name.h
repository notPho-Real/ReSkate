#pragma once
#include <atomic>
#include <cstdint>
#include <string>

namespace dingosdk::crash_report {
inline std::wstring capture_file_stem(std::uint32_t process, std::uint64_t tick, std::uint32_t thread) {
    // A stuck-load capture can finish and crash on the same thread before the
    // Windows tick advances. Each capture must own distinct dump sidecars too.
    static std::atomic<std::uint64_t> sequence{};
    return std::to_wstring(process) + L"-" + std::to_wstring(tick) + L"-" +
        std::to_wstring(thread) + L"-" + std::to_wstring(sequence.fetch_add(1, std::memory_order_relaxed));
}
}

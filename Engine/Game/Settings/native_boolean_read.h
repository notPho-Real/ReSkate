#pragma once
#include <atomic>
#include <optional>
#include <string_view>

namespace dingosdk::settings {
// A scoped consumer may borrow a boolean for one native read. The consumer
// must validate its key and execution context; no stored value is changed.
// This internal adapter is not a plugin API and exposes no native addresses.
class NativeBooleanRead {
public:
    using Filter = std::optional<bool> (*)(unsigned, std::string_view, bool) noexcept;
    bool attach(Filter filter) noexcept {
        if (!filter) return false;
        Filter expected{};
        return filter_.compare_exchange_strong(expected, filter) || expected == filter;
    }
    void detach(Filter filter) noexcept { (void)filter_.compare_exchange_strong(filter, nullptr); }
    std::optional<bool> apply(unsigned location, std::string_view key, bool original) const noexcept {
        const auto filter = filter_.load();
        return filter ? filter(location, key, original) : std::nullopt;
    }
private:
    std::atomic<Filter> filter_{};
};
inline NativeBooleanRead& native_boolean_read() { static NativeBooleanRead value; return value; }
}

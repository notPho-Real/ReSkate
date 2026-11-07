#pragma once
#include <cstdint>

namespace dingosdk::crash_report {
struct DumpWriteResult {
    std::uint32_t error{};
    unsigned attempts{};
    bool reset_failed{};
};
// A live target can change while DbgHelp reads it. Retry only its partial-copy
// HRESULT, with the same capture flags and a cleared temporary file. Other
// failures and a persistent partial read remain failures, never published dumps.
template<class Write, class Reset>
DumpWriteResult write_dump_retry(Write&& write, Reset&& reset, std::uint32_t partial_copy) {
    for (unsigned attempt = 1; ; ++attempt) {
        const auto error = write();
        if (!error || error != partial_copy || attempt == 3) return {error, attempt, false};
        if (const auto reset_error = reset()) return {reset_error, attempt, true};
    }
}
}

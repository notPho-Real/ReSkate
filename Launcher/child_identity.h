#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace dingosdk::launcher_app::identity {
struct Observation {
    std::uint32_t pid{};
    std::uint64_t created{}, observed{}, tick{};
    std::int32_t utc_offset_seconds{};
    bool alive{}, image_matches{}, module_matches{};
};
inline bool hex(std::string_view value, std::size_t length) {
    if (value.size() != length) return false;
    for (const auto c : value) if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    return true;
}
inline void validate(const Observation& first, const Observation& last,
                     std::uint32_t pid, std::uint64_t created) {
    if (!pid || !created || first.pid != pid || last.pid != pid || first.created != created || last.created != created ||
        !first.alive || !last.alive || !first.image_matches || !last.image_matches ||
        !first.module_matches || !last.module_matches || first.observed < created || last.observed < first.observed ||
        last.tick < first.tick || last.tick - first.tick > 5000 || last.observed - first.observed > 50000000 ||
        first.utc_offset_seconds != last.utc_offset_seconds || first.utc_offset_seconds < -86400 || first.utc_offset_seconds > 86400)
        throw std::runtime_error("Process identity observation unavailable or changed");
    // A wall-clock adjustment cannot masquerade as a valid monotonic bracket.
    const auto wall_ms = (last.observed - first.observed) / 10000;
    const auto tick_ms = last.tick - first.tick;
    if (wall_ms > tick_ms + 100 || tick_ms > wall_ms + 100)
        throw std::runtime_error("Process observation clock changed");
}
inline std::string response(const Observation& first, const Observation& last,
                            std::string_view module_hash, std::string_view context, std::string_view challenge) {
    validate(first, last, first.pid, first.created);
    if (!hex(module_hash, 64) || !hex(context, 64) || !hex(challenge, 32))
        throw std::runtime_error("Invalid process observation binding");
    return "{\"schema\":1,\"windows_pid\":" + std::to_string(first.pid) +
        ",\"created_100ns\":" + std::to_string(first.created) +
        ",\"before_100ns\":" + std::to_string(first.observed) + ",\"after_100ns\":" + std::to_string(last.observed) +
        ",\"tick_before_ms\":" + std::to_string(first.tick) + ",\"tick_after_ms\":" + std::to_string(last.tick) +
        ",\"log_utc_offset_seconds\":" + std::to_string(first.utc_offset_seconds) +
        ",\"runtime_file_sha256\":\"" + std::string(module_hash) + "\",\"context_sha256\":\"" + std::string(context) +
        "\",\"challenge\":\"" + std::string(challenge) +
        "\",\"alive\":true,\"image_matches\":true,\"module_matches\":true,\"loaded_memory_identity\":\"UNVERIFIED\"}\n";
}
}

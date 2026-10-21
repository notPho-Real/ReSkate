#pragma once
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace dingosdk::logging::detail {
// Caller holds the shared interprocess lock. No handle survives append(), so a
// second writer cannot keep writing into a renamed archive after rotation.
class RotatingFile {
public:
    RotatingFile(std::filesystem::path path, std::uintmax_t capacity, unsigned archives)
        : path_(std::move(path)), capacity_(capacity), archives_(archives) {
        if (!capacity || archives > 2) throw std::runtime_error("Invalid log retention");
    }
    void validate() const {
        for (unsigned i = 0; i <= archives_; ++i) {
            const auto p = slot(i);
            if (std::filesystem::is_symlink(p)) throw std::runtime_error("Unsafe log slot");
            if (std::filesystem::exists(p) && (!std::filesystem::is_regular_file(p) ||
                std::filesystem::file_size(p) > capacity_))
                throw std::runtime_error("Existing log exceeds retention; preserve before maintenance");
        }
        // Startup rotation must reject an interrupted active record before it
        // can rename the file or evict any archive, just as append does.
        if (std::filesystem::exists(path_) && std::filesystem::file_size(path_)) {
            std::ifstream input(path_, std::ios::binary);
            input.seekg(-1, std::ios::end);
            char last{};
            if (!input.get(last) || last != '\n') throw std::runtime_error("Partial log record; preserve evidence");
        }
    }
    void rotate() {
        validate();
        if (!std::filesystem::exists(path_)) return;
        if (archives_ == 0) throw std::runtime_error("Log retention cannot rotate");
        // Failed renames stop the writer. The active evidence is never truncated.
        std::filesystem::remove(slot(archives_));
        for (unsigned i = archives_; i > 1; --i)
            if (std::filesystem::exists(slot(i - 1))) std::filesystem::rename(slot(i - 1), slot(i));
        std::filesystem::rename(path_, slot(1));
    }
    void start_session(bool rotate_existing) {
        validate();
        if (rotate_existing) rotate();
    }
    void append(std::string_view record) {
        if (failed_) throw std::runtime_error("Log writer requires maintenance after failure");
        try {
            validate();
            if (record.empty() || record.back() != '\n' || record.size() > capacity_)
                throw std::runtime_error("Incomplete or oversized log record");
            const auto size = std::filesystem::exists(path_) ? std::filesystem::file_size(path_) : 0;
            if (record.size() > capacity_ - size) rotate();
            std::ofstream output(path_, std::ios::binary | std::ios::app);
            output.write(record.data(), static_cast<std::streamsize>(record.size()));
            output.flush();
            if (!output) throw std::runtime_error("Incomplete log write");
            output.close();
            if (!output) throw std::runtime_error("Cannot close log write");
        } catch (...) { failed_ = true; throw; }
    }
private:
    std::filesystem::path slot(unsigned index) const {
        if (!index) return path_;
        auto p = path_; p += "." + std::to_string(index); return p;
    }
    std::filesystem::path path_;
    std::uintmax_t capacity_;
    unsigned archives_;
    bool failed_{};
};
inline constexpr std::uintmax_t log_segment_bytes = 2 * 1024 * 1024;
inline constexpr std::uintmax_t session_segment_bytes = 32 * 1024;
}

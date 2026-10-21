#include "Engine/Core/Log/rotating_file.h"
#include "Engine/Core/Log/types.h"
#include <iostream>
#include <iterator>
#include <functional>
#include <sstream>
#include <type_traits>

namespace fs = std::filesystem;
using dingosdk::logging::detail::RotatingFile;
unsigned checks{};
void check_at(unsigned line, bool value, const char* reason = "Rotation regression failed: expected=true actual=false") {
    ++checks;
    if (!value) throw std::runtime_error(std::string(reason) + " (check " +
        std::to_string(checks) + ", line " + std::to_string(line) + ")");
}
#define check(...) check_at(__LINE__, __VA_ARGS__)
template<class T> std::string display(const T& value) {
    if constexpr (std::is_convertible_v<T, std::string_view>) {
        std::string output = "\"";
        for (const char c : std::string_view(value)) {
            if (c == '\r') output += "\\r";
            else if (c == '\n') output += "\\n";
            else output += c;
        }
        return output + "\"";
    } else {
        std::ostringstream output; output << value; return output.str();
    }
}
template<class A, class E> void equal_at(unsigned line, const A& actual, const E& expected) {
    if (actual == expected) { check_at(line, true); return; }
    const auto reason = "Rotation regression failed: expected=" + display(expected) + " actual=" + display(actual);
    check_at(line, false, reason.c_str());
}
#define check_equal(actual, expected) equal_at(__LINE__, actual, expected)
void rejects(const std::function<void()>& call) {
    bool rejected{}; try { call(); } catch (const std::exception&) { rejected = true; }
    check(rejected);
}
std::string read(const fs::path& path) {
    std::ifstream f(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    const fs::path root = argv[1];
    if (!fs::create_directory(root)) return 2;
    try {
        const auto path = root / "ReSkate.log";
        RotatingFile a(path, 64, 2), b(path, 64, 2);
        const std::string first = std::string(31, 'a') + '\n', second = std::string(31, 'b') + '\n';
        a.append(first); b.append(second); check_equal(fs::file_size(path), 64u);
        a.append(first); check_equal(read(path), first); check_equal(read(root / "ReSkate.log.1"), first + second);
        b.append(second); b.append(second); // b must reopen the new active path.
        check_equal(read(path), second); check_equal(read(root / "ReSkate.log.1"), first + second);
        check_equal(read(root / "ReSkate.log.2"), first + second);
        for (unsigned i = 0; i < 100; ++i) {
            (i % 2 ? a : b).append(first);
            std::uintmax_t total{};
            for (const auto& entry : fs::directory_iterator(root)) {
                check(entry.file_size() <= 64); total += entry.file_size();
            }
            check(total <= 3 * 64);
        }
        const auto before = read(path);
        rejects([&] { a.append("partial"); }); check_equal(read(path), before);
        rejects([&] { a.append(first); }); check_equal(read(path), before);
        RotatingFile oversized(root / "oversized.log", 64, 2);
        rejects([&] { oversized.append(std::string(64, 'x') + '\n'); });
        check(!fs::exists(root / "oversized.log"));
        // Fixtures assert exact on-disk bytes, including LF and sparse offsets.
        // Match the production binary writer; text mode translates LF on Windows.
        const auto startup_partial = root / "startup-partial.log";
        { std::ofstream f(startup_partial, std::ios::binary); f << "interrupted"; }
        { std::ofstream f(root / "startup-partial.log.1", std::ios::binary); f << "previous-session\n"; }
        { std::ofstream f(root / "startup-partial.log.2", std::ios::binary); f << "oldest-session\n"; }
        RotatingFile startup_guard(startup_partial, 64, 2);
        bool startup_rejected{};
        try { startup_guard.start_session(true); } catch (const std::exception&) { startup_rejected = true; }
        check(startup_rejected, "startup rotation must reject a partial active record before moving or evicting evidence");
        check_equal(read(startup_partial), "interrupted");
        check_equal(read(root / "startup-partial.log.1"), "previous-session\n");
        check_equal(read(root / "startup-partial.log.2"), "oldest-session\n");
        rejects([&] { startup_guard.append(first); });
        const auto empty = root / "empty-session.log";
        { std::ofstream f(empty, std::ios::binary); }
        RotatingFile empty_start(empty, 64, 2); empty_start.start_session(true);
        empty_start.append("new-session\n"); check_equal(read(empty), "new-session\n");
        const auto broken = root / "partial.log";
        { std::ofstream f(broken, std::ios::binary); f << "interrupted record"; }
        RotatingFile partial(broken, 64, 2);
        rejects([&] { partial.append(first); }); check_equal(read(broken), "interrupted record");
        const auto huge = root / "old-evidence.log";
        { std::ofstream f(huge, std::ios::binary); f.seekp(58'469'398); f << '\n'; }
        RotatingFile preserved(huge, 64, 2);
        rejects([&] { preserved.rotate(); }); check_equal(fs::file_size(huge), 58'469'399u);
        const auto archive = root / "archive.log.2";
        { std::ofstream f(archive, std::ios::binary); f << std::string(65, 'x'); }
        RotatingFile guarded(root / "archive.log", 64, 2);
        rejects([&] { guarded.append(first); }); check_equal(fs::file_size(archive), 65u);
        // Explicit startup rotation preserves the previous complete session.
        RotatingFile startup(path, 64, 2); const auto prior = read(path);
        startup.rotate(); check(!fs::exists(path)); check_equal(read(root / "ReSkate.log.1"), prior);
        startup.append("new-session\n"); check_equal(read(path), "new-session\n");
        // Main rollover never removes independent, bounded identity records.
        RotatingFile identity(root / "ReSkate.session.log", 64, 1);
        identity.append("pid=1 created=1 role=runtime hash=old\n");
        identity.append("pid=2 created=2 role=runtime hash=new\n");
        startup.rotate(); startup.append(first);
        check_equal(read(root / "ReSkate.session.log"), "pid=2 created=2 role=runtime hash=new\n");
        check_equal(read(root / "ReSkate.session.log.1"), "pid=1 created=1 role=runtime hash=old\n");
        check(dingosdk::logging::default_level == dingosdk::logging::Level::warning);
        check(dingosdk::logging::parse_level("warning") == dingosdk::logging::Level::warning);
        static_assert(dingosdk::logging::detail::log_segment_bytes * 3 +
            dingosdk::logging::detail::session_segment_bytes * 2 + 16 * 1024 < 16 * 1024 * 1024);
        std::cout << checks << " rotating log checks passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; fs::remove_all(root); return 1;
    }
    fs::remove_all(root);
}

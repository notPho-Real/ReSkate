#include "Engine/Core/Log/rotating_file.h"
#include "Engine/Core/Log/types.h"
#include <iostream>
#include <iterator>
#include <functional>

namespace fs = std::filesystem;
using dingosdk::logging::detail::RotatingFile;
unsigned checks{};
void check(bool value, const char* reason = "Rotation regression failed") { ++checks; if (!value) throw std::runtime_error(reason); }
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
        a.append(first); b.append(second); check(fs::file_size(path) == 64);
        a.append(first); check(read(path) == first); check(read(root / "ReSkate.log.1") == first + second);
        b.append(second); b.append(second); // b must reopen the new active path.
        check(read(path) == second); check(read(root / "ReSkate.log.1") == first + second);
        check(read(root / "ReSkate.log.2") == first + second);
        for (unsigned i = 0; i < 100; ++i) {
            (i % 2 ? a : b).append(first);
            std::uintmax_t total{};
            for (const auto& entry : fs::directory_iterator(root)) {
                check(entry.file_size() <= 64); total += entry.file_size();
            }
            check(total <= 3 * 64);
        }
        const auto before = read(path);
        rejects([&] { a.append("partial"); }); check(read(path) == before);
        rejects([&] { a.append(first); }); check(read(path) == before);
        RotatingFile oversized(root / "oversized.log", 64, 2);
        rejects([&] { oversized.append(std::string(64, 'x') + '\n'); });
        check(!fs::exists(root / "oversized.log"));
        const auto startup_partial = root / "startup-partial.log";
        { std::ofstream f(startup_partial); f << "interrupted"; }
        { std::ofstream f(root / "startup-partial.log.1"); f << "previous-session\n"; }
        { std::ofstream f(root / "startup-partial.log.2"); f << "oldest-session\n"; }
        RotatingFile startup_guard(startup_partial, 64, 2);
        bool startup_rejected{};
        try { startup_guard.start_session(true); } catch (const std::exception&) { startup_rejected = true; }
        check(startup_rejected, "startup rotation must reject a partial active record before moving or evicting evidence");
        check(read(startup_partial) == "interrupted");
        check(read(root / "startup-partial.log.1") == "previous-session\n");
        check(read(root / "startup-partial.log.2") == "oldest-session\n");
        rejects([&] { startup_guard.append(first); });
        const auto empty = root / "empty-session.log";
        { std::ofstream f(empty); }
        RotatingFile empty_start(empty, 64, 2); empty_start.start_session(true);
        empty_start.append("new-session\n"); check(read(empty) == "new-session\n");
        const auto broken = root / "partial.log";
        { std::ofstream f(broken); f << "interrupted record"; }
        RotatingFile partial(broken, 64, 2);
        rejects([&] { partial.append(first); }); check(read(broken) == "interrupted record");
        const auto huge = root / "old-evidence.log";
        { std::ofstream f(huge); f.seekp(58'469'398); f << '\n'; }
        RotatingFile preserved(huge, 64, 2);
        rejects([&] { preserved.rotate(); }); check(fs::file_size(huge) == 58'469'399);
        const auto archive = root / "archive.log.2";
        { std::ofstream f(archive); f << std::string(65, 'x'); }
        RotatingFile guarded(root / "archive.log", 64, 2);
        rejects([&] { guarded.append(first); }); check(fs::file_size(archive) == 65);
        // Explicit startup rotation preserves the previous complete session.
        RotatingFile startup(path, 64, 2); const auto prior = read(path);
        startup.rotate(); check(!fs::exists(path)); check(read(root / "ReSkate.log.1") == prior);
        startup.append("new-session\n"); check(read(path) == "new-session\n");
        // Main rollover never removes independent, bounded identity records.
        RotatingFile identity(root / "ReSkate.session.log", 64, 1);
        identity.append("pid=1 created=1 role=runtime hash=old\n");
        identity.append("pid=2 created=2 role=runtime hash=new\n");
        startup.rotate(); startup.append(first);
        check(read(root / "ReSkate.session.log") == "pid=2 created=2 role=runtime hash=new\n");
        check(read(root / "ReSkate.session.log.1") == "pid=1 created=1 role=runtime hash=old\n");
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

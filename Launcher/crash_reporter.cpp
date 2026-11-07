#include <Windows.h>
#include "crash_reporter.h"
#include "crash_capture_name.h"
#include "crash_dump_retry.h"
#include "crash_dump_snapshot.h"
#include <DbgHelp.h>
#include "Engine/Core/Debug/protocol.h"
#include "Engine/Core/Debug/upload.h"
#include "Engine/Core/Debug/native_dump.h"
#include "Engine/Core/Debug/report_envelope.h"
#include "Engine/Core/Debug/report_details.h"
#include "Engine/Vfs/mod_list.h"
#include "Engine/Game/Build/supported_build.h"
#include "crash_report_config.h"
#include "reskate_version.h"
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <format>
#include <mutex>
#include <thread>
#include <vector>

namespace {
using namespace dingosdk;
using namespace dingosdk::crash_report;
std::string utf8(const wchar_t* text) {
    const auto size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) return {};
    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), size, nullptr, nullptr);
    result.pop_back();
    return result;
}
template<std::size_t N> std::string field(const std::array<wchar_t, N>& value) {
    return utf8(std::wstring(value.data(), wcsnlen(value.data(), N)).c_str());
}
// "Levels/Game/DingoLevel_Root/DingoLevel_Root + levels/game/x/x @ Start" -> "x": the sublevel when
// there is one, else the level, by its last name.
std::string map_name(std::string destination) {
    if (const auto at = destination.find(" @ "); at != std::string::npos) destination.resize(at);
    if (const auto plus = destination.find(" + "); plus != std::string::npos) destination.erase(0, plus + 3);
    if (const auto slash = destination.find_last_of('/'); slash != std::string::npos) destination.erase(0, slash + 1);
    return destination;
}
// The folder the game runs from (its Mods folder beside it), or empty. Read while the game runs:
// a native crash is collected only after the game has exited, and Windows may refuse to name an
// exited process's image (error 31, seen on Windows 11 build 26200). (Never throws: a report goes
// without its mods list rather than this helper not starting.)
std::filesystem::path game_folder(HANDLE process) noexcept {
    try {
        std::array<wchar_t, 32768> image{};
        DWORD length = static_cast<DWORD>(image.size());
        if (!QueryFullProcessImageNameW(process, 0, image.data(), &length)) return {};
        return std::filesystem::path(std::wstring(image.data(), length)).parent_path();
    } catch (...) { return {}; }
}
// The event fields a report carries besides its tags (report_details.h): the log's last lines,
// the mods the game ran with, an anonymous ID for this install and how long the game had run.
Json event_details(HANDLE process, const std::filesystem::path& game_root, const SharedReport& report,
    const std::filesystem::path& dump, std::size_t& mod_count) {

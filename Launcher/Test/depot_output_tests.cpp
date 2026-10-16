// DepotDownloader output: progress, QR encodings and updates across successive lines.
#include "Launcher/depot_output.h"
#include "Launcher/depot_login.h"

#include <cmath>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

using dingosdk::launcher_gui::DepotOutput;

namespace {

int failures = 0;

void check(bool condition, const char* what) {
    if (condition) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

constexpr std::string_view announcement = "Please sign in with this QR code:";

// A deterministic 21 x 21 module pattern with a quiet zone at either side.
// These fixtures test the text transport, not whether a QR code can be scanned.
std::vector<std::string> qr_modules(bool alternate = false) {
    std::vector<std::string> rows;
    for (int y = 0; y < 21; ++y) {
        std::string row = "  ";
        for (int x = 0; x < 21; ++x) row += (x + y + alternate) % 3 ? ' ' : '#';
        rows.push_back(row + "  ");
    }
    return rows;
}

std::string qr_line(std::string_view modules, std::string_view block) {
    std::string line;
    for (char module : modules) {
        if (module == '#') { line += block; line += block; }
        else line += "  ";
    }
    return line;
}

void feed_rows(DepotOutput& output, const std::vector<std::string>& rows, std::string_view block) {
    for (const auto& row : rows) {
        output.feed(qr_line(row, block));
        check(!output.qr_update && !output.percent, "Collecting rows does not publish a partial QR code or progress");
    }
}

} // namespace

int main() {
    using dingosdk::launcher_update::DepotQrAccount;
    using dingosdk::launcher_update::depot_login_options;
    check(depot_login_options(true, true, false) == L" -qr -remember-password",
          "QR sign-in honors the remembered-session preference");
    check(depot_login_options(true, false, true) == L" -qr",
          "One-time QR sign-in neither persists nor requests a typed code");
    check(depot_login_options(false, true, true) == L" -remember-password -no-mobile",
          "Account sign-in retains remembered-session and typed-code options");
    check(depot_login_options(false, false, false).empty(), "One-time account sign-in has no extra options");
    const std::string hint = "Success! Next time you can login with -username sample_skater -remember-password instead of -qr.";
    {
        DepotQrAccount account;
        check(!account.remembered(true, true, true, false), "No hint cannot manufacture a remembered account");
        const auto safe = account.feed(hint);
        check(safe.find("sample_skater") == std::string::npos, "QR account hint is removed from log and display text");
        check(account.remembered(true, true, true, false) == "sample_skater",
              "Successful remembered QR download supplies the next account-name sign-in");
        check(account.feed("Logging 'sample_skater' into Steam3...") == "Logging '<hidden>' into Steam3...",
              "Reconnection output does not expose the QR account name");
        check(!account.remembered(true, true, false, false), "Failed download cannot save the candidate account");
        check(!account.remembered(true, true, true, true), "Cancelled download cannot save the candidate account");
        check(!account.remembered(true, false, true, false), "Remember-me opt-out is respected");
        check(!account.remembered(false, true, true, false), "Account sign-in cannot replace its user from a QR hint");
        account.feed(hint);
        check(account.remembered(true, true, true, false) == "sample_skater", "Duplicate hints are harmless");
        account.feed("Success! Next time you can login with -username other_skater -remember-password instead of -qr.");
        check(!account.remembered(true, true, true, false), "Conflicting hints cannot select an account");
    }
    for (const auto& name : {std::string(), std::string("bad name"), std::string("name\n"),
                            std::string("../name"), std::string(65, 'x')}) {
        DepotQrAccount account;
        const auto safe = account.feed("Success! Next time you can login with -username " + name +
                                      " -remember-password instead of -qr.");
        check(safe == "Steam QR sign-in returned an account.", "Malformed hints are not echoed");
        check(!account.remembered(true, true, true, false), "Malformed account is not remembered");
        account.feed(hint);
        check(!account.remembered(true, true, true, false), "Later valid hint cannot repair an ambiguous run");
    }
    {
        DepotQrAccount account;
        check(account.feed("12.34% C:\\Games\\Skate\\file.bin") == "12.34% C:\\Games\\Skate\\file.bin",
              "Authentication handling preserves normal progress input");
        account.feed("Success! Next time you can login with -username truncated");
        check(!account.remembered(true, true, true, false), "Truncated hint is unresolved");
    }
    for (const auto* cause : {"Failed to authenticate with Steam: timed out", "Unable to login to Steam3: ServiceUnavailable",
                             "Access token was rejected (Expired).", "Could not connect to Steam after 10 tries"}) {
        DepotOutput output;
        output.feed(cause);
        output.feed("Error: InitializeSteam failed");
        check(output.failure == cause, "Generic Steam initialization failure retains its specific cause");
        DepotOutput next;
        next.feed("Error: InitializeSteam failed");
        check(next.failure.empty(), "A new attempt cannot inherit the previous attempt's failure cause");
    }
    // ------------------------------------------------ progress and retained messages
    {
        DepotOutput output;
        check(!output.percent && !output.qr_update && output.file.empty() && output.message.empty(),
              "A new parser has no output");
        output.feed("\t Connecting to Steam \t");
        check(output.message == "Connecting to Steam", "Ordinary messages are trimmed");
        output.feed(R"(12.34% C:\Games\Skate\file.bin)");
        check(output.percent && std::abs(*output.percent - 12.34f) < 0.001f && output.file == "file.bin",
              "Windows progress extracts the percentage and basename");
        check(output.message == "Connecting to Steam", "Progress retains the last ordinary message for failure reporting");
        output.feed(" \t50.00% /games/Skate/a file with spaces.bin \t");
        check(output.percent == 50.0f && output.file == "a file with spaces.bin",
              "Unix paths, spaces in filenames and surrounding whitespace are supported");
        output.feed("0.00% file.bin");
        check(output.percent == 0.0f && output.file == "file.bin", "Zero progress and a bare filename are supported");
        output.feed("100.00% /games/Skate/done.bin");
        check(output.percent == 100.0f && output.file == "done.bin", "One hundred percent is supported");
        output.feed(" \t ");
        check(!output.percent && !output.qr_update && output.file == "done.bin" && output.message == "Connecting to Steam",
              "Blank input resets transient updates and retains the last filename and message");
        output.feed("Download complete");
        check(!output.percent && output.message == "Download complete", "Ordinary output replaces the message after progress");
        const std::string long_message = std::string(160, 'a') + "end";
        output.feed(" \t" + long_message + " \t");
        check(output.message == long_message.substr(0, 160), "Long messages are trimmed and limited to 160 bytes");
        output.feed("Short message");
        check(output.message == "Short message", "A short message replaces a truncated one");
    }

    // ------------------------------------------------ malformed progress regression
    // The caller divides percent by 100 and passes it directly to the progress bar.
    // std::stof alone accepts non-finite values, out-of-range values and numeric prefixes.
    for (const char* line : {
            "NaN% file.bin", "inf% file.bin", "-inf% file.bin", "-1% file.bin",
            "100.01% file.bin", "999999% file.bin", "1e999% file.bin",
            "12oops% file.bin", "12.3.4% file.bin", "12 % file.bin",
            "% file.bin", "oops% file.bin", "12.34%", "12.34%   ",
            "12.34%file.bin", "12.34%\tfile.bin", "Saved 12.34% of files"}) {
        DepotOutput output;
        output.feed("25% previous.bin");
        output.feed(line);
        check(!output.percent, line);
        auto expected = std::string_view(line);
        while (expected.back() == ' ') expected.remove_suffix(1);
        check(output.file == "previous.bin" && output.message == expected,
              "Invalid progress is an ordinary message and does not replace the filename");
        output.feed("75% next.bin");
        check(output.percent == 75.0f && output.file == "next.bin", "Valid progress resumes after malformed input");
    }

    // ------------------------------------------------ supported QR encodings and dismissal
    const auto rows = qr_modules();
    for (std::string_view block : {std::string_view("\xE2\x96\x88"), std::string_view("\xDB"), std::string_view("?")}) {
        DepotOutput output;
        output.feed("10% old.bin");
        output.feed(announcement);
        check(!output.percent && !output.qr_update, "A QR announcement resets transient progress");
        output.feed("");
        output.feed(std::string(50, ' '));
        check(!output.qr_update, "Leading empty rows and quiet-zone spaces do not finish a QR code");
        feed_rows(output, rows, block);
        output.feed(std::string(50, ' '));
        check(output.qr_update && *output.qr_update == rows, "All block encodings preserve dark modules, spaces and row order");
        output.feed("");
        check(!output.qr_update, "Blank output after completion consumes the update without dismissing the displayed QR code");
        output.feed("QR code has changed");
        check(!output.qr_update, "A refresh notice keeps the displayed QR code until its replacement arrives");
        output.feed("Logged in to Steam");
        check(output.qr_update && output.qr_update->empty() && output.message == "Logged in to Steam",
              "Ordinary output dismisses the QR code with an empty update and supplies its message");
        output.feed("Depot download starting");
        check(!output.qr_update && output.message == "Depot download starting", "QR dismissal is emitted only once");
    }

    // ------------------------------------------------ QR restarts, refreshes and later sessions
    {
        DepotOutput output;
        output.feed(announcement);
        output.feed("????");
        output.feed(announcement);
        const auto replacement = qr_modules(true);
        for (std::size_t y = 0; y < replacement.size(); ++y) {
            output.feed(qr_line(replacement[y], "?"));
            if (y == 0 || y == 19) {
                output.feed("");
                output.feed("QR code has changed");
            }
            check(!output.qr_update, "Empty rows and refresh notices before 21 rows keep collection active");
        }
        output.feed("");
        check(output.qr_update && *output.qr_update == replacement, "A repeated announcement discards previously collected rows");
        output.feed("QR code has changed");
        output.feed(announcement);
        feed_rows(output, rows, "\xE2\x96\x88");
        output.feed("");
        check(output.qr_update && *output.qr_update == rows, "A refreshed QR code replaces the old one without leaking rows");
        output.feed("12.34% /games/Skate/new.bin");
        check(output.qr_update && output.qr_update->empty() && output.percent && output.file == "new.bin",
              "Progress can dismiss a QR code and report a download in the same update");
        output.feed(announcement);
        feed_rows(output, replacement, "\xDB");
        output.feed("");
        check(output.qr_update && *output.qr_update == replacement && !output.percent,
              "A later QR session starts with fresh rows and no stale progress");
    }

    // ------------------------------------------------ interrupted and malformed QR output
    for (const char* interruption : {"Download cancelled", "??x??", "\xE2\x96"}) {
        DepotOutput output;
        output.feed(announcement);
        output.feed("????");
        output.feed(interruption);
        check((!output.qr_update || output.qr_update->empty()) && output.message == interruption,
              "Non-module output ends collection and is handled as an ordinary message");
        output.feed("????");
        check(!output.qr_update && output.message == "????", "QR-looking text is ordinary output after an interruption");
        output.feed(announcement);
        feed_rows(output, rows, "?");
        output.feed("");
        check(output.qr_update && *output.qr_update == rows, "Interrupted rows do not leak into a later QR code");
    }

    if (failures) {
        std::cerr << failures << " DepotDownloader output check(s) failed\n";
        return 1;
    }
    std::cout << "DepotDownloader output checks passed.\n";
    return 0;
}

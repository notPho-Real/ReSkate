#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace dingosdk::launcher_update {

inline std::wstring depot_login_options(bool qr, bool remember, bool prefer_code) {
    std::wstring options = qr ? L" -qr" : L"";
    // QR sessions need the same persistence request as account-name sessions.
    if (remember) options += L" -remember-password";
    if (!qr && prefer_code) options += L" -no-mobile";
    return options;
}

// DepotDownloader's explicit QR success hint supplies an account name, never a
// password or token. Keep it separate from display/log text and only save it
// after a successful download. Missing or conflicting hints remain unresolved.
class DepotQrAccount {
public:
    std::string feed(std::string_view line) {
        constexpr std::string_view prefix = "Success! Next time you can login with -username ";
        constexpr std::string_view suffix = " -remember-password instead of -qr.";
        if (!line.starts_with(prefix)) {
            std::string safe(line);
            if (!account_.empty())
                for (auto at = safe.find(account_); at != std::string::npos; at = safe.find(account_, at + 8))
                    safe.replace(at, account_.size(), "<hidden>");
            return safe;
        }
        const auto value = line.substr(prefix.size());
        if (!value.ends_with(suffix)) {
            ambiguous_ = true;
        } else {
            const auto name = value.substr(0, value.size() - suffix.size());
            if (name.empty() || name.size() > 64 || name.find_first_not_of(
                    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") != std::string_view::npos
                || (!account_.empty() && account_ != name)) {
                ambiguous_ = true;
            } else {
                account_ = name;
            }
        }
        return "Steam QR sign-in returned an account.";
    }

    std::optional<std::string> remembered(bool qr, bool remember, bool succeeded, bool cancelled) const {
        if (!qr || !remember || !succeeded || cancelled || ambiguous_ || account_.empty()) return {};
        return account_;
    }

private:
    std::string account_;
    bool ambiguous_{};
};

} // namespace dingosdk::launcher_update

#pragma once
#include <cstdint>
#include <optional>

namespace dingosdk {
// Bounded SDK-side lifecycle for the native impact-request method. Delivery
// follows the GPL-3.0 reference in lebigtee/ReSkate-BIGT-Fork at
// d5f6e47ec4c527720da0f02ae5534d9e43bfe72d; native state selection stays native.
enum class NativeBailResult : unsigned {
    idle, queued, delivered, observed_wipeout, rejected, expired, cancelled, owner_lost, protected_skater
};
struct NativeBailStatus { std::uint64_t serial{}; NativeBailResult result{}; };
template<class Owner> class NativeBailRequest {
public:
    struct Ticket { Owner owner; std::uint64_t serial{}; };
    NativeBailStatus status() const noexcept { return {serial_, result_}; }
    std::optional<Ticket> ticket() const noexcept {
        return result_ == NativeBailResult::queued ? std::optional(Ticket{owner_, serial_}) : std::nullopt;
    }
    std::uint64_t queue(const Owner& requested_owner, std::uint64_t now) noexcept {
        if (result_ == NativeBailResult::delivered ||
            (result_ == NativeBailResult::queued && now < until_)) return 0;
        owner_ = requested_owner;
        until_ = now + 300;
        ++serial_;
        result_ = NativeBailResult::queued;
        return serial_;
    }
    bool take(const Ticket& ticket, const Owner& current, std::uint64_t now, bool protected_skater) noexcept {
        if (ticket.serial != serial_ || ticket.owner != owner_ || result_ != NativeBailResult::queued) return false;
        if (now >= until_) { result_ = NativeBailResult::expired; return false; }
        if (current != owner_) { result_ = NativeBailResult::owner_lost; return false; }
        if (protected_skater) { result_ = NativeBailResult::protected_skater; return false; }
        result_ = NativeBailResult::delivered;
        return true;
    }
    void finish(std::uint64_t serial, bool issued, std::uint32_t native_state) noexcept {
        if (serial != serial_ || result_ != NativeBailResult::delivered) return;
        result_ = issued && native_state == 300 ? NativeBailResult::observed_wipeout : NativeBailResult::rejected;
    }
    void cancel() noexcept {
        if (result_ == NativeBailResult::queued) result_ = NativeBailResult::cancelled;
    }
private:
    Owner owner_{};
    std::uint64_t serial_{}, until_{};
    NativeBailResult result_{};
};
}

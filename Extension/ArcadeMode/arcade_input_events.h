#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include "arcade_input_policy.h"

namespace dingosdk::arcade_mode {
enum class InputSource : std::uint8_t { buttons, stick, pad, triggers, vertical_stick, vertical_pad, direction };
enum class InputEdge : std::uint8_t { press, release };
struct InputToken {
    InputSource source{};
    std::int16_t code{};
    InputEdge edge{};
    bool operator==(const InputToken&) const = default;
};
struct InputEvent {
    InputToken token{};
    std::uint64_t at{}, sequence{};
    std::uint32_t owners{};
};
struct InputPattern {
    std::span<const InputToken> steps;
    std::uint64_t window_ms{};
    std::uint32_t reject_owners{~std::uint32_t{0}};
    bool allow_intervening{true};
    bool simultaneous{};
    bool inclusive_boundary{};
};
struct InputMatch {
    std::array<std::uint64_t, 100> events{};
    std::uint64_t first_at{}, last_at{};
    std::size_t count{};
    std::uint64_t held_reuse{};
    explicit operator bool() const noexcept { return count != 0; }
};
// Allocation-free event journal. Timestamps and sequence identities survive ring
// rotation; consumption is transactional and never represented by held state.
class InputHistory {
public:
    static constexpr std::size_t capacity = 100;
    void reset() noexcept { size_ = start_ = 0; now_ = 0; clocked_ = false; }
    void clear_intent() noexcept { size_ = start_ = 0; }
    bool advance(std::uint64_t now) noexcept {
        if (clocked_ && now < now_) { reset(); now_ = now; clocked_ = true; return false; }
        now_ = now; clocked_ = true;
        return true;
    }
    void record(InputToken token, std::uint64_t now) noexcept {
        if (!advance(now)) return;
        if (size_ == capacity) { erase_front(); ++overflows_; }
        at(size_++) = {token, now, ++sequence_, 0};
    }
    struct EventView {
        const InputHistory* journal;
        struct Iterator {
            const InputHistory* journal;
            std::size_t index;
            const InputEvent& operator*() const noexcept { return journal->at(index); }
            Iterator& operator++() noexcept { ++index; return *this; }
            bool operator!=(const Iterator& other) const noexcept { return index != other.index; }
        };
        Iterator begin() const noexcept { return {journal, 0}; }
        Iterator end() const noexcept { return {journal, journal->size_}; }
        std::size_t size() const noexcept { return journal->size_; }
        bool empty() const noexcept { return journal->size_ == 0; }
        const InputEvent& operator[](std::size_t index) const noexcept { return journal->at(index); }
        const InputEvent& back() const noexcept { return journal->at(journal->size_ - 1); }
    };
    EventView events() const noexcept { return {this}; }
    std::uint64_t overflows() const noexcept { return overflows_; }
    InputMatch match(const InputPattern& pattern, std::uint64_t now) const noexcept {
        InputMatch result{};
        if (pattern.steps.empty() || pattern.steps.size() > 6) return {};
        // Latest complete match wins. Recognition age starts at the first
        // required event and is measured at query time, including delayed polls.
        for (std::size_t end = size_; end > 0; --end) {
            const auto& last = at(end - 1);
            if (last.at > now || (pattern.inclusive_boundary ? now - last.at > pattern.window_ms : now - last.at >= pattern.window_ms) ||
                last.token != pattern.steps.back() || (last.owners & pattern.reject_owners)) continue;
            result = {}; result.count = pattern.steps.size(); result.last_at = last.at;
            result.events[result.count - 1] = last.sequence;
            std::size_t wanted = result.count - 1;
            for (std::size_t i = end - 1; i > 0 && wanted; ) {
                const auto& event = at(--i);
                if (pattern.inclusive_boundary ? now - event.at > pattern.window_ms : now - event.at >= pattern.window_ms) break;
                if (pattern.simultaneous && event.at != last.at) break;
                if (event.token == pattern.steps[wanted - 1] && !(event.owners & pattern.reject_owners)) {
                    result.events[--wanted] = event.sequence;
                    result.first_at = event.at;
                } else if (!pattern.allow_intervening) break;
            }
            if (!wanted) {
                if (result.count == 1) result.first_at = last.at;
                return result;
            }
        }
        return {};
    }
    InputMatch overlap(InputToken a, InputToken b, std::uint64_t window,
                       std::uint64_t now, bool require_current = false,
                       bool a_held = false, bool b_held = false,
                       std::uint32_t reject = ~std::uint32_t{0}) const noexcept {
        if (require_current && (!a_held || !b_held)) return {};
        InputMatch found{};
        bool seen_a{}, seen_b{};
        for (std::size_t i = size_; i > 0; ) {
            const auto& event = at(--i);
            if (event.at > now || now - event.at >= window) break;
            if (event.owners & reject) continue;
            const bool is_a = event.token.source == a.source && event.token.code == a.code;
            const bool is_b = event.token.source == b.source && event.token.code == b.code;
            if (is_a || is_b) {
                if (event.token.edge == InputEdge::release && require_current) continue;
                // The footprint includes every encountered relevant transition,
                // including candidates preceding a historical release barrier.
                if (!found.count) found.last_at = event.at;
                found.events[found.count++] = event.sequence;
                if (event.token.edge == InputEdge::release) { seen_a = seen_b = false; continue; }
                seen_a = seen_a || is_a; seen_b = seen_b || is_b;
            } else if (require_current && event.token.edge == InputEdge::press && (seen_a || seen_b)) return {};
            if (seen_a && seen_b) {
                found.first_at = event.at;
                for (std::size_t j = 0; j < found.count / 2; ++j) {
                    const auto value = found.events[j];
                    found.events[j] = found.events[found.count - 1 - j];
                    found.events[found.count - 1 - j] = value;
                }
                return found;
            }
        }
        return {};
    }
    InputMatch held_pair_then_press(InputToken a, InputToken b, InputToken c,
                                    std::uint64_t window, std::uint64_t now,
                                    std::uint32_t reject = ~std::uint32_t{0}) const noexcept {
        InputMatch result{};
        bool found_c{}, found_a{}, found_b{};
        std::uint64_t a_sequence{}, b_sequence{}, c_sequence{}, c_at{};
        for (std::size_t i = size_; i > 0; ) {
            const auto& event = at(--i);
            if (event.at > now || now - event.at >= window) return {};
            if (event.owners & reject) continue;
            if (!found_c) {
                if (event.token == c) { found_c = true; c_sequence = event.sequence; c_at = event.at; }
                continue;
            }
            const bool is_a = event.token.source == a.source && event.token.code == a.code;
            const bool is_b = event.token.source == b.source && event.token.code == b.code;
            if ((is_a && !found_a) || (is_b && !found_b)) {
                if (event.token.edge == InputEdge::release) return {};
                if (is_a) { found_a = true; a_sequence = event.sequence; }
                if (is_b) { found_b = true; b_sequence = event.sequence; }
            }
            if (found_a && found_b) {
                result.count = 3; result.first_at = event.at; result.last_at = c_at;
                result.events = {a_sequence < b_sequence ? a_sequence : b_sequence,
                    a_sequence < b_sequence ? b_sequence : a_sequence, c_sequence};
                return result;
            }
        }
        return {};
    }
    static bool direction_token(InputToken token) noexcept {
        return token.source == InputSource::stick || token.source == InputSource::pad || token.source == InputSource::direction;
    }
    static bool cardinal_token(InputToken token) noexcept {
        return direction_token(token) && (token.code == 2 || token.code == 4 || token.code == 6 || token.code == 8);
    }
    InputMatch air_action(InputToken face, InputToken direction, bool direction_held,
                          std::uint64_t window, std::uint64_t now,
                          std::uint32_t reject = ~std::uint32_t{0}) const noexcept {
        if (!direction_held) {
            // This branch shares the press-only intervening rule; it does not
            // require the historical pair to remain physically held.
            return overlap(face, direction, window, now, true, true, true, reject);
        }
        const std::array steps{face};
        auto result = match({steps, window, reject}, now);
        if (!result) return {};
        for (std::size_t i = size_; i > 0; ) {
            const auto& event = at(--i);
            if (event.token == direction) {
                result.events[result.count++] = event.sequence;
                result.held_reuse = event.sequence;
                if (event.at < result.first_at) result.first_at = event.at;
                if (event.at > result.last_at) result.last_at = event.at;
                break;
            }
        }
        return result;
    }
    InputMatch release_and_tap(InputToken first, InputToken second, std::uint64_t window,
                               std::uint64_t now, std::uint32_t reject = ~std::uint32_t{0}) const noexcept {
        std::array<const InputEvent*, 3> recent{};
        std::size_t count{};
        for (std::size_t i = size_; i > 0 && count < recent.size(); ) {
            const auto& event = at(--i);
            const auto source = event.token.source;
            // Vertical projections and raw pad bits are compatibility views,
            // not additional reference events in this predicate's channel.
            if ((event.owners & reject) || cardinal_token(event.token) ||
                source == InputSource::vertical_stick || source == InputSource::vertical_pad ||
                (source == InputSource::buttons && event.token.code < 4)) continue;
            recent[count++] = &event;
        }
        if (count != recent.size()) return {};
        first.edge = InputEdge::release;
        const InputToken released{second.source, second.code, InputEdge::release};
        if (recent[0]->token != released || recent[1]->token != second || recent[2]->token != first) return {};
        InputMatch result{}; result.count = 3;
        for (std::size_t i = 0; i < recent.size(); ++i) {
            const auto& event = *recent[recent.size() - 1 - i];
            if (event.at > now || now - event.at >= window) return {};
            result.events[i] = event.sequence;
        }
        result.first_at = recent[2]->at; result.last_at = recent[0]->at;
        return result;
    }
    InputMatch released_pair(InputToken a, InputToken b, std::uint64_t window, std::uint64_t now,
                             InputToken press = {}, bool require_press = false,
                             std::uint32_t reject = ~std::uint32_t{0}) const noexcept {
        a.edge = b.edge = InputEdge::release;
        bool got_a{}, got_b{}, got_press = !require_press;
        InputMatch result{};
        for (std::size_t i = size_; i > 0; ) {
            const auto& event = at(--i);
            if (event.at > now || now - event.at >= window) return {};
            if (event.owners & reject) continue;
            bool selected{};
            if (!got_a && event.token == a) { got_a = true; selected = true; }
            if (!got_b && event.token == b) { got_b = true; selected = true; }
            if (!got_press && event.token == press) { got_press = true; selected = true; }
            if (selected) {
                if (!result.count) result.last_at = event.at;
                result.events[result.count++] = event.sequence; result.first_at = event.at;
            }
            if (event.token.edge == InputEdge::press &&
                ((got_a && !got_b && event.token.source == a.source && event.token.code == a.code) ||
                 (got_b && !got_a && event.token.source == b.source && event.token.code == b.code))) return {};
            if (got_a && got_b && got_press) return result;
        }
        return {};
    }
    InputMatch extra_action(InputToken face, InputToken previous_direction, std::uint64_t window,
                            std::uint64_t now, std::uint32_t reject = ~std::uint32_t{0}) const noexcept {
        InputMatch result{};
        const InputEvent* selected{};
        for (std::size_t i = size_; i > 0; ) {
            const auto& event = at(--i);
            if (event.at > now || now - event.at >= window) break;
            if ((event.owners & reject) || event.token.edge != InputEdge::press) continue;
            if (event.token == face) selected = &event;
            else if (direction_token(event.token)) {
                if (event.token != previous_direction) return {};
                result.events[result.count++] = event.sequence;
            }
        }
        if (!selected) return {};
        result.events[result.count++] = selected->sequence;
        result.first_at = result.last_at = selected->at;
        for (std::size_t i = 0; i < size_; ++i)
            for (std::size_t m = 0; m < result.count; ++m) if (at(i).sequence == result.events[m]) {
                if (at(i).at < result.first_at) result.first_at = at(i).at;
                if (at(i).at > result.last_at) result.last_at = at(i).at;
            }
        return result;
    }
    void invalidate(InputSource source, std::uint32_t category) noexcept {
        for (std::size_t i = 0; i < size_; ++i)
            if (at(i).token.source == source) at(i).owners |= category;
    }
    void restore(std::uint32_t category, std::uint64_t now, std::uint64_t window) noexcept {
        for (std::size_t i = 0; i < size_; ++i)
            if (now >= at(i).at && now - at(i).at < window) at(i).owners &= ~category;
    }
    InputMatch triple(InputToken a, InputToken b, InputToken c, std::uint64_t window,
                      unsigned predicate, std::uint64_t now, std::uint32_t reject = ~std::uint32_t{0}) const noexcept {
        // 0: ordered presses. 1: first two released before the next press.
        // 2: first released, final two presses in either order.
        const InputToken release_a{a.source, a.code, InputEdge::release};
        const InputToken release_b{b.source, b.code, InputEdge::release};
        if (predicate == 0) {
            const std::array steps{a, b, c}; return match({steps, window, reject}, now);
        }
        if (predicate == 1) {
            const std::array steps{a, release_a, b, release_b, c}; return match({steps, window, reject}, now);
        }
        if (predicate == 2) {
            const std::array forward{a, release_a, b, c};
            const std::array reverse{a, release_a, c, b};
            const auto one = match({forward, window, reject}, now);
            const auto two = match({reverse, window, reject}, now);
            return two && (!one || two.events[two.count - 1] > one.events[one.count - 1]) ? two : one;
        }
        return {};
    }
    bool consume(const InputMatch& match, std::uint32_t owner,
                 std::uint32_t reject_owners = ~std::uint32_t{0}) noexcept {
        if (!match || !owner || match.count > match.events.size()) return false;
        std::array<std::size_t, InputHistory::capacity> indices{};
        for (std::size_t m = 0; m < match.count; ++m) {
            bool found{};
            for (std::size_t i = 0; i < size_; ++i) if (at(i).sequence == match.events[m]) {
                if (at(i).sequence != match.held_reuse && (at(i).owners & reject_owners)) return false;
                indices[m] = i; found = true; break;
            }
            if (!found) return false;
        }
        for (std::size_t m = 0; m < match.count; ++m) at(indices[m]).owners |= owner;
        return true;
    }
private:
    InputEvent& at(std::size_t logical) noexcept { return events_[(start_ + logical) % capacity]; }
    const InputEvent& at(std::size_t logical) const noexcept { return events_[(start_ + logical) % capacity]; }
    void erase_front() noexcept { start_ = (start_ + 1) % capacity; --size_; }
    std::array<InputEvent, capacity> events_{};
    std::size_t size_{}, start_{};
    std::uint64_t sequence_{}, now_{}, overflows_{};
    bool clocked_{};
};
struct PhysicalInputSample {
    std::uint16_t buttons{};
    std::int16_t x{}, y{};
    std::uint8_t lt{}, rt{};
};
// One physical baseline is independent of whether intent is currently eligible.
// Unknown sessions are baselined without edges; conflicts discard intent only.
class InputEvents {
public:
    void reset() noexcept {
        history_.reset(); previous_ = {}; stick_ = pad_ = vertical_stick_ = vertical_pad_ = canonical_ = 0; armed_ = false; held_clocks_ = {};
    }
    void discard_intent() noexcept { history_.clear_intent(); }
    void sample(std::uint64_t now, PhysicalInputSample input, bool balance_exclusions = false, bool manual_owned = false, InputPolicy policy = {}) noexcept {
        if (policy != policy_) { reset(); policy_ = policy; }
        if (policy_.normalization == InputNormalization::analog) { sample_analog(now, input, balance_exclusions); return; }
        balance_exclusions_ = balance_exclusions;
        const bool continuous = history_.advance(now);
        if (!armed_ || !continuous) {
            if (!continuous) held_clocks_ = {};
            previous_ = input; armed_ = true; stick_ = quantize(input.x, input.y);
            const auto pad = input.buttons & 15u;
            pad_ = ((pad & 3u) == 3u || (pad & 12u) == 12u) ? 0 :
                encode((pad & 8u ? 1 : 0) - (pad & 4u ? 1 : 0),
                       (pad & 1u ? 1 : 0) - (pad & 2u ? 1 : 0));
            vertical_stick_ = vertical(input.x, input.y, 0);
            vertical_pad_ = (input.buttons & 3u) == 1 ? 1 : (input.buttons & 3u) == 2 ? -1 : 0;
            observe_held(now); return;
        }
        // Native balance correction does not advance the manual command
        // projection. General direction/button held state still updates.
        if (!manual_owned) {
            direction(now, InputSource::vertical_stick, vertical(input.x, input.y, vertical_stick_), vertical_stick_);
            direction(now, InputSource::vertical_pad, (input.buttons & 3u) == 1 ? 1 : (input.buttons & 3u) == 2 ? -1 : 0, vertical_pad_);
        }
        direction(now, InputSource::stick, quantize(input.x, input.y), stick_);
        const auto pad = input.buttons & 15u;
        const int px = (pad & 8u ? 1 : 0) - (pad & 4u ? 1 : 0);
        const int py = (pad & 1u ? 1 : 0) - (pad & 2u ? 1 : 0);
        direction(now, InputSource::pad, ((pad & 3u) == 3u || (pad & 12u) == 12u) ? 0 : encode(px, py), pad_);
        for (unsigned bit = 0; bit < 16; ++bit) {
            const auto mask = static_cast<std::uint16_t>(std::uint16_t{1} << bit);
            if (balance_exclusions_ && bit < 2) continue;
            if ((input.buttons ^ previous_.buttons) & mask)
                history_.record({InputSource::buttons, static_cast<std::int16_t>(bit),
                    input.buttons & mask ? InputEdge::press : InputEdge::release}, now);
        }
        for (unsigned trigger = 0; trigger < 2; ++trigger) {
            const bool held = (trigger ? input.rt > 128 : input.lt > 30);
            const bool was = (trigger ? previous_.rt > 128 : previous_.lt > 30);
            if (held != was) history_.record({InputSource::triggers, static_cast<std::int16_t>(trigger),
                held ? InputEdge::press : InputEdge::release}, now);
        }
        previous_ = input; observe_held(now);
    }
    static std::int16_t vertical(std::int16_t x, std::int16_t y, std::int16_t previous) noexcept {
        const int v = y < 0 ? -static_cast<int>(y) : y;
        const int h = x < 0 ? -static_cast<int>(x) : x;
        const auto sign = static_cast<std::int16_t>(y > 0 ? 1 : y < 0 ? -1 : 0);
        return v >= (sign == previous ? 8000 : 11000) && v * 4 >= h * 3 ? sign : std::int16_t{0};
    }
    void reset_manual_stick_baseline() noexcept { vertical_stick_ = 0; held_clocks_[21] = {}; }
    std::int16_t vertical_direction(bool pad) const noexcept {
        if (policy_.normalization == InputNormalization::analog) {
            if (pad != ((previous_.buttons & 15u) != 0)) return 0;
            return canonical_ == encode(0, 1) ? std::int16_t{1} : canonical_ == encode(0, -1) ? std::int16_t{-1} : std::int16_t{0};
        }
        return pad ? vertical_pad_ : vertical_stick_;
    }
    bool held(InputToken token) const noexcept {
        if (!armed_) return false;
        switch (token.source) {
        case InputSource::buttons:
            return token.code >= 0 && token.code < 16 &&
                (previous_.buttons & (std::uint16_t{1} << static_cast<unsigned>(token.code))) != 0;
        case InputSource::triggers: return token.code == 0 ? previous_.lt > 30 : token.code == 1 && previous_.rt > 128;
        case InputSource::stick: return token.code != 0 && stick_ == token.code;
        case InputSource::pad: return token.code != 0 && pad_ == token.code;
        case InputSource::vertical_stick: return token.code != 0 && vertical_stick_ == token.code;
        case InputSource::direction: return token.code != 0 && canonical_ == token.code;
        case InputSource::vertical_pad: return token.code != 0 && vertical_pad_ == token.code;
        }
        return false;
    }
    bool held_longer(InputToken token, std::uint64_t duration, std::uint64_t now) const noexcept {
        const auto index = held_index(token);
        if (index >= held_clocks_.size() || !held(token)) return false;
        const auto& clock = held_clocks_[index];
        return clock.value == held_value(token) && now >= clock.since && now - clock.since > duration;
    }
    bool held_all(std::span<const InputToken> tokens) const noexcept {
        for (auto token : tokens) if (!held(token)) return false;
        return !tokens.empty();
    }
    std::int16_t held_direction() const noexcept { return policy_.normalization == InputNormalization::analog ? canonical_ : (previous_.buttons & 15u) ? pad_ : stick_; }
    InputHistory& history() noexcept { return history_; }
    const InputHistory& history() const noexcept { return history_; }
    static constexpr std::int16_t encode(int x, int y) noexcept {
        return static_cast<std::int16_t>(x == 0 && y == 0 ? 0 : 1 + (y + 1) * 3 + x + 1);
    }
private:
    static int held_value(InputToken token) noexcept {
        return token.code + (token.source == InputSource::vertical_stick || token.source == InputSource::vertical_pad ? 2 : 1);
    }
    static std::size_t held_index(InputToken token) noexcept {
        switch (token.source) {
        case InputSource::buttons: return token.code >= 0 && token.code < 16 ? static_cast<std::size_t>(token.code) : 23;
        case InputSource::triggers: return token.code >= 0 && token.code < 2 ? 16u + static_cast<std::size_t>(token.code) : 23;
        case InputSource::stick: return 18; case InputSource::pad: return 19; case InputSource::direction: return 20;
        case InputSource::vertical_stick: return 21; case InputSource::vertical_pad: return 22;
        }
        return 23;
    }
    void observe_held(std::uint64_t now) noexcept {
        const auto update = [&](std::size_t index, int value) noexcept {
            auto& clock = held_clocks_[index];
            if (clock.value != value) { clock.value = value; clock.since = now; }
        };
        for (unsigned bit = 0; bit < 16; ++bit)
            update(bit, previous_.buttons & (std::uint16_t{1} << bit) ? static_cast<int>(bit) + 1 : 0);
        update(16, previous_.lt > 30 ? 1 : 0); update(17, previous_.rt > 128 ? 2 : 0);
        update(18, stick_ ? stick_ + 1 : 0); update(19, pad_ ? pad_ + 1 : 0); update(20, canonical_ ? canonical_ + 1 : 0);
        update(21, vertical_stick_ ? vertical_stick_ + 2 : 0); update(22, vertical_pad_ ? vertical_pad_ + 2 : 0);
    }
    static std::int16_t analog_direction(PhysicalInputSample input) noexcept {
        const auto pad = input.buttons & 15u;
        const int x = pad ? (pad & 8u ? 1 : pad & 4u ? -1 : 0) : input.x >= 13057 ? 1 : input.x <= -12800 ? -1 : 0;
        const int y = pad ? (pad & 2u ? -1 : pad & 1u ? 1 : 0) : input.y >= 13057 ? 1 : input.y <= -12800 ? -1 : 0;
        return encode(x, y);
    }
    void sample_analog(std::uint64_t now, PhysicalInputSample input, bool exclude) noexcept {
        balance_exclusions_ = exclude;
        const bool continuous = history_.advance(now);
        if (!armed_ || !continuous) { if (!continuous) held_clocks_ = {}; previous_ = input; canonical_ = analog_direction(input); armed_ = true; observe_held(now); return; }
        direction(now, InputSource::direction, analog_direction(input), canonical_);
        // Pad and analog projections resolve to one canonical direction. Raw pad
        // bits and manual compatibility projections do not spend journal slots.
        for (unsigned bit = 4; bit < 16; ++bit) {
            const auto mask = static_cast<std::uint16_t>(std::uint16_t{1} << bit);
            if ((input.buttons ^ previous_.buttons) & mask)
                history_.record({InputSource::buttons, static_cast<std::int16_t>(bit),
                    input.buttons & mask ? InputEdge::press : InputEdge::release}, now);
        }
        for (unsigned trigger = 0; trigger < 2; ++trigger) {
            const bool held = trigger ? input.rt > 128 : input.lt > 30;
            const bool was = trigger ? previous_.rt > 128 : previous_.lt > 30;
            if (held != was) history_.record({InputSource::triggers, static_cast<std::int16_t>(trigger),
                held ? InputEdge::press : InputEdge::release}, now);
        }
        previous_ = input; observe_held(now);
    }
    static constexpr std::int16_t quantize(std::int16_t x, std::int16_t y) noexcept {
        return encode(x > 13000 ? 1 : x < -13000 ? -1 : 0, y > 13000 ? 1 : y < -13000 ? -1 : 0);
    }
    void direction(std::uint64_t now, InputSource source, std::int16_t value, std::int16_t& old) noexcept {
        if (value == old) return;
        const auto excluded = [&](std::int16_t code) noexcept {
            return balance_exclusions_ && (source == InputSource::vertical_stick || source == InputSource::vertical_pad ||
                ((source == InputSource::stick || source == InputSource::pad || source == InputSource::direction) &&
                 (code == encode(0, 1) || code == encode(0, -1))));
        };
        if (old && !excluded(old)) history_.record({source, old, InputEdge::release}, now);
        if (value && !excluded(value)) history_.record({source, value, InputEdge::press}, now);
        old = value;
    }
    InputHistory history_{};
    PhysicalInputSample previous_{};
    std::int16_t stick_{}, pad_{}, vertical_stick_{}, vertical_pad_{}, canonical_{};
    InputPolicy policy_{};
    struct HeldClock { int value{}; std::uint64_t since{}; };
    std::array<HeldClock, 23> held_clocks_{};
    bool armed_{}, balance_exclusions_{};
};

}

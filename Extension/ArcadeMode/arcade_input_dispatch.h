#pragma once
#include "arcade_input_events.h"
#include "arcade_input_policy.h"
#include <limits>

namespace dingosdk::arcade_mode {
enum class InputContext : std::uint8_t { unknown, ground, air, balance, rail, foot, lip };
enum class InputAction : std::uint8_t {
    none, crouch, ollie_release, flip, grab, rail_request, revert, manual, nose_manual,
    balance_branch_one, balance_branch_two, balance_variant, lip_request, balance_return
};
enum class InputDisposition : std::uint8_t { waiting, started, unsupported, cancelled, expired };
struct InputRequest {
    InputAction action{};
    InputContext context{};
    std::uint64_t recognized_at{}, deadline{}, session{};
    std::int16_t direction{};
    unsigned presses{1};
    bool nollie{};
    bool inclusive_deadline{};
    std::uint8_t variant{};
    std::uint64_t ready_at{};
    bool unbounded{};
};
struct InputEnvironment {
    InputContext context{};
    bool gameplay{}, conflict{}, nollie{};
    // Only an independent native observer may admit a branch context. Merely
    // recognizing an unsupported request must never advance this value.
    unsigned balance_branch{};
    bool manual_safe{true}, manual_prepare{}, manual_active{};
    bool auxiliary_enabled{true};
    bool flip_available{true};
    bool ollie_allowed{true};
    bool manual_context_known{true};
    bool deferred_grabs{};
};
struct InputDispatchEvent { InputRequest request{}; InputDisposition disposition{}; };
// Adapter callbacks request existing methods only. They cannot establish native
// identity, score, contact or combo continuation. Waiting does not own the pad.
struct InputAdapter {
    void* state{};
    InputDisposition (*request)(void*, const InputRequest&) noexcept{};
};
class InputRequestQueue {
public:
    static constexpr std::size_t capacity = 100;
    bool push(InputRequest request) noexcept {
        if (manual(request.action)) { slots_[0] = request; return true; }
        if (size_ + reserved_ >= capacity) return false;
        slots_[1 + (front_ + size_) % capacity] = request;
        ++size_; return true;
    }
    void clear() noexcept { slots_ = {}; front_ = size_ = 0; ++epoch_; }
    void clear_ordinary() noexcept {
        const auto manual = slots_[0]; clear(); slots_[0] = manual;
    }
    template<class Report>
    void dispatch(std::uint64_t now, std::uint64_t session, InputContext context,
                  InputAdapter adapter, Report report) noexcept {
        bool started{};
        // Ordinary requests retain table/insertion order. The independent
        // manual slot is replaceable, rather than a second ordinary FIFO entry.
        for (std::size_t visited = 0; visited < capacity && size_; ++visited) {
            auto& slot = slots_[1 + front_];
            if (slot.action == InputAction::none) { pop(); continue; }
            const auto request = slot;
            const auto gate = eligibility(request, now, session, context);
            if (gate != InputDisposition::waiting) { pop(); report(InputDispatchEvent{request, gate}); continue; }
            if (started || now < request.ready_at) break;
            pop(); // Removal precedes any adapter/reentrant boundary.
            const auto epoch = epoch_; ++reserved_;
            const auto result = adapter.request ? adapter.request(adapter.state, request) : InputDisposition::unsupported;
            --reserved_;
            if (result == InputDisposition::waiting) {
                if (epoch != epoch_) { report(InputDispatchEvent{request, InputDisposition::cancelled}); continue; }
                // The callback's insertions reserve this slot and cannot make
                // restoring the unchanged waiting request overflow the FIFO.
                front_ = (front_ + capacity - 1) % capacity;
                slots_[1 + front_] = request; ++size_; break;
            }
            report(InputDispatchEvent{request, result});
            started = result == InputDisposition::started;
        }
        if (slots_[0].action != InputAction::none) {
            const auto request = slots_[0];
            auto result = eligibility(request, now, session, context);
            if (result == InputDisposition::waiting && started) return;
            slots_[0] = {};
            const auto epoch = epoch_;
            if (result == InputDisposition::waiting)
                result = adapter.request ? adapter.request(adapter.state, request) : InputDisposition::unsupported;
            if (result == InputDisposition::waiting) {
                if (epoch == epoch_ && slots_[0].action == InputAction::none) slots_[0] = request;
            } else report(InputDispatchEvent{request, result});
        }
    }
    void cancel(InputAction action) noexcept {
        for (auto& slot : slots_) if (slot.action == action) slot = {};
    }
    std::span<const InputRequest> slots() const noexcept { return slots_; }
private:
    static bool manual(InputAction action) noexcept { return action == InputAction::manual || action == InputAction::nose_manual; }
    static InputDisposition eligibility(const InputRequest& request, std::uint64_t now,
                                        std::uint64_t session, InputContext context) noexcept {
        if (request.session != session || (request.context != context &&
            !(manual(request.action) && request.context == InputContext::air && context == InputContext::ground)))
            return InputDisposition::cancelled;
        if (!request.unbounded && (request.inclusive_deadline ? now > request.deadline : now >= request.deadline)) return InputDisposition::expired;
        return InputDisposition::waiting;
    }
    void pop() noexcept { slots_[1 + front_] = {}; front_ = (front_ + 1) % capacity; --size_; }
    std::array<InputRequest, capacity + 1> slots_{};
    std::size_t front_{}, size_{}, reserved_{};
    std::uint64_t epoch_{};
};
// Shared production recognition and dispatch. Native adapters remain responsible for
// all admission and safety contracts. No native memory or score dependencies.
class UnifiedInput {
public:
    struct Diagnostic {
        std::uint64_t session{}, samples{}, recognized{}, queue_full{}, journal_overflows{}, dispatch_dropped{};
        int stick{}, pad{}, stick_first{}, pad_first{};
        std::uint64_t stick_at{}, pad_at{};
        bool suppress_reverse{};
        std::array<InputDispatchEvent, 8> dispatch{};
        std::size_t dispatched{};
    };
    void reset() noexcept {
        events_.reset(); pending_.clear(); ++session_; crouched_ = false;
        previous_context_ = InputContext::unknown; direction_ = 0; direction_at_ = 0;
        diagnostic_.stick_first = diagnostic_.pad_first = 0;
        diagnostic_.stick_at = diagnostic_.pad_at = 0; diagnostic_.suppress_reverse = false;
        flip_count_ = 0; last_event_ = 0; waiting_direction_ = false; clocked_ = false; manual_unknown_ = stick_resume_ = false;
    }
    const Diagnostic& diagnostic() const noexcept { return diagnostic_; }
    bool pending_stick() const noexcept {
        if (waiting_direction_) return true;
        for (const auto& request : pending_.slots()) if (request.action == InputAction::flip) return true;
        return false;
    }
    const InputHistory& history() const noexcept { return events_.history(); }
    InputRequest pending_manual() const noexcept { return pending_.slots()[0]; }
    void reset_manual_continuity() noexcept {
        cancel_manual(); manual_unknown_ = true;
        diagnostic_.stick_first = diagnostic_.pad_first = 0;
        diagnostic_.stick_at = diagnostic_.pad_at = 0;
    }
    void cancel_manual() noexcept {
        events_.history().invalidate(InputSource::vertical_stick, manual_owner);
        events_.history().invalidate(InputSource::vertical_pad, manual_owner);
        pending_.cancel(InputAction::manual); pending_.cancel(InputAction::nose_manual); stick_resume_ = false;
    }
    void poll(std::uint64_t now, PhysicalInputSample physical, InputEnvironment environment,
              InputAdapter adapter = {}) noexcept {
        diagnostic_.dispatched = 0;
        if (!environment.gameplay || environment.context == InputContext::unknown ||
            environment.context == InputContext::foot || (clocked_ && now < last_)) {
            reset(); clocked_ = false; return;
        }
        const auto directional = physical.buttons & 15u;
        if ((physical.buttons & 0x0030u) || (directional & 3u) == 3u || (directional & 12u) == 12u) {
            reset(); return;
        }
        clocked_ = true; last_ = now; ++diagnostic_.samples;
        if (environment.context == InputContext::balance && previous_context_ != InputContext::balance) {
            // Contact installs a new ordinary table; eligible physical history
            // and its category ownership survive the admitted transition.
            pending_.clear_ordinary(); direction_ = 0;
        }
        // Physical held state always updates. Balance-event exclusions apply
        // from the preceding admitted context, separately from native ownership.
        events_.sample(now, physical, previous_context_ == InputContext::balance, environment.manual_active);
        diagnostic_.stick = events_.vertical_direction(false); diagnostic_.pad = events_.vertical_direction(true);
        if (environment.conflict) {
            events_.discard_intent(); pending_.clear(); crouched_ = false; stick_resume_ = false;
            diagnostic_.stick_first = diagnostic_.pad_first = 0;
            diagnostic_.stick_at = diagnostic_.pad_at = 0; diagnostic_.suppress_reverse = false;
            direction_ = 0; flip_count_ = 0; waiting_direction_ = false;
            previous_context_ = environment.context;
            return;
        }
        if (previous_context_ != InputContext::air && environment.context == InputContext::air &&
            now >= direction_at_ && now - direction_at_ > InputTiming::takeoff_preload_ms) direction_ = 0;
        if (previous_context_ == InputContext::air && environment.context != InputContext::air) {
            pending_.cancel(InputAction::flip); pending_.cancel(InputAction::grab);
            direction_ = 0; flip_count_ = 0; waiting_direction_ = false;
        }
        previous_context_ = environment.context;
        if (waiting_direction_ && (environment.context != InputContext::air || now - face_at_ >= InputTiming::direction_ms))
            waiting_direction_ = false;
        const auto held_direction = events_.held_direction();
        const bool balancing_vertical = environment.context == InputContext::balance &&
            (held_direction == InputEvents::encode(0, 1) || held_direction == InputEvents::encode(0, -1));
        if (held_direction && !balancing_vertical) { direction_ = held_direction; direction_at_ = now; }
        recognize_branches(now, environment);
        // Every sample's edges are evaluated once, even if no adapter is ready.
        const auto journal = events_.history().events();
        for (const auto& event : journal) {
            if (event.sequence <= last_event_) continue;
            if (event.token.source == InputSource::stick || event.token.source == InputSource::pad) {
                if (event.token.edge == InputEdge::press && !(environment.context == InputContext::balance &&
                    (event.token.code == InputEvents::encode(0, 1) || event.token.code == InputEvents::encode(0, -1)))) {
                    direction_ = event.token.code; direction_at_ = event.at;
                }
                continue;
            }
            if (environment.auxiliary_enabled && event.token.source == InputSource::triggers && event.token.code == 1 &&
                event.token.edge == InputEdge::press && environment.context == InputContext::ground)
                { consume(event); enqueue(InputAction::revert, now, 1, environment); }
            if (event.token.source != InputSource::buttons) continue;
            const auto code = event.token.code;
            if (code == 12) {
                if (event.token.edge == InputEdge::press && environment.ollie_allowed) {
                    consume(event); crouched_ = true; crouch_nollie_ = environment.nollie;
                    enqueue(InputAction::crouch, now, 1, environment);
                } else if (event.token.edge == InputEdge::release && crouched_) {
                    consume(event); auto context = environment; context.nollie = crouch_nollie_;
                    enqueue(InputAction::ollie_release, now, 1, context); crouched_ = false;
                }
            }
            if (event.token.edge != InputEdge::press) continue;
            if (code != 14) waiting_direction_ = false;
            if (environment.context == InputContext::air && (code == 14 || code == 13)) {
                consume(event); pending_.cancel(code == 14 ? InputAction::grab : InputAction::flip);
                if (code == 14) {
                    if (!environment.flip_available) continue;
                    if (!direction_ || now - direction_at_ >= InputTiming::direction_ms) {
                        waiting_direction_ = true; face_at_ = now;
                    } else recognize_flip(now, environment);
                } else if (environment.auxiliary_enabled) {
                    recognize_grab(now, environment);
                    consume_direction(); direction_ = 0;
                }
            }
            if (code == 15 && (environment.context == InputContext::ground ||
                environment.context == InputContext::air || environment.context == InputContext::rail || environment.context == InputContext::lip))
                {
                    consume(event); pending_.cancel(InputAction::flip); pending_.cancel(InputAction::grab);
                    enqueue(environment.context == InputContext::lip ? InputAction::lip_request : InputAction::rail_request,
                        now, InputTiming::readiness_ms, environment);
                }
        }
        if (!journal.empty()) last_event_ = journal.back().sequence;
        if (waiting_direction_ && direction_ && now - direction_at_ < InputTiming::direction_ms && environment.flip_available) {
            waiting_direction_ = false; recognize_flip(now, environment);
        }
        recognize_manual(now, physical, environment);
        pending_.dispatch(now, session_, environment.context, adapter, [&](InputDispatchEvent event) {
            if (diagnostic_.dispatched < diagnostic_.dispatch.size())
                diagnostic_.dispatch[diagnostic_.dispatched++] = event;
            else ++diagnostic_.dispatch_dropped;
        });
        diagnostic_.session = session_;
        diagnostic_.journal_overflows = events_.history().overflows();
    }
private:
    static constexpr std::uint32_t manual_owner = 2;
    static constexpr std::uint32_t branch_owner = 1u << 4;
    void consume(const InputEvent& event) noexcept {
        InputMatch match{}; match.count = 1; match.events[0] = event.sequence;
        events_.history().consume(match, 1);
    }
    void enqueue(InputAction action, std::uint64_t now, std::uint64_t duration,
                 InputEnvironment environment, unsigned presses = 1, std::uint8_t variant = 0, std::uint64_t delay = 0) noexcept {
        const auto limit = std::numeric_limits<std::uint64_t>::max();
        const auto deadline = now > limit - duration ? limit : now + duration;
        const InputRequest request{action, environment.context, now, deadline, session_,
            now >= direction_at_ && now - direction_at_ <= InputTiming::direction_ms ? direction_ : std::int16_t{0},
            presses, environment.nollie, action == InputAction::manual || action == InputAction::nose_manual, variant, now > limit - delay ? limit : now + delay,
            (action == InputAction::manual || action == InputAction::nose_manual) && environment.context == InputContext::ground};
        ++diagnostic_.recognized;
        if (!pending_.push(request)) ++diagnostic_.queue_full;
    }
    void consume_direction() noexcept {
        InputMatch match{};
        for (const auto& event : events_.history().events())
            if ((event.token.source == InputSource::stick || event.token.source == InputSource::pad) &&
                event.token.edge == InputEdge::press && event.token.code == direction_) {
                match.count = 1; match.events[0] = event.sequence;
            }
        if (match) events_.history().consume(match, 1, 0);
    }
    void recognize_grab(std::uint64_t now, InputEnvironment environment) noexcept {
        unsigned presses = 1;
        const auto selected = now >= direction_at_ && now - direction_at_ <= InputTiming::direction_ms ? direction_ : std::int16_t{0};
        if (environment.deferred_grabs) {
            for (const auto& request : pending_.slots()) if (request.action == InputAction::grab) {
                // A late fresh edge must not delete a completed earlier
                // selection. Keep it ahead of the new request in the FIFO.
                if (now - request.recognized_at > 180) break;
                if (request.direction == selected && request.presses < 2) presses = request.presses + 1;
                else { pending_.cancel(InputAction::grab); return; }
                break;
            }
            if (presses > 1) pending_.cancel(InputAction::grab);
        }
        enqueue(InputAction::grab, now, InputTiming::readiness_ms + (environment.deferred_grabs ? 180u : 0u),
            environment, presses, 0, environment.deferred_grabs ? 180u : 0u);
    }
    void recognize_flip(std::uint64_t now, InputEnvironment environment) noexcept {
        const bool repeated = flip_count_ && now >= flip_at_ && now - flip_at_ < InputTiming::extra_press_ms &&
            flip_direction_ == direction_ && flip_nollie_ == environment.nollie;
        flip_count_ = repeated ? flip_count_ + 1 : 1;
        if (flip_count_ > 3) flip_count_ = 3;
        flip_at_ = now; flip_direction_ = direction_; flip_nollie_ = environment.nollie;
        enqueue(InputAction::flip, now, InputTiming::readiness_ms, environment, flip_count_);
        // A released modifier cannot be reused by a later face press. A still
        // held modifier remains available through the physical-state path.
        consume_direction(); direction_ = 0;
    }
    void recognize_branches(std::uint64_t now, InputEnvironment environment) noexcept {
        if (environment.context != InputContext::balance) return;
        struct Branch { std::int16_t first, second; std::uint8_t variant; };
        // Context-table order is intentional. Unsupported native variants still
        // consume only their matched events and never install a new pose.
        constexpr std::array leading{Branch{14, 13, 1}, Branch{14, 13, 13},
            Branch{14, 15, 2}, Branch{14, 15, 0}, Branch{13, 14, 3}, Branch{13, 14, 0},
            Branch{13, 15, 4}, Branch{13, 15, 14}};
        constexpr std::array trailing{Branch{13, 13, 5}, Branch{15, 15, 6}, Branch{15, 15, 0},
            Branch{15, 14, 7}, Branch{15, 14, 0}, Branch{15, 13, 8}, Branch{15, 13, 0}, Branch{13, 13, 0}};
        // Posture-specific extras precede the generic branch table.
        const std::array<InputToken, 2> posture{{{InputSource::buttons, 14, InputEdge::press},
            {InputSource::buttons, 14, InputEdge::press}}};
        const bool posture_extras = environment.balance_branch == 0 || environment.balance_branch == 2 || environment.balance_branch == 6;
        const auto extra = posture_extras ? events_.history().match({posture, 200, ~manual_owner}, now) : InputMatch{};
        if (extra && events_.history().consume(extra, branch_owner, ~manual_owner))
            enqueue(InputAction::balance_variant, now, 1, environment, 1, 10);
        for (auto trigger : {std::int16_t{1}, std::int16_t{0}}) {
            const std::array<InputToken, 1> press{{{InputSource::triggers, trigger, InputEdge::press}}};
            const bool branched = environment.balance_branch == 2 || environment.balance_branch == 6;
            if ((!branched && (environment.balance_branch != 0 || trigger == 0))) continue;
            const auto spin = events_.history().match({press, branched ? 300u : 200u, ~manual_owner}, now);
            if (spin && events_.history().consume(spin, branch_owner, ~manual_owner))
                enqueue(InputAction::balance_variant, now, 1, environment, 1, static_cast<std::uint8_t>(11 + trigger));
        }
        const auto pair_rule = [&](const Branch& rule) noexcept {
            if (environment.balance_branch == rule.variant) return;
            const std::array<InputToken, 2> steps{{{InputSource::buttons, rule.first, InputEdge::press},
                {InputSource::buttons, rule.second, InputEdge::press}}};
            const auto match = events_.history().match({steps, 300, ~manual_owner}, now);
            if (match && events_.history().consume(match, branch_owner, ~manual_owner)) {
                const auto action = rule.variant == 2 ? InputAction::balance_branch_one :
                    rule.variant == 6 ? InputAction::balance_branch_two :
                    rule.variant == 0 || rule.variant == 13 ? InputAction::balance_return : InputAction::balance_variant;
                enqueue(action, now, 1, environment, 1, rule.variant);
            }
        };
        for (const auto& rule : leading) pair_rule(rule);
        if (environment.balance_branch != 9) for (auto source : {InputSource::stick, InputSource::pad}) {
            const auto match = events_.history().triple({source, InputEvents::encode(-1, 0), InputEdge::press},
                {source, InputEvents::encode(1, 0), InputEdge::press}, {InputSource::buttons, 14, InputEdge::press},
                500, 2, now, ~manual_owner);
            if (match && events_.history().consume(match, branch_owner, ~manual_owner))
                enqueue(InputAction::balance_variant, now, 1, environment, 1, 9);
        }
        for (const auto& rule : trailing) pair_rule(rule);
    }
    void recognize_manual(std::uint64_t now, PhysicalInputSample physical, InputEnvironment environment) noexcept {
        diagnostic_.suppress_reverse = false;
        if (!environment.manual_context_known ||
            (environment.context != InputContext::ground && environment.context != InputContext::air)) {
            manual_unknown_ = true; cancel_manual();
            diagnostic_.stick_first = diagnostic_.pad_first = 0;
            diagnostic_.stick_at = diagnostic_.pad_at = 0; return;
        } else if (manual_unknown_) {
            manual_unknown_ = false; cancel_manual();
            diagnostic_.stick_first = diagnostic_.pad_first = 0;
            diagnostic_.stick_at = diagnostic_.pad_at = 0; return;
        }
        if (environment.manual_active) { cancel_manual(); return; }
        if ((environment.context != InputContext::ground && environment.context != InputContext::air) || (!environment.manual_safe && !environment.manual_prepare) ||
            (physical.buttons & 0xf030u) || (physical.buttons & 0x0300u) == 0x0300u) {
            cancel_manual(); diagnostic_.stick_first = diagnostic_.pad_first = 0;
            diagnostic_.stick_at = diagnostic_.pad_at = 0; return;
        }
        const auto pad = physical.buttons & 15u;
        if (pad || stick_resume_) events_.history().invalidate(InputSource::vertical_stick, manual_owner);
        stick_resume_ = pad != 0;
        if (pad) events_.reset_manual_stick_baseline();
        const auto window = environment.context == InputContext::air ? InputTiming::air_pair_ms :
            InputTiming::ground_pair_ms + InputTiming::ground_sample_slack_ms;
        bool requested{};
        for (auto source : {InputSource::vertical_pad, InputSource::vertical_stick}) {
            const InputEvent* latest{}; const InputEvent* prior{};
            for (const auto& event : events_.history().events())
                if (event.token.source == source && event.token.edge == InputEdge::press && !(event.owners & manual_owner)) {
                    prior = latest; latest = &event;
                }
            const bool fresh = latest && now - latest->at <= window;
            auto& first = source == InputSource::vertical_pad ? diagnostic_.pad_first : diagnostic_.stick_first;
            auto& at = source == InputSource::vertical_pad ? diagnostic_.pad_at : diagnostic_.stick_at;
            first = fresh ? latest->token.code : 0; at = latest ? latest->at : 0;
            if (requested || !fresh || latest->at != now || !prior ||
                prior->token.code != -latest->token.code || now - prior->at > window) continue;
            const std::array steps{prior->token, latest->token};
            const auto match = events_.history().match({steps, window, manual_owner, true, false, true}, now);
            if (match && events_.history().consume(match, manual_owner, manual_owner)) {
                // Recognition and native readiness remain separate. The current
                // safe-state gate, including preparation-only contexts, is retained.
                requested = true;
                if (environment.manual_safe) enqueue(latest->token.code > 0 ? InputAction::nose_manual : InputAction::manual,
                    now, environment.context == InputContext::air ? InputTiming::air_request_ms : 0, environment);
            }
        }
        diagnostic_.suppress_reverse = environment.context == InputContext::ground && physical.y < -11000 &&
            diagnostic_.stick_first == -1 && events_.vertical_direction(false) == -1;
    }
    InputEvents events_{};
    InputRequestQueue pending_{};
    Diagnostic diagnostic_{};
    std::uint64_t session_{1}, last_{}, last_event_{}, direction_at_{}, flip_at_{}, face_at_{};
    std::int16_t direction_{}, flip_direction_{};
    InputContext previous_context_{};
    unsigned flip_count_{};
    bool clocked_{}, crouched_{}, crouch_nollie_{}, waiting_direction_{}, flip_nollie_{}, manual_unknown_{}, stick_resume_{};
};
}

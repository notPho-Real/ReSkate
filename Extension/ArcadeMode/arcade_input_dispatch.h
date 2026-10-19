#pragma once
#include "arcade_input_events.h"
#include "arcade_input_policy.h"
#include <limits>

namespace dingosdk::arcade_mode {
enum class InputContext : std::uint8_t { unknown, ground, air, balance, rail, foot, lip };
enum class InputAction : std::uint8_t {
    none, crouch, ollie_release, flip, grab, rail_request, revert, manual, nose_manual,
    balance_branch_one, balance_branch_two, balance_variant, lip_request, balance_return, rail_change, lip_change, wall_command, transfer_command, rail_select, lip_select, board_toggle, ground_turn
};
enum class InputDisposition : std::uint8_t { waiting, started, unsupported, cancelled, expired };
enum class InputForm : std::uint8_t { ordinary, repeated_direction };
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
    std::uint8_t modifier{};
    std::uint8_t geometry{};
    bool geometry_known{};
    std::uint64_t rail_generation{};
    InputSource direction_source{InputSource::stick};
    std::uint16_t control{};
    std::uint64_t physical_pressed_at{}, physical_released_at{};
    bool direction_inherited{}, named_binding{};
    std::uint64_t surface_generation{}, first_input_at{}, last_input_at{};
    unsigned balance_origin{};
    InputGenerations generations{};
    std::uint8_t generation_mask{};
    InputForm form{};
    InputAirEpisode air_episode{};
};
struct InputEnvironment {
    InputContext context{};
    bool gameplay{}, conflict{}, nollie{};
    // Only an independent native observer may admit a branch context. Merely
    // recognizing an unsupported request must never advance this value.
    unsigned balance_branch{};
    bool manual_safe{true}, manual_prepare{}, manual_active{};
    bool auxiliary_enabled{true};
    bool ollie_allowed{true};
    bool manual_context_known{true};
    bool deferred_grabs{};
    InputPolicy policy{InputPolicy::production()};
    bool branch_ready{true};
    InputRailState rail{};
    InputSurfaceState surface{};
    bool named_grabs{};
    InputBalanceControls balance_controls{};
    bool board_allowed{};
    InputBoardBinding board_binding{};
    InputNativeContext native_context{};
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
        if (manual(request.action)) { slots_[0] = request; ++manual_epoch_; return true; }
        if (rail(request.action)) { slots_[capacity + 1] = request; ++rail_epoch_; return true; }
        if (size_ + reserved_ >= capacity) return false;
        slots_[1 + (front_ + size_) % capacity] = request;
        ++size_; return true;
    }
    void clear() noexcept { slots_ = {}; front_ = size_ = 0; ++epoch_; ++manual_epoch_; ++rail_epoch_; }
    void clear_ordinary() noexcept {
        for (std::size_t i = 1; i <= capacity; ++i) slots_[i] = {};
        front_ = size_ = 0; ++epoch_;
    }
    template<class Report>
    void dispatch(std::uint64_t now, std::uint64_t session, InputContext context,
                  InputAdapter adapter, Report report, InputNativeContext native = {}) noexcept {
        bool started{};
        // Ordinary requests retain table/insertion order. The independent
        // manual slot is replaceable, rather than a second ordinary FIFO entry.
        for (std::size_t visited = 0; visited < capacity && size_; ++visited) {
            auto& slot = slots_[1 + front_];
            if (slot.action == InputAction::none) { pop(); continue; }
            const auto request = slot;
            const auto gate = eligibility(request, now, session, context, native);
            if (gate != InputDisposition::waiting) { pop(); report(InputDispatchEvent{request, gate}); continue; }
            if (started || now < request.ready_at) break;
            pop(); // Removal precedes any adapter/reentrant boundary.
            const auto epoch = epoch_;
            const auto cancellation = cancellations_[static_cast<std::size_t>(request.action)];
            ++reserved_;
            const auto result = adapter.request ? adapter.request(adapter.state, request) : InputDisposition::unsupported;
            --reserved_;
            if (result == InputDisposition::waiting) {
                if (epoch != epoch_ || cancellation != cancellations_[static_cast<std::size_t>(request.action)]) {
                    report(InputDispatchEvent{request, InputDisposition::cancelled}); continue;
                }
                // The callback's insertions reserve this slot and cannot make
                // restoring the unchanged waiting request overflow the FIFO.
                front_ = (front_ + capacity - 1) % capacity;
                slots_[1 + front_] = request; ++size_; break;
            }
            report(InputDispatchEvent{request, result});
            started = result == InputDisposition::started;
        }
        const auto slot_dispatch = [&](std::size_t index, std::uint64_t& slot_epoch) noexcept {
            if (slots_[index].action == InputAction::none) return;
            const auto request = slots_[index];
            auto result = eligibility(request, now, session, context, native);
            if (result == InputDisposition::waiting && started) return;
            slots_[index] = {};
            const auto epoch = slot_epoch;
            const auto cancellation = cancellations_[static_cast<std::size_t>(request.action)];
            if (result == InputDisposition::waiting)
                result = adapter.request ? adapter.request(adapter.state, request) : InputDisposition::unsupported;
            if (result == InputDisposition::waiting) {
                if (epoch == slot_epoch && cancellation == cancellations_[static_cast<std::size_t>(request.action)] &&
                    slots_[index].action == InputAction::none) slots_[index] = request;
                else report(InputDispatchEvent{request, InputDisposition::cancelled});
            } else { report(InputDispatchEvent{request, result}); started = started || result == InputDisposition::started; }
        };
        slot_dispatch(0, manual_epoch_);
        slot_dispatch(capacity + 1, rail_epoch_);

    }
    void cancel(InputAction action) noexcept {
        // A request is absent from slots while its adapter runs. Invalidate
        // that reservation too, without cancelling unrelated waiting actions.
        ++cancellations_[static_cast<std::size_t>(action)];
        for (auto& slot : slots_) if (slot.action == action) slot = {};
    }
    std::span<const InputRequest> slots() const noexcept { return slots_; }
private:
    static bool rail(InputAction action) noexcept { return action == InputAction::rail_select; }
    static bool manual(InputAction action) noexcept { return action == InputAction::manual || action == InputAction::nose_manual; }
    static InputDisposition eligibility(const InputRequest& request, std::uint64_t now,
                                        std::uint64_t session, InputContext context, InputNativeContext native) noexcept {
        if (request.generation_mask && (!native.required || !native.fresh(now) ||
            request.generations.owner != native.generations.owner ||
            ((request.generation_mask & 2u) && request.generations.contact != native.generations.contact) ||
            ((request.generation_mask & 4u) && request.generations.action != native.generations.action)))
            return InputDisposition::cancelled;
        if (request.air_episode.number && (!native.airborne || request.air_episode != native.air_episode()))
            return InputDisposition::cancelled;
        if (request.session != session || (request.context != context &&
            !(manual(request.action) && request.context == InputContext::air && context == InputContext::ground) &&
            !(request.action == InputAction::rail_select &&
              (context == InputContext::ground || context == InputContext::air || context == InputContext::rail || context == InputContext::lip))))
            return InputDisposition::cancelled;
        if (!request.unbounded && (request.inclusive_deadline ? now > request.deadline : now >= request.deadline)) return InputDisposition::expired;
        return InputDisposition::waiting;
    }
    void pop() noexcept { slots_[1 + front_] = {}; front_ = (front_ + 1) % capacity; --size_; }
    std::array<InputRequest, capacity + 2> slots_{};
    std::size_t front_{}, size_{}, reserved_{};
    std::uint64_t epoch_{}, manual_epoch_{}, rail_epoch_{};
    std::array<std::uint64_t, static_cast<std::size_t>(InputAction::ground_turn) + 1> cancellations_{};
};
// Shared production recognition and dispatch. Native adapters remain responsible for
// all admission and safety contracts. No native memory or score dependencies.
class UnifiedInput {
public:
    struct Diagnostic {
        std::uint64_t session{}, samples{}, recognized{}, queue_full{}, journal_overflows{}, dispatch_dropped{};
        int stick{}, pad{}, stick_first{}, pad_first{};
        std::uint64_t stick_at{}, pad_at{}, flip_table_at{}, grab_table_at{};
        bool suppress_reverse{}, unresolved_definition{}, branch_context_unavailable{};
        std::array<InputDispatchEvent, 8> dispatch{};
        std::size_t dispatched{};
        InputPolicy policy{InputPolicy::production()};
    };
    void reset() noexcept {
        events_.reset(); balance_controls_ = {}; pending_.clear(); ++session_; crouched_ = false;
        previous_context_ = InputContext::unknown; direction_ = 0; direction_at_ = 0;
        diagnostic_.stick_first = diagnostic_.pad_first = 0;
        diagnostic_.stick_at = diagnostic_.pad_at = 0; diagnostic_.suppress_reverse = false;
        diagnostic_.dispatched = 0; diagnostic_.unresolved_definition = diagnostic_.branch_context_unavailable = false;
        active_flip_ = active_grab_ = surface_clocked_ = lip_clocked_ = false; last_event_ = manual_after_ = 0; waiting_direction_ = false; clocked_ = false; manual_unknown_ = stick_resume_ = false;
    }
    const Diagnostic& diagnostic() const noexcept { return diagnostic_; }
    bool pending_stick() const noexcept {
        if (waiting_direction_) return true;
        for (const auto& request : pending_.slots()) if (request.action == InputAction::flip) return true;
        return false;
    }
    const InputHistory& history() const noexcept { return events_.history(); }
    InputRequest pending_manual() const noexcept { return pending_.slots()[0]; }
    InputRequest pending_grab() const noexcept {
        for (const auto& request : pending_.slots()) if (request.action == InputAction::grab) return request;
        return {};
    }
    void acknowledge_jump_release() noexcept { pending_.cancel(InputAction::ollie_release); }
    void cancel_auxiliary() noexcept {
        pending_.cancel(InputAction::grab); pending_.cancel(InputAction::revert); active_grab_ = false;
    }
    void cancel_air_episode() noexcept {
        pending_.cancel(InputAction::flip); pending_.cancel(InputAction::grab);
        active_flip_ = active_grab_ = waiting_direction_ = false; direction_ = 0;
        // Keep physical held state and independently queued Manual/rail intent.
        events_.discard_intent();
    }
    InputRequest pending_rail() const noexcept { return pending_.slots()[InputRequestQueue::capacity + 1]; }
    void reset_manual_continuity() noexcept {
        cancel_manual(); manual_unknown_ = true;
        diagnostic_.stick_first = diagnostic_.pad_first = 0;
        diagnostic_.stick_at = diagnostic_.pad_at = 0;
    }
    void cancel_manual() noexcept {
        events_.history().invalidate(InputSource::vertical_stick, manual_owner);
        events_.history().invalidate(InputSource::vertical_pad, manual_owner);
        // Cancelling Manual intent must not consume shared physical directions.
        const auto journal = events_.history().events();
        if (!journal.empty()) manual_after_ = journal.back().sequence;
        pending_.cancel(InputAction::manual); pending_.cancel(InputAction::nose_manual); stick_resume_ = false;
    }
    void poll(std::uint64_t now, PhysicalInputSample physical, InputEnvironment environment,
              InputAdapter adapter = {}) noexcept {
        const auto native = environment.native_context;
        native_required_ = native_required_ || native.required;
        if (native_required_ && (!native.required || !native.fresh(now) || !native.follows(native_))) {
            reset(); return;
        }
        if (native.required) {
            if (!native_.valid || native.generations.owner != native_.generations.owner) reset();
            else {
                if (native.air_episode() != native_.air_episode()) cancel_air_episode();
                if (native.generations.action != native_.generations.action) {
                    waiting_direction_ = false;
                }
                if (native.generations.contact != native_.generations.contact) lip_clocked_ = false;
            }
            native_ = native;
            if (!environment.balance_controls.correlated(native)) environment.balance_controls.admitted = false;
            if (!native.correlates_action(environment.rail.generations)) environment.rail.fresh = environment.rail.ready = false;
            if (!native.correlates_action(environment.surface.generations)) environment.surface.fresh = false;
        }
        if (clocked_ && environment.policy != policy_) reset();
        policy_ = environment.policy; diagnostic_.policy = policy_;
        diagnostic_.dispatched = 0; diagnostic_.unresolved_definition = diagnostic_.branch_context_unavailable = false;
        if (!environment.gameplay || environment.context == InputContext::unknown ||
            (clocked_ && now < last_)) {
            reset(); clocked_ = false; return;
        }
        const auto directional = physical.buttons & 15u;
        if ((physical.buttons & 0x0030u) || (environment.policy.reject_opposing && ((directional & 3u) == 3u || (directional & 12u) == 12u))) {
            reset(); return;
        }
        clocked_ = true; last_ = now; ++diagnostic_.samples;
        if (previous_context_ == InputContext::balance && environment.context != InputContext::balance)
            events_.history().restore(branch_owner, now, 100);
        if (environment.context == InputContext::balance && previous_context_ != InputContext::balance) {
            // Contact installs a new ordinary table; eligible physical history
            // and its category ownership survive the admitted transition.
            pending_.clear_ordinary(); direction_ = 0;
        }
        // Physical held state always updates. Balance-event exclusions apply
        // from the preceding admitted context, separately from native ownership.
        events_.sample(now, physical, balance_controls_, environment.manual_active, environment.policy);
        balance_controls_ = environment.balance_controls;
        if (!balance_controls_.admitted && environment.context == InputContext::balance && environment.branch_ready)
            balance_controls_ = InputBalanceControls::vertical(environment.balance_branch == 13 ? InputBalanceKind::nose : InputBalanceKind::manual);
        diagnostic_.stick = events_.vertical_direction(false); diagnostic_.pad = events_.vertical_direction(true);
        if (environment.conflict) {
            events_.discard_intent(); pending_.clear(); crouched_ = false; stick_resume_ = false;
            diagnostic_.stick_first = diagnostic_.pad_first = 0;
            diagnostic_.stick_at = diagnostic_.pad_at = 0; diagnostic_.suppress_reverse = false;
            direction_ = 0; active_flip_ = active_grab_ = false; waiting_direction_ = false;
            previous_context_ = environment.context;
            return;
        }
        if (previous_context_ != InputContext::air && environment.context == InputContext::air &&
            now >= direction_at_ && now - direction_at_ > InputTiming::takeoff_preload_ms) direction_ = 0;
        if (previous_context_ == InputContext::air && environment.context != InputContext::air) {
            pending_.cancel(InputAction::flip); pending_.cancel(InputAction::grab);
            direction_ = 0; active_flip_ = active_grab_ = false; waiting_direction_ = false;
        }
        previous_context_ = environment.context;
        if (waiting_direction_ && (environment.context != InputContext::air || now - face_at_ >= InputTiming::direction_ms))
            waiting_direction_ = false;
        const auto held_direction = events_.held_direction();
        const bool balancing_vertical = environment.context == InputContext::balance &&
            (held_direction == InputEvents::encode(0, 1) || held_direction == InputEvents::encode(0, -1));
        if (held_direction && !balancing_vertical) {
            direction_ = held_direction; direction_at_ = now;
            direction_source_ = InputSource::direction;
        }
        recognize_surface(now, environment);
        recognize_branches(now, environment);
        recognize_rail_extras(now, environment);
        recognize_ground_prefix(now,environment);
        // Every sample's edges are evaluated once, even if no adapter is ready.
        const auto journal = events_.history().events();
        for (const auto& event : journal) {
            if (event.sequence <= last_event_) continue;
            if (event.token.source == InputSource::stick || event.token.source == InputSource::pad || event.token.source == InputSource::direction) {
                if (event.token.edge == InputEdge::press && !(environment.context == InputContext::balance &&
                    (event.token.code == InputEvents::encode(0, 1) || event.token.code == InputEvents::encode(0, -1)))) {
                    direction_ = event.token.code; direction_at_ = event.at; direction_source_ = event.token.source;
                }
                continue;
            }
            if (environment.auxiliary_enabled && event.token.source == InputSource::triggers && event.token.code == 1 &&
                event.token.edge == InputEdge::press && environment.context == InputContext::ground)
                { consume(event); enqueue(InputAction::revert, now, 1, environment); }
            if (event.token.source != InputSource::buttons || event.owners) continue;
            const auto code = event.token.code;
            if (code == 12) {
                if (event.token.edge == InputEdge::press && environment.ollie_allowed && environment.context != InputContext::foot) {
                    consume(event); crouched_ = true; crouch_nollie_ = environment.nollie; crouch_at_ = event.at;
                    enqueue(InputAction::crouch, now, 1, environment);
                } else if (event.token.edge == InputEdge::release && crouched_) {
                    consume(event); auto context = environment; context.nollie = crouch_nollie_;
                    enqueue(InputAction::ollie_release, now, 1, context); crouched_ = false;
                }
            }
            if (event.token.edge != InputEdge::press) continue;
            if (code != 14) waiting_direction_ = false;
            if (environment.context == InputContext::air && (code == 14 || code == 13)) {
                pending_.cancel(code == 14 ? InputAction::grab : InputAction::flip);
                if (code == 14) {
                    if (!recognize_flip(now, environment)) { waiting_direction_ = true; face_at_ = now; }
                } else if (environment.auxiliary_enabled) {
                    recognize_grab(now, environment);
                    direction_ = 0;
                }
            }
            if (code == 15 && !(environment.surface.fresh && environment.surface.can_push) && environment.rail.phase == InputRailPhase::unavailable && (environment.context == InputContext::ground ||
                environment.context == InputContext::air || environment.context == InputContext::rail || environment.context == InputContext::lip))
                {
                    pending_.cancel(InputAction::flip); pending_.cancel(InputAction::grab);
                    enqueue(environment.context == InputContext::lip ? InputAction::lip_request : InputAction::rail_request,
                        now, InputTiming::readiness_ms, environment);
                }
        }
        if (!journal.empty()) last_event_ = journal.back().sequence;
        if (waiting_direction_ && direction_ && now - direction_at_ < InputTiming::direction_ms) {
            if (recognize_flip(now, environment)) waiting_direction_ = false;
        }
        recognize_manual(now, physical, environment);
        recognize_rail_selection(now, environment);
        recognize_board(now, environment);
        pending_.dispatch(now, session_, environment.context, adapter, [&](InputDispatchEvent event) {
            if (event.disposition == InputDisposition::started && event.request.action == InputAction::lip_select) {
                pending_.cancel(InputAction::lip_select); pending_.cancel(InputAction::rail_select);
            }
            if (event.disposition == InputDisposition::started && event.request.action == InputAction::flip) {
                active_flip_ = event.request.form == InputForm::ordinary;
                active_flip_count_ = active_flip_ ? event.request.presses : 0; flip_started_ = now;
                flip_direction_ = event.request.direction; flip_source_ = event.request.direction_source; flip_nollie_ = event.request.nollie;
                flip_generations_ = event.request.generations;
                diagnostic_.flip_table_at = now;
            }
            if (event.disposition == InputDisposition::started && event.request.action == InputAction::grab) {
                active_grab_ = event.request.named_binding && event.request.form == InputForm::ordinary;
                active_grab_count_ = active_grab_ ? event.request.presses : 0; grab_started_ = now;
                grab_direction_ = event.request.direction; grab_source_ = event.request.direction_source; diagnostic_.grab_table_at = now;
                grab_generations_ = event.request.generations;
            }
            if (event.disposition == InputDisposition::started && (event.request.action == InputAction::manual ||
                event.request.action == InputAction::nose_manual || event.request.action == InputAction::flip || event.request.action == InputAction::grab))
                pending_.cancel(InputAction::rail_select);
            if (diagnostic_.dispatched < diagnostic_.dispatch.size())
                diagnostic_.dispatch[diagnostic_.dispatched++] = event;
            else ++diagnostic_.dispatch_dropped;
        }, native);
        diagnostic_.session = session_;
        diagnostic_.journal_overflows = events_.history().overflows();
    }
private:
    static constexpr std::uint32_t manual_owner = InputConsumption::manual;
    static constexpr std::uint32_t ordinary_reject = ~manual_owner;
    static constexpr std::uint32_t branch_owner = InputConsumption::extra;
    static constexpr std::uint32_t rail_owner = InputConsumption::extra_grind;
    static constexpr std::uint8_t generation_dependencies(InputAction action, InputContext context) noexcept {
        // Manual preparation and an admitted jump release intentionally span
        // contact/action transitions. Native generations cannot prove completion.
        switch (action) {
        case InputAction::manual: case InputAction::nose_manual:
        case InputAction::crouch: case InputAction::ollie_release: return 1;
        case InputAction::rail_select:
            return context == InputContext::air || context == InputContext::ground ? 1 : 7;
        case InputAction::flip: case InputAction::grab: case InputAction::rail_request: return 5;
        default: return 7;
        }
    }
    void consume(const InputEvent& event) noexcept {
        InputMatch match{}; match.count = 1; match.events[0] = event.sequence;
        events_.history().consume(match, InputConsumption::regular);
    }
    void enqueue(InputAction action, std::uint64_t now, std::uint64_t duration,
                 InputEnvironment environment, unsigned presses = 1, std::uint8_t variant = 0, std::uint64_t delay = 0, std::uint8_t modifier = 0, InputToken inherited = {}, const InputMatch* footprint = nullptr,
                 InputForm form = InputForm::ordinary, std::int16_t defined_direction = 0) noexcept {
        const auto limit = std::numeric_limits<std::uint64_t>::max();
        const auto deadline = now > limit - duration ? limit : now + duration;
        InputRequest request{action, environment.context, now, deadline, session_,
            InputHistory::direction_token(inherited) ? inherited.code : now >= direction_at_ && now - direction_at_ <= InputTiming::direction_ms ? direction_ : std::int16_t{0},
            presses, environment.nollie, action == InputAction::manual || action == InputAction::nose_manual || action == InputAction::rail_select, variant, now > limit - delay ? limit : now + delay,
            ((action == InputAction::manual || action == InputAction::nose_manual) && environment.context == InputContext::ground) ||
                action == InputAction::rail_change || action == InputAction::lip_change || action == InputAction::lip_select ||
                action == InputAction::balance_branch_one || action == InputAction::balance_branch_two ||
                action == InputAction::balance_variant || action == InputAction::balance_return || action == InputAction::ground_turn ||
                (!environment.policy.bounded_handoff && (action == InputAction::flip || action == InputAction::grab)), modifier,
            environment.rail.geometry_index(), environment.rail.geometry_known, environment.rail.generation, InputHistory::direction_token(inherited) ? inherited.source : direction_source_,
            static_cast<std::uint16_t>(action == InputAction::grab ? 0x2000u : action == InputAction::flip ? 0x4000u :
                action == InputAction::crouch || action == InputAction::ollie_release ? 0x1000u :
                action == InputAction::ground_turn ? 0x0200u :
                action == InputAction::board_toggle ? (environment.board_binding.pattern == InputBoardPattern::shoulder_overlap ? 0x0300u : environment.board_binding.control_mask) : 0u),
            action == InputAction::crouch || action == InputAction::ollie_release ? crouch_at_ : now,
            action == InputAction::ollie_release ? now : 0, InputHistory::direction_token(inherited), environment.named_grabs, environment.surface.generation,
            footprint ? footprint->first_at : now, footprint ? footprint->last_at : now, environment.balance_branch};
        if (environment.native_context.required) {
            request.generations = environment.native_context.generations;
            request.generation_mask = generation_dependencies(action,environment.context);
            if (action == InputAction::flip || action == InputAction::grab)
                request.air_episode = environment.native_context.air_episode();
        }
        request.form = form;
        if (action == InputAction::ground_turn) { request.direction = 2; request.direction_source = InputSource::direction; }
        if (form == InputForm::repeated_direction) {
            request.direction = defined_direction; request.direction_source = InputSource::direction;
            request.direction_inherited = false;
        }
        ++diagnostic_.recognized;
        if (!pending_.push(request)) ++diagnostic_.queue_full;
    }
    void recognize_board(std::uint64_t now, InputEnvironment environment) noexcept {
        if (!environment.board_allowed) return;
        InputMatch match{};
        const auto& binding = environment.board_binding;
        if (binding.pattern == InputBoardPattern::shoulder_overlap) {
            match = events_.history().overlap({InputSource::buttons,8,InputEdge::press},
                {InputSource::buttons,9,InputEdge::press},400,now);
        } else if (binding.pattern == InputBoardPattern::single_control) {
            const auto mask = binding.control_mask;
            if (!mask || (mask & static_cast<std::uint16_t>(mask-1)) || (mask & 0x003fu)) return;
            std::int16_t bit{}; while (!(mask & (std::uint16_t{1} << bit))) ++bit;
            const std::array token{InputToken{InputSource::buttons,bit,InputEdge::press}};
            match = events_.history().match({token,10},now);
        }
        if (match && events_.history().consume(match,InputConsumption::regular))
            enqueue(InputAction::board_toggle,now,1,environment,1,0,0,0,{},&match);
    }
    InputMatch directional_prefix(InputAction action, InputContext context, std::uint64_t now, std::int16_t& selected) const noexcept {
        // Ordinary rows precede base directional actions. Repeating a direction
        // is distinct from a face press within an installed parent's extra table.
        struct Definition { InputAction action; InputContext context; std::int16_t direction, terminal; std::uint64_t window; };
        static constexpr std::array definitions{
            Definition{InputAction::ground_turn,InputContext::ground,2,9,333},
            Definition{InputAction::flip,InputContext::air,4,14,300},
            Definition{InputAction::flip,InputContext::air,6,14,300},
            Definition{InputAction::flip,InputContext::air,8,14,300},
            Definition{InputAction::flip,InputContext::air,2,14,300},
            Definition{InputAction::grab,InputContext::air,4,13,300},
            Definition{InputAction::grab,InputContext::air,6,13,300},
            Definition{InputAction::grab,InputContext::air,8,13,300},
            Definition{InputAction::grab,InputContext::air,2,13,300}};
        for (const auto& definition : definitions) {
            if (definition.action != action || definition.context != context) continue;
            const InputToken token{InputSource::direction,definition.direction,InputEdge::press};
            const auto match = events_.history().triple(token,token,{InputSource::buttons,definition.terminal,InputEdge::press},
                definition.window,2,now,ordinary_reject);
            if (match) { selected = definition.direction; return match; }
        }
        return {};
    }
    void recognize_ground_prefix(std::uint64_t now, InputEnvironment environment) noexcept {
        std::int16_t selected{};
        const auto match = directional_prefix(InputAction::ground_turn,environment.context,now,selected);
        if (match && events_.history().consume(match,InputConsumption::regular,ordinary_reject))
            enqueue(InputAction::ground_turn,now,0,environment,1,1,0,0,{},&match);
    }
    void recognize_grab(std::uint64_t now, InputEnvironment environment) noexcept {
        unsigned presses = 1;
        auto selected = now >= direction_at_ && now - direction_at_ <= InputTiming::direction_ms ? direction_ : std::int16_t{0};
        const InputToken face{InputSource::buttons,13,InputEdge::press};
        InputMatch match{};
        InputForm form{};
        if (!environment.deferred_grabs && active_grab_ && active_grab_count_ == 1 && grab_direction_ && now - grab_started_ <= 250) {
            match = events_.history().extra_action(face, {grab_source_,grab_direction_,InputEdge::press}, 300, now, ~manual_owner);
            if (match) presses = active_grab_count_ < std::numeric_limits<unsigned>::max() ? active_grab_count_ + 1 : active_grab_count_;
        }
        if (!match && !environment.deferred_grabs) {
            match = directional_prefix(InputAction::grab,environment.context,now,selected);
            if (match) form = InputForm::repeated_direction;
        }
        if (!match) {
            if (selected && !environment.deferred_grabs) {
                const InputToken direction{direction_source_,selected,InputEdge::press};
                match = events_.history().air_action(face,direction,events_.held(direction),500,now,ordinary_reject);
            } else { const std::array tokens{face}; match = events_.history().match({tokens,500,ordinary_reject},now); }
        }
        if (!match) return;
        if (environment.deferred_grabs) {
            for (const auto& request : pending_.slots()) if (request.action == InputAction::grab) {
                if (now - request.recognized_at > 180) break;
                if (request.direction == selected && request.presses < 2) presses = request.presses + 1;
                else { pending_.cancel(InputAction::grab); return; }
                break;
            }
            if (presses > 1) pending_.cancel(InputAction::grab);
        }
        const bool extra = presses > 1 && !environment.deferred_grabs;
        if (!events_.history().consume(match, extra ? branch_owner : InputConsumption::regular, ordinary_reject)) return;
        if (extra && environment.native_context.required) environment.native_context.generations = grab_generations_;
        enqueue(InputAction::grab, now, InputTiming::readiness_ms + (environment.deferred_grabs ? 180u : 0u),
            environment, presses, 0, environment.deferred_grabs ? 180u : 0u, 0,
            extra ? InputToken{grab_source_,grab_direction_,InputEdge::press} : InputToken{}, &match,form,selected);
    }
    bool recognize_flip(std::uint64_t now, InputEnvironment environment) noexcept {
        const InputToken face{InputSource::buttons,14,InputEdge::press};
        InputMatch match{}; unsigned presses = 1;
        InputForm form{}; std::int16_t defined_direction{};
        const bool child = active_flip_count_ == 1 || (active_flip_count_ == 2 &&
            (flip_direction_ == 2 || flip_direction_ == 4 || flip_direction_ == 6 || flip_direction_ == 8));
        if (active_flip_ && child && now - flip_started_ <= 233) {
            match = events_.history().extra_action(face, {flip_source_,flip_direction_,InputEdge::press}, 300, now, ~manual_owner);
            if (match) presses = active_flip_count_ < std::numeric_limits<unsigned>::max() ? active_flip_count_ + 1 : active_flip_count_;
        }
        if (!match) {
            match = directional_prefix(InputAction::flip,environment.context,now,defined_direction);
            if (match) form = InputForm::repeated_direction;
        }
        if (!match) {
            const InputToken direction{direction_source_, direction_, InputEdge::press};
            match = events_.history().air_action(face, direction, events_.held(direction), 500, now,ordinary_reject);
        }
        if (!match || !events_.history().consume(match, presses > 1 ? branch_owner : InputConsumption::regular,
            ordinary_reject)) return false;
        auto selected = environment;
        if (presses > 1) selected.nollie = flip_nollie_;
        if (presses > 1 && selected.native_context.required) selected.native_context.generations = flip_generations_;
        enqueue(InputAction::flip, now, InputTiming::readiness_ms, selected, presses, 0, 0, 0,
            presses > 1 ? InputToken{flip_source_,flip_direction_,InputEdge::press} : InputToken{}, &match,form,defined_direction);
        direction_ = 0; return true;
    }
    void recognize_surface(std::uint64_t now, InputEnvironment environment) noexcept {
        const auto& surface = environment.surface;
        if (!surface.fresh || environment.context == InputContext::foot) return;
        if (surface_clocked_ && surface.generation == surface_generation_ && surface.physics_step == surface_step_) return;
        surface_clocked_ = true; surface_generation_ = surface.generation; surface_step_ = surface.physics_step;
        const InputToken jump{InputSource::buttons,12,InputEdge::press};
        if (surface.can_plant) {
            for (auto code : {std::int16_t{2},std::int16_t{1},std::int16_t{3}})
                for (bool reverse : {false,true}) {
                    const InputToken direction{InputSource::direction,code,InputEdge::press};
                    const std::array tokens{reverse ? jump : direction, reverse ? direction : jump};
                    const auto match = events_.history().match({tokens,450},now);
                    if (match && events_.history().consume(match,InputConsumption::regular)) {
                        enqueue(InputAction::wall_command,now,1,environment,1,1,0,0,{},&match); return;
                    }
                }
        }
        if (surface.can_push && events_.held({InputSource::buttons,15,InputEdge::press})) {
            enqueue(InputAction::wall_command,now,1,environment,1,2); return;
        }
        if (surface.can_transfer && surface.transfer_buttons &&
            (physical_buttons() & surface.transfer_buttons) == surface.transfer_buttons)
            enqueue(InputAction::transfer_command,now,1,environment);
    }
    std::uint16_t physical_buttons() const noexcept {
        std::uint16_t result{};
        for (unsigned bit=0; bit<16; ++bit) if(events_.held({InputSource::buttons,static_cast<std::int16_t>(bit),InputEdge::press}))
            result |= static_cast<std::uint16_t>(std::uint16_t{1} << bit);
        return result;
    }
    void recognize_rail_extras(std::uint64_t now, InputEnvironment environment) noexcept {
        const auto& rail = environment.rail;
        if (!rail.fresh || !rail.ready) return;
        if (environment.context != InputContext::rail && environment.context != InputContext::lip) return;
        constexpr InputToken y{InputSource::buttons, 15, InputEdge::press};
        if (rail.phase == InputRailPhase::early) {
            constexpr std::array<std::int16_t, 8> front{9, 3, 1, 7, 8, 6, 4, 2};
            constexpr std::array<std::int16_t, 8> back{9, 3, 1, 7, 8, 6, 2, 4};
            const auto& directions = rail.frontside ? front : back;
            for (auto direction : directions) {
                const auto match = events_.history().triple({InputSource::direction, direction, InputEdge::press}, y, y, 1000, 0, now, ~manual_owner);
                if (match && events_.history().consume(match, branch_owner, ~manual_owner)) {
                    const auto index = direction == 9 ? 10u : direction == 3 ? 11u : direction == 1 ? 12u : direction == 7 ? 13u :
                        direction == 8 ? 14u : direction == 6 ? 15u : direction == 4 ? 16u : 17u;
                    enqueue(InputAction::rail_change, now, 0, environment, 1, static_cast<std::uint8_t>(index),0,0,{},&match); return;
                }
            }
        }
        struct Pair { std::int16_t first, second; std::uint8_t target; };
        constexpr std::array front{Pair{15,15,1}, Pair{15,14,2}, Pair{15,13,3}, Pair{13,13,4}, Pair{13,14,5},
            Pair{13,15,6}, Pair{14,14,7}, Pair{14,13,8}, Pair{14,15,9}};
        constexpr std::array back{Pair{15,15,1}, Pair{15,14,2}, Pair{15,13,3}, Pair{13,13,4}, Pair{13,14,5},
            Pair{13,15,6}, Pair{14,15,9}, Pair{14,14,7}, Pair{14,13,8}};
        constexpr std::array lips{Pair{13,13,40}, Pair{13,14,41}, Pair{14,14,42}, Pair{14,13,43},
            Pair{15,15,44}, Pair{15,14,45}, Pair{15,13,46}};
        constexpr std::array inverted{Pair{14,14,50}, Pair{15,15,51}, Pair{13,13,52}, Pair{13,14,53}, Pair{14,13,54}, Pair{15,13,55}};
        const auto query = [&](Pair pair, std::uint64_t window, InputAction action) noexcept {
            if (rail.excluded_variant == pair.target && !(action == InputAction::rail_change && !rail.frontside && pair.target == 3)) return false;
            const std::array tokens{InputToken{InputSource::buttons, pair.first, InputEdge::press}, InputToken{InputSource::buttons, pair.second, InputEdge::press}};
            const auto match = events_.history().match({tokens, window, ~manual_owner}, now);
            if (!match || !events_.history().consume(match, branch_owner, ~manual_owner)) return false;
            enqueue(action, now, 0, environment, 1, pair.target,0,0,{},&match); return true;
        };
        if (rail.phase == InputRailPhase::established) {
            for (auto pair : rail.frontside ? front : back) if (query(pair, 300, InputAction::rail_change)) return;
        } else if (rail.phase == InputRailPhase::lip_established) {
            if (rail.lip_table == 0) { for (auto pair : lips) if (query(pair, 300, InputAction::lip_change)) return; }
            else if (rail.lip_table == 1) { for (auto pair : inverted) if (query(pair, 300, InputAction::lip_change)) return; }
            else if (rail.lip_table == 2) query({15,15,56}, 500, InputAction::lip_change);
        }
    }
    void recognize_rail_selection(std::uint64_t now, InputEnvironment environment) noexcept {
        if (environment.context == InputContext::ground || environment.context == InputContext::air) {
            for (auto direction : {std::int16_t{8}, std::int16_t{2}}) {
                const InputToken token{InputSource::direction, direction, InputEdge::press};
                const auto match = events_.history().triple(token, token, {InputSource::buttons, 15, InputEdge::press}, 400, 2, now, ~manual_owner);
                if (match && events_.history().consume(match, rail_owner, ~manual_owner))
                    enqueue(InputAction::rail_select, now, 1000, environment, 1, direction == 8 ? 20 : 21,0,0,{},&match);
            }
        }
        if (environment.context != InputContext::lip || !environment.rail.fresh || environment.rail.phase != InputRailPhase::lip_selection) {
            lip_clocked_ = false; return;
        }
        const auto& rail = environment.rail;
        if (!lip_clocked_ || lip_generation_ != rail.generation) {
            lip_clocked_ = true; lip_generation_ = rail.generation; lip_had_input_ = lip_defaulted_ = false;
            pending_.clear_ordinary();
        }
        if (rail.gameframe < rail.selection_frame) return;
        if (rail.gameframe - rail.selection_frame >= 2) {
            if (!lip_had_input_ && !lip_defaulted_) { enqueue(InputAction::lip_select,now,0,environment,1,0); lip_defaulted_ = true; }
            return;
        }
        constexpr std::array<std::int16_t,8> order{7,9,1,3,4,6,2,8};
        for (auto direction : order) {
            const std::array tokens{InputToken{InputSource::direction, direction, InputEdge::press}};
            const auto match = events_.history().match({tokens,500,~rail_owner},now);
            if (match && events_.history().consume(match,InputConsumption::regular,~rail_owner)) {
                enqueue(InputAction::lip_select,now,0,environment,1,static_cast<std::uint8_t>(direction),0,0,{},&match); lip_had_input_ = true;
            }
        }
        const std::array token{InputToken{InputSource::buttons,15,InputEdge::press}};
        const auto match = events_.history().match({token,1000,~rail_owner},now);
        if (match && events_.history().consume(match,InputConsumption::regular,~rail_owner)) {
            enqueue(InputAction::lip_select,now,0,environment,1,0,0,0,{},&match); lip_had_input_ = true;
        }
    }
    void recognize_branches(std::uint64_t now, InputEnvironment environment) noexcept {
        if (environment.context != InputContext::balance || !environment.branch_ready) return;
        // Ordered context bindings and semantic destinations, independent of any
        // native identity or asset. Undefined rows never consume their query.
        struct Rule { unsigned kind; std::int16_t first, second; std::uint64_t window; std::uint8_t target; };
        static constexpr std::array rules{
            Rule{1, 1, 0, 200, 21},
            Rule{0, 14, 14, 200, 22},
            Rule{0, 13, 15, 300, 4},
            Rule{0, 14, 13, 300, 1},
            Rule{0, 14, 13, 300, 13},
            Rule{0, 14, 15, 300, 2},
            Rule{0, 14, 15, 300, 0},
            Rule{0, 13, 14, 300, 3},
            Rule{0, 13, 14, 300, 0},
            Rule{0, 13, 15, 300, 4},
            Rule{0, 13, 15, 300, 14},
            Rule{2, 4, 6, 500, 9},
            Rule{0, 13, 13, 300, 5},
            Rule{0, 15, 15, 300, 6},
            Rule{0, 15, 15, 300, 0},
            Rule{0, 15, 14, 300, 7},
            Rule{0, 15, 14, 300, 255},
            Rule{0, 15, 13, 300, 8},
            Rule{0, 15, 13, 300, 0},
            Rule{0, 13, 13, 300, 0},
            Rule{0, 14, 14, 200, 29},
            Rule{0, 14, 14, 200, 34},
            Rule{1, 1, 0, 200, 39},
            Rule{0, 14, 14, 200, 40},
            Rule{0, 13, 15, 300, 14},
            Rule{0, 14, 14, 200, 42},
            Rule{1, 1, 0, 300, 43},
            Rule{1, 0, 0, 300, 43},
            Rule{0, 14, 14, 200, 44},
            Rule{1, 1, 0, 300, 45},
            Rule{1, 0, 0, 300, 45},
            Rule{0, 14, 14, 200, 46},
            Rule{0, 13, 13, 400, 47},
            Rule{1, 1, 0, 300, 48},
            Rule{1, 0, 0, 300, 48},
            Rule{0, 14, 14, 200, 49},
            Rule{1, 1, 0, 300, 50},
            Rule{1, 0, 0, 300, 50},
            Rule{0, 14, 14, 200, 51},
            Rule{1, 1, 0, 300, 52},
            Rule{1, 0, 0, 300, 52},
        };
        struct Table { std::size_t first, count; };
        static constexpr std::array tables{
            Table{0, 3},
            Table{3, 17},
            Table{20, 1},
            Table{21, 1},
            Table{22, 3},
            Table{25, 3},
            Table{28, 3},
            Table{31, 4},
            Table{35, 3},
            Table{38, 3},
        };
        struct Context { unsigned id, first, second, excluded; };
        static constexpr std::array contexts{
            Context{0, 0, 1, 0},
            Context{1, 6, 1, 1},
            Context{2, 5, 1, 2},
            Context{3, 3, 1, 3},
            Context{4, 1, 0, 4},
            Context{5, 2, 1, 5},
            Context{6, 9, 1, 6},
            Context{7, 7, 1, 7},
            Context{8, 8, 1, 8},
            Context{9, 0, 1, 9},
            Context{13, 4, 1, 13},
            Context{14, 1, 4, 14},
            Context{20, 0, 1, 20},
            Context{21, 4, 1, 255},
            Context{22, 4, 1, 255},
            Context{23, 0, 1, 23},
            Context{24, 0, 1, 24},
            Context{25, 0, 1, 25},
            Context{26, 0, 1, 26},
            Context{27, 0, 1, 27},
            Context{28, 2, 255, 28},
            Context{29, 2, 1, 5},
            Context{30, 0, 1, 30},
            Context{31, 1, 255, 31},
            Context{32, 1, 255, 32},
            Context{33, 3, 1, 33},
            Context{34, 3, 1, 3},
            Context{35, 0, 1, 35},
            Context{36, 1, 255, 36},
            Context{37, 3, 1, 37},
            Context{38, 4, 1, 38},
            Context{39, 0, 1, 255},
            Context{40, 0, 1, 255},
            Context{41, 1, 255, 41},
            Context{42, 5, 1, 2},
            Context{43, 5, 1, 2},
            Context{44, 6, 1, 1},
            Context{45, 6, 1, 1},
            Context{46, 7, 1, 7},
            Context{47, 1, 7, 7},
            Context{48, 7, 1, 7},
            Context{49, 8, 1, 8},
            Context{50, 8, 1, 8},
            Context{51, 9, 1, 6},
            Context{52, 9, 1, 6},
        };
        const Context* context{};
        for (const auto& candidate : contexts) if (candidate.id == environment.balance_branch) { context = &candidate; break; }
        if (!context) { diagnostic_.branch_context_unavailable = true; return; }
        const auto query = [&](const Rule& rule) noexcept {
            if (rule.kind == 1) {
                const std::array tokens{InputToken{InputSource::triggers, rule.first, InputEdge::press}};
                return events_.history().match({tokens, rule.window, ~manual_owner}, now);
            }
            if (rule.kind == 2) {
                return events_.history().triple({InputSource::direction, rule.first, InputEdge::press},
                    {InputSource::direction, rule.second, InputEdge::press}, {InputSource::buttons, 14, InputEdge::press},
                    rule.window, 2, now, ~manual_owner);
            }
            const std::array tokens{InputToken{InputSource::buttons, rule.first, InputEdge::press},
                InputToken{InputSource::buttons, rule.second, InputEdge::press}};
            return events_.history().match({tokens, rule.window, ~manual_owner}, now);
        };
        for (auto table : {context->first, context->second}) {
            if (table >= tables.size()) continue;
            const auto span = tables[table];
            for (std::size_t i = span.first; i < span.first + span.count; ++i) {
                const auto& rule = rules[i];
                if (rule.target != 255 && rule.target == context->excluded) continue;
                const auto match = query(rule);
                if (!match) continue;
                if (rule.target == 255) { diagnostic_.unresolved_definition = true; continue; }
                if (!events_.history().consume(match, branch_owner, ~manual_owner)) continue;
                const auto action = rule.target == 2 ? InputAction::balance_branch_one :
                    rule.target == 6 ? InputAction::balance_branch_two :
                    rule.target == 0 || rule.target == 13 ? InputAction::balance_return : InputAction::balance_variant;
                enqueue(action, now, 1, environment, 1, rule.target, 0,
                    rule.kind == 1 ? static_cast<std::uint8_t>(rule.first + 1) : std::uint8_t{0}, {}, &match);
                return;
            }
        }
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
            InputTiming::ground_pair_ms + environment.policy.manual_sample_allowance;
        const bool inclusive = environment.context == InputContext::ground && environment.policy.inclusive_manual;
        bool requested{};
        const bool canonical = environment.policy.normalization == InputNormalization::analog;
        const auto reject = canonical ? ~std::uint32_t{0} : manual_owner;
        if (canonical) { diagnostic_.stick_first = diagnostic_.pad_first = 0; diagnostic_.stick_at = diagnostic_.pad_at = 0; }
        for (auto source : {InputSource::vertical_pad, InputSource::vertical_stick, InputSource::direction}) {
            if (canonical != (source == InputSource::direction)) continue;
            const InputEvent* latest{}; const InputEvent* prior{};
            for (const auto& event : events_.history().events())
                if (event.sequence > manual_after_ && event.token.source == source && event.token.edge == InputEdge::press && !(event.owners & reject) &&
                    (source != InputSource::direction || event.token.code == 2 || event.token.code == 8)) {
                    prior = latest; latest = &event;
                }
            const bool fresh = latest && (inclusive ? now - latest->at <= window : now - latest->at < window);
            auto& first = (source == InputSource::vertical_pad || (canonical && pad)) ? diagnostic_.pad_first : diagnostic_.stick_first;
            auto& at = (source == InputSource::vertical_pad || (canonical && pad)) ? diagnostic_.pad_at : diagnostic_.stick_at;
            const auto sign = [&](const InputEvent* event) { return event ? canonical ? event->token.code == 8 ? 1 : -1 : static_cast<int>(event->token.code) : 0; };
            first = fresh ? sign(latest) : 0; at = latest ? latest->at : 0;
            if (requested || !fresh || latest->at != now || !prior ||
                sign(prior) != -sign(latest) || (inclusive ? now - prior->at > window : now - prior->at >= window)) continue;
            const std::array steps{prior->token, latest->token};
            const auto match = events_.history().match({steps, window, reject, true, false, inclusive}, now);
            if (match && events_.history().consume(match, manual_owner, reject)) {
                // Recognition and native readiness remain separate. The current
                // safe-state gate, including preparation-only contexts, is retained.
                requested = true;
                enqueue(sign(latest) > 0 ? InputAction::nose_manual : InputAction::manual,
                    now, environment.context == InputContext::air ? InputTiming::air_request_ms : 0, environment, 1, 0, 0, 0, {}, &match);
            }
        }
        diagnostic_.suppress_reverse = environment.context == InputContext::ground && physical.y < -11000 &&
            diagnostic_.stick_first == -1 && events_.vertical_direction(false) == -1;
    }
    InputPolicy policy_{InputPolicy::production()};
    InputNativeContext native_{};
    InputGenerations flip_generations_{}, grab_generations_{};
    bool native_required_{};
    InputBalanceControls balance_controls_{};
    InputEvents events_{};
    InputRequestQueue pending_{};
    Diagnostic diagnostic_{};
    bool surface_clocked_{}, lip_clocked_{}, lip_had_input_{}, lip_defaulted_{};
    std::uint64_t lip_generation_{}, manual_after_{};
    std::uint64_t surface_generation_{}, surface_step_{}, crouch_at_{};
    std::uint64_t session_{1}, last_{}, last_event_{}, direction_at_{}, face_at_{}, flip_started_{}, grab_started_{};
    std::int16_t direction_{}, flip_direction_{}, grab_direction_{};
    InputSource direction_source_{InputSource::stick}, flip_source_{InputSource::stick}, grab_source_{InputSource::stick};
    InputContext previous_context_{};
    unsigned active_flip_count_{}, active_grab_count_{};
    bool active_flip_{}, active_grab_{};
    bool clocked_{}, crouched_{}, crouch_nollie_{}, waiting_direction_{}, flip_nollie_{}, manual_unknown_{}, stick_resume_{};
};
}

#pragma once
#include "arcade_action_plan.h"
#include "arcade_revert_input.h"

namespace dingosdk::arcade_mode {
enum class DispatchResult : std::uint8_t { started, disabled, busy, unsafe_context, unavailable, needs_geometry };
struct ActionContext {
    bool experimental_enabled{}, gameplay_fresh{}, airborne{}, riding{}, inspected_grab_stance{};
    bool other_input_owner{};
};
// One owner for future grab/revert actuation. Authored flips, manuals and board
// chords remain external owners. Runtime wiring must supply fresh native context.
class ActionDispatch {
public:
    struct Output {
        bool owns_right_stick{}, owns_left_stick{}, owns_triggers{}, owns_face_buttons{};
        std::int16_t left_x{}, left_y{}, right_x{}, right_y{};
        std::uint8_t left_trigger{}, right_trigger{};
        std::uint16_t face_buttons{};
    };
    void cancel() noexcept { grab_.cancel(); revert_.cancel(); }
    bool busy() const noexcept { return grab_.busy() || revert_.busy(); }
    DispatchResult request_grab(ArcadeTrick trick, std::uint64_t now, const ActionContext& context) noexcept {
        const auto gate = eligibility(context);
        if (gate != DispatchResult::started) return gate;
        if (!context.airborne) return DispatchResult::unsafe_context;
        const auto plan = grab_plan(trick, context.inspected_grab_stance);
        if (plan.readiness == ActionReadiness::context_unverified) return DispatchResult::unsafe_context;
        if (plan.readiness != ActionReadiness::candidate) return DispatchResult::unavailable;
        return grab_.start(plan, now, true, true) ? DispatchResult::started : DispatchResult::unavailable;
    }
    DispatchResult request_revert(RevertSide side, RevertContext stance, std::uint64_t now,
                                  const ActionContext& context) noexcept {
        const auto gate = eligibility(context);
        if (gate != DispatchResult::started) return gate;
        if (!context.riding || context.airborne) return DispatchResult::unsafe_context;
        return revert_.start(now, side, stance, true) ? DispatchResult::started : DispatchResult::unavailable;
    }
    DispatchResult request_grind(ArcadeTrick trick, RailRelationship rail, const ActionContext& context) const noexcept {
        const auto gate = eligibility(context);
        if (gate != DispatchResult::started) return gate;
        return grind_readiness(trick, rail) == ActionReadiness::geometry_required ?
            DispatchResult::needs_geometry : DispatchResult::unavailable;
    }
    Output sample(std::uint64_t now, const ActionContext& context) noexcept {
        if (!context.experimental_enabled || !context.gameplay_fresh || context.other_input_owner) { cancel(); return {}; }
        if (grab_.busy()) {
            const auto out = grab_.sample(now, true, context.airborne);
            if (out.owns_pad) return {true, false, true, true, 0, 0, out.pad.right_x, out.pad.right_y,
                                     out.pad.left_trigger, out.pad.right_trigger, out.pad.buttons};
        } else if (revert_.busy()) {
            const auto out = revert_.sample(now, context.riding && !context.airborne);
            if (out.owns_sticks) return {true, true, true, true, out.left_x, out.left_y, out.right_x, out.right_y,
                                        out.left_trigger, out.right_trigger, 0};
        }
        return {};
    }
private:
    DispatchResult eligibility(const ActionContext& context) const noexcept {
        if (!context.experimental_enabled) return DispatchResult::disabled;
        if (!context.gameplay_fresh) return DispatchResult::unsafe_context;
        if (context.other_input_owner || busy()) return DispatchResult::busy;
        return DispatchResult::started;
    }
    NativeActionPlayback grab_;
    RevertPlayback revert_;
};
}

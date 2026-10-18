#pragma once
#include "Extension/ArcadeMode/arcade_input_session.h"
namespace dingosdk::arcade_mode::test {
// Compatibility harness exercises the production session, not a second decoder.
class SessionManual {
public:
    using Output = ManualActuation::Output;
    using Diagnostic = ManualInputDiagnostic;
    static constexpr auto ground_pair_ms = ManualActuation::ground_pair_ms;
    static constexpr auto ground_sample_slack_ms = ManualActuation::ground_sample_slack_ms;
    static constexpr auto air_pair_ms = ManualActuation::air_pair_ms;
    static constexpr auto air_request_ms = ManualActuation::air_request_ms;
    void cancel() noexcept { session_.reset(); }
    const Diagnostic& diagnostic() const noexcept { return diagnostic_; }
    Output poll(std::uint64_t now, std::uint16_t buttons, std::int16_t x, std::int16_t y,
                bool gameplay, bool safe, SkatePose pose, bool conflict, bool prepare = false) noexcept {
        InputAdmission admission{};
        admission.gameplay = gameplay; admission.pose = pose; admission.manual_safe = safe;
        admission.manual_prepare = prepare; admission.external_owner = conflict;
        admission.tick = now;
        const auto out = session_.poll(now, {buttons, x, y}, 0, 0, admission);
        diagnostic_ = out.manual;
        return {out.manual.owns, out.manual.confirmed, out.manual.owns ? out.right_y : std::int16_t{0}, out.suppress_reverse};
    }
private:
    InputSession session_;
    Diagnostic diagnostic_{};
};
}

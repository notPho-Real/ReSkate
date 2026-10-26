#pragma once
#include "arcade_ground_motion_step.h"
#include <array>
#include <exception>

namespace dingosdk::arcade_mode {
// Optional scalar-writer diagnostics, never a motion permission or final engine
// velocity. Identity is the existing capture-local token; no native addresses.
enum class GroundWriterStage : unsigned { observed, policy, staged, validated, writing, verified };
struct GroundWriterEvidence {
    // ms is arcade_air_apply entry time, NOT write completion time. native_ms
    // is the original physics observation; no post-write clock/owner is read.
    std::uint64_t capture{}, sequence{}, ms{}, native_ms{}, step{}, owner_generation{}, contact_generation{}, action_generation{};
    std::uint64_t input_ms{}, input_generation{}, proof_ms{}, proof_step{}, proof_owner_generation{};
    unsigned owner{}, controls{}, state{}, mode{}, contacts{}, requests{}, bodies{}, completed{};
    float seconds{}, ground_seconds{}, requested_resistance{}, resistance{};
    std::array<float,3> before{}, staged{};
    GroundStepStatus step_status{};
    GroundWriterStage stage{};
    bool motion_valid{}, proof_valid{}, proof_owner_matches{}, controls_current{}, ground_eligible{}, baseline{}, ordinary{}, balancing{};
    bool native_brake{}, selected_known{}, selected_brake{}, linear{}, angular{}, unwinding{};
};
inline bool ground_writer_gate(bool enabled, bool capture_live, std::uint64_t until,
        std::uint64_t now, bool down, unsigned requests) noexcept {
    return enabled && capture_live && until >= now && until-now <= 150 && (down || (requests&6u));
}
inline bool ground_writer_valid(const GroundWriterEvidence& r) noexcept {
    if (!r.capture || !r.sequence || r.sequence>16 || !r.owner || r.owner>16 || !r.step ||
        !r.owner_generation || !r.contact_generation || !r.action_generation ||
        r.ms<r.native_ms || r.ms-r.native_ms>150 || r.ms<r.input_ms || r.ms-r.input_ms>150 ||
        !std::isfinite(r.seconds) || r.seconds<.001f || r.seconds>.1f ||
        !std::isfinite(r.ground_seconds) || r.ground_seconds<0 || r.ground_seconds>.04f ||
        !std::isfinite(r.resistance) || r.resistance>0 || r.resistance < -1000 ||
        !std::isfinite(r.requested_resistance) || r.requested_resistance>0 || r.requested_resistance < -1000 ||
        r.controls>511 || !r.bodies || r.bodies>32 || r.completed>r.bodies ||
        r.step_status>GroundStepStatus::next || r.stage>GroundWriterStage::verified) return false;
    for (const auto& v : {r.before,r.staged})
        for (float value:v) if (!std::isfinite(value) || std::abs(value)>=1000) return false;
    if (r.selected_known != (r.stage>=GroundWriterStage::policy && r.step_status==GroundStepStatus::next)) return false;
    if (r.selected_brake && !r.selected_known) return false;
    if (r.controls_current && (!r.motion_valid || !r.proof_valid || !r.proof_owner_matches ||
        r.proof_owner_generation!=r.owner_generation || !r.proof_step || r.proof_step>r.step ||
        r.proof_ms>r.native_ms || r.ms<r.proof_ms || r.ms-r.proof_ms>150)) return false;
    if (r.ground_eligible && !r.controls_current) return false;
    if (r.step_status==GroundStepStatus::next && (!r.ground_eligible || r.ground_seconds<.005f)) return false;
    if (r.stage>=GroundWriterStage::validated && !r.linear && !r.angular) return false;
    if (r.completed && r.stage<GroundWriterStage::writing) return false;
    return r.stage!=GroundWriterStage::verified || r.completed==r.bodies;
}
// Serialized by the existing input mutex. No queue capacity or capture limit
// changes. Caller charges every denied eligible attempt to client loss.
class GroundWriterBudget {
    std::uint64_t capture_{}, step_{};
    unsigned count_{};
public:
    unsigned take(std::uint64_t capture,std::uint64_t step) noexcept {
        if (!capture || !step) return 0;
        if (capture!=capture_) { capture_=capture; step_=0; count_=0; }
        if (step<=step_ || count_==16) return 0;
        step_=step; return ++count_;
    }
};
// The existing writer validates every copy with memory::peek equality. A
// verified stage means ALL those calls returned after prewrite owner checking;
// partial writes and later engine changes remain unknown. No extra reads.
class GroundWriterObservation {
    void (*sink_)(const GroundWriterEvidence&) noexcept;
    int exceptions_{std::uncaught_exceptions()};
public:
    GroundWriterEvidence row;
    GroundWriterObservation(GroundWriterEvidence value,void (*sink)(const GroundWriterEvidence&) noexcept) noexcept
        : sink_(sink), row(value) {}
    GroundWriterObservation(const GroundWriterObservation&)=delete;
    ~GroundWriterObservation() noexcept {
        if (row.capture) { row.unwinding=std::uncaught_exceptions()>exceptions_; sink_(row); }
    }
    void policy(bool current,bool eligible,const GroundStepAdmission& step,bool native_brake) noexcept {
        row.controls_current=current;row.ground_eligible=eligible;row.step_status=step.status;
        row.baseline=step.baseline;row.ground_seconds=step.seconds;row.native_brake=native_brake;row.stage=GroundWriterStage::policy;
    }
    void selected(bool brake) noexcept { row.selected_known=true;row.selected_brake=brake; }
    void staging(bool linear,bool angular,const std::array<float,3>& velocity,float resistance) noexcept {
        row.linear=linear;row.angular=angular;row.staged=velocity;row.resistance=resistance;row.stage=GroundWriterStage::staged;
    }
    void validated() noexcept { row.stage=GroundWriterStage::validated; }
    void writing() noexcept { row.stage=GroundWriterStage::writing; }
    void body_completed() noexcept { ++row.completed; }
    void verified() noexcept { row.stage=GroundWriterStage::verified; }
};
}

#pragma once
#include "arcade_ground_motion_step.h"
#include <array>
#include <exception>
#include <atomic>

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

// Explicit angular observation with base mask39 uses the existing client queue.
// Legacy masks alone never opt in. These receipts
// describe observed controller decisions and the existing verified body copies,
// not target-callback execution or final engine angular motion. No addresses
// leave the capture; body_set is a capture-local equality token for all parts.
enum class AngularWriterKind : unsigned { begin, controller, body, end };
struct AngularWriterEvidence {
    AngularWriterKind kind{};
    std::uint64_t capture{}, sequence{}, ms{}, native_ms{}, step{}, episode{}, departure{};
    std::uint64_t owner_generation{}, contact_generation{}, action_generation{};
    std::uint64_t input_ms{}, input_generation{}, proof_ms{}, proof_step{}, spin_epoch{}, spin_serial{};
    std::uint64_t proof_owner_generation{}, proof_episode{}, proof_departure{};
    unsigned owner{}, body_set{}, state{}, mode{}, contacts{}, buttons{}, phase{}, spin_mode{}, facts{};
    unsigned bodies{}, completed{}, reserved{}, queued{}, lost{};
    std::uint64_t pending{};
    float seconds{}, requested_rate{}, published_rate{}, output_rate{};
    std::array<float,3> before{}, staged{}, up{}, forward{};
    GroundWriterStage stage{};
    bool native_valid{}, context_owner_matches{}, airborne{}, enabled{}, allowed{}, ready{}, conflict{}, owns_left{}, armed{};
    bool proof_valid{}, proof_owner_matches{}, output_known{}, output_ready{}, output_angular{}, ground_selected{}, release{}, angular{}, unwinding{};
    bool late{}, complete{};
};
inline bool angular_writer_valid(const AngularWriterEvidence& r) noexcept {
    if (!r.capture || r.kind>AngularWriterKind::end || r.owner>16 || r.reserved>64 || r.queued>64 ||
        r.bodies>32 || r.completed>r.bodies || r.stage>GroundWriterStage::verified || r.spin_mode>1 || r.facts>0x3f) return false;
    for (float v : {r.seconds,r.requested_rate,r.published_rate,r.output_rate}) if (!std::isfinite(v) || std::abs(v)>=1000) return false;
    for (const auto& values : {r.before,r.staged,r.up,r.forward}) for (float v:values)
        if (!std::isfinite(v) || std::abs(v)>=1000) return false;
    if (r.kind==AngularWriterKind::begin) return !r.sequence && !r.reserved && !r.queued && !r.pending && !r.complete;
    if (r.kind==AngularWriterKind::end) return !r.sequence && r.queued<=r.reserved &&
        r.complete==(!r.lost && !r.pending && r.queued==r.reserved);
    if (!r.sequence || r.sequence>64 || r.complete) return false;
    if (r.kind==AngularWriterKind::controller) return !r.bodies && !r.completed && r.stage==GroundWriterStage::observed;
    if (r.completed && r.stage<GroundWriterStage::writing) return false;
    return r.stage!=GroundWriterStage::verified || (r.angular && r.bodies && r.completed==r.bodies);
}
// Calls selecting rows are serialized by ollie_mutex. Finish may run on the
// physics thread without that lock; re-arm is refused until every ticket has
// settled. End with pending work is explicitly incomplete, never upgraded by a
// late row. Counts describe enqueues; the reader must also retain begin/end and
// every unique sequence, and reject ordinary queue/log/capture loss.
class AngularWriterCapture {
    std::atomic<std::uint64_t> id_{};
    // Existing camera-diagnostics admission pattern: the high bit admits
    // attempts; low bits pin this epoch before any gameplay-mutex try_lock.
    // Closing is nonblocking and reports pinned attempts as incomplete.
    static constexpr std::uint64_t open_bit=std::uint64_t{1}<<63;
    std::atomic<std::uint64_t> admission_{};
    std::atomic<unsigned> pending_{}, queued_{}, lost_{};
    unsigned reserved_{}, body_set_{};
    bool interested_{}, controller_air_{}, body_air_{}, controller_done_{}, body_done_{}, controller_seen_{}, body_seen_{};
    AngularWriterEvidence previous_{};
    AngularWriterEvidence previous_body_{};
    std::array<std::uintptr_t,32> parts_{};
    bool reserve(AngularWriterEvidence& r) noexcept {
        if (reserved_==64) { lost_.fetch_add(1,std::memory_order_relaxed);return false; }
        r.capture=id_.load(std::memory_order_relaxed);r.sequence=++reserved_;
        pending_.fetch_add(1,std::memory_order_relaxed);return true;
    }
public:
    class Attempt {
        AngularWriterCapture* capture_{};
    public:
        explicit Attempt(AngularWriterCapture& capture) noexcept {
            const auto previous=capture.admission_.fetch_add(1,std::memory_order_acq_rel);
            if(previous&open_bit)capture_=&capture;
            else capture.admission_.fetch_sub(1,std::memory_order_release);
        }
        Attempt(const Attempt&)=delete;
        ~Attempt(){release();}
        explicit operator bool()const noexcept{return capture_!=nullptr;}
        void release()noexcept{if(capture_){capture_->admission_.fetch_sub(1,std::memory_order_release);capture_=nullptr;}}
    };
    bool live() const noexcept { return (admission_.load(std::memory_order_acquire)&open_bit)!=0; }
    bool pending() const noexcept { return pending_.load(std::memory_order_acquire)!=0 || (admission_.load(std::memory_order_acquire)&~open_bit)!=0; }
    std::uint64_t capture_id()const noexcept{return id_.load(std::memory_order_acquire);}
    void lose() noexcept { lost_.fetch_add(1,std::memory_order_relaxed); }
    AngularWriterEvidence begin(std::uint64_t id,std::uint64_t now) noexcept {
        if (!id || live() || pending()) return {};
        reserved_=body_set_=0;interested_=controller_air_=body_air_=controller_done_=body_done_=controller_seen_=body_seen_=false;
        previous_={};previous_body_={};parts_={};queued_=0;lost_=0;
        id_.store(id,std::memory_order_relaxed);admission_.fetch_or(open_bit,std::memory_order_release);
        AngularWriterEvidence r;r.capture=id;r.ms=now;return r;
    }
    bool controller(AngularWriterEvidence& r) noexcept {
        if (!live()) return false;
        if (r.buttons&0x300u) interested_=true;
        if (!interested_ || controller_done_) return false;
        const bool same=controller_seen_ && r.step==previous_.step && r.native_ms==previous_.native_ms &&
            r.owner==previous_.owner && r.owner_generation==previous_.owner_generation &&
            r.contact_generation==previous_.contact_generation && r.action_generation==previous_.action_generation &&
            r.buttons==previous_.buttons && r.phase==previous_.phase && r.spin_mode==previous_.spin_mode && r.facts==previous_.facts &&
            r.spin_epoch==previous_.spin_epoch && r.spin_serial==previous_.spin_serial &&
            r.native_valid==previous_.native_valid && r.airborne==previous_.airborne && r.enabled==previous_.enabled &&
            r.allowed==previous_.allowed && r.ready==previous_.ready && r.conflict==previous_.conflict &&
            r.owns_left==previous_.owns_left && r.armed==previous_.armed &&
            r.requested_rate==previous_.requested_rate && r.published_rate==previous_.published_rate;
        if (same) return false;
        previous_=r;controller_seen_=true;
        if (r.native_valid && r.airborne) controller_air_=true;
        else if (r.native_valid && controller_air_) controller_done_=true;
        r.kind=AngularWriterKind::controller;return reserve(r);
    }
    bool body(AngularWriterEvidence& r,const std::array<std::uintptr_t,32>& parts) noexcept {
        if (!live() || !interested_ || body_done_) return false;
        if (body_seen_ && r.step==previous_body_.step && r.native_ms==previous_body_.native_ms &&
            r.owner==previous_body_.owner && r.owner_generation==previous_body_.owner_generation &&
            r.contact_generation==previous_body_.contact_generation && r.action_generation==previous_body_.action_generation &&
            r.native_valid==previous_body_.native_valid && r.context_owner_matches==previous_body_.context_owner_matches && parts==parts_) return false;
        body_seen_=true;previous_body_=r;
        if (r.native_valid && r.airborne) body_air_=true;
        else if (r.native_valid && body_air_) body_done_=true;
        if (!body_set_ || parts!=parts_) { parts_=parts;++body_set_; }
        r.body_set=body_set_;r.kind=AngularWriterKind::body;return reserve(r);
    }
    bool late(const AngularWriterEvidence& r) const noexcept {
        return !live() || r.capture!=id_.load(std::memory_order_acquire);
    }
    void settle(bool queued) noexcept {
        if (queued) queued_.fetch_add(1,std::memory_order_relaxed);else lose();
        pending_.fetch_sub(1,std::memory_order_release);
    }
    AngularWriterEvidence end(std::uint64_t now) noexcept {
        if (!(admission_.fetch_and(~open_bit,std::memory_order_acq_rel)&open_bit)) return {};
        AngularWriterEvidence r;r.kind=AngularWriterKind::end;r.capture=id_.load(std::memory_order_relaxed);r.ms=now;
        r.pending=pending_.load(std::memory_order_acquire)+(admission_.load(std::memory_order_acquire)&~open_bit);r.reserved=reserved_;
        r.queued=queued_.load(std::memory_order_relaxed);r.lost=lost_.load(std::memory_order_relaxed);
        // Pending is sampled first. A concurrent finish may also appear in
        // queued; retain both observations and conservatively stay incomplete.
        r.complete=!r.lost && !r.pending && r.queued==r.reserved;return r;
    }
};
inline bool angular_writer_request_valid(unsigned duration,unsigned sources,bool angular)noexcept{
    return !angular || (duration>=1 && duration<=5000 && sources==39);
}
class AngularWriterObservation {
    void (*sink_)(const AngularWriterEvidence&) noexcept;
    int exceptions_{std::uncaught_exceptions()};
public:
    AngularWriterEvidence row;
    AngularWriterObservation(AngularWriterEvidence value,void (*sink)(const AngularWriterEvidence&) noexcept) noexcept : sink_(sink),row(value) {}
    AngularWriterObservation(const AngularWriterObservation&)=delete;
    ~AngularWriterObservation() noexcept { if(row.capture){row.unwinding=std::uncaught_exceptions()>exceptions_;sink_(row);} }
    void staging(bool angular,const std::array<float,3>& spins) noexcept { row.angular=angular;row.staged=spins;row.stage=GroundWriterStage::staged; }
    void validated() noexcept { row.stage=GroundWriterStage::validated; }
    void writing() noexcept { row.stage=GroundWriterStage::writing; }
    void body_completed() noexcept { ++row.completed; }
    void verified() noexcept { if(row.angular)row.stage=GroundWriterStage::verified; }
};
}

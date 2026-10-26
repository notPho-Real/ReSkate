#pragma once
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <mutex>

namespace dingosdk::camera_diagnostics {
// Observation only. These values never select a camera or authorize a write.
enum class Reason : unsigned { none, reset, policy, focus, input, binding, local,
    transform, before_publish, after_publish, exception };
enum class Writer : unsigned { debug, arcade, party, first_person };
struct Input {
    std::uint64_t owner_generation{}, route_generation{};
    unsigned owner{}, slot{4};
    short x{}, y{};
    bool inspected{}, valid{};
};
struct Pose {
    unsigned camera{}, kind{}; // 1 native gameplay, 2 FreeCamera; zero unknown.
    std::array<float,3> eye{}, back{};
    float fov{};
    bool valid{}; // No roll or final presented-image claim.
};
struct Operation {
    unsigned requester{}, before_mode{}, after_mode{}, before_camera{}, after_camera{}, stage{};
    unsigned lease_identity{}, observed_identity{};
    std::uint64_t phase_sample{}, phase_generation{}, phase_calls{};
    bool requested{}, transform{}, owned{}, uncertain{}, before_known{}, after_known{}, returned{};
};
struct Evidence {
    std::uint64_t capture{}, sequence{}, ms{}, end_ms{}, phase_sample{}, phase_generation{}, phase_calls{};
    std::uint64_t attempted{}, queued{}, lost{}, uninspected{}, client_drop_begin{}, client_drop_end{};
    unsigned kind{}, duration{}, input_count{}, operation_count{}, writer_mask{}, restore_requests{}, restore_successes{}, pending{};
    Reason reason{};
    std::array<Input,3> input{};
    std::array<Operation,3> operation{};
    std::array<std::uint64_t,4> writer_totals{}; // Includes writes outside this tick, notably render handoff.
    std::array<std::uint64_t,2> mode_requests{}; // OFF, ON calls; never suppresses or delays a mode change.
    std::array<Pose,3> view{}; // First adapter pre-operation, requested orbit, post-known-writers.
    Pose follow; // Requested constructed follow frame, not native collision output.
    std::array<float,3> root{};
    bool enabled{}, ready{}, phase{}, phase_known{}, root_known{}, nested{}, truncated{}, late{};
};
inline bool valid(const Evidence& r) noexcept {
    if(!r.capture || r.kind>2 || r.duration>3000 || r.input_count>3 || r.operation_count>3 ||
       r.writer_mask>15 || r.reason>Reason::exception || r.end_ms<r.ms || r.sequence>192) return false;
    for(const auto& i:r.input)if(i.owner>64 || i.slot>4 || (i.valid&&(!i.inspected||!i.owner||!i.owner_generation||i.slot>=4)))return false;
    for(const auto& p:{r.view[0],r.view[1],r.view[2],r.follow}){
        if(p.camera>64 || p.kind>2 || (p.valid&&(!p.camera&&p.kind)))return false;
        if(p.valid){for(float v:p.eye)if(!std::isfinite(v))return false;
            for(float v:p.back)if(!std::isfinite(v))return false;
            if(!std::isfinite(p.fov)||p.fov<=1||p.fov>=175)return false;}
    }
    for(const auto& op:r.operation)if(op.requester>1 || op.before_camera>64 || op.after_camera>64 ||
       op.lease_identity>64 || op.observed_identity>64 || op.stage>4)return false;
    for(float v:r.root)if(r.root_known&&!std::isfinite(v))return false;
    return true;
}
using Key=std::array<std::uintptr_t,11>; // Complete private camera identity or padded input owner; never serialized.
struct Frame {
    Evidence row;
    std::array<Key,3> owners{};
    std::array<std::uintptr_t,3> cameras{};
    std::array<std::uintptr_t,6> operation_cameras{};
    std::array<Key,3> leases{}, observed_identities{};
    unsigned depth{};
    bool sampled{};
};
using Sink=bool (*)(const Evidence&) noexcept;
class Capture {
    std::mutex mutex_;
    // The high bit admits observations; low bits pin the current capture until
    // each admitted begin/writer/mode attempt has finished. Retirement never
    // waits: it closes admission and defers its footer until references drain.
    static constexpr std::uint64_t open_bit=std::uint64_t{1}<<63;
    std::atomic<std::uint64_t> admission_{};
    std::atomic<std::uint64_t> active_id_{};
    class Admission {
        Capture* capture_{};
    public:
        explicit Admission(Capture& capture) noexcept {
            const auto previous=capture.admission_.fetch_add(1,std::memory_order_acq_rel);
            if(previous&open_bit)capture_=&capture;
            else capture.admission_.fetch_sub(1,std::memory_order_release);
        }
        Admission(const Admission&)=delete;
        Admission& operator=(const Admission&)=delete;
        ~Admission(){release();}
        explicit operator bool()const noexcept{return capture_!=nullptr;}
        void release()noexcept{if(capture_){capture_->admission_.fetch_sub(1,std::memory_order_release);capture_=nullptr;}}
    };
    std::atomic<std::uint64_t> contention_{};
    std::atomic<std::uint64_t> begin_misses_{};
    std::uint64_t id_{}, start_{}, until_locked_{}, attempted_{}, queued_{}, lost_{}, contention_start_{}, drop_start_{};
    std::uint64_t begin_misses_start_{};
    unsigned duration_{};
    std::atomic<unsigned> pending_{};
    std::array<std::atomic<std::uint64_t>,4> writers_{};
    std::array<std::uint64_t,4> writer_start_{};
    std::array<std::atomic<std::uint64_t>,2> modes_{};
    std::array<std::uint64_t,2> mode_start_{};
    bool stopping_{}, truncated_{};
    Sink sink_{};
    std::array<Key,64> identities_{};
    unsigned identities_used_{};
    unsigned token(const Key& key) noexcept {
        if(key==Key{})return 0;
        for(unsigned i=0;i<identities_used_;++i)if(identities_[i]==key)return i+1;
        if(identities_used_==identities_.size()){truncated_=true;return 0;}
        identities_[identities_used_]=key;return ++identities_used_;
    }
    void close(std::uint64_t now,std::uint64_t drops) noexcept {
        if(!sink_)return;
        admission_.fetch_and(~open_bit,std::memory_order_acq_rel);
        if(pending_.load()||admission_.load(std::memory_order_acquire))return;
        Evidence row;row.kind=2;row.capture=id_;row.ms=start_;row.end_ms=now;row.duration=duration_;
        row.uninspected=begin_misses_.load()-begin_misses_start_;
        row.attempted=attempted_+row.uninspected;row.queued=queued_;row.lost=lost_+contention_.load()-contention_start_;
        row.client_drop_begin=drop_start_;row.client_drop_end=drops;row.truncated=truncated_;
        for(unsigned i=0;i<4;++i)row.writer_totals[i]=writers_[i].load()-writer_start_[i];
        for(unsigned i=0;i<2;++i)row.mode_requests[i]=modes_[i].load()-mode_start_[i];
        // A missing end or client-drop change is UNKNOWN; enqueue is not disk delivery.
        (void)sink_(row);sink_=nullptr;active_id_.store(0,std::memory_order_release);
    }
public:
    bool arm(std::uint64_t now,unsigned duration,std::uint64_t drops,Sink sink) {
        std::unique_lock lock(mutex_,std::try_to_lock);
        if(!lock.owns_lock()||duration>3000||now>UINT64_MAX-duration)return false;
        if(!duration){if(!sink_)return false;stopping_=true;close(now,drops);return true;}
        if(sink_||pending_||admission_.load()||!sink||id_==UINT64_MAX)return false;
        ++id_;start_=now;until_locked_=now+duration;duration_=duration;attempted_=queued_=lost_=0;
        stopping_=truncated_=false;identities_used_=0;contention_start_=contention_.load();drop_start_=drops;sink_=sink;
        begin_misses_start_=begin_misses_.load();
        for(unsigned i=0;i<4;++i)writer_start_[i]=writers_[i].load();
        for(unsigned i=0;i<2;++i)mode_start_[i]=modes_[i].load();
        Evidence row;row.capture=id_;row.kind=0;row.ms=row.end_ms=now;row.duration=duration;row.client_drop_begin=drops;
        if(!sink_(row)){sink_=nullptr;return false;}
        admission_.fetch_or(open_bit,std::memory_order_release);
        active_id_.store(id_,std::memory_order_release);return true;
    }
    void begin(Frame& f,std::uint64_t now,std::uint64_t drops) {
        if(f.depth++){if(f.sampled){f.row.nested=true;}return;}
        f.sampled=false;
        const auto epoch=active_id_.load(std::memory_order_acquire);
        if(!epoch)return;
        Admission admission(*this);
        // Fields cannot be rearmed while an admitted reference is held.
        if(admission&&(id_!=epoch||now<start_))return;
        std::unique_lock lock(mutex_,std::try_to_lock);
        if(!lock.owns_lock()){if(admission){begin_misses_.fetch_add(1);contention_.fetch_add(1);}return;}
        if(!sink_||id_!=epoch)return;
        if(!admission||stopping_||now>until_locked_){stopping_=true;admission.release();close(now,drops);return;}
        ++attempted_;
        if(attempted_>192){++lost_;truncated_=true;return;}
        f.row={};f.owners={};f.cameras={};f.operation_cameras={};f.leases={};f.observed_identities={};f.sampled=true;++pending_;
        f.row.capture=id_;f.row.kind=1;f.row.sequence=attempted_;f.row.ms=f.row.end_ms=now;f.row.duration=duration_;
    }
    void finish(Frame& f,std::uint64_t now,std::uint64_t drops) {
        if(!f.depth||--f.depth)return;
        if(!f.sampled)return;
        f.sampled=false;f.row.end_ms=now;
        // No wait in a camera callback. A missed diagnostic remains explicit loss.
        std::unique_lock lock(mutex_,std::try_to_lock);
        if(!lock.owns_lock()){contention_.fetch_add(1);--pending_;return;}
        --pending_;
        if(!sink_||f.row.capture!=id_)return;
        f.row.late=stopping_||now>until_locked_;
        f.row.pending=pending_.load();
        for(unsigned i=0;i<4;++i)f.row.writer_totals[i]=writers_[i].load()-writer_start_[i];
        for(unsigned i=0;i<2;++i)f.row.mode_requests[i]=modes_[i].load()-mode_start_[i];
        for(unsigned i=0;i<3;++i){f.row.input[i].owner=token(f.owners[i]);f.row.view[i].camera=token({f.cameras[i],0,0,0,0});}
        for(unsigned i=0;i<3;++i){f.row.operation[i].before_camera=token({f.operation_cameras[2*i],0,0,0,0});f.row.operation[i].after_camera=token({f.operation_cameras[2*i+1],0,0,0,0});}
        for(unsigned i=0;i<3;++i){f.row.operation[i].lease_identity=token(f.leases[i]);f.row.operation[i].observed_identity=token(f.observed_identities[i]);}
        if(valid(f.row)&&sink_(f.row))++queued_;else ++lost_;
        if(stopping_||now>until_locked_){stopping_=true;close(now,drops);}
    }
    void writer(Writer value,std::uint64_t now) noexcept {
        const auto epoch=active_id_.load(std::memory_order_acquire);
        if(!epoch||static_cast<unsigned>(value)>=4)return;
        Admission admission(*this);
        if(admission&&id_==epoch&&now>=start_&&now<=until_locked_)writers_[static_cast<unsigned>(value)].fetch_add(1);
    }
    void mode(bool enabled,std::uint64_t now) noexcept {
        const auto epoch=active_id_.load(std::memory_order_acquire);
        if(!epoch)return;
        Admission admission(*this);
        if(admission&&id_==epoch&&now>=start_&&now<=until_locked_)modes_[enabled?1u:0u].fetch_add(1);
    }
};
inline Capture capture;
inline thread_local Frame frame;
inline bool observing() noexcept {return frame.sampled&&frame.depth==1;}
inline void reason(Reason value) noexcept {if(observing())frame.row.reason=value;}
inline void gate(bool enabled,bool ready,bool phase) noexcept {if(observing()){frame.row.enabled=enabled;frame.row.ready=ready;frame.row.phase=phase;}}
inline void input(bool inspected,bool valid,const Key& owner,std::uint64_t owner_generation,
        std::uint64_t route_generation,unsigned slot,short x,short y) noexcept {
    if(!observing())return;
    const auto at=frame.row.input_count++;
    if(at>=3){frame.row.input_count=3;frame.row.truncated=true;return;}
    frame.row.input[at]={owner_generation,route_generation,0,slot,x,y,inspected,valid};frame.owners[at]=owner;
}
inline void restore(bool result=false) noexcept {if(observing()){if(result)++frame.row.restore_successes;else ++frame.row.restore_requests;}}
inline void writer(Writer value,std::uint64_t now) noexcept {capture.writer(value,now);if(observing())frame.row.writer_mask|=1u<<static_cast<unsigned>(value);}
inline unsigned operation(bool transform,unsigned requester,bool owned,bool uncertain) noexcept {
    if(!observing())return 3;
    const auto at=frame.row.operation_count++;
    if(at>=3){frame.row.operation_count=3;frame.row.truncated=true;return 3;}
    auto& op=frame.row.operation[at];op.requested=true;op.transform=transform;op.requester=requester;op.owned=owned;op.uncertain=uncertain;return at;
}
inline void selection(unsigned at,bool after,unsigned mode,std::uintptr_t camera) noexcept {
    if(!observing()||at>=3)return;
    auto& op=frame.row.operation[at];frame.operation_cameras[2*at+(after?1u:0u)]=camera;
    if(after){op.after_known=true;op.after_mode=mode;}else{op.before_known=true;op.before_mode=mode;}
}
inline void returned(unsigned at) noexcept {if(observing()&&at<3)frame.row.operation[at].returned=true;}
inline void stage(unsigned at,unsigned value) noexcept {if(observing()&&at<3)frame.row.operation[at].stage=value;}
inline void identity(unsigned at,const Key& lease,const Key& observed) noexcept {
    if(observing()&&at<3){frame.leases[at]=lease;frame.observed_identities[at]=observed;}
}
inline void phase(unsigned at,std::uint64_t sample,std::uint64_t generation,std::uint64_t calls) noexcept {
    if(observing()){
        frame.row.phase_known=sample!=0;frame.row.phase_sample=sample;frame.row.phase_generation=generation;frame.row.phase_calls=calls;
        if(at<3){auto& op=frame.row.operation[at];op.phase_sample=sample;op.phase_generation=generation;op.phase_calls=calls;}
    }
}
inline void pose(unsigned at,std::uintptr_t camera,unsigned kind,const std::array<float,16>& matrix,float fov) noexcept {
    if(!observing()||(at>=3&&at!=4))return;
    for(float v:matrix)if(!std::isfinite(v))return;
    if(!std::isfinite(fov)||fov<=1||fov>=175)return;
    const Pose value{0,kind,{matrix[12],matrix[13],matrix[14]},{matrix[8],matrix[9],matrix[10]},fov,true};
    if(at==4){frame.row.follow=value;return;}
    frame.cameras[at]=camera;frame.row.view[at]=value;
}
inline void root(const std::array<float,3>& value) noexcept {if(observing()){frame.row.root=value;frame.row.root_known=true;}}
}

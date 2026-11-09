#include "arcade_wallplant_animation.h"
#include "arcade_wallplant_animation_measurement.h"
#include "Extension/AnimationStudio/game_clips.h"
#include "Extension/Skater/skater_animation.h"
#include "Extension/Skater/skater_skeleton.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Core/Log/logging.h"
#include "Engine/Game/Build/20260929/animation_values.h"
#include "Engine/Game/Build/20260929/arcade_held_flip.h"
#include <Windows.h>
#include <array>
#include <atomic>
#include <cmath>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace dingosdk::arcade_mode::wallplant_animation {
namespace {
// Original build-20260929 ClipControllerAssets in the level manifest. These
// are read from the installed game; no animation data is shipped by RC4.
constexpr std::array names{
    "animation/dingo/c_proto_onb_grab_bs_1ft_judo_footplant_full",
    "animation/dingo/c_proto_onb_grab_fakie_bs_1ft_judo_footplant_full"
};
constexpr float duration = .16f; // The admitted physical hold; visual quality still needs native measurements.
constexpr std::uint64_t lifetime_ms = 200; // 160 ms hold plus its 40 ms native-pose blend out.
struct Clips { std::array<std::shared_ptr<const anim_studio::Clip>, 4> poses; };
struct Loader {
    std::once_flag started;
    std::atomic<std::shared_ptr<const Clips>> clips;
    std::atomic<bool> loading_started{};
    std::atomic<bool> failed{};
};
Loader& loader() { static auto* value = new Loader; return *value; }
bool permitted{};
NoBailSkater held_owner{};
std::uint64_t ticket{}, began{}, occurrence{};
bool pose_reported{};
bool holding{};
bool hold_pose{};
std::array<float,3> wall_point{},wall_normal{},outgoing{};
unsigned alignment_reports{};
std::uint64_t measured_draw{};
NoBailSkater previous_hold_owner{};
std::uint64_t previous_hold_occurrence{},previous_hold_start{};
enum class Availability { unknown, not_started, loading, failed, busy, cannot_play, context_rejected, ready };
Availability last_availability{Availability::unknown};
unsigned availability_reports{};
std::optional<PlantFrame> plant_frame(const anim_studio::Clip& clip,const std::array<float,3>& wall_direction) {
    if(clip.joints<=bonecam::joint::left_foot || clip.frames.size()/clip.joints<clip.times.size())return {};
    std::optional<PlantFrame> best;
    // Select from authored native poses by reach toward the actual wall in
    // the outgoing target's upright basis. No guessed timestamp, imported
    // animation, added body translation, or fabricated contact is involved.
    for(std::size_t i=0;i<clip.times.size();++i) {
        if(!std::isfinite(clip.times[i]) || clip.times[i]<0 || clip.times[i]>clip.duration())return {};
        const auto pose=clip.frame(i);
        const auto placement=anim_studio::world(pose,anim_studio::placement_joint);
        const auto scale=(placement.scale[0]+placement.scale[1]+placement.scale[2])/3;
        if(!anim_studio::finite(placement) || !std::isfinite(scale) || scale<=.00001f)return {};
        for(const auto joint:{bonecam::joint::left_foot,bonecam::joint::right_foot}) {
            const auto foot=anim_studio::world(pose,joint);
            if(!anim_studio::finite(foot))return {};
            auto relative=anim_studio::rotate(anim_studio::conjugate(placement.rotation),
                {foot.position[0]-placement.position[0],foot.position[1]-placement.position[1],foot.position[2]-placement.position[2]});
            for(auto& value:relative)value/=scale;
            if(!consider_plant_frame(best,clip.times[i],joint,relative,wall_direction))return {};
        }
    }
    return best;
}

void report_availability(const NoBailSkater& expected, Availability reason) noexcept {
    if (reason == last_availability || availability_reports >= 32) return;
    try {
        namespace physics = game::build::v20260929::arcade_held_flip;
        NoBailSkater before{}, after{};
        std::uint32_t state{}, checked_state{};
        if (!no_bail_skater(before) || before != expected ||
            !memory::peek(before.context + physics::physics_state, state) || (state != 200 && state != 201) ||
            !no_bail_skater(after) || after != before ||
            !memory::peek(after.context + physics::physics_state, checked_state) || checked_state != state) return;
        constexpr std::array labels{"unknown", "loader_not_started", "loader_loading", "loader_failed",
            "busy_ticket", "can_play_false", "native_context_rejected", "ready"};
        logging::log(logging::Level::info, logging::Channel::skater,
            "ARCADE WALLPLANT ANIMATION AVAILABILITY: ms={} core={} state={} reason={} ticket={} approximation=true.",
            GetTickCount64(), before.core, state, labels[static_cast<std::size_t>(reason)], ticket);
        last_availability = reason;
        ++availability_reports;
    } catch (...) { }
}

void preload() {
    const auto library = anim_studio::game_clips::library();
    if (!library) return;
    auto& state = loader();
    std::call_once(state.started, [library, &state] {
        // Match game_clips' process-lifetime loader: only immutable clips cross
        // to the client thread, and no worker touches a skater or playback plan.
        std::thread([library, &state] {
            state.loading_started.store(true, std::memory_order_release);
            try {
                auto clips = std::make_shared<Clips>();
                for (std::size_t i = 0; i < names.size(); ++i) {
                    auto clip = library->load(names[i]);
                    if (clip.empty() || !std::isfinite(clip.duration()) || clip.duration() <= 0)
                        throw std::runtime_error("empty native footplant clip");
                    clips->poses[i * 2 + 1] = std::make_shared<const anim_studio::Clip>(library->mirror(clip));
                    clips->poses[i * 2] = std::make_shared<const anim_studio::Clip>(std::move(clip));
                }
                state.clips.store(std::move(clips), std::memory_order_release);
            } catch (...) { state.failed.store(true, std::memory_order_release); }
        }).detach();
    });
}

// Same independently corroborated native frame/export sources as the SDK's
// skater_frame reader. A leading-foot input bit alone cannot select a pose.
bool context(const NoBailSkater& expected, unsigned& pose,bool admitted_hold=false) noexcept {
    namespace layout = game::build::v20260929::animation_values;
    namespace physics = game::build::v20260929::arcade_held_flip;
    NoBailSkater before{}, after{};
    std::uintptr_t source{}, root{}, exported{};
    std::array<std::uint8_t, 5> flags{};
    std::array<std::uint8_t, 3> exports{};
    std::uint32_t state{}, native{}, basis{};
    if (!permitted || !no_bail_skater(before) || before != expected ||
        !memory::peek(before.context + physics::physics_state, state) ||
        (state != 200 && state != 201 && !(admitted_hold && state==100)) ||
        !memory::peek(before.core + layout::frame_source, source) || !source ||
        !memory::peek(source + layout::frame_flags, flags) || flags[0] > 1 || flags[2] > 1 || flags[3] > 1 || flags[4] > 1 ||
        !memory::peek(before.context + layout::frame_context, native) ||
        (flags[4] != 0) != ((native & layout::native_mask) != 0) ||
        !memory::peek(before.context + layout::basis_context, basis) ||
        (flags[0] != 0) != ((basis & layout::basis_mask) != 0) ||
        !memory::peek(before.core + layout::state_root, root) || !root ||
        !memory::peek(root + layout::export_state, exported) || !exported ||
        !memory::peek(exported + layout::exported_flags, exports) || exports[0] != flags[4] ||
        exports[1] != flags[2] || exports[2] != flags[3] ||
        !no_bail_skater(after) || after != before) return false;
    pose = (flags[4] ? 2u : 0u) + (flags[2] ? 1u : 0u);
    return true;
}
void report_pose(bool final) noexcept {
    if(!ticket)return;
    try {
        const auto evidence=skater_animation::playback_evidence(ticket);
        if(!evidence || (!final && (pose_reported || evidence->samples<2 || !evidence->draws)))return;
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE WALLPLANT POSE EVIDENCE: occurrence={} ticket={} final={} samples={} writes={} draws={} sample_first={} sample_last={} approximation=true actor_initialized=false foot_wall_alignment_verified=false.",
            occurrence,ticket,final,evidence->samples,evidence->writes,evidence->draws,
            evidence->first_sample,evidence->last_sample);
        pose_reported=true;
    } catch(...) {}
}
void report_alignment() noexcept {
    if(!holding || !ticket || alignment_reports>=8)return;
    try {
        const auto evidence=skater_animation::playback_evidence(ticket);
        const auto now=GetTickCount64();
        NoBailSkater owner{};
        if(!evidence || !evidence->feet_valid || !evidence->draws || evidence->draws==measured_draw ||
           evidence->feet_drawn_at<began || evidence->feet_drawn_at-began>=hold_duration_ms ||
           now<evidence->feet_drawn_at || now-evidence->feet_drawn_at>100 ||
           !no_bail_skater(owner) || owner!=held_owner)return;
        const auto left=measure_foot_wall(evidence->left_foot,wall_point,wall_normal);
        const auto right=measure_foot_wall(evidence->right_foot,wall_point,wall_normal);
        if(!left || !right)return;
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE WALLPLANT FOOT MEASUREMENT: ms={} occurrence={} ticket={} component={} drawn_at={} draws={} hold_started={} left={},{},{} right={},{},{} point={},{},{} normal={},{},{} left_plane={} left_tangent={} right_plane={} right_tangent={} rendered_joint_origins=true physical_contact_verified=false foot_wall_alignment_verified=false.",
            now,occurrence,ticket,evidence->component,evidence->feet_drawn_at,evidence->draws,began,
            evidence->left_foot[0],evidence->left_foot[1],evidence->left_foot[2],
            evidence->right_foot[0],evidence->right_foot[1],evidence->right_foot[2],
            wall_point[0],wall_point[1],wall_point[2],wall_normal[0],wall_normal[1],wall_normal[2],
            left->signed_plane_distance,left->tangent_distance,right->signed_plane_distance,right->tangent_distance);
        measured_draw=evidence->draws;++alignment_reports;
    } catch(...) {}
}
}

void cancel() noexcept {
    report_alignment();
    report_pose(true);
    try { skater_animation::cancel_play(ticket); } catch (...) { }
    ticket = began = occurrence = 0;
    held_owner = {};
    pose_reported = false;
    holding=hold_pose=false;wall_point={};wall_normal={};outgoing={};alignment_reports=0;measured_draw=0;
}
void tick(bool gameplay) noexcept {
    permitted = gameplay;
    try {
        if (gameplay) {
            preload();
            skater_animation::prepare_playback();
        }
        if (!ticket) return;
        unsigned pose{};
        const auto now = GetTickCount64();
        if (!context(held_owner, pose,hold_pose) || now < began ||
            now-began >= lifetime_ms || !skater_animation::owns_play(ticket)) {
            cancel(); return;
        }
        if(holding && now-began>=hold_duration_ms)release(held_owner,occurrence);
        // Original local callbacks report sampling, successful render writes,
        // and the renderer's copied-pose callback for this owned ticket only.
        report_pose(false);
        report_alignment();
    } catch (...) { cancel(); }
}
bool available(const NoBailSkater& owner) noexcept {
    unsigned pose{};
    auto reason = Availability::ready;
    if (ticket) reason = Availability::busy;
    else if (!loader().clips.load(std::memory_order_acquire)) {
        if (loader().failed.load(std::memory_order_acquire)) reason = Availability::failed;
        else reason = loader().loading_started.load(std::memory_order_acquire) ?
            Availability::loading : Availability::not_started;
    } else if (!skater_animation::can_play()) reason = Availability::cannot_play;
    else if (!context(owner, pose)) reason = Availability::context_rejected;
    report_availability(owner, reason);
    return reason == Availability::ready;
}
bool begin_hold(const NoBailSkater& owner,std::uint64_t identity,std::uint64_t started_at,
        const std::array<float,3>& point,const std::array<float,3>& normal,const std::array<float,3>& forward) noexcept {
    try {
        unsigned pose{};
        // A repeated client observation cannot renew the original physical clock.
        if(ticket)return holding && held_owner==owner && occurrence==identity && began==started_at &&
            point==wall_point && normal==wall_normal && forward==outgoing && hold_remaining_ms(began,GetTickCount64()).has_value() &&
            context(owner,pose,true) && skater_animation::owns_play(ticket);
        if(previous_hold_owner==owner && previous_hold_occurrence==identity && previous_hold_start==started_at)return false;
        if(!identity || !measure_foot_wall(point,point,normal) || !context(owner,pose,true) ||
           !skater_animation::can_play())return false;
        const auto clips=loader().clips.load(std::memory_order_acquire);
        if(!clips)return false;
        const auto& clip=clips->poses[pose];
        if(!hold_remaining_ms(started_at,GetTickCount64()) || started_at>UINT64_MAX-hold_duration_ms)return false;
        const auto direction=hold_wall_direction(normal,forward);
        if(!direction)return false;
        const auto planted=plant_frame(*clip,*direction);
        if(!planted)return false;
        skater_animation::Play play;
        play.clip=clip;play.blend=.04f;play.at=planted->time;
        const auto accepted=skater_animation::try_hold(std::move(play),started_at+hold_duration_ms);
        if(!accepted)return false;
        ticket=accepted;held_owner=owner;occurrence=identity;began=started_at;holding=hold_pose=true;
        previous_hold_owner=owner;previous_hold_occurrence=identity;previous_hold_start=started_at;
        wall_point=point;wall_normal=normal;outgoing=forward;pose_reported=false;alignment_reports=0;measured_draw=0;
        if(!hold_remaining_ms(started_at,GetTickCount64()) || !context(owner,pose,true)){cancel();return false;}
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE WALLPLANT POSE HOLD: ms={} occurrence={} ticket={} started={} clip_at={} joint={} reach={} clip={} mirrored={} duration={} held=true selection_basis=upright_outgoing_native_clip selection_pitch_roll_unobserved=true native_bind_pose_unobserved=true approximation=true actor_initialized=false foot_wall_alignment_verified=false.",
            GetTickCount64(),identity,ticket,started_at,planted->time,planted->joint,planted->reach,names[pose/2],(pose&1u)!=0,duration);
        return true;
    } catch(...) {return false;}
}
void release(const NoBailSkater& owner,std::uint64_t identity) noexcept {
    if(!holding || held_owner!=owner || occurrence!=identity)return;
    try {
        report_alignment();
        if(!skater_animation::release_play(ticket)){cancel();return;}
        holding=false;
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE WALLPLANT POSE RELEASE: ms={} occurrence={} ticket={} started={}.",
            GetTickCount64(),identity,ticket,began);
    } catch(...) {cancel();}
}
}

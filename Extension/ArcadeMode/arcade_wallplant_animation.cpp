#include "arcade_wallplant_animation.h"
#include "arcade_wallplant_animation_measurement.h"
#include "arcade_wallplant_animation_assets.h"
#include "arcade_wallplant_animation_contact.h"
#include "Extension/AnimationStudio/game_clips.h"
#include "Extension/Skater/skater_animation.h"
#include "Extension/Skater/body_pose.h"
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
// The native sequence uses this stationary clip for its held phase. The
// similarly named full clips contain absorb and trick-out motion, not the
// plant hold. Read the shared held clip from the installed game and mirror
// it for the observed stance; no animation data is shipped by RC4.
constexpr float duration = .16f; // The admitted physical hold; visual quality still needs native measurements.
constexpr std::uint64_t blend_out_ms = 40;
struct Loader {
    std::once_flag started;
    std::atomic<std::shared_ptr<const Clips>> clips;
    std::atomic<bool> loading_started{};
    std::atomic<bool> failed{};
};
Loader& loader() { static auto* value = new Loader; return *value; }
bool permitted{};
NoBailSkater held_owner{};
std::uint64_t ticket{}, began{}, occurrence{}, hold_expires{};
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
                auto clips = std::make_shared<Clips>(load_held_poses(*library));
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
    // Native/fakie context remains corroborated above. Its distinct full
    // release clips do not select a different stationary held phase.
    pose = flags[2] ? 1u : 0u;
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
           !hold_remaining_ms(began,evidence->feet_drawn_at,hold_expires) ||
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
        if(evidence->contact) {
            const auto& contact=*evidence->contact;
            const auto reference=measure_foot_wall(contact.reference_point,wall_point,wall_normal);
            if(reference) {
                float alignment{};
                for(std::size_t i=0;i<3;++i)alignment+=contact.normal[i]*wall_normal[i];
                logging::log(logging::Level::info,logging::Channel::skater,
                    "ARCADE WALLPLANT CONTACT FRAME: occurrence={} ticket={} drawn_at={} joint={} reference={},{},{} normal={},{},{} plane={} normal_alignment={} target_error={} reachable={} body_frame_applied={} authored_reference=true sole_surface_verified=false.",
                    occurrence,ticket,evidence->feet_drawn_at,contact.joint,contact.reference_point[0],contact.reference_point[1],
                    contact.reference_point[2],contact.normal[0],contact.normal[1],contact.normal[2],
                    reference->signed_plane_distance,alignment,contact.target_error,contact.reachable,contact.body_frame_applied);
            }
        }
        measured_draw=evidence->draws;++alignment_reports;
    } catch(...) {}
}
}

void cancel() noexcept {
    report_alignment();
    report_pose(true);
    try { skater_animation::cancel_play(ticket); } catch (...) { }
    ticket = began = occurrence = hold_expires = 0;
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
            (void)skater_animation::pose_observation();
        }
        if (!ticket) return;
        unsigned pose{};
        const auto now = GetTickCount64();
        if (!context(held_owner, pose,hold_pose) || now < began ||
            now>=hold_expires+blend_out_ms || !skater_animation::owns_play(ticket)) {
            cancel(); return;
        }
        // The physical receipt releases this pose. This fixed expiry only
        // bounds a missed client notification; it never renews a hold.
        if(holding && now>=hold_expires)release(held_owner,occurrence);
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
bool begin_hold(const NoBailSkater& owner,std::uint64_t identity,std::uint64_t started_at,std::uint64_t expires_at,
        const std::array<float,3>& point,const std::array<float,3>& normal,const std::array<float,3>& forward) noexcept {
    try {
        unsigned pose{};
        // A repeated client observation cannot renew the original physical clock.
        if(ticket)return holding && held_owner==owner && occurrence==identity && began==started_at &&
            expires_at==hold_expires && point==wall_point && normal==wall_normal && forward==outgoing && hold_remaining_ms(began,GetTickCount64(),hold_expires).has_value() &&
            context(owner,pose,true) && skater_animation::owns_play(ticket);
        if(previous_hold_owner==owner && previous_hold_occurrence==identity && previous_hold_start==started_at)return false;
        if(!identity || !measure_foot_wall(point,point,normal) || !context(owner,pose,true) ||
           !skater_animation::can_play())return false;
        const auto clips=loader().clips.load(std::memory_order_acquire);
        if(!clips)return false;
        const auto& clip=clips->poses[pose];
        if(!hold_remaining_ms(started_at,GetTickCount64(),expires_at) || !expires_at || expires_at>UINT64_MAX-blend_out_ms)return false;
        const auto native_pose=skater_animation::pose_observation();
        unsigned checked_pose{};
        // Match the existing render-publication freshness bound. A missing or
        // stale observation cannot be replaced by the requested future facing.
        if(!native_pose || !std::isfinite(native_pose->age) || native_pose->age<0 || native_pose->age>.1 ||
           !context(owner,checked_pose,true) || checked_pose!=pose)return false;
        const auto placement=anim_studio::world(native_pose->joints,anim_studio::placement_joint);
        if(!anim_studio::finite(placement))return false;
        const auto direction=hold_wall_direction(normal,placement.rotation);
        if(!direction)return false;
        const auto contact=plant_contact(*clip,clips->ground,*direction,native_pose->joints,true,ContactSurface{point,normal});
        if(!contact)return false;
        const auto planted=std::optional(contact->sample);
        if(!planted)return false;
        skater_animation::Play play;
        play.clip=clip;play.blend=.04f;play.at=planted->time;
        // The authored clip supplies the pose and leg bend. The existing
        // render-pose path anchors its selected foot on this genuine wall plane
        // using current native placement, without changing physical bodies.
        play.contact=skater_animation::Play::Contact{planted->joint,point,normal,contact->frame};
        play.contact->level_body=true;
        const auto accepted=skater_animation::try_hold(std::move(play),expires_at);
        if(!accepted)return false;
        ticket=accepted;held_owner=owner;occurrence=identity;began=started_at;hold_expires=expires_at;holding=hold_pose=true;
        previous_hold_owner=owner;previous_hold_occurrence=identity;previous_hold_start=started_at;
        wall_point=point;wall_normal=normal;outgoing=forward;pose_reported=false;alignment_reports=0;measured_draw=0;
        if(!hold_remaining_ms(started_at,GetTickCount64(),hold_expires) || !context(owner,checked_pose,true) || checked_pose!=pose){cancel();return false;}
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE WALLPLANT POSE HOLD: ms={} occurrence={} ticket={} started={} clip_at={} joint={} reach={} clip={} mirrored={} duration={} held=true selection_basis=observed_native_body_pose component={} pose_sequence={} pose_age={} placement_rotation={},{},{},{} native_body_pose_observed=true approximation=true actor_initialized=false foot_wall_alignment_verified=false.",
            GetTickCount64(),identity,ticket,started_at,planted->time,planted->joint,planted->reach,held_clip_name,pose!=0,duration,
            native_pose->component,native_pose->sequence,native_pose->age,placement.rotation[0],placement.rotation[1],
            placement.rotation[2],placement.rotation[3]);
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

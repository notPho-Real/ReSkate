#include "arcade_wallplant_animation.h"
#include "Extension/AnimationStudio/game_clips.h"
#include "Extension/Skater/skater_animation.h"
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
constexpr float duration = .16f; // classic arcade Physics_Wallplant_Duration; prototype, not Skate acceptance.
constexpr std::uint64_t lifetime_ms = 200; // 160 ms clip plus its 40 ms native-pose blend out.
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
bool context(const NoBailSkater& expected, unsigned& pose) noexcept {
    namespace layout = game::build::v20260929::animation_values;
    namespace physics = game::build::v20260929::arcade_held_flip;
    NoBailSkater before{}, after{};
    std::uintptr_t source{}, root{}, exported{};
    std::array<std::uint8_t, 5> flags{};
    std::array<std::uint8_t, 3> exports{};
    std::uint32_t state{}, native{}, basis{};
    if (!permitted || !no_bail_skater(before) || before != expected ||
        !memory::peek(before.context + physics::physics_state, state) || (state != 200 && state != 201) ||
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
}

void cancel() noexcept {
    report_pose(true);
    try { skater_animation::cancel_play(ticket); } catch (...) { }
    ticket = began = occurrence = 0;
    held_owner = {};
    pose_reported = false;
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
        if (!context(held_owner, pose) || now < began || now - began >= lifetime_ms || !skater_animation::owns_play(ticket)) {
            cancel(); return;
        }
        // Original local callbacks report sampling, successful render writes,
        // and the renderer's copied-pose callback for this owned ticket only.
        report_pose(false);
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
bool request(const NoBailSkater& owner, std::uint64_t identity) noexcept {
    try {
        if (!identity || !available(owner)) return false;
        unsigned pose{};
        if (!context(owner, pose)) return false;
        const auto clips = loader().clips.load(std::memory_order_acquire);
        if (!clips) return false;
        const auto& clip = clips->poses[pose];
        skater_animation::Play play;
        play.clip = clip;
        play.speed = clip->duration() / duration;
        play.blend = .04f;
        // No floor placement, loop, pause, state ID, trajectory or scoring write.
        const auto accepted = skater_animation::try_play(std::move(play));
        if (!accepted) return false;
        ticket = accepted; held_owner = owner; occurrence = identity; began = GetTickCount64();
        pose_reported = false;
        logging::log(logging::Level::info, logging::Channel::skater,
            "ARCADE WALLPLANT POSE: occurrence={} ticket={} clip={} mirrored={} duration={} approximation=true requested=true actor_initialized=false render_verified=false.",
            occurrence, ticket, names[pose / 2], (pose & 1u) != 0, duration);
        return true;
    } catch (...) { return false; }
}
}

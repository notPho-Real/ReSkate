#include "Extension/World/local_population_controls.h"
#include "arcade_mode.h"
#include "arcade_air_motion.h"
#include "arcade_movement.h"
#include "arcade_fixture_geometry.h"
#include "arcade_observation.h"
#include "arcade_native_probe.h"
#include "arcade_input_telemetry.h"
#include "arcade_gesture.h"
#include "arcade_rotation.h"
#include "arcade_score_probe.h"
#include "arcade_native_score.h"
#include "arcade_native_events.h"
#include "arcade_manual_observation.h"
#include "arcade_manual_release.h"
#include "arcade_manual_hop.h"
#include "arcade_manual_acquisition.h"
#include "arcade_manual_balance_entry.h"
#include "arcade_native_readiness.h"
#include "arcade_native_trick.h"
#include "arcade_authored_gestures.h"
#include "arcade_playback_packet.h"
#include "arcade_camera.h"
#include "arcade_balance_input.h"
#include "arcade_balance_hud.h"
#include "arcade_grind_runtime.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Game/Build/20260929/arcade_native_probe.h"
#include "Engine/Game/Build/20260929/skater_state.h"
#include "Engine/Game/Build/20260929/no_bail.h"
#include "Engine/Game/Build/20260929/grind_exit.h"
#include "Extension/Skater/no_bail.h"
#include "Extension/HallOfMeat/hall_of_meat_skater.h"
#include "Extension/Trainer/trainer.h"
#include "Extension/Skater/client_source_spawn.h"
#include "Extension/Multiplayer/Remote/native_skater.h"
#include "Extension/Multiplayer/Hud/native_party.h"
#include "Extension/Multiplayer/Hud/follow_camera.h"
#include "Engine/Core/Log/logging.h"
#include "Engine/Game/Input/controller_bindings.h"
#include "Engine/Game/Settings/simulation_time.h"
#include "Extension/UI/Overlay/overlay.h"
#include "Extension/Profile/local_profile_runtime.h"
#include "Extension/Multiplayer/Hud/game_ui_state.h"
#include <array>
#include <atomic>
#include <cmath>
#include <limits>
#include <mutex>
#include <string>
#include <stdexcept>
#include <memory>
#include <thread>
#include <Windows.h>

namespace dingosdk::arcade_mode {
namespace {
std::atomic<bool> ready{};
std::atomic<bool> mode_enabled{};
std::atomic<bool> auto_kick{true};
std::atomic<SpinMode> selected_spin_mode{SpinMode::held};
bool preference_loaded{};
std::uintptr_t score_probe_base{};
// Only touched by the client thread, after initialization has been published.
std::uint32_t previous_buttons{};
ObservationTracker observations;
std::array<std::uint64_t, 4> press_counts{};
constexpr std::array<std::uint32_t, 4> face_buttons{0x1000u, 0x2000u, 0x4000u, 0x8000u};
constexpr std::array<char, 4> button_names{'A', 'B', 'X', 'Y'};
NativeRequestCapture request_capture;
bool request_probe_ready{};
std::uint64_t capture_started{}, capture_id{};
std::uint64_t hil_heartbeat_at{}, hil_heartbeat_id{};
std::atomic<std::uint64_t> ollie_eligible_until{};
std::atomic<std::uint64_t> gameplay_valid_until{};
std::atomic<std::uint64_t> push_eligible_until{};
std::atomic<std::uint64_t> board_eligible_until{};
std::atomic<std::uint64_t> offboard_valid_until{};
std::atomic<std::uint64_t> manual_eligible_until{};
std::atomic<std::uint64_t> manual_prepare_until{};
std::atomic<std::uint64_t> manual_release_until{};
std::atomic<std::uint64_t> action_eligible_until{};
std::atomic<std::uint64_t> air_flip_ready_until{};
std::atomic<SkatePose> manual_observed{SkatePose::unknown};
std::atomic<bool> manual_balance_admitted{};
std::atomic<float> push_speed{};
std::atomic<std::uint64_t> motion_capture_until{};
std::atomic<std::uint64_t> movement_eligible_until{};
bool native_board_velocity(std::array<float, 3>& velocity) noexcept;
NativeInputReadiness native_readiness; // Protected by ollie_mutex.
// Preserve the named production Manual sampling allowance. Canonical strict
// normalization remains an explicit policy rather than an implicit migration.
ArcadeInput arcade_input(true, InputPolicy::production());
std::mutex ollie_mutex;
std::atomic<PlaybackExperiment> playback_mode{PlaybackExperiment::delivery};
bool playback_diagnostics{}; // Protected by ollie_mutex; off in ordinary gameplay.
std::array<PlaybackPacket, 4> playback_packets;
std::array<std::uint64_t, 4> playback_polls{};
std::array<NeutralStickDelivery, 4> neutral_delivery;
std::atomic<ActionExperiment> action_mode{default_action_mode};
SurfaceGeometrySample surface_geometry_snapshot;
std::atomic<bool> surface_geometry_available{};
TrafficSurfaceMotion traffic_surface_snapshot;
std::atomic<bool> traffic_surface_available{};
std::array<AirRotation, 4> rotations;
std::array<NativeCharge, 4> native_charges;
// Bounded edge evidence at the game-facing input boundary, not requested lab input.
InputTelemetryLink input_telemetry;
CaptureCorrelationProducer input_capture_correlation; // ollie_mutex, same owners as native/input adapters.
std::uint64_t capture_input_cursor{},capture_recognition_sequence{},capture_admission_sequence{},capture_termination_sequence{};
std::uint64_t capture_attempt_cursor{},capture_omission_cursor{},capture_sample_cursor{};
bool capture_row(const CaptureCorrelationEvidence& row) noexcept {return input_telemetry.client(row);}
void capture_event(CaptureCorrelationSource source,std::uint64_t sequence,std::uint64_t now,const NativeInputFrame& proof,bool known=true) noexcept {
    if(!input_capture_correlation.active())return;
    if(!input_capture_correlation.captures(source)){
        input_capture_correlation.maintain(now,proof,input_telemetry.client_dropped(),capture_row);return;
    }
    CaptureCorrelationEvidence row;row.source=source;row.source_sequence=sequence;row.ms=now;row.events_known=known;
    input_capture_correlation.event(row,proof,input_telemetry.client_dropped(),capture_row);
}

// Consume the actual per-poll batch while holding ollie_mutex. Each observed
// attempt has one summary; exact ordered members use the SAME bounded channel.
void capture_recognitions(const UnifiedInput::Diagnostic& diagnostic,std::uint64_t delivery_now,const NativeInputFrame& authority)noexcept{
    if(!input_capture_correlation.captures(CaptureCorrelationSource::recognition)||diagnostic.samples==capture_sample_cursor)return;
    std::uint64_t span{},omission_delta{};
    const auto previous=capture_attempt_cursor;
    if(diagnostic.samples<capture_sample_cursor||!capture_correlation_delta(previous,diagnostic.recognized,span)||
       !capture_correlation_delta(capture_omission_cursor,diagnostic.recognition_dropped,omission_delta))input_capture_correlation.lost();
    capture_sample_cursor=diagnostic.samples;capture_attempt_cursor=diagnostic.recognized;capture_omission_cursor=diagnostic.recognition_dropped;
    if(diagnostic.recognition_overflow)input_capture_correlation.lost();
    std::uint64_t seen{},last_sequence=previous,unidentified{},charged_missing{};
    const auto charge_missing=[&](std::uint64_t count){
        input_capture_correlation.omitted(count);
        if(!capture_correlation_add(charged_missing,count)||!capture_correlation_add(capture_recognition_sequence,count))input_capture_correlation.lost();
    };
    if(diagnostic.recognition_count>diagnostic.recognition.size())input_capture_correlation.lost();
    const auto count=std::min(diagnostic.recognition_count,diagnostic.recognition.size());
    for(std::size_t i=0;i<count;++i){
        const auto& attempt=diagnostic.recognition[i];const auto& request=attempt.request;
        const auto identity=request.recognition; // Constant-size immutable ownership, not an array copy.
        if(!identity||!identity->sequence||identity->sequence_overflow){++unidentified;input_capture_correlation.lost();continue;}
        if(identity->sequence<=previous)continue; // A reset without a new poll cannot republish a row.
        if(identity->sequence<=last_sequence||identity->sequence>diagnostic.recognized){input_capture_correlation.lost();continue;}
        charge_missing(identity->sequence-last_sequence-1);
        last_sequence=identity->sequence;++seen;
        CaptureCorrelationEvidence row;row.source=CaptureCorrelationSource::recognition;row.ms=row.recognized_ms=request.recognized_at;
        row.recognition_id=identity->sequence;row.recognition_session=request.session;row.action=static_cast<unsigned>(request.action);row.context=static_cast<unsigned>(request.context);
        row.queue_admitted=attempt.queued;row.footprint_kind=static_cast<unsigned>(identity->footprint_kind);
        const auto& footprint=identity->footprint;
        if(identity->footprint_kind==InputFootprintKind::events){
            if(!footprint.count||footprint.count>footprint.events.size()){input_capture_correlation.omitted(1);input_capture_correlation.lost();continue;}
            row.member_count=footprint.count;row.first_event=UINT64_MAX;
            bool valid=true,held=!footprint.held_reuse;
            for(std::size_t j=0;j<footprint.count;++j){const auto id=footprint.events[j];valid=valid&&id;held=held||id==footprint.held_reuse;row.first_event=std::min(row.first_event,id);row.last_event=std::max(row.last_event,id);}
            if(!valid||!held||footprint.first_at>footprint.last_at||footprint.last_at>request.recognized_at){input_capture_correlation.omitted(1);input_capture_correlation.lost();continue;}
            row.first_ms=footprint.first_at;row.last_ms=footprint.last_at;row.held_reuse=footprint.held_reuse;row.events_known=true;
        }else if(identity->footprint_kind==InputFootprintKind::input_free){
            if(footprint.count||footprint.held_reuse||footprint.first_at||footprint.last_at){input_capture_correlation.omitted(1);input_capture_correlation.lost();continue;}
            row.events_known=true;
        }
        const auto emit=[&](CaptureCorrelationEvidence event){
            if(!capture_correlation_add(capture_recognition_sequence)){input_capture_correlation.omitted(1);input_capture_correlation.lost();return;}
            event.source_sequence=capture_recognition_sequence;
            input_capture_correlation.recognition(event,identity->observation,identity->observation_known,delivery_now,authority,input_telemetry.client_dropped(),capture_row);
        };
        emit(row);
        for(std::size_t j=0;j<row.member_count;++j){row.member_index=j+1;row.member_event=footprint.events[j];emit(row);}
    }
    // The cumulative attempt span already includes current diagnostic omissions
    // AND missed polls. Never add the omission counter a second time. Unknown
    // omitted-member arrays were never encoded: count their mandatory summary
    // as lost, with unavailable completeness instead of inventing member rows.
    if(seen>span){input_capture_correlation.lost();return;}
    auto missing=span-seen;
    if(omission_delta>missing||diagnostic.recognition_omitted>missing)input_capture_correlation.lost();
    if(diagnostic.recognized==UINT64_MAX&&previous==UINT64_MAX){
        if(!capture_correlation_add(missing,unidentified)||!capture_correlation_add(missing,diagnostic.recognition_omitted))input_capture_correlation.lost();
    }else if(unidentified)input_capture_correlation.lost();
    if(charged_missing>missing)input_capture_correlation.lost();
    else charge_missing(missing-charged_missing);
}
std::array<PadEvidence, 4> previous_pad{};
std::array<PadEvidence, 4> previous_source_pad{};
std::uint64_t pad_sequence{};
std::once_flag authored_started;
std::atomic<std::shared_ptr<const std::vector<AuthoredGesture>>> authored_catalog;
std::atomic<std::shared_ptr<const std::vector<PreparedGesture>>> prepared_catalog;
std::atomic<std::uint64_t> input_game_tick{}, input_step_serial{}, input_step_at{}, input_owner_epoch{1};
NativeInputState native_input_state; // Protected by ollie_mutex; native physics is the sole producer.
NativeInputOwner input_owner(const NoBailSkater& owner) noexcept { return {owner.base,owner.entity,owner.core,owner.context,owner.rig}; }
NativePushContact push_contact_policy;
NoBailSkater push_contact_owner{};
std::atomic<bool> reset_push_contact_policy{true};
std::atomic<bool> input_clock_valid{};
AuthoredTickClock authored_clock;
CameraOrbit camera_orbit;
std::uint64_t camera_at{};
bool camera_lease_requested{};
std::atomic<unsigned> authored_status{};
std::atomic<unsigned> authored_stage{}, authored_field{}, authored_error{}, authored_rejected{};
std::size_t authored_reported{};
void prepare_authored_catalog() noexcept {
    try {
        std::call_once(authored_started, [] {
            authored_status.store(1, std::memory_order_release);
            std::thread([] {
                AuthoredLoadDiagnostics diagnostic{};
                bool loaded{};
                try {
                    std::vector<wchar_t> path(32768);
                    const auto length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
                    if (!length || length >= path.size()) throw std::runtime_error("Game folder unavailable");
                    const auto root = std::filesystem::path(std::wstring(path.data(), length)).parent_path();
                    auto catalog = std::make_shared<const std::vector<AuthoredGesture>>(read_authored_gestures(root, &diagnostic));
                    auto prepared = std::make_shared<const std::vector<PreparedGesture>>(prepare_gestures(*catalog));
                    if (prepared->empty()) throw std::runtime_error("No playable authored gestures");
                    prepared_catalog.store(std::move(prepared), std::memory_order_release);
                    authored_catalog.store(std::move(catalog), std::memory_order_release);
                    loaded = true;
                } catch (...) {
                    if (!diagnostic.error) diagnostic.error = 3;
                }
                authored_stage.store(diagnostic.stage, std::memory_order_release);
                authored_field.store(diagnostic.field, std::memory_order_release);
                authored_error.store(diagnostic.error, std::memory_order_release);
                authored_rejected.store(diagnostic.rejected, std::memory_order_release);
                authored_status.store(loaded ? 2u : 3u, std::memory_order_release);
            }).detach();
        });
    } catch (...) {
        authored_status.store(3, std::memory_order_release);
    }
}
ComboSession combo_session(native_combo_tuning());
ManualPhysics physical_manual;
ManualRelease manual_release;
ManualAcquisition manual_acquisition; // Input mutex protects request-to-native cleanup.
GroundManualBalance ground_manual_balance; // Same mutex; delivered basic ground contact only.
ManualHopContinuity manual_hop; // Input mutex protects admitted jump continuity.
std::atomic<bool> reset_physical_manual{true};
ManualObservation manual_identity;
std::uint64_t manual_identity_epoch{1};
std::uint64_t combo_tick_at{};
std::uint64_t observed_wipeouts{};
bool had_combo_owner{};
NoBailSkater combo_owner{};
NativeBailStatus previous_balance_bail{};
SkatePose previous_pose{SkatePose::unknown};
std::uint64_t next_balance_sample{};
BalanceControl balance_control;
void update_combo(const NativeRequestSample& native, const NoBailSkater& owner, const ControllerInput& input,
                  bool watch_valid, std::uint64_t wipeouts, std::uint64_t now,
                  const ScoreLabelOccurrence& label, const ScoreLabelOccurrence& manual_label,
                  bool native_push_contact, float simulation_step) noexcept {
    if (reset_physical_manual.exchange(false, std::memory_order_acq_rel)) {
        { std::lock_guard lock(ollie_mutex); manual_hop.reset(); manual_acquisition.reset(); ground_manual_balance.reset(); }
        physical_manual.reset(); manual_release.reset(); manual_identity.reset(manual_label); combo_session.invalidate();
        balance_control.reset_manual();
    }
    const auto score = score_activity();
    NativeInputFrame native_frame{};
    {
        std::lock_guard lock(ollie_mutex);
        native_frame = native_input_state.correlate(GetTickCount64(),input_owner(owner),native);
    }
    const bool valid = native_frame.valid && native.valid && input.available && watch_valid &&
        std::isfinite(simulation_step) && simulation_step >= .001f && simulation_step <= .1f;
    const bool owner_changed = valid && had_combo_owner &&
        (owner != combo_owner || manual_identity_epoch != native_frame.owner_generation);
    if (valid) manual_identity_epoch = native_frame.owner_generation;
    const bool observation_gap = combo_tick_at &&
        (now < combo_tick_at || now - combo_tick_at > ScoreLabelEdges::freshness_ms);
    if (!valid || owner_changed) {
        physical_manual.reset();
        balance_control.reset_manual();
        cancel_balance_bail();
        combo_session.invalidate();
        combo_tick_at = 0;
    }
    const bool bail = valid && had_combo_owner && !owner_changed && wipeouts != observed_wipeouts;
    if (bail || observation_gap) { physical_manual.reset(); balance_control.reset_manual(); }
    had_combo_owner = valid;
    combo_owner = valid ? owner : NoBailSkater{};
    observed_wipeouts = wipeouts;
    const bool activity_fresh = score.valid && now >= score.sampled_at && now - score.sampled_at <= 150;
    const auto candidate_pose = observed_pose({valid, activity_fresh,
        score.active, bail, native.state, native.crouch_mode, native.words[1] & 0xc000u});
    // Basic delivered ground gestures join a new physical contact before name
    // presentation. Independent history remains the route for other entries.
    // Neither path establishes monetary ownership or an animation clock.
    // A delivering putdown owns termination. Delayed presentation cannot
    // re-admit balance or retain the old identity after actual completion.
    if (manual_release.pending()) manual_identity.reset(manual_label);
    const auto history_observation = manual_identity.poll(candidate_pose, manual_label,
        {now, manual_identity_epoch, enabled(), valid && !bail, activity_fresh,
         owner_changed, observation_gap,
         manual_contact_pose({valid, false, false, bail, native.state, native.crouch_mode, native.words[1] & 0xc000u},
             (native.words[0] & 0x40u) != 0), (native.words[0] & 0x40u) != 0});
    ManualObservationResult manual_observation{};
    const auto physical_contact = manual_acquisition_pose(native_frame,bail);
    bool acquisition_putdown{}, acquiring_history{};
    {
        std::lock_guard lock(ollie_mutex);
        const auto ground = ground_manual_balance.sample(now,native_frame,
            enabled() && valid && !owner_changed && !observation_gap && !bail,
            !manual_release.pending() && !combo_session.failed_section() && !combo_session.manual_putdown());
        // Original history and owned physical contact retain distinct admission
        // evidence; an absent/delayed name cannot suppress acquired balance.
        manual_observation = manual_balance_observation(ground,history_observation);
        if (ground.confirmed) manual_identity.reset(manual_label); // One physical section cannot also queue a delayed history admission.
        acquisition_putdown = manual_acquisition.step(now,native_frame,physical_contact,manual_observation.confirmed,
            valid && !owner_changed && !observation_gap && !bail, (native.words[0] & 0x40u) != 0);
        acquiring_history = manual_acquisition.preserves_history(now,native_frame);
    }
    // Late presentation cannot reopen a section already delivering putdown.
    const auto pose = acquisition_putdown && balance_kind(manual_observation.pose) == BalanceKind::manual ?
        SkatePose::riding : manual_observation.pose;
    const bool manual_safe = valid && !owner_changed && !bail &&
        ((native.state == 100 && native.crouch_mode == 1) || (native.state >= 200 && native.state < 300));
    action_eligible_until.store(action_input_eligible({valid, false, false, bail, native.state,
        native.crouch_mode, 0}) ? now + 150 : 0, std::memory_order_release);
    const float dt = combo_tick_at && now >= combo_tick_at ? static_cast<float>(now - combo_tick_at) / 1000.0f : 0;
    combo_tick_at = now;
    const auto kind = combo_session.model().state().balance_kind;
    const bool manual_pose = balance_kind(pose) == BalanceKind::manual;
    if (manual_pose && physical_manual.active() && physical_manual.nose() != (pose == SkatePose::nose_manual))
        balance_control.reset_manual();
    InputControlPair configured_manual{};
    if(manual_pose){
        std::lock_guard lock(ollie_mutex);
        auto controls=InputBalanceControls::vertical(pose==SkatePose::nose_manual ? InputBalanceKind::nose : InputBalanceKind::manual);
        controls.admitted=manual_observation.confirmed && !manual_release.pending() && !combo_session.failed_section() && !combo_session.manual_putdown();
        if (controls.admitted) balance_control.begin_manual(now);
        else balance_control.reset_manual();
        configured_manual=arcade_input.configured_controls(now,controls,native_frame);
    }
    const auto correction = manual_pose ? balance_control.poll_manual(now,combo_session.model().state().balance,
        {configured_manual.first,configured_manual.second},input.available && configured_manual.available) :
        balance_control.poll(now,balance_kind(pose),combo_session.model().state().balance,input.left_x,input.left_y,input.available,input.buttons);
    const bool was_active = combo_session.model().state().combo_active;
    const bool had_revert = combo_session.model().state().revert_pending;
    const bool native_revert = observed_native_revert({valid && !owner_changed,
        score.valid && now >= score.sampled_at && now - score.sampled_at <= 150,
        score.active, bail, native.state, native.crouch_mode, native.words[1] & 0xc000u}, label, now);
    bool preserve_hop{};
    {
        std::lock_guard lock(ollie_mutex);
        preserve_hop = manual_hop.preserve(native_frame,pose,valid && !owner_changed && !observation_gap && !bail);
    }
    const auto physical = manual_physics_pose(physical_manual,pose,simulation_step,balance_control.manual_input(),preserve_hop || acquiring_history);
    const bool failed = combo_session.step(pose, dt, correction, native_revert,
        manual_pose || kind == BalanceKind::manual ? &physical : nullptr);

    const bool release_context = native.state == 100 || native.state == 103 || native.state == 200 ||
        native.state == 201 || native_frame.rail_family;
    const auto release = manual_release.step(now, manual_identity_epoch,
        valid && !owner_changed && !observation_gap && release_context, physical_contact, combo_session.manual_putdown() || acquisition_putdown);
    const bool manual_closed = release.pending || combo_session.manual_putdown() || combo_session.failed_section();
    const auto termination = combo_session.model().balance_exit();
    if (termination != BalanceExit::none)
        input_telemetry.client(BalanceTerminationEvidence{now, static_cast<unsigned>(pose),
            static_cast<unsigned>(termination)});
    const bool manual_prepare = manual_revert_preparation(valid && !owner_changed, bail,
        native.state, native.crouch_mode, combo_session.model().state().revert_pending);
    {
        // Publish one pose/eligibility snapshot to XInput. A pending direction
        // must not cross a context change even when no input poll sees the gap.
        std::lock_guard lock(ollie_mutex);
        // Safe putdown clears the complete old input episode once. Repeated
        // contact polls keep neutral ownership without erasing fresh escape input.
        if (!valid || owner_changed || observation_gap || bail || release.started) {
            arcade_input.cancel_commands();
            for (auto& rotation : rotations) rotation.reset();
            for (auto& charge : native_charges) charge.cancel();
            neutral_delivery = {}; playback_packets = {};
            set_arcade_push_intent(false); set_arcade_air_intent(false,0,0,0,0,false);
        }
        if (!valid || owner_changed || bail ||
            manual_closed ||
            (kind == BalanceKind::manual && balance_kind(pose) != BalanceKind::manual))
            arcade_input.cancel_manuals();
        if (manual_closed) manual_hop.reset();
        // An admitted endpoint already owns its safe-putdown or bail path.
        // Unadmitted cleanup must never substitute a putdown for a failed bail.
        if (combo_session.failed_section() || combo_session.manual_putdown()) manual_acquisition.reset();
        const bool balance_admitted = manual_observation.confirmed &&
            combo_session.model().state().balance_kind == BalanceKind::manual && !manual_closed;
        manual_balance_admitted.store(balance_admitted, std::memory_order_relaxed);
        arcade_input.publish_balance_controls(balance_admitted ? InputBalanceControls::vertical(
            pose == SkatePose::nose_manual ? InputBalanceKind::nose : InputBalanceKind::manual) :
            InputBalanceControls{}, native_frame);
        arcade_input.publish_manual_correction(physical.applied_correction,native_frame,
            balance_admitted && physical.valid && physical.active);
        if(balance_admitted){
            if(capture_correlation_add(capture_admission_sequence))capture_event(CaptureCorrelationSource::manual_admission,capture_admission_sequence,now,native_frame);
            else input_capture_correlation.lost();
        }
        if(termination!=BalanceExit::none){
            if(capture_correlation_add(capture_termination_sequence))capture_event(CaptureCorrelationSource::manual_termination,capture_termination_sequence,now,native_frame);
            else input_capture_correlation.lost();
        }

        const auto control_pose = balance_kind(pose) == BalanceKind::manual ? pose : candidate_pose;
        manual_observed.store(manual_input_pose(control_pose,
            valid && native.state == 100 && native.crouch_mode == 1 &&
                ((native.words[0] & 0x40u) != 0 || native_push_contact),
            combo_session.model().state().balance_kind != BalanceKind::none,
            manual_contact_pose({valid && !bail, false, false, bail, native.state,
                native.crouch_mode, native.words[1] & 0xc000u}, false)), std::memory_order_relaxed);
        manual_eligible_until.store(manual_safe && !manual_closed ? now + 150 : 0, std::memory_order_release);
        manual_prepare_until.store(manual_prepare && !manual_closed ? now + 150 : 0, std::memory_order_release);
        manual_release_until.store(release.pending ? now + 150 : 0, std::memory_order_release);
    }
    if (native_revert) {
        const auto& linked = combo_session.model().state();
        input_telemetry.client(RevertLinkEvidence{now, label.sequence, label.labels,
            !had_revert && linked.revert_pending, static_cast<unsigned>(std::lround(linked.link_grace_s * 1000.0f))});
    }
    if (failed) {
        const auto serial = request_balance_bail(owner);
        {
            std::lock_guard lock(ollie_mutex);
            arcade_input.cancel_commands();
            for (auto& rotation : rotations) rotation.reset();
        }
        input_telemetry.client(BalanceFailureEvidence{now, static_cast<unsigned>(kind), serial});
    }
    const auto& balance = combo_session.model().state();
    // Rate-bound observations continue through a hot session; a lifetime cap
    // would silently remove the evidence needed by later fatigue trials.
    if (balance.balance_kind != BalanceKind::none && now >= next_balance_sample) {
        next_balance_sample = now + 250;
        input_telemetry.client(BalanceSampleEvidence{now, static_cast<unsigned>(balance.balance_kind),
            balance.balance, balance.balance_velocity, correction, combo_session.model().difficulty(), input.left_x, input.left_y});
        if (manual_pose && physical.valid)
            input_telemetry.client(ManualPhysicsEvidence{now, simulation_step, physical.elapsed,
                physical.repeat_pressure, physical.position, physical.velocity, 10});
    }
    if (pose != previous_pose || failed || was_active != balance.combo_active) {
        input_telemetry.client(ComboEventEvidence{now, static_cast<unsigned>(pose), balance.combo_active, failed,
            combo_session.model().state().fatigue, combo_session.model().state().grind_fatigue,
            combo_session.model().state().manual_fatigue});
        previous_pose = pose;
    }
    const auto& display = combo_session.balance_display_state();
    publish_balance_hud(display, combo_session.failed_section() && display.balance_kind == BalanceKind::manual ?
        2 : combo_session.model().danger(), BalanceHudSource{native_frame.sampled_at, native_frame.valid});
}
bool write_capture_correlation(const CaptureCorrelationEvidence& r) noexcept;
// Only this background sink formats the live input/client snapshots.
void write_client_telemetry(const ClientTelemetry& row) noexcept {
    if(const auto* capture=std::get_if<CaptureCorrelationEvidence>(&row)){write_capture_correlation(*capture);return;}
    std::visit([](const auto& r) {
        using T = std::decay_t<decltype(r)>;
        if constexpr (std::is_same_v<T, Observation>)
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE MODE STATE: available={} native={} selector_changes={} wipeouts={} (diagnostic sample).",
                r.valid, r.state, r.changes, r.wipeouts);
        else if constexpr (std::is_same_v<T, RevertLinkEvidence>)
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE REVERT LINK: ms={} sequence={} labels={} accepted={} remaining_ms={}.",
                r.ms, r.sequence, r.labels, r.accepted, r.remaining_ms);
        else if constexpr (std::is_same_v<T, BalanceFailureEvidence>)
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE BALANCE FAILURE: ms={} kind={} request={}.", r.ms, r.kind, r.request);
        else if constexpr (std::is_same_v<T, BoardMotionEvidence>)
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE BOARD MOTION: seq={} ms={} dt={} valid={} state={} mode={} requests={} contacts={} vx={} vy={} vz={} wipeout={}.",
                r.sequence, r.ms, r.seconds, r.valid, r.state, r.mode, r.requests, r.contacts, r.vx, r.vy, r.vz, r.wipeout);
        else if constexpr (std::is_same_v<T, BalanceSampleEvidence>)
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE BALANCE SAMPLE: ms={} kind={} x={} v={} input={} difficulty={} raw_x={} raw_y={}.",
                r.ms, r.kind, r.x, r.v, r.input, r.difficulty, r.raw_x, r.raw_y);
        else if constexpr (std::is_same_v<T, ComboEventEvidence>)
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE COMBO EVENT: ms={} pose={} active={} failed={} fatigue={} grind={} manual={} native_failure_verified=false.",
                r.ms, r.pose, r.active, r.failed, r.fatigue, r.grind, r.manual);
        else if constexpr (std::is_same_v<T, BalanceBailEvidence>)
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE BALANCE BAIL: ms={} request={} result={}.", r.ms, r.request, r.result);
        else if constexpr (std::is_same_v<T, BalanceTerminationEvidence>)
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE BALANCE TERMINATION: ms={} pose={} outcome={} native_exit_verified=false.",
                r.ms, r.pose, r.outcome);
        else if constexpr (std::is_same_v<T, ManualPhysicsEvidence>)
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE MANUAL PHYSICS: ms={} dt={} elapsed={} pressure={} x={} v={} profile={}.",
                r.ms, r.step, r.elapsed, r.repeat_pressure, r.position, r.velocity, r.profile);
        else if constexpr (std::is_same_v<T, ScoreEdgeEvidence>)
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE SCORE EDGE: seq={} observed={} sampled={} labels={} uncertainty={}.",
                r.sequence, r.observed, r.sampled, r.labels, r.uncertainty);
        else if constexpr (std::is_same_v<T, HeartbeatEvidence>) {
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE HIL STATE: seq={} valid={} state={} mode={}.", r.sequence, r.valid, r.state, r.mode);
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE PUSH OBSERVATION: seq={} ms={} valid={} eligible={} speed={} contacts={}.",
                r.sequence, r.ms, r.speed_valid, r.push_valid, r.speed, r.contacts);
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE SCORE NUMBERS: ms={} sampled={} score_valid={} score={} multiplier_valid={} multiplier={} owner_verified=false.",
                r.ms, r.sampled, r.score_valid, r.score, r.multiplier_valid, r.multiplier);
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE AUTHORED STATUS: state={}.", r.authored);
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE AUTHORED DIAGNOSTIC: stage={} field={} error={} rejected={}.", r.stage, r.field, r.error, r.rejected);
        } else if constexpr (std::is_same_v<T, NativeCaptureEvidence>) {
            if (r.begin) logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE NATIVE CAPTURE: id={} begin=true.", r.id);
            else logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE NATIVE CAPTURE: id={} end=true samples={}.", r.id, r.samples);
        } else if constexpr (std::is_same_v<T, NativeSampleEvidence>) {
            const auto& s = r.sample;
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE NATIVE SAMPLE: id={} ms={} valid={} state={} crouch_mode={} words={:08x},{:08x},{:08x},{:08x},{:08x},{:08x},{:08x},{:08x}.",
                r.id, r.ms, s.valid, s.state, s.crouch_mode, s.words[0], s.words[1], s.words[2], s.words[3],
                s.words[4], s.words[5], s.words[6], s.words[7]);
        } else if constexpr (std::is_same_v<T, InputEdgeEvidence>) {
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE MODE DEV: {} press #{} seen on ReSkate client tick.", r.button, r.count);
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE MODE INPUT: {} #{} native_state_available={} native_state={} selector_changes={} wipeouts={}.",
                r.button, r.count, r.sample.valid, r.sample.state, r.sample.changes, r.sample.wipeouts);
        } else if constexpr (std::is_same_v<T, ScorePresenceEvidence>) {
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE SCORE PRESENCE: ms={} source={} rows={} stage={} parts={} labels={}.",
                r.ms, r.source, r.rows, r.stage, r.parts, r.labels);
        } else if constexpr (std::is_same_v<T, ManualHistoryEvidence>) {
            const auto& d = r.diagnostic;
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE MANUAL HISTORY: seq={} ms={} status={} available={} comparable={} rows={} manual={} nose={} distinct={} manual_distinct={} nose_distinct={} manual_added={} manual_removed={} nose_added={} nose_removed={} duplicates={} relabels={}.",
                r.sequence, r.ms, static_cast<unsigned>(d.status), d.available, d.comparable,
                d.rows, d.manual_rows, d.nose_rows, d.distinct, d.manual_distinct, d.nose_distinct,
                d.manual_added, d.manual_removed, d.nose_added, d.nose_removed, d.duplicates, d.relabels);
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE ACTION HISTORY: seq={} ms={} status={} available={} comparable={} added={} removed={} relabels={}.",
                r.sequence, r.ms, static_cast<unsigned>(d.status), d.available, d.comparable,
                d.labels_added, d.labels_removed, d.relabels);
        } else if constexpr (std::is_same_v<T, FootMotionEvidence>) {
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE FOOT MOTION: seq={} ms={} position_ms={} epoch={} valid={} airborne={} ragdoll={} wipeout={} y={} vy={}.",
                r.sequence, r.ms, r.position_ms, r.owner_epoch, r.valid, r.airborne, r.ragdoll, r.wipeout, r.y, r.vy);
        } else if constexpr (std::is_same_v<T, FixtureTrajectoryEvidence>) {
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE FIXTURE TRAJECTORY: session={} seq={} ms={} sampled={} generation={} revision={} status={} dropped={} x_mm={} y_mm={} z_mm={} speed_mm_s={} heading_mdeg={} state={}.",
                r.session, r.sequence, r.ms, r.sampled, r.generation, r.revision, static_cast<unsigned>(r.status),
                r.dropped, r.x_mm, r.y_mm, r.z_mm, r.speed_mm_s, r.heading_mdeg, r.state);
        } else if constexpr (std::is_same_v<T, ScoreNodeEvidence>) {
            const auto path = score_probe_path_text(r.path);
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE SCORE NODE: path={} kind={} size={}.", path.view(), r.kind, r.size);
        } else if constexpr (std::is_same_v<T, ScoreValueEvidence>) {
            const auto path = score_probe_path_text(r.path);
            if (r.label) logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE SCORE LABEL: ms={} path={} id={}.", r.ms, path.view(), r.value);
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE SCORE VALUE: ms={} path={} value={}.", r.ms, path.view(), r.value);
            if (r.limit) logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE SCORE LIMIT: reached=true.");
        }
    }, row);
}
bool write_capture_correlation(const CaptureCorrelationEvidence& r) noexcept {
    if(!capture_correlation_valid(r))return false;
    try {
        if(r.schema==1){
            const auto legacy=std::format("ARCADE INPUT CAPTURE: schema={} capture={} kind={} source={} required_sources={} ordinal={} source_sequence={} ms={} native_ms={} owner={} owner_generation={} contact_generation={} action_generation={} physics_step={} first_event={} last_event={} first_ms={} last_ms={} recognized_ms={} expected={} drained={} dropped={} drop_begin={} drop_end={} first_ordinal={} last_ordinal={} native_valid={} events_known={} complete={} overflow={}.",
            r.schema,r.capture,static_cast<unsigned>(r.kind),static_cast<unsigned>(r.source),r.required_sources,r.ordinal,r.source_sequence,r.ms,r.native_ms,r.owner,r.owner_generation,r.contact_generation,r.action_generation,r.physics_step,r.first_event,r.last_event,r.first_ms,r.last_ms,r.recognized_ms,r.expected,r.drained,r.dropped,r.drop_begin,r.drop_end,r.first_ordinal,r.last_ordinal,r.native_valid,r.events_known,r.complete,r.overflow);
            return logging::write_command_evidence(legacy);
        }
        const auto text=std::format("ARCADE INPUT CAPTURE: schema={} capture={} kind={} source={} required_sources={} ordinal={} source_sequence={} ms={} native_ms={} owner={} owner_generation={} contact_generation={} action_generation={} physics_step={} first_event={} last_event={} first_ms={} last_ms={} recognized_ms={} recognition_id={} recognition_session={} action={} context={} footprint_kind={} member_index={} member_count={} member_event={} held_reuse={} expected={} drained={} dropped={} drop_begin={} drop_end={} first_ordinal={} last_ordinal={} native_valid={} events_known={} complete={} overflow={} queue_admitted={}.",
            r.schema,r.capture,static_cast<unsigned>(r.kind),static_cast<unsigned>(r.source),r.required_sources,r.ordinal,r.source_sequence,r.ms,r.native_ms,r.owner,r.owner_generation,r.contact_generation,r.action_generation,r.physics_step,r.first_event,r.last_event,r.first_ms,r.last_ms,r.recognized_ms,r.recognition_id,r.recognition_session,r.action,r.context,r.footprint_kind,r.member_index,r.member_count,r.member_event,r.held_reuse,r.expected,r.drained,r.dropped,r.drop_begin,r.drop_end,r.first_ordinal,r.last_ordinal,r.native_valid,r.events_known,r.complete,r.overflow,r.queue_admitted);
        return logging::write_command_evidence(text);
    }catch(...){return false;}
}
void write_input_gap(bool pad, std::uint64_t dropped) noexcept {
    if (!dropped) return;
    // Identical loss deltas must not be suppressed as repeated messages. The
    // command channel already provides this guarantee; bodies stay parseable.
    if (pad) logging::log(logging::Level::info, logging::Channel::command,
        "ARCADE PAD GAP: dropped={}.", dropped);
    else logging::log(logging::Level::info, logging::Channel::command,
        "ARCADE COMBO GAP: dropped={}.", dropped);
}
DWORD WINAPI input_telemetry_worker(void* context) noexcept {
    CaptureCorrelationDrain capture_drain;
    auto& telemetry = *static_cast<InputTelemetry*>(context);
    for (;;) {
        std::array<PadTelemetry, 16> pad_rows{};
        const auto pad_rows_count = telemetry.pad.take(pad_rows);
        const auto pad_lost = telemetry.pad.take_dropped();
        write_input_gap(true, pad_lost);
        for (std::size_t i = 0; i < pad_rows_count; ++i) {
            const auto& row = pad_rows[i];
            if (row.diagnostic) {
                const auto& d = row.request;
                if (d.ollie) logging::log(logging::Level::info, logging::Channel::skater,
                    "ARCADE OLLIE: A started native right-stick gesture (experimental).");
                if (d.flip) logging::log(logging::Level::info, logging::Channel::skater,
                    "ARCADE FLIP: X started candidate={} (experimental).", d.started);
                if (d.requested_id || d.unsupported) logging::log(logging::Level::info, logging::Channel::skater,
                    "ARCADE AUTHORED REQUEST: id={} unsupported={}.", d.requested_id, d.unsupported);
                continue;
            }
            const auto& p = row.pad;
            if (p.source) {
                logging::log(logging::Level::info, logging::Channel::skater,
                    "ARCADE PAD SOURCE: seq={} ms={} user={} buttons={} lt={} rt={}.",
                    p.sequence, p.ms, p.user, p.buttons, p.lt, p.rt);
                continue;
            }
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE PAD SAMPLE: seq={} ms={} user={} buttons={} lt={} rt={} output={} phase={}.",
                p.sequence, p.ms, p.user, p.buttons, p.lt, p.rt, p.output, p.phase);
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE STICK SAMPLE: seq={} ms={} tick={} user={} x={} y={} phase={}.",
                p.sequence, p.ms, p.tick, p.user, p.right_x, p.right_y, p.phase);
            if (p.poll) logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE PLAYBACK SAMPLE: seq={} poll={} tick={} mode={} input_packet={} output_packet={}.",
                p.sequence, p.poll, p.tick, p.playback_mode, p.input_packet, p.output_packet);
            if (p.action_sample) logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE ACTION SAMPLE: seq={} ms={} tick={} user={} mode={} requested={} kind={} trick={} result={} owned={} lx={} ly={} lt={} rt={} packet={}.",
                p.sequence, p.ms, p.tick, p.user, p.action_mode, p.action_requested, p.action_kind,
                p.action_trick, p.action_result, p.action_owned, p.left_x, p.left_y, p.output_lt, p.output_rt, p.output_packet);
            if (p.manual_sample) {
                const auto& m = p.manual;
                logging::log(logging::Level::info, logging::Channel::skater,
                    "ARCADE MANUAL INPUT: seq={} ms={} user={} lx={} ly={} buttons={} pose={} safe={} prepare={} rejected={} stick={} pad={} stick_first={} pad_first={} stick_at={} pad_at={} request={} request_at={} owned={} confirmed={} output_y={}.",
                    p.sequence, p.ms, p.user, p.manual_x, p.manual_y, p.buttons, p.manual_pose,
                    m.safe, m.preparation, m.rejected, m.stick, m.pad, m.stick_first, m.pad_first,
                    m.stick_at, m.pad_at, m.request, m.request_at, m.owns, m.confirmed, p.right_y);
            }
        }
        std::array<ComboSample, 16> samples{};
        const auto sample_count = telemetry.combo.queue.take(samples);
        const auto dropped = telemetry.combo.queue.take_dropped();
        write_input_gap(false, dropped);
        for (std::size_t i = 0; i < sample_count; ++i) {
            const auto& s = samples[i];
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE COMBO SAMPLE: ms={} state={} mode={} words={:08x},{:08x},{:08x},{:08x},{:08x},{:08x},{:08x},{:08x}.",
                s.ms, s.sample.state, s.sample.crouch_mode,
                s.sample.words[0], s.sample.words[1], s.sample.words[2], s.sample.words[3],
                s.sample.words[4], s.sample.words[5], s.sample.words[6], s.sample.words[7]);
            logging::log(logging::Level::info, logging::Channel::skater,
                "ARCADE FLIP OBSERVATION: ms={} valid={} id={}.", s.ms, s.flip.valid, s.flip.id);
        }
        std::array<ClientTelemetry, 16> client_rows{};
        const auto client_count = telemetry.client.take(client_rows);
        write_input_gap(false, telemetry.client.take_dropped());
        for (std::size_t i = 0; i < client_count; ++i) {
            if(const auto* capture=std::get_if<CaptureCorrelationEvidence>(&client_rows[i])){
                const auto row=capture->kind==CaptureCorrelationKind::end ? capture_drain.footer(*capture,telemetry.client.dropped()):*capture;
                const bool accepted=write_capture_correlation(row);
                if(row.kind!=CaptureCorrelationKind::end)capture_drain.written(row,accepted);
                if(!accepted)telemetry.client.lose();
                continue;
            }
            // A queued old heartbeat must not masquerade as newly fresh riding.
            if (!client_telemetry_fresh(client_rows[i], GetTickCount64())) {
                telemetry.client.lose();
                write_input_gap(false, telemetry.client.take_dropped());
            } else write_client_telemetry(client_rows[i]);
        }
        Sleep(25);
    }
}
bool start_input_telemetry() noexcept {
    try {
        auto state = std::make_unique<InputTelemetry>();
        const auto worker = CreateThread(nullptr, 0, input_telemetry_worker, state.get(), 0, nullptr);
        if (!worker) return false;
        // The runtime DLL and logger are process-lifetime objects. Match the
        // existing observer workers: no static destructor, join, or unload race.
        auto* live = state.release();
        CloseHandle(worker);
        input_telemetry.publish(*live);
        return true;
    } catch (...) { return false; }
}

// Read-only physics-step evidence retains short transitions between client ticks.
void observe_combo_step(const NoBailSkater& owner, float seconds, bool wipeout) noexcept {
    // One original physics-observation clock joins optional native fields to
    // the contact/action generations produced below. Client polls cannot rebind it.
    const auto now = GetTickCount64();
    const auto lip_observation = observe_grind_step(owner,now);
    using namespace game::build::v20260929::arcade_native_probe;
    NativeRequestSample sample{};
    if (!memory::peek(owner.context + request_words, sample.words) ||
        !memory::peek(owner.context + selected_state, sample.state) ||
        !memory::peek(owner.rig + crouch_mode, sample.crouch_mode)) {
        std::lock_guard lock(ollie_mutex);
        native_input_state.invalidate(); native_readiness.reset(); arcade_input.publish_native_context({}); arcade_input.invalidate_authority();
        input_capture_correlation.maintain(GetTickCount64(),{},input_telemetry.client_dropped(),capture_row);
        set_arcade_push_intent(false); set_arcade_air_intent(false,0,0,0,0,false);
        input_clock_valid.store(false,std::memory_order_release);
        return;
    }
    const auto flip = read_flip_selection([](auto address, auto& value) { return memory::peek(address, value); }, owner.core);
    const bool offboard = sample.state == game::build::v20260929::no_bail::offboard_physics_state;
    // Same physics-step reader used by Hall of Meat; Trainer's position carries
    // separate provenance, just as it does for the existing fixture observer.
    const auto foot = offboard && hall_of_meat::skater_available()
        ? hall_of_meat::read_step(owner, seconds, wipeout) : hall_of_meat::Step{};
    const auto motion = offboard ? trainer::telemetry() : trainer::Telemetry{};
    NoBailSkater after{};
    if (!no_bail_skater(after) || owner.base != after.base || owner.core != after.core || owner.entity != after.entity ||
        owner.context != after.context || owner.rig != after.rig) {
        std::lock_guard lock(ollie_mutex);
        native_input_state.invalidate(); native_readiness.reset(); arcade_input.publish_native_context({}); arcade_input.invalidate_authority();
        input_capture_correlation.maintain(GetTickCount64(),{},input_telemetry.client_dropped(),capture_row);
        set_arcade_push_intent(false); set_arcade_air_intent(false,0,0,0,0,false);
        input_clock_valid.store(false,std::memory_order_release);
        return;
    }
    sample.valid = true;
    // The existing validated local physics listener is the update authority.
    // Client callbacks and repeated XInput polls never advance authored time.
    const bool clock_valid = std::isfinite(seconds) && seconds >= .001f && seconds <= .1f;
    {
        std::lock_guard lock(ollie_mutex);
        if (clock_valid) {
            const auto frame = native_input_state.observe(now,input_owner(owner),sample,flip,wipeout);
            arcade_input.publish_native_context(frame);
            if(frame.valid)capture_event(CaptureCorrelationSource::native,frame.physics_step,frame.sampled_at,frame);
            else input_capture_correlation.maintain(now,{},input_telemetry.client_dropped(),capture_row);
            arcade_input.publish_native_lip(lip_observation,frame);
            if (frame.valid) input_owner_epoch.store(frame.owner_generation,std::memory_order_release);
            else arcade_input.invalidate_authority();
        } else {
            native_input_state.invalidate(); native_readiness.reset(); arcade_input.publish_native_context({}); arcade_input.invalidate_authority();
        input_capture_correlation.maintain(GetTickCount64(),{},input_telemetry.client_dropped(),capture_row);
            set_arcade_push_intent(false); set_arcade_air_intent(false,0,0,0,0,false);
        }
    }
    input_clock_valid.store(clock_valid,std::memory_order_release);
    if (clock_valid) {
        input_game_tick.store(authored_clock.advance(simulation_time::rate_of(seconds)),std::memory_order_release);
        input_step_serial.fetch_add(1,std::memory_order_release);
        input_step_at.store(now,std::memory_order_release);
    }

    const auto capture = motion_capture_until.load(std::memory_order_acquire);
    if (capture >= now && capture - now <= 150 && seconds > 0 && seconds <= .1f) {
        static std::uint64_t sequence{};
        std::array<float, 3> velocity{};
        const bool valid = !offboard && native_board_velocity(velocity);
        NoBailSkater final_owner{};
        if (no_bail_skater(final_owner) && owner == final_owner)
            input_telemetry.client(BoardMotionEvidence{++sequence, now, seconds, velocity[0], velocity[1], velocity[2],
                sample.state, sample.crouch_mode, sample.words[0], sample.words[1] & 0xc000u, valid, wipeout});
    }
    // This listener runs on the local skater physics thread. Retain every flag
    // transition and otherwise sample at20Hz; the existing bounded queue reports
    // drops. Include invalidation/owner changes instead of bridging evidence gaps.
    static NoBailSkater previous_owner{};
    static FootMotionEvidence previous_foot{};
    static std::uint64_t foot_sequence{}, foot_epoch{};
    if (offboard || previous_foot.valid) {
        if (owner.entity != previous_owner.entity || owner.core != previous_owner.core ||
            owner.context != previous_owner.context || owner.rig != previous_owner.rig) ++foot_epoch;
        previous_owner = owner;
        FootMotionEvidence row{};
        row.ms = now; row.owner_epoch = foot_epoch;
        row.airborne = foot.airborne; row.ragdoll = foot.ragdoll.value_or(false); row.wipeout = wipeout;
        row.valid = offboard && foot.ragdoll.has_value() && foot.velocity.has_value() &&
            motion.skater && motion.sampled_entity == owner.entity && motion.physics_state == sample.state &&
            motion.sampled_ms && now >= motion.sampled_ms && now - motion.sampled_ms <= 150;
        if (row.valid) { row.position_ms = motion.sampled_ms; row.y = motion.position[1]; row.vy = (*foot.velocity)[1]; }
        if (row.valid != previous_foot.valid || row.airborne != previous_foot.airborne ||
            row.ragdoll != previous_foot.ragdoll || row.wipeout != previous_foot.wipeout ||
            row.owner_epoch != previous_foot.owner_epoch || now < previous_foot.ms || now - previous_foot.ms >= 50) {
            row.sequence = ++foot_sequence;
            input_telemetry.client(row);
            previous_foot = row;
        }
    }
    air_flip_ready_until.store(mode_enabled.load(std::memory_order_acquire) && flip.valid && flip.id == -1 &&
        sample.state >= 200 && sample.state < 300 ? now + 150 : 0, std::memory_order_release);
    input_telemetry.combo({sample, now, flip});
}

bool validate_request_probe(std::uintptr_t base) noexcept {
    using namespace game::build::v20260929::arcade_native_probe;
    for (const auto& contract : {crouch_predicate, crouch_producer}) {
        decltype(contract.bytes) actual{};
        if (!memory::read(base + contract.rva, actual) || actual != contract.bytes) return false;
    }
    return true;
}

NativeRequestSample native_request_sample(NoBailSkater& validated_owner) noexcept {
    using namespace game::build::v20260929::arcade_native_probe;
    NoBailSkater owner{}, after{};
    NativeRequestSample result;
    if (!request_probe_ready || !no_bail_skater(owner) ||
        !memory::peek(owner.context + request_words, result.words) ||
        !memory::peek(owner.context + selected_state, result.state) ||
        !memory::peek(owner.rig + crouch_mode, result.crouch_mode) ||
        !no_bail_skater(after) || owner.base != after.base || owner.core != after.core || owner.context != after.context ||
        owner.rig != after.rig || owner.entity != after.entity) return {};
    result.valid = true;
    validated_owner = after;
    return result; // Independent guarded reads, NOT an atomic physics snapshot.
}

// Reuse the board-root layout and owner/vtable checks used by Grind Exit and
// Hall of Meat. This observer never writes physics or scales native velocity.
bool native_board_velocity(std::array<float, 3>& velocity) noexcept {
    namespace layout = game::build::v20260929::skater_state;
    NoBailSkater skater{}, after{};
    std::uintptr_t holder{}, physics{}, type{}, bodies{}, owner{};
    std::uint32_t count{};
    if (!no_bail_skater(skater) ||
        !memory::peek(skater.core + layout::board_holder_offset, holder) ||
        !memory::peek(holder + layout::board_physics_offset, physics) ||
        !memory::peek(physics, type) || type != skater.base + layout::board_physics_vtable ||
        !memory::peek(physics + layout::physics_bodies_offset, bodies) ||
        !memory::peek(bodies, count) || count != layout::board_body_count) return false;
    const auto body = bodies + layout::board_root_body * layout::physics_body_size;
    if (!memory::peek(body + layout::body_owner_offset, owner) || owner != physics ||
        !memory::peek(body + layout::body_velocity_offset, velocity) ||
        !no_bail_skater(after) || skater.core != after.core || skater.entity != after.entity ||
        skater.context != after.context || skater.rig != after.rig) return false;
    for (const auto value : velocity) if (!std::isfinite(value) || std::abs(value) >= 1000) return false;
    return true;
}
bool native_board_speed(float& speed) noexcept {
    std::array<float, 3> velocity{};
    if (!native_board_velocity(velocity)) return false;
    speed = std::hypot(velocity[0], velocity[2]);
    return true;
}

bool native_board_toggle_safe(const NativeRequestSample& sample) noexcept {
    namespace layout = game::build::v20260929::skater_state;
    if (!sample.valid) return false;
    if (sample.state == 100 && sample.crouch_mode == 1 && (sample.words[1] & 0xc000u) == 0xc000u) return true;
    if (sample.state != game::build::v20260929::no_bail::offboard_physics_state) return false;
    // Reuse Hall of Meat's native offboard flags to distinguish standing from a bail.
    NoBailSkater owner{}, after{};
    std::uintptr_t trick{}, offboard{};
    std::uint8_t ragdoll{}, airborne{};
    std::uint32_t current{};
    const bool safe = no_bail_skater(owner) &&
        memory::peek(owner.core + layout::trick_state_offset, trick) &&
        memory::peek(trick + layout::offboard_state_offset, offboard) &&
        memory::peek(offboard + layout::ragdoll_offset, ragdoll) && ragdoll == 0 &&
        memory::peek(offboard + layout::in_the_air_offset, airborne) && airborne <= 1 &&
        memory::peek(owner.context + layout::physics_state_offset, current) && current == sample.state &&
        no_bail_skater(after) && owner.core == after.core && owner.context == after.context &&
        owner.rig == after.rig && owner.entity == after.entity;
    return safe && airborne == 0;
}
}

bool enqueue_client_telemetry(const ClientTelemetry& row) noexcept { return input_telemetry.client(row); }
bool set_input_capture(unsigned duration_ms,unsigned required_sources) noexcept {
    std::lock_guard lock(ollie_mutex);const auto now=GetTickCount64();const auto frame=arcade_input.native_context(now);
    if(!duration_ms){
        if(!input_capture_correlation.active())return false;
        input_capture_correlation.end(now,frame,input_telemetry.client_dropped(),capture_row);
        arcade_input.set_recognition_capture(0,false);return true;
    }
    if(!enabled()||!input_telemetry.available())return false;
    const bool begun=input_capture_correlation.begin(now,duration_ms,required_sources,frame,input_telemetry.client_dropped(),capture_row);
    if(begun){
        arcade_input.set_recognition_capture(0,(required_sources&2u)!=0);
        if(const auto* diagnostic=arcade_input.input_diagnostic(0)){
            capture_attempt_cursor=diagnostic->recognized;capture_omission_cursor=diagnostic->recognition_dropped;capture_sample_cursor=diagnostic->samples;
        }
        capture_input_cursor=0;
        if(const auto* history=arcade_input.history(0);history&&!history->events().empty())capture_input_cursor=history->events().back().sequence;
    }
    return begun;
}

NativeInputFrame native_air_context(std::uint64_t now, const NativeInputOwner& owner,
                                   const NativeRequestSample& sample) noexcept {
    std::unique_lock lock(ollie_mutex,std::try_to_lock);
    if (!lock.owns_lock() || !mode_enabled.load(std::memory_order_acquire) || !arcade_input.admitted(0,now)) return {};
    const auto frame = native_input_state.correlate(now,owner,sample);
    return frame.valid && frame.owner_generation == arcade_input.owner_epoch() ? frame : NativeInputFrame{};
}
NativeInputFrame surface_geometry_native_frame(std::uint64_t now) noexcept {
    std::unique_lock lock(ollie_mutex,std::try_to_lock);
    return lock.owns_lock() && mode_enabled.load(std::memory_order_acquire) && arcade_input.admitted(0,now) ?
        arcade_input.native_context(now) : NativeInputFrame{};
}
void invalidate_traffic_surface() noexcept {traffic_surface_available.store(false,std::memory_order_release);}
void invalidate_surface_geometry() noexcept {surface_geometry_available.store(false,std::memory_order_release);}
void publish_surface_geometry(const SurfaceGeometrySample& sample) noexcept {
    invalidate_surface_geometry();
    if (!sample.valid) return;
    std::unique_lock lock(ollie_mutex,std::try_to_lock);
    if (!lock.owns_lock() || !mode_enabled.load(std::memory_order_acquire)) return;
    const auto now=GetTickCount64();const auto native=arcade_input.native_context(now);
    auto current=sample.context;current.owner={native.owner.base,native.owner.entity,native.owner.core,native.owner.context,native.owner.rig};
    current.native=InputNativeContext::from_frame(native);
    if (!sample.fresh_for(current,now)) return;
    surface_geometry_snapshot=sample;
    surface_geometry_available.store(true,std::memory_order_release);
}
SurfaceGeometrySample surface_geometry_observation(std::uint64_t now) noexcept {
    std::unique_lock lock(ollie_mutex,std::try_to_lock);
    if (!lock.owns_lock() || !surface_geometry_available.load(std::memory_order_acquire) || !mode_enabled.load(std::memory_order_acquire)) return {};
    const auto native=arcade_input.native_context(now);auto current=surface_geometry_snapshot.context;
    current.owner={native.owner.base,native.owner.entity,native.owner.core,native.owner.context,native.owner.rig};
    current.native=InputNativeContext::from_frame(native);
    // Cached identity/age only. This getter never calls a native query or grants writes.
    return surface_geometry_snapshot.fresh_for(current,now) && surface_geometry_available.load(std::memory_order_acquire) ?
        surface_geometry_snapshot : SurfaceGeometrySample{};
}
void publish_traffic_surface(const TrafficSurfaceMotion& sample) noexcept {
    invalidate_traffic_surface();if(!sample.valid)return;
    std::unique_lock lock(ollie_mutex,std::try_to_lock);
    const auto now=GetTickCount64();
    if(!lock.owns_lock()||!mode_enabled.load(std::memory_order_acquire)||!profile_runtime::population_surface_current(sample.selected))return;
    const auto frame=arcade_input.native_context(now);const auto native=InputNativeContext::from_frame(frame);
    if(!frame.valid||!native.fresh(now)||!sample.context.native.fresh(now)||now<sample.observed_ms||now-sample.observed_ms>150||
       sample.context.owner!=std::array<std::uintptr_t,5>{frame.owner.base,frame.owner.entity,frame.owner.core,frame.owner.context,frame.owner.rig}||
       sample.context.native.generations!=native.generations||sample.context.native.air_episode()!=native.air_episode()||sample.context.native.airborne!=native.airborne)return;
    traffic_surface_snapshot=sample;traffic_surface_available.store(true,std::memory_order_release);
}
TrafficSurfaceMotion traffic_surface_observation(std::uint64_t now,double seconds) noexcept {
    std::unique_lock lock(ollie_mutex,std::try_to_lock);
    if(!lock.owns_lock()||!mode_enabled.load(std::memory_order_acquire)||!traffic_surface_available.load(std::memory_order_acquire))return {};
    const auto& s=traffic_surface_snapshot;const auto frame=arcade_input.native_context(now);const auto native=InputNativeContext::from_frame(frame);
    if(!s.valid||!frame.valid||!native.fresh(now)||!s.context.native.fresh(now)||now<s.observed_ms||now-s.observed_ms>150||
       !std::isfinite(seconds)||seconds<s.sampled_at||seconds-s.sampled_at>.1||
       s.context.owner!=std::array<std::uintptr_t,5>{frame.owner.base,frame.owner.entity,frame.owner.core,frame.owner.context,frame.owner.rig}||
       s.context.native.generations!=native.generations||s.context.native.air_episode()!=native.air_episode()||s.context.native.airborne!=native.airborne||
       !profile_runtime::population_surface_current(s.selected)||!traffic_surface_available.load(std::memory_order_acquire))return {};
    return s; // Copied observed point/time only; no native call or support/write permission.
}
bool native_manual_contact_current(const NativeManualContact& proof) noexcept {
    std::unique_lock lock(ollie_mutex,std::try_to_lock);
    if (!lock.owns_lock()) return false;
    const auto now = GetTickCount64();
    const auto pose = manual_observed.load(std::memory_order_relaxed);
    const auto frame = arcade_input.balance_admission(now,pose == SkatePose::nose_manual ? InputBalanceKind::nose : InputBalanceKind::manual);
    return mode_enabled.load(std::memory_order_acquire) && arcade_input.admitted(0,now) && frame.valid &&
        manual_balance_admitted.load(std::memory_order_relaxed) && proof.generation_matches(frame.owner_generation,frame.contact_generation,frame.action_generation);
}
bool enabled() noexcept { return mode_enabled.load(std::memory_order_acquire); }
bool auto_kick_enabled() noexcept { return auto_kick.load(std::memory_order_acquire); }
void set_auto_kick(bool value, bool persist) noexcept {
    auto_kick.store(value,std::memory_order_release);
    // Withdraw the old drive budget until a physical poll publishes new controls.
    set_arcade_push_intent(false);
    if (persist) profile_runtime::set_local_preference("ArcadeAutoKick",value);
}
SpinMode spin_mode() noexcept { return selected_spin_mode.load(std::memory_order_acquire); }
void set_spin_mode(SpinMode mode, bool persist) noexcept {
    selected_spin_mode.store(mode,std::memory_order_release);
    if (persist) profile_runtime::set_local_preference("ArcadeTapSpin",mode==SpinMode::tap);
}

void on_camera_tick(std::uintptr_t base, std::uintptr_t client, bool ready_for_camera, bool phase) noexcept {
    const auto restore = [&]() noexcept {
        camera_orbit.reset(); camera_at = 0;
        try {
            if (camera_lease_requested) {
                std::string detail;
                if (update_arcade_camera(base, client, false, phase, nullptr, detail)) camera_lease_requested = false;
            }
        } catch (...) {}
    };
    try {
        const auto ui = multiplayer::sample_game_ui_state(base);
        if (!enabled() || !prepared_catalog.load(std::memory_order_acquire) || !ready_for_camera ||
            !ui.available || ui.in_menu || multiplayer::party_camera_requested()) {
            restore(); return;
        }
        ControllerInput input{};
        DingoSDKOverlayReadControllerInput(&input);
        if (!input.available || (input.buttons & 0x0010u)) { restore(); return; }
        const auto local = multiplayer::capture_local(base, client, false);
        if (!local.ready) { restore(); return; }
        float fov{};
        const auto follow = multiplayer::follow_camera(UINT64_MAX, local.pose.root, fov);
        auto target = local.pose.root.position;
        target[1] += 1.0f;
        const auto now = GetTickCount64();
        const float dt = camera_at && now >= camera_at ? static_cast<float>(std::min<std::uint64_t>(now - camera_at, 50)) / 1000 : 0;
        camera_at = now;
        const auto transform = camera_orbit.update(follow, target,
            static_cast<float>(input.right_x) / 32767, static_cast<float>(input.right_y) / 32767, dt);
        if (!transform) { restore(); return; }
        std::string detail;
        // Activation can succeed before a subsequent verification fails: always retain restoration intent.
        camera_lease_requested = true;
        if (!update_arcade_camera(base, client, true, phase, &*transform, detail, fov)) restore();
    } catch (...) { restore(); }
}

PlaybackExperiment playback_experiment() noexcept {
    return playback_mode.load(std::memory_order_acquire);
}
ActionExperiment action_experiment() noexcept { return action_mode.load(std::memory_order_acquire); }
bool set_action_experiment(ActionExperiment value) noexcept {
    if (value > ActionExperiment::classic) return false;
    std::lock_guard lock(ollie_mutex);
    if (mode_enabled.load(std::memory_order_acquire)) return false;
    arcade_input.cancel_commands();
    action_mode.store(value, std::memory_order_release);
    return true;
}
bool set_playback_experiment(PlaybackExperiment value, bool diagnostics) noexcept {
    if (value != PlaybackExperiment::delivery && value != PlaybackExperiment::timeline &&
        value != PlaybackExperiment::reference) return false;
    std::lock_guard lock(ollie_mutex);
    if (mode_enabled.load(std::memory_order_acquire)) return false;
    arcade_input.set_playback_schedule(value == PlaybackExperiment::delivery ?
        AuthoredPlaybackSchedule::delivery_relative : AuthoredPlaybackSchedule::absolute_ticks);
    playback_mode.store(value, std::memory_order_release);
    playback_diagnostics = diagnostics;
    return true;
}

void set_enabled(bool value, bool persist) noexcept {
    invalidate_surface_geometry();invalidate_traffic_surface();
    reset_push_contact_policy.store(true, std::memory_order_release);
    cancel_balance_bail();
    invalidate_grind_observer();
    if (value) prepare_authored_catalog();
    {
        std::lock_guard lock(ollie_mutex);
        if(!value){
            input_capture_correlation.end(GetTickCount64(),{},input_telemetry.client_dropped(),capture_row,true);
            arcade_input.set_recognition_capture(0,false);
        }
        native_input_state.invalidate(); native_readiness.reset(); arcade_input.publish_native_context({}); manual_hop.reset(); manual_acquisition.reset(); ground_manual_balance.reset();
        arcade_input.enable(value);
        arcade_input.cancel_commands();
        for (auto& rotation : rotations) rotation.reset();
        mode_enabled.store(value, std::memory_order_release);
        ollie_eligible_until.store(0, std::memory_order_release);
        gameplay_valid_until.store(0, std::memory_order_release);
        push_eligible_until.store(0, std::memory_order_release);
        movement_eligible_until.store(0, std::memory_order_release);
        set_arcade_push_intent(false);
        set_arcade_air_intent(false, 0, 0, 0, 0, false);
        for (auto& charge : native_charges) charge.cancel();
        board_eligible_until.store(0, std::memory_order_release);
        offboard_valid_until.store(0, std::memory_order_release);
        manual_eligible_until.store(0, std::memory_order_release);
        manual_prepare_until.store(0, std::memory_order_release);
        manual_release_until.store(0, std::memory_order_release);
        action_eligible_until.store(0, std::memory_order_release);
        air_flip_ready_until.store(0, std::memory_order_release);
        manual_observed.store(SkatePose::unknown, std::memory_order_relaxed);
        manual_balance_admitted.store(false, std::memory_order_relaxed);
        reset_physical_manual.store(true, std::memory_order_release);
        previous_pad = {};
        previous_source_pad = {};
    }
    // A toggle never banks an unfinished combo or inherits a held command.
    combo_session.invalidate();
    manual_identity.reset();
    reset_score_label_occurrences();
    combo_tick_at = 0;
    previous_pose = SkatePose::unknown;
    had_combo_owner = false;
    request_capture = {};
    publish_balance_hud({}, 0, {});
    if (persist) {
        preference_loaded = true;
        profile_runtime::set_local_preference("ArcadeMode", value);
    }
    logging::log(logging::Level::info, logging::Channel::skater,
        "ARCADE MODE: enabled={} synthetic_reset=true.", value);
}

bool start(std::uintptr_t image_base) noexcept {
    if (ready.load(std::memory_order_acquire)) return true;
    if (!image_base) return false;
    score_probe_base = image_base;
    request_probe_ready = validate_request_probe(image_base);
    const bool telemetry_ready = start_input_telemetry();
    logging::log(logging::Level::info, logging::Channel::skater,
        "ARCADE INPUT TELEMETRY: available={} queue_limit=64 background=true.", telemetry_ready);
    const bool combo_observer = request_probe_ready && add_no_bail_step_listener(observe_combo_step);
    logging::log(logging::Level::info, logging::Channel::skater,
        "ARCADE COMBO OBSERVER: available={} read_only=true queue_limit=64.", combo_observer);
    logging::log(logging::Level::info, logging::Channel::skater,
        "ARCADE NATIVE PROBE: available={} read_only=true duration_ms=5000 limit=128.", request_probe_ready);
    logging::log(logging::Level::info, logging::Channel::skater,
        "Arcade Mode ready (controller observer and experimental A-to-native-input ollie).");
    ready.store(true, std::memory_order_release);
    return true;
}

void on_client_tick() noexcept {
    if (!ready.load(std::memory_order_acquire)) return;
    try {
        const float step = hall_of_meat::step_length();
        const bool valid_clock = std::isfinite(step) && step >= .001f && step <= .1f;
        const auto stepped_at = input_step_at.load(std::memory_order_acquire);
        const auto clock_now = GetTickCount64();
        {
            std::lock_guard lock(ollie_mutex);
            input_capture_correlation.maintain(clock_now,arcade_input.native_context(clock_now),input_telemetry.client_dropped(),capture_row);
        }
        const bool observed_clock = valid_clock && input_clock_valid.load(std::memory_order_acquire) &&
            stepped_at && clock_now >= stepped_at && clock_now-stepped_at <= 150;
        if (!preference_loaded && profile_runtime::local_profile_store_ready()) {
            set_auto_kick(profile_runtime::local_preference("ArcadeAutoKick").value_or(true),false);
            set_spin_mode(profile_runtime::local_preference("ArcadeTapSpin").value_or(false) ? SpinMode::tap : SpinMode::held,false);
            set_enabled(profile_runtime::local_preference("ArcadeMode").value_or(false), false);
            preference_loaded = true;
        }
        const auto watch = watched_physics_state();
        if (observations.update({watch.valid, watch.state, watch.changes, watch.wipeouts})) {
            const auto& sample = observations.state();
            input_telemetry.client(sample);
        }
        ControllerInput input{};
        DingoSDKOverlayReadControllerInput(&input);
        const bool focus_available = input.available;
        {
            std::lock_guard lock(ollie_mutex);
            const auto pad = arcade_input.physical(GetTickCount64());
            input.available = input.available && input.device < 0x100 && (input.device & 1u) && pad.valid;
            if (input.available) {
                input.buttons = pad.buttons | (pad.lt > 30 ? 0x10000u : 0u) | (pad.rt > 30 ? 0x20000u : 0u);
                input.left_x=pad.lx; input.left_y=pad.ly; input.right_x=pad.rx; input.right_y=pad.ry;
            }
        }
        const auto buttons = input.available ? input.buttons : 0u;
        const auto pressed = buttons & ~previous_buttons;
        previous_buttons = buttons; // Reporting failures must never replay an edge.
        const auto now = GetTickCount64();
        if (const auto catalog = authored_catalog.load(std::memory_order_acquire)) {
            if (authored_reported < catalog->size()) {
                const auto index = authored_reported++;
                const auto& gesture = (*catalog)[index];
                logging::log(logging::Level::info, logging::Channel::skater,
                    "ARCADE AUTHORED GESTURE: index={} total={} id={} nollie={} context={} arcs={}.",
                    index, catalog->size(), gesture.native_id, gesture.nollie,
                    static_cast<unsigned>(gesture.context), gesture.arcs.size());
                logging::log(logging::Level::info, logging::Channel::skater,
                    "ARCADE AUTHORED LABEL: index={} tokens={}.", index, gesture.source_tokens);
                // Quantized, bounded game data only; no asset names or memory dumps.
                for (std::size_t arc_index = 0; arc_index < gesture.arcs.size(); ++arc_index) {
                    const auto& arc = gesture.arcs[arc_index];
                    logging::log(logging::Level::info, logging::Channel::skater,
                        "ARCADE AUTHORED ARC: index={} arc={} start={} end={} min={} max={} wait={}.",
                        index, arc_index, static_cast<int>(std::lround(arc.start * 1000)),
                        static_cast<int>(std::lround(arc.end * 1000)),
                        static_cast<int>(std::lround(arc.minimum * 1000)),
                        static_cast<int>(std::lround(arc.maximum * 1000)),
                        static_cast<int>(std::lround(arc.wait * 1000)));
                }
            }
        }
        const auto ui = multiplayer::sample_game_ui_state(score_probe_base);
        const auto trainer_view = trainer::view();
        motion_capture_until.store(trainer_view && trainer_view->logging && ui.available && !ui.in_menu ? now + 150 : 0,
            std::memory_order_release);
        bool gameplay = enabled() && observed_clock && ui.available && !ui.in_menu && input.available;
        tick_arcade_fixture_geometry(now, gameplay);
        gameplay_valid_until.store(gameplay ? now + 150 : 0, std::memory_order_release);
        observe_score_hud(score_probe_base, now);
        native_score_tick(score_probe_base, now, gameplay);
        float grind_speed{};
        const bool grind_speed_valid = native_board_speed(grind_speed);
        grind_observer_tick(score_probe_base, now, request_probe_ready && ui.available && !ui.in_menu && input.available,
            enabled(), grind_speed_valid, grind_speed);
        const auto score_edge = take_score_label_occurrence(now);
        const auto manual_label = take_manual_label_evidence(now);
        if (gameplay && score_edge.valid && score_edge.labels) {
            // History base-name occurrences are independently observed native
            // actions. Amounts and FS/BS context remain separate evidence.
            input_telemetry.client(ScoreEdgeEvidence{
                score_edge.sequence, score_edge.observed_at, score_edge.sampled_at, score_edge.labels,
                static_cast<unsigned>(score_edge.uncertainty)});
        }
        NoBailSkater native_owner{};
        const auto native = input.available && request_probe_ready ? native_request_sample(native_owner) : NativeRequestSample{};
        NativeInputFrame client_frame{};
        {
            std::lock_guard lock(ollie_mutex);
            if (!gameplay || !focus_available || !watch.valid || !native.valid) native_input_state.invalidate();
            const auto authority_now = GetTickCount64();
            const auto frame = native_input_state.correlate(authority_now,input_owner(native_owner),native);
            client_frame = frame;
            gameplay = gameplay && frame.valid;
            gameplay_valid_until.store(gameplay ? now + 150 : 0,std::memory_order_release);
            arcade_input.publish_native_context(frame);
            if (arcade_input.publish_authority(authority_now, frame.valid ? frame.owner_generation : 0,
                    input.device, gameplay && focus_available && watch.valid && frame.valid)) {
                for (auto& rotation : rotations) rotation.reset();
                for (auto& charge : native_charges) charge.cancel();
                neutral_delivery = {}; playback_packets = {};
                set_arcade_push_intent(false); set_arcade_air_intent(false,0,0,0,0,false);
                reset_physical_manual.store(true,std::memory_order_release);
            }
        }
        float speed{};
        // Initial push requires ordinary two-wheel contact. Its independently
        // observed native request may continue through its own one-wheel kick.
        // Manual/grind balance and unknown state/speed still prohibit pushing.
        const bool speed_valid = native.valid && native.state == 100 && native.crouch_mode == 1 && native_board_speed(speed);
        if (reset_push_contact_policy.exchange(false, std::memory_order_acq_rel) || native_owner != push_contact_owner) {
            push_contact_policy.reset(); push_contact_owner = native_owner;
        }
        const bool push_valid = push_contact_policy.poll(now, gameplay && watch.valid && speed_valid,
            native.state, native.crouch_mode, native.words[1] & 0xc000u, (native.words[0] & 0x40u) != 0,
            combo_session.model().state().balance_kind != BalanceKind::none);
        if (push_valid) push_speed.store(speed, std::memory_order_relaxed);
        push_eligible_until.store(push_valid ? now + 150 : 0, std::memory_order_release);
        const auto contacts = native.words[1] & 0xc000u;
        const bool drive_valid = gameplay && watch.valid && native.valid &&
            (native.state == 100 || native.state == 103) && native.crouch_mode <= 2 &&
            combo_session.model().state().balance_kind == BalanceKind::none &&
            (contacts == 0xc000u || (((native.words[0] & 0x40u) || push_valid) &&
                (contacts == 0x4000u || contacts == 0x8000u)));
        movement_eligible_until.store(drive_valid ? now + 150 : 0, std::memory_order_release);
        const bool board_safe = gameplay && watch.valid && native_board_toggle_safe(native);
        offboard_valid_until.store(gameplay && watch.valid && native.valid &&
            native.state == game::build::v20260929::no_bail::offboard_physics_state ? now + 150 : 0,
            std::memory_order_release);
        board_eligible_until.store(gameplay && watch.valid && board_safe ? now + 150 : 0,
            std::memory_order_release);
        if (gameplay) update_combo(native, native_owner, input, watch.valid, watch.wipeouts, now, score_edge, manual_label, push_valid, step);
        else {
            cancel_balance_bail();
            manual_identity.reset(manual_label);
            physical_manual.reset(); manual_release.reset();
            balance_control.reset_manual();
            had_combo_owner = false;
            combo_owner = {};
            {
                std::lock_guard lock(ollie_mutex);
                arcade_input.cancel_manuals(); manual_hop.reset(); manual_acquisition.reset(); ground_manual_balance.reset();
                manual_eligible_until.store(0, std::memory_order_release);
                manual_prepare_until.store(0, std::memory_order_release);
                manual_release_until.store(0, std::memory_order_release);
                manual_observed.store(SkatePose::unknown, std::memory_order_relaxed);
                manual_balance_admitted.store(false, std::memory_order_relaxed);
                set_arcade_air_intent(false, 0, 0, 0, 0, false);
                for (auto& charge : native_charges) charge.cancel();
            }
            action_eligible_until.store(0, std::memory_order_release);
            combo_session.invalidate();
            combo_tick_at = 0;
            publish_balance_hud({}, 0, {});
        }
        const auto failure_status = balance_bail_status();
        if (failure_status.serial != previous_balance_bail.serial || failure_status.result != previous_balance_bail.result) {
            input_telemetry.client(BalanceBailEvidence{now, failure_status.serial,
                static_cast<unsigned>(failure_status.result)});
            previous_balance_bail = failure_status;
        }
        // Bounded numeric read-only HIL heartbeat. Enables fresh reset/state
        // checks without injecting an A press merely to arm a capture.
        if (now >= hil_heartbeat_at && now - hil_heartbeat_at >= 250) {
            hil_heartbeat_at = now;
            const auto score_numbers = score_hud_numbers();
            input_telemetry.client(HeartbeatEvidence{++hil_heartbeat_id, now, native.valid, native.state,
                native.crouch_mode, speed_valid, push_valid, speed, native.words[1] & 0xc000u,
                score_numbers.sampled_at, score_numbers.score, score_numbers.multiplier,
                score_numbers.score_valid, score_numbers.multiplier_valid, authored_status.load(std::memory_order_acquire),
                authored_stage.load(std::memory_order_acquire), authored_field.load(std::memory_order_acquire),
                authored_error.load(std::memory_order_acquire), authored_rejected.load(std::memory_order_acquire)});
        }
        // State 100/mode 1 is the verified on-board idle baseline. Expire this
        // permission quickly if client ticks, game focus, or owner reads stop.
        ollie_eligible_until.store(gameplay && native.valid && native.state == 100 && native.crouch_mode == 1
            ? now + 150 : 0, std::memory_order_release);
        {
            std::lock_guard lock(ollie_mutex);
            const auto publication_at = GetTickCount64();
            const auto frame = native_input_state.correlate(publication_at,input_owner(native_owner),native);
            // Revalidate the originating observation after client-side work. A
            // same-address owner replacement must not acquire these permissions.
            const bool correlated = client_frame.valid && frame.valid && client_frame.owner == frame.owner &&
                client_frame.owner_generation == frame.owner_generation &&
                client_frame.contact_generation == frame.contact_generation &&
                client_frame.action_generation == frame.action_generation;
            const auto live = [publication_at](const auto& until) {
                const auto value = until.load(std::memory_order_acquire);
                return value >= publication_at && value-publication_at <= 150;
            };
            if (correlated) native_readiness.publish(client_frame,
                {gameplay && focus_available && watch.valid, live(ollie_eligible_until), live(push_eligible_until),
                 live(board_eligible_until), live(movement_eligible_until), live(manual_eligible_until),
                 live(manual_prepare_until), live(manual_release_until), live(action_eligible_until), speed});
            else if (!gameplay || !focus_available || !watch.valid) native_readiness.reset();
            // A concurrent same-owner selection change defers new permissions;
            // it must not erase an actor's own continuously verified AIR flight.
        }
        if (enabled() && request_probe_ready && (pressed & face_buttons[0])) {
            capture_started = now;
            ++capture_id;
            request_capture.arm(now);
            input_telemetry.client(NativeCaptureEvidence{capture_id, true});
        }
        const bool capturing = request_capture.active();
        if (capturing && request_capture.sample(now, native)) {
            const auto& sample = request_capture.last();
            input_telemetry.client(NativeSampleEvidence{capture_id, now - capture_started, sample});
        }
        if (capturing && !request_capture.active())
            input_telemetry.client(NativeCaptureEvidence{capture_id, false, request_capture.count()});
        for (std::size_t i = 0; i < face_buttons.size(); ++i) {
            if (!(pressed & face_buttons[i])) continue;
            auto& count = press_counts[i];
            if (count < std::numeric_limits<std::uint64_t>::max()) ++count;
            const auto& sample = observations.state();
            input_telemetry.client(InputEdgeEvidence{button_names[i], count, sample});
        }
    } catch (...) {
        input_clock_valid.store(false, std::memory_order_release);
        {
            std::lock_guard lock(ollie_mutex);
            native_input_state.invalidate(); native_readiness.reset(); arcade_input.publish_native_context({}); manual_hop.reset(); manual_acquisition.reset(); ground_manual_balance.reset();
            arcade_input.invalidate_authority();
            for (auto& rotation : rotations) rotation.reset();
            for (auto& charge : native_charges) charge.cancel();
            set_arcade_air_intent(false,0,0,0,0,false);
        }
        ollie_eligible_until.store(0, std::memory_order_release);
        gameplay_valid_until.store(0, std::memory_order_release);
        push_eligible_until.store(0, std::memory_order_release);
        movement_eligible_until.store(0, std::memory_order_release);
        motion_capture_until.store(0, std::memory_order_release);
        set_arcade_push_intent(false);
        board_eligible_until.store(0, std::memory_order_release);
        offboard_valid_until.store(0, std::memory_order_release);
    }
}

void observe_xinput_source(unsigned user, unsigned short buttons,
                           unsigned char left_trigger, unsigned char right_trigger) noexcept {
    if (user >= previous_source_pad.size() || !ready.load(std::memory_order_acquire) ||
        !mode_enabled.load(std::memory_order_acquire)) return;
    std::lock_guard lock(ollie_mutex);
    auto& old = previous_source_pad[user];
    if (old.buttons == buttons && old.lt == left_trigger && old.rt == right_trigger) return;
    const PadEvidence row{++pad_sequence, GetTickCount64(), user, buttons,
        left_trigger, right_trigger, 0, 0, true};
    old = row;
    input_telemetry.pad({row});
}

unsigned remap_xinput(unsigned user, unsigned short& buttons, short& right_x,
                      short& right_y, bool game_input, short& left_x, short& left_y,
                      unsigned char& left_trigger, unsigned char& right_trigger,
                      std::uint32_t physical_packet) noexcept {
    if (user != 0 || !ready.load(std::memory_order_acquire)) return 0;
    const auto physical_buttons = buttons;
    const auto physical_right_x = right_x, physical_right_y = right_y;
    const auto physical_left_trigger = left_trigger, physical_right_trigger = right_trigger;
    const ActionPhysicalPad action_physical{buttons, left_x, left_y, left_trigger, right_trigger};
    auto now = GetTickCount64();
    NativeGesture::Output result{};
    const auto prepared = prepared_catalog.load(std::memory_order_acquire);
    const bool authored = prepared && !prepared->empty();
    const auto tick = input_game_tick.load(std::memory_order_acquire);
    std::uint32_t packet_mask{};
    {
        std::lock_guard lock(ollie_mutex);
        // A client publication while acquiring the lock must not look future-dated.
        now = GetTickCount64();
        arcade_input.observe_physical(user,{true,now,physical_buttons,action_physical.left_x,action_physical.left_y,
            physical_right_x,physical_right_y,physical_left_trigger,physical_right_trigger});
        const auto owner_epoch = input_owner_epoch.load(std::memory_order_acquire);
        if (owner_epoch != arcade_input.owner_epoch()) {
            arcade_input.invalidate_authority();
            for (auto& rotation : rotations) rotation.reset();
            for (auto& charge : native_charges) charge.cancel();
            neutral_delivery = {}; playback_packets = {};
            set_arcade_push_intent(false); set_arcade_air_intent(false,0,0,0,0,false);
        }
        const auto admission = native_readiness.sample(now,arcade_input.native_context(now));
        game_input = game_input && input_clock_valid.load(std::memory_order_acquire) &&
            admission.gameplay && !(physical_buttons & 0x0010u);
        const bool may_start = admission.start;
        const bool may_push = admission.push;
        const bool may_toggle_board = admission.board;
        const bool offboard = admission.offboard;
        const bool action_safe = admission.action;
        const bool flip_ready = admission.flip_ready;
        if (!game_input) arcade_input.invalidate_authority();
        game_input = game_input && arcade_input.admitted(user,now);
        const auto release_deadline = manual_release_until.load(std::memory_order_acquire);
        const bool releasing_manual = game_input && admission.releasing && !offboard && release_deadline >= now && release_deadline - now <= 150;
        if (releasing_manual) arcade_input.cancel_manuals();
        const auto manual_deadline = manual_eligible_until.load(std::memory_order_acquire);
        const bool manual_safe = game_input && admission.manual && manual_deadline >= now && manual_deadline - now <= 150;
        const auto prepare_deadline = manual_prepare_until.load(std::memory_order_acquire);
        const bool manual_prepare = game_input && admission.prepare && prepare_deadline >= now && prepare_deadline - now <= 150;
        const auto action_experiment = action_mode.load(std::memory_order_relaxed);
        const auto observed = manual_observed.load(std::memory_order_relaxed);
        const bool may_exit_grind = game_input && admission.matched && observed == SkatePose::grind;
        input_capture_correlation.maintain(now,arcade_input.native_context(now),input_telemetry.client_dropped(),capture_row);
        arcade_input.set_recognition_capture(user,input_capture_correlation.captures(CaptureCorrelationSource::recognition));
        result = arcade_input.poll(user, now, buttons, left_x, left_y, right_x, right_y,
            game_input, may_start || may_exit_grind, may_push && auto_kick_enabled(), admission.speed,
            left_trigger > 30 || right_trigger > 30, may_toggle_board, authored, tick,
            left_trigger > 30, prepared ? std::span<const PreparedGesture>(*prepared) : std::span<const PreparedGesture>{},
            admission.mirror_x, manual_safe,
            manual_observed.load(std::memory_order_relaxed), action_experiment == ActionExperiment::grabs,
            manual_prepare, offboard, false, flip_ready, neutral_delivery[user].ready(now), left_trigger, right_trigger,
            action_experiment, action_safe, admission.mirror_x,
            manual_balance_admitted.load(std::memory_order_relaxed));
        const auto capture_proof=arcade_input.native_context(now);
        input_capture_correlation.maintain(now,capture_proof,input_telemetry.client_dropped(),capture_row);
        if(input_capture_correlation.active()){
            std::uint64_t preceding_sequence{};
            if(const auto* history=arcade_input.history(user))for(const auto& event:history->events()){
                if(preceding_sequence&&event.sequence<=preceding_sequence)input_capture_correlation.lost();
                preceding_sequence=event.sequence;
                if(event.sequence<=capture_input_cursor)continue;
                if(capture_input_cursor&&event.sequence-capture_input_cursor!=1)input_capture_correlation.lost();
                capture_input_cursor=event.sequence;
                CaptureCorrelationEvidence row;row.source=CaptureCorrelationSource::input;row.source_sequence=event.sequence;row.ms=event.at;
                row.first_event=row.last_event=event.sequence;row.first_ms=row.last_ms=event.at;row.events_known=true;
                input_capture_correlation.event(row,capture_proof,input_telemetry.client_dropped(),capture_row);
            }
            if(const auto* diagnostic=arcade_input.input_diagnostic(user))capture_recognitions(*diagnostic,now,capture_proof);
        }
        if (result.suppress_reverse) left_y = 0;
        // The inspected guide stance has a clear native switch bit, also the
        // stance needing authored flip reflection. Do not invert this for grabs.
        // Revert mirroring outside that baseline remains a separate native test.
        const auto& action = result.action;
        const auto manual_delivery_frame = arcade_input.native_context(now);
        const bool manual_delivery = game_input && !offboard && !result.authored_playback && !action.owned &&
            !(physical_buttons & (classic_a | 0x0030u));
        manual_acquisition.observe(now,manual_delivery_frame,result.manual,manual_delivery);
        ground_manual_balance.observe(now,manual_delivery_frame,result.manual,manual_delivery);
        result.buttons &= ~action.consume_buttons;
        if (action.consume_rt) right_trigger = 0;
        if (action.pad.owns_right_stick) { result.right_x = action.pad.right_x; result.right_y = action.pad.right_y; }
        if (action.pad.owns_left_stick) { left_x = action.pad.left_x; left_y = action.pad.left_y; }
        if (action.pad.owns_triggers) { left_trigger = action.pad.left_trigger; right_trigger = action.pad.right_trigger; }
        if (action.pad.owns_face_buttons) result.buttons = (result.buttons & ~0xf000u) | action.pad.face_buttons;
        // Neutral is the existing native manual release recipe. Keep it owned
        // through continuing manual contact, but let authored jump/trick escape.
        if (releasing_manual && !result.authored_playback && !action.owned) result.right_x = result.right_y = 0;
        const auto experiment = playback_mode.load(std::memory_order_relaxed);
        const bool reference = experiment == PlaybackExperiment::reference && result.authored_playback;
        const bool consume_nollie = authored && mode_enabled.load(std::memory_order_acquire) && game_input && !offboard && left_trigger != 0;
        // The reference arm matches upstream's complete scripted pad during the
        // active flip. The timeline arm isolates scheduling with ordinary masking.
        if (reference) { result.buttons = 0; left_x = left_y = 0; left_trigger = right_trigger = 0; }
        else if (consume_nollie && !action.pad.owns_triggers) left_trigger = 0;
        // The semantic session owns recognition; this existing actor owns only
        // its admitted RS channel. Native Manual release remains independent.
        const auto rail_pad = arcade_input.sample_native_rail(result.rail,now,
            game_input && !offboard && !releasing_manual && !reference && !action.owned);
        if (rail_pad.owns_right_stick) { result.right_x = rail_pad.right_x; result.right_y = rail_pad.right_y; }

        const auto pose = manual_observed.load(std::memory_order_relaxed);
        const auto hop_origin = arcade_input.balance_admission(now,pose == SkatePose::nose_manual ? InputBalanceKind::nose : InputBalanceKind::manual);
        manual_hop.command(now,arcade_input.native_context(now),hop_origin,pose,
            manual_balance_admitted.load(std::memory_order_relaxed),result,(physical_buttons & classic_a) != 0,
            game_input && !offboard && !reference && !action.owned && !(physical_buttons & 0xe330u) && physical_right_trigger <= 30);
        const bool rotation_enabled = mode_enabled.load(std::memory_order_acquire) && game_input && !offboard;
        const bool crouched = pose == SkatePose::riding && (physical_buttons & classic_a) &&
            result.authored_playback && (result.right_y > 27850 || result.right_y < -27850);
        const auto physical_state=arcade_input.digital_state(now);
        const auto ground_controls=GroundControl::from_state(auto_kick_enabled(),physical_state);
        const bool square_push = rotation_enabled && admission.drive && ground_controls.known && ground_controls.square && pose == SkatePose::riding &&
            !manual_hop.active() && physical_left_trigger <= 30 && physical_right_trigger <= 30 &&
            !result.suppress_reverse && (!result.command_busy || crouched) && !action.owned && !reference;
        if (!ground_controls.auto_kick && square_push) result.buttons |= classic_x;
        const auto drive_until = movement_eligible_until.load(std::memory_order_acquire);
        // A balance hop must not gain an ordinary crouch-kick assistance budget.
        const bool drive_safe = mode_enabled.load(std::memory_order_acquire) && game_input && admission.drive && !offboard && !manual_hop.active() &&
            drive_until >= now && drive_until - now <= 150 && ((ground_controls.auto_kick && result.movement_intent) || square_push) && !ground_controls.down &&
            pose == SkatePose::riding && !(physical_buttons & static_cast<std::uint16_t>(~(classic_x | (crouched ? classic_a : 0)))) &&
            physical_left_trigger <= 30 && physical_right_trigger <= 30 && !action.owned && !reference &&
            !result.suppress_reverse && (!result.command_busy || crouched);
        if (user == 0) set_arcade_push_intent(drive_safe, crouched);
        const auto rotation_native = arcade_input.native_context(now);
        const auto rotation_pose = native_rotation_pose(rotation_native,pose,hop_origin.valid);
        const bool native_rate_ready = arcade_air_motion_ready(&rotation_native);
        // Ordinary native AIR alone admits this writer. Unknown orientation,
        // NoSpin and transfer flags stay unknown; no automatic-turn or prewind
        // equivalence is claimed for this selected ordinary-air policy.
        auto rotation = rotations[user].poll_qualified(now,physical_buttons,ground_controls.left,ground_controls.right,
            rotation_enabled,rotation_pose,native_rate_ready,reference || action.owned || result.phase==3,spin_mode(),{},&rotation_native);
        if (user == 0) {
            const bool charge_valid = rotation_enabled && !action.owned && !(physical_buttons & 0xe030u);
            if(result.jump.setup_started)native_charges[user].admit_setup(now,result.jump.pressed_at,result.jump.launch,charge_valid);
            const auto release = native_charges[user].semantic_release(now, result.jump.released,
                result.jump.setup_admitted, result.jump.from_rail, result.jump.pressed_at, result.jump.released_at, charge_valid,result.jump.launch);
            const bool angular_allowed = !reference && !action.owned &&
                arcade_input.allows_air_rotation(user,result.authored_playback) && result.phase != 3;
            const auto contact = arcade_input.balance_admission(now,pose == SkatePose::nose_manual ? InputBalanceKind::nose : InputBalanceKind::manual);
            const bool manual_admitted = contact.valid && manual_balance_admitted.load(std::memory_order_relaxed) &&
                (pose == SkatePose::manual || pose == SkatePose::nose_manual) &&
                !result.authored_playback && !action.owned && !reference;
            const NativeManualContact manual{{contact.owner.base,contact.owner.entity,contact.owner.core,
                contact.owner.context,contact.owner.rig},contact.sampled_at,contact.contacts,manual_admitted,
                contact.owner_generation,contact.contact_generation,contact.action_generation};
            const auto& spin_proof = rotation_native;
            set_arcade_air_intent(rotation_enabled, release.sequence, release.ms, release.charge_ms,
                angular_allowed ? rotation.yaw_rate : 0, angular_allowed, &manual, &spin_proof,&ground_controls,&rotation.spin,&release.launch,
                rotation_enabled && angular_allowed && pose==SkatePose::riding && !result.authored_playback && !releasing_manual ? &result.ground_turn : nullptr);
        }
        // BoardChord has already consumed a valid pair and emitted native Y.
        // Single shoulders must not leak to native catch/grab bindings.
        if (rotation_enabled) result.buttons &= ~BoardChord::pair;
        if (rotation.owns_left) { left_x = rotation.x; left_y = rotation.y; }
        const auto configured_balance=arcade_input.configured_balance(now);
        const auto manual_correction=arcade_input.manual_correction(now,
            {configured_balance.first,configured_balance.second},
            configured_balance.available && rotation_enabled && !manual_hop.active() && !reference && !action.owned && !result.authored_playback &&
            (!result.command_busy || result.manual.owns) &&
            result.phase!=3 && !rail_pad.owns_right_stick && !rail_pad.button_mask && !rotation.owns_left &&
            !(physical_buttons & (classic_a | 0x0030u)) &&
            physical_left_trigger<=30 && physical_right_trigger<=30);
        if(manual_correction.owns_y)left_y=manual_correction.y;
        observe_grind_pad(user, now, (physical_buttons & 0x8000u) != 0,
            result.right_x, result.right_y, mode_enabled.load(std::memory_order_acquire));
        const auto normal_mask = synthetic_packet_mask(result.phase, result.buttons != physical_buttons || consume_nollie ||
            result.right_x != physical_right_x || result.right_y != physical_right_y) ^
            manual_reverse_packet_mask(result.suppress_reverse) ^
            (authored && result.phase == 1 ? static_cast<unsigned>(tick & 0x00ffffffu) : 0u);
        const auto output_packet = playback_packets[user].sample(physical_packet, physical_packet ^ normal_mask,
            reference || action.owned || action.consume_rt || action.consume_buttons || rotation.owns_left || manual_correction.owns_y || rail_pad.owns_right_stick);
        neutral_delivery[user].observe(now, tick, mode_enabled.load(std::memory_order_acquire) && game_input && !offboard,
            result.right_x, result.right_y);
        packet_mask = physical_packet ^ output_packet;
        const auto poll = ++playback_polls[user];
        const auto& old = previous_pad[user];
        const bool manual_sample = old.manual != result.manual || old.manual_x != action_physical.left_x ||
            old.manual_y != action_physical.left_y || old.manual_pose != static_cast<unsigned>(observed);
        const bool rotation_sample = rotation.owns_left || (rotation_enabled && (physical_buttons & BoardChord::pair)) || old.action_kind == 5;
        const bool action_owned = action.owned || rotation.owns_left;
        const bool diagnostic = playback_diagnostics && (result.authored_playback || old.playback_sample);
        // Keep one cadence row per game tick plus every changed output. The
        // cumulative poll count exposes extra reads without logging each one.
        // Action cadence is useful only while owning input. Idle rows from
        // multiple controller slots otherwise evict the actual trial evidence.
        if (mode_enabled.load(std::memory_order_acquire) &&
            ((diagnostic && old.tick != tick) || old.buttons != physical_buttons || old.lt != physical_left_trigger || old.rt != physical_right_trigger ||
             old.output != result.buttons || old.phase != result.phase ||
             old.right_x != result.right_x || old.right_y != result.right_y || action.requested || manual_sample ||
             ((action_experiment != ActionExperiment::off || old.action_sample || rotation_sample) &&
              (((action_owned || old.action_owned) && old.tick != tick) || old.left_x != left_x || old.left_y != left_y || old.output_lt != left_trigger ||
               old.output_rt != right_trigger || old.action_owned != action_owned)))) {
            PadEvidence row{++pad_sequence, now, user, physical_buttons, physical_left_trigger,
                physical_right_trigger, result.buttons, result.phase, false, tick, result.right_x, result.right_y,
                playback_diagnostics && result.authored_playback, static_cast<unsigned>(experiment),
                physical_packet, output_packet, diagnostic ? poll : 0};
            row.action_sample = action_experiment != ActionExperiment::off || rotation_sample || old.action_owned;
            row.action_requested = action.requested; row.action_owned = action_owned;
            row.action_mode = static_cast<unsigned>(action_experiment);
            row.action_kind = rotation_enabled && (physical_buttons & BoardChord::pair) ? 5 : action.request_kind;
            row.action_trick = static_cast<unsigned>(action.trick); row.action_result = static_cast<unsigned>(action.result);
            row.left_x = left_x; row.left_y = left_y; row.output_lt = left_trigger; row.output_rt = right_trigger;
            row.manual = result.manual; row.manual_x = action_physical.left_x; row.manual_y = action_physical.left_y;
            row.manual_pose = static_cast<unsigned>(observed); row.manual_sample = manual_sample;
            previous_pad[user] = row;
            input_telemetry.pad({row});
        }
    }
    buttons = result.buttons;
    right_x = result.right_x;
    right_y = result.right_y;
    const RemapEvidence diagnostic{result.started == NativeTrick::ollie,
        result.started == NativeTrick::kickflip || result.started == NativeTrick::heelflip,
        result.unsupported, static_cast<unsigned>(result.started), result.requested_id};
    if (diagnostic.ollie || diagnostic.flip || diagnostic.requested_id || diagnostic.unsupported)
        input_telemetry.pad({{}, diagnostic, true});
    return packet_mask;
}
}

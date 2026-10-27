#pragma once
#include <cstdint>
#include <array>
#include "arcade_controller_capture_types.h"
#include "arcade_surface_geometry.h"
#include "arcade_traffic_surface_provider.h"
#include "arcade_input_types.h"
#include "arcade_spin_control.h"

namespace dingosdk::arcade_mode {
struct NativeManualContact;
struct NativeInputFrame;
struct NativeInputOwner;
struct NativeRequestSample;
struct MotionCleanupProof;
struct GroundWriterEvidence;
struct AngularWriterEvidence;
AngularWriterEvidence begin_angular_writer_observation(std::uint64_t now,const NativeInputOwner& owner,
    const NativeInputFrame& consumed,const std::array<std::uintptr_t,32>& parts) noexcept;
void finish_angular_writer_observation(const AngularWriterEvidence&) noexcept;
// Optional finite numeric observation only; never grants motion authority.
GroundWriterEvidence begin_ground_writer_observation(std::uint64_t now,const NativeInputOwner& owner,const NativeInputFrame& consumed,
    bool intent_enabled,bool down,unsigned requests) noexcept;
void finish_ground_writer_observation(const GroundWriterEvidence&) noexcept;
// Nonblocking original physics correlation for the existing motion writer.
// Optional typed cleanup proof is sampled under the same lock. It never makes
// the returned input frame valid while mode/authority is disabled.
NativeInputFrame native_air_context(std::uint64_t now, const NativeInputOwner& owner,
                                   const NativeRequestSample& sample, MotionCleanupProof* cleanup = nullptr) noexcept;
// Bounded physics-side check; contention or changed native generation yields.
bool native_manual_contact_current(const NativeManualContact& proof) noexcept;
// Existing client callback supplies immutable query observations only.
NativeInputFrame surface_geometry_native_frame(std::uint64_t now) noexcept;
void publish_surface_geometry(const SurfaceGeometrySample&) noexcept;
void invalidate_surface_geometry() noexcept;
SurfaceGeometrySample surface_geometry_observation(std::uint64_t now) noexcept;
void publish_traffic_surface(const TrafficSurfaceMotion&) noexcept;
void invalidate_traffic_surface() noexcept;
TrafficSurfaceMotion traffic_surface_observation(std::uint64_t now,double seconds) noexcept;
// Development observer and guarded native-input gesture on this branch.
bool start(std::uintptr_t image_base) noexcept;
bool enabled() noexcept;
// Saved ordinary-ground preference; the default retains automatic kicking.
bool auto_kick_enabled() noexcept;
void set_auto_kick(bool value, bool persist = true) noexcept;
SpinMode spin_mode() noexcept;
void set_spin_mode(SpinMode mode, bool persist = true) noexcept;
// Game update thread, via the normal menu/console command dispatcher.
void set_enabled(bool value, bool persist = true) noexcept;
// Game-thread dispatcher only. Reset transient state while retaining the exact
// current runtime mode; refuses until initial preferences have been applied.
// No deferred restoration and no preference writes.
bool reset_runtime_transient() noexcept;
// Explicit finite software source policy, not an engine selected-user claim.
// Requires mode ON; admission waits for fresh neutral input and native owner.
// OFF drops the policy; clear blocks all sources until OFF or a new request.
bool set_controller_source(unsigned slot,unsigned duration_ms) noexcept;
void clear_controller_source() noexcept;
// Outer game-facing XInput ERROR_DEVICE_NOT_CONNECTED only.
void disconnect_xinput_source(unsigned slot) noexcept;
// Unsaved development A/B selection; changing it requires Arcade Mode OFF.
enum class PlaybackExperiment : unsigned { delivery, timeline, reference };
bool set_playback_experiment(PlaybackExperiment value, bool diagnostics = true) noexcept;
PlaybackExperiment playback_experiment() noexcept;
// Unsaved, explicit lab-only action selection; requires Arcade Mode OFF.
bool set_action_experiment(ActionExperiment value) noexcept;
ActionExperiment action_experiment() noexcept;
// Explicit finite diagnostic capture on the existing client channel; zero stops.
bool set_input_capture(unsigned duration_ms,unsigned required_sources=96,bool angular_writer=false) noexcept;
// Independent, unsaved finite camera observations (1..3000ms; zero stops).
// No input/physics/camera authority; continues observing OFF restoration.
bool set_camera_capture(unsigned duration_ms) noexcept;
void camera_capture_tick_begin() noexcept;
void camera_capture_tick_end() noexcept;
// Client thread only; observes the existing overlay controller input path.
void on_client_tick() noexcept;
// Uses the existing validated native camera lease on the client thread.
void on_camera_tick(std::uintptr_t base, std::uintptr_t client, bool ready, bool phase) noexcept;
// Numeric read-only evidence before any ReSkate controller filter runs.
void observe_xinput_source(unsigned user, unsigned short buttons,
                           unsigned char left_trigger, unsigned char right_trigger,
                           short left_x, short left_y, short right_x, short right_y) noexcept;
// Finite observational capture only; outer hooks join stack-local raw/returned
// values from the same successful call, including OFF and neutral polls.
bool begin_controller_poll(unsigned slot,ControllerPollCapture&) noexcept;
void complete_controller_poll(ControllerPollCapture&) noexcept;
// Called only for the game's XInput reads, after normal menu input capture.
// Returns a packet-number XOR mask so synthetic changes and restoration are seen.
unsigned remap_xinput(unsigned user, unsigned short& buttons, short& right_x,
                      short& right_y, bool game_input, short& left_x, short& left_y,
                      unsigned char& left_trigger, unsigned char& right_trigger,
                      std::uint32_t physical_packet, ControllerPollCapture* capture = nullptr) noexcept;
}

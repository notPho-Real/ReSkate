#pragma once
#include <cstdint>
#include "arcade_input_types.h"

namespace dingosdk::arcade_mode {
struct NativeManualContact;
struct NativeInputFrame;
struct NativeInputOwner;
struct NativeRequestSample;
// Nonblocking original physics correlation for the existing motion writer.
NativeInputFrame native_air_context(std::uint64_t now, const NativeInputOwner& owner,
                                   const NativeRequestSample& sample) noexcept;
// Bounded physics-side check; contention or changed native generation yields.
bool native_manual_contact_current(const NativeManualContact& proof) noexcept;
// Development observer and guarded native-input gesture on this branch.
bool start(std::uintptr_t image_base) noexcept;
bool enabled() noexcept;
// Game update thread, via the normal menu/console command dispatcher.
void set_enabled(bool value, bool persist = true) noexcept;
// Unsaved development A/B selection; changing it requires Arcade Mode OFF.
enum class PlaybackExperiment : unsigned { delivery, timeline, reference };
bool set_playback_experiment(PlaybackExperiment value, bool diagnostics = true) noexcept;
PlaybackExperiment playback_experiment() noexcept;
// Unsaved, explicit lab-only action selection; requires Arcade Mode OFF.
bool set_action_experiment(ActionExperiment value) noexcept;
ActionExperiment action_experiment() noexcept;
// Client thread only; observes the existing overlay controller input path.
void on_client_tick() noexcept;
// Uses the existing validated native camera lease on the client thread.
void on_camera_tick(std::uintptr_t base, std::uintptr_t client, bool ready, bool phase) noexcept;
// Numeric read-only evidence before any ReSkate controller filter runs.
void observe_xinput_source(unsigned user, unsigned short buttons,
                           unsigned char left_trigger, unsigned char right_trigger) noexcept;
// Called only for the game's XInput reads, after normal menu input capture.
// Returns a packet-number XOR mask so synthetic changes and restoration are seen.
unsigned remap_xinput(unsigned user, unsigned short& buttons, short& right_x,
                      short& right_y, bool game_input, short& left_x, short& left_y,
                      unsigned char& left_trigger, unsigned char& right_trigger,
                      std::uint32_t physical_packet) noexcept;
}

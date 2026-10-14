#pragma once
#include <cstdint>

namespace dingosdk::arcade_mode {
// Development observer and guarded native-input gesture on this branch.
bool start(std::uintptr_t image_base) noexcept;
bool enabled() noexcept;
// Game update thread, via the normal menu/console command dispatcher.
void set_enabled(bool value, bool persist = true) noexcept;
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
                      short& right_y, bool game_input, short left_x, short left_y,
                      unsigned char& left_trigger, unsigned char& right_trigger) noexcept;
}

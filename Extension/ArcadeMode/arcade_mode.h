#pragma once
#include <cstdint>

namespace dingosdk::arcade_mode {
// Development observer and guarded native-input gesture on this branch.
bool start(std::uintptr_t image_base) noexcept;
// Client thread only; observes the existing overlay controller input path.
void on_client_tick() noexcept;
// Called only for the game's XInput reads, after normal menu input capture.
unsigned remap_xinput(unsigned user, unsigned short& buttons, short right_x,
                      short& right_y, bool game_input) noexcept;
}

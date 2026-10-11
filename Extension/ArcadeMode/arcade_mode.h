#pragma once
#include <cstdint>

namespace dingosdk::arcade_mode {
// Always enabled on this development branch. No hooks or gameplay writes.
bool start(std::uintptr_t image_base) noexcept;
// Client thread only; observes the existing overlay controller input path.
void on_client_tick() noexcept;
}

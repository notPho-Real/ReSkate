#pragma once
#include "arcade_native_input_state.h"
#include <array>
#include <cstdint>

namespace dingosdk::arcade_mode {
// Plain observations only. No native pointers, device identity or success claim.
struct ControllerCapturePad {
    std::uint16_t buttons{};
    std::int16_t lx{},ly{},rx{},ry{};
    std::uint8_t lt{},rt{};
    std::uint32_t packet{};
    bool operator==(const ControllerCapturePad&) const = default;
};
struct ControllerCaptureDetails {
    std::uint64_t poll_started{},raw_ms{},returned_ms{},remap_ms{},route_generation{},binding_deadline{},binding_armed{};
    std::uint64_t binding_native_ms{},binding_physics_step{},binding_owner{},binding_owner_generation{},binding_contact_generation{},binding_action_generation{};
    bool binding_observed{};unsigned route_reason{};
    std::uint64_t source_ms{},cached_at{},readiness_native_ms{},readiness_physics_step{},readiness_owner{},readiness_owner_generation{},readiness_contact_generation{},readiness_action_generation{};
    std::uint64_t current_native_ms{},current_physics_step{},current_owner_generation{};
    ControllerCapturePad raw{},returned{};
    std::array<std::uint32_t,8> native_words{};
    float physics_dt{},cached_dt{};
    std::uint32_t poll_result{};
    bool raw_valid{},returned_valid{},ownership_known{};
    unsigned bitmap_as_read{};bool bitmap_readable{};
    unsigned slot{4},selected_slot{4},bitmap{},binding_state{},readiness{},ownership{},phase{};
    std::uint32_t native_state{},native_mode{},native_contacts{};
    std::int32_t native_flip{-1};
    bool context_known{},enabled{},focus{},gameplay{},clock_valid{},bitmap_known{},source_available{},capture_blocked{};
    bool native_observed{},native_flip_known{},readiness_observed{},current_observed{},cached_dt_valid{};
};
// Stack-only join carrier. The owner tuple is converted to a capture-local
// token by the existing producer; it is never a telemetry payload.
struct ControllerPollCapture {
    std::uint64_t capture_id{},poll{},manual_request{},manual_recognized_ms{};
    NativeInputFrame proof{},readiness_proof{},binding_proof{};ControllerCaptureDetails details{};
    std::int8_t manual_family{};std::uint8_t final_writer{};
    bool manual_owns{},manual_reserved{},owned_neutral{};
};
}

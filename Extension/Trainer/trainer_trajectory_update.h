#pragma once
#include "trainer_jump.h"
#include <algorithm>
#include <cmath>

namespace dingosdk::trainer::trajectory_detail {
// Share packet preparation and the single native call with the contract replay.
// Callbacks must not consume a query: selection clears its change notification
// on entry, so an eager selection can hide the handoff from the next AIR tick.
template<class Settings, class Ready, class Submit>
bool update_air(const NativeTrajectoryParameters& parameters, Settings settings, Ready ready, Submit submit) noexcept {
    namespace native = game::build::v20260929::trajectory;
    const auto& values = parameters.values;
    if(!std::all_of(values.begin(),values.begin()+native::parameter_launch/sizeof(float),
        [](float value){return std::isfinite(value);}))return false;
    const auto velocity=values.begin()+native::parameter_velocity/sizeof(float);
    if(!std::all_of(velocity,velocity+3,[](float value){return std::abs(value)<1000;}))return false;
    auto packet=parameters;
    // AIR prepares these fields from its own native tuning slots. A generic
    // rig packet is appropriate for a new launch, not an existing flight.
    float first{},second{};
    if(!settings(first,second) || !std::isfinite(first) || !std::isfinite(second))return false;
    packet.values[native::parameter_setting_first/sizeof(float)]=first;
    packet.values[native::parameter_setting_second/sizeof(float)]=second;
    std::copy_n(velocity,3,packet.values.begin()+native::parameter_reference_velocity/sizeof(float));
    reinterpret_cast<unsigned char*>(packet.values.data())[native::parameter_launch]=0;
    if(!ready())return false;
    return submit(native::update_air.rva,packet.values.data());
}
}

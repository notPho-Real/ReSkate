#pragma once
#include "arcade_custom_flatland.h"
#include "Extension/Skater/skater_animation.h"

namespace dingosdk::arcade_mode {
// Client-thread adapter to the existing player. Not installed by the runtime.
// prepare_playback()/can_play() and a validated seed must precede admission.
inline CustomTrickPlayback custom_flatland_native_playback() noexcept {
    return {nullptr,
        [](void*,std::shared_ptr<const anim_studio::Clip> clip) noexcept ->std::uint64_t {
            try {
                skater_animation::Play play{};play.clip=std::move(clip);
                play.blend=.15f;play.floor=false;
                return skater_animation::try_play(std::move(play));
            } catch(...) {return 0;}
        },
        [](void*,std::uint64_t ticket){return skater_animation::owns_play(ticket);},
        [](void*,std::uint64_t ticket){skater_animation::cancel_play(ticket);}};
}
}

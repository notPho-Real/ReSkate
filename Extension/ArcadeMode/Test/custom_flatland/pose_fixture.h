#pragma once
#include "Extension/AnimationStudio/anim_core.h"
// Synthetic anatomical proportions, not extracted game data.
inline std::vector<dingosdk::anim_studio::Joint> flatland_pose_fixture(bool goofy=false) {
    using namespace dingosdk::anim_studio;
    std::vector<Joint> p(skeleton_joints);
    p[7].position={0,.92f,0};
    const float s=goofy?-1.f:1.f;
    p[341].position={-.12f,-.04f,.22f*s};p[8].position={.12f,-.04f,-.22f*s};
    p[342].position=p[9].position={0,-.4f,.08f};
    p[343].position=p[10].position={0,-.4f,-.08f};
    p[42].position=p[43].position=p[44].position=p[45].position={0,.10f,0};
    p[46].position={.12f,0,0};p[275].position={-.12f,0,0};
    p[47].position={.10f,0,0};p[276].position={-.10f,0,0};
    p[48].position={.20f,-.15f,0};p[277].position={-.20f,-.15f,0};
    p[49].position={.20f,-.12f,0};p[278].position={-.20f,-.12f,0};
    return p;
}

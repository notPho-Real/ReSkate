// Generated from authored motion JSON by compile_motion.py; edit the source data.
#pragma once
#include <array>
namespace dingosdk::arcade_mode::casper_motion {
struct Key { float time, roll, pitch, lift, hip_drop, hip_back, contact, front_lift, back_lift, front_side, back_side, upper, weight; };
inline constexpr std::array<Key,5> entry{{
    {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,1.0f},
    {0.06f,0.0f,-3.0f,0.015f,0.035f,-0.035f,0.0f,0.025f,0.0f,0.0f,0.0f,0.4f,1.0f},
    {0.13f,65.0f,-20.0f,0.13f,0.025f,-0.055f,0.25f,0.1f,0.065f,0.12f,0.06f,0.75f,1.0f},
    {0.21f,145.0f,-43.0f,0.23f,0.025f,-0.07f,0.75f,0.04f,0.02f,0.04f,0.025f,1.0f,1.0f},
    {0.3f,180.0f,-55.0f,0.26f,0.035f,-0.07f,1.0f,0.0f,0.0f,0.0f,0.0f,1.0f,1.0f},
}};
inline constexpr std::array<Key,9> flip{{
    {0.0f,180.0f,-55.0f,0.26f,0.035f,-0.07f,1.0f,0.0f,0.0f,0.0f,0.0f,1.0f,1.0f},
    {0.07f,180.0f,-58.0f,0.27f,0.06f,-0.075f,1.0f,0.0f,0.0f,0.0f,0.0f,1.0f,1.0f},
    {0.15f,240.0f,-38.0f,0.18f,-0.015f,-0.055f,0.5f,0.13f,0.12f,0.1f,0.06f,0.9f,1.0f},
    {0.23f,330.0f,-18.0f,0.12f,-0.035f,-0.035f,0.0f,0.3f,0.28f,0.12f,0.08f,0.8f,1.0f},
    {0.31f,420.0f,-5.0f,0.1f,-0.025f,-0.025f,0.0f,0.3f,0.28f,0.12f,0.08f,0.7f,1.0f},
    {0.39f,510.0f,0.0f,0.07f,-0.015f,-0.015f,0.0f,0.25f,0.24f,0.1f,0.06f,0.6f,1.0f},
    {0.47f,600.0f,0.0f,0.05f,0.0f,-0.005f,0.0f,0.18f,0.17f,0.06f,0.04f,0.4f,1.0f},
    {0.54f,690.0f,0.0f,0.025f,0.02f,0.0f,0.0f,0.055f,0.055f,0.02f,0.02f,0.2f,1.0f},
    {0.6f,720.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
}};
inline constexpr std::array<Key,1> hold{{
    {1.0f,180.0f,-55.0f,0.26f,0.035f,-0.07f,1.0f,0.0f,0.0f,0.0f,0.0f,1.0f,1.0f},
}};
}

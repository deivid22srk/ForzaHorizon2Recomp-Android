// input.h — unified driving input (touch HUD + Xbox-360-pattern gamepad).
#pragma once
#include <cstdint>

namespace fh2 {

struct DrivingInput {
    int steering = 0;  // -1000..1000
    int throttle = 0;  // 0..1000 (right trigger)
    int brake = 0;     // 0..1000 (left trigger)
    uint32_t buttons = 0;
};

enum Button : uint32_t {
    BTN_HANDBRAKE = 1u << 0,
    BTN_NITRO     = 1u << 1,
    BTN_CAMERA    = 1u << 2,
    BTN_GEAR_UP   = 1u << 3,
    BTN_GEAR_DOWN = 1u << 4,
    BTN_PAUSE     = 1u << 5,
    BTN_RADIO     = 1u << 6,
};

void Input_Push(int steering, int throttle, int brake, uint32_t buttons);
DrivingInput Input_Get();
// Normalized helpers for recompiled code.
float Input_Steer01();     // -1..1
float Input_Throttle01();  // 0..1
float Input_Brake01();     // 0..1

} // namespace fh2

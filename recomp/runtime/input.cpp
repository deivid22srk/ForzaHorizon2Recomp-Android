#include "input.h"
#include <algorithm>
#include <chrono>

namespace fh2 {
namespace {
using Clock = std::chrono::steady_clock;
struct Slot {
    int steer = 0, thr = 0, brk = 0;
    uint32_t btn = 0;
    Clock::time_point t = Clock::now();
    bool ever = false;
};
Slot g_slots[SRC_COUNT];
int clampi(int v, int lo, int hi) { return std::max(lo, std::min(hi, v)); }
// A source stays authoritative this long after its last event.
constexpr long kHoldMs = 3000;
bool fresh(const Slot& s, Clock::time_point now) {
    return s.ever && (now - s.t) < std::chrono::milliseconds(kHoldMs);
}
} // namespace

void Input_PushSource(int source, int steering, int throttle, int brake, uint32_t buttons) {
    if (source < 0 || source >= SRC_COUNT) source = SRC_TOUCH;
    Slot& s = g_slots[source];
    s.steer = clampi(steering, -1000, 1000);
    s.thr = clampi(throttle, 0, 1000);
    s.brk = clampi(brake, 0, 1000);
    s.btn = buttons;
    s.t = Clock::now();
    s.ever = true;
}

void Input_Push(int steering, int throttle, int brake, uint32_t buttons) {
    Input_PushSource(SRC_TOUCH, steering, throttle, brake, buttons);
}

DrivingInput Input_Get() {
    auto now = Clock::now();
    // Most-recent fresh slot drives analog; buttons OR across fresh slots so
    // a handbrake held on touch survives gamepad steering (and vice-versa).
    const Slot* win = nullptr;
    for (int i = 0; i < SRC_COUNT; i++) {
        if (!fresh(g_slots[i], now)) continue;
        if (!win || g_slots[i].t > win->t) win = &g_slots[i];
    }
    DrivingInput o;
    if (!win) return o;
    o.steering = win->steer; o.throttle = win->thr; o.brake = win->brk;
    for (int i = 0; i < SRC_COUNT; i++)
        if (fresh(g_slots[i], now)) o.buttons |= g_slots[i].btn;
    return o;
}

float Input_Steer01() { return Input_Get().steering / 1000.f; }
float Input_Throttle01() { return Input_Get().throttle / 1000.f; }
float Input_Brake01() { return Input_Get().brake / 1000.f; }

} // namespace fh2

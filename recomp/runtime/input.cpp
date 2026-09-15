#include "input.h"
#include <algorithm>
#include <atomic>

namespace fh2 {
namespace {
std::atomic<int> g_steer{0}, g_thr{0}, g_brk{0}, g_btn{0};
int clampi(int v, int lo, int hi) { return std::max(lo, std::min(hi, v)); }
} // namespace

void Input_Push(int steering, int throttle, int brake, uint32_t buttons) {
    g_steer.store(clampi(steering, -1000, 1000));
    g_thr.store(clampi(throttle, 0, 1000));
    g_brk.store(clampi(brake, 0, 1000));
    g_btn.store((int)buttons);
}

DrivingInput Input_Get() {
    DrivingInput i;
    i.steering = g_steer.load(); i.throttle = g_thr.load();
    i.brake = g_brk.load(); i.buttons = (uint32_t)g_btn.load();
    return i;
}

float Input_Steer01() { return g_steer.load() / 1000.f; }
float Input_Throttle01() { return g_thr.load() / 1000.f; }
float Input_Brake01() { return g_brk.load() / 1000.f; }

} // namespace fh2

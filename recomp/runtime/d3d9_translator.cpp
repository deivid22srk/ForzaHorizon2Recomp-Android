#include "d3d9_translator.h"
#include <android/log.h>
#include <atomic>

#define LOG_TAG "FH2D3D9"
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace fh2::d3d9 {
namespace { std::atomic<uint64_t> g_unimpl{0}; }

void OnCreateDevice() {}
void OnReset(uint32_t, uint32_t) {}
void OnLostDevice() {}
void DrawIndexedPrimitive(uint32_t, uint32_t, int) {}
void SetRenderState(uint32_t, uint32_t) {}
void SetTexture(uint32_t, uint32_t) {}
void HintWorldPosition(float, float, float, float) {}

uint64_t UnimplementedCount() { return g_unimpl.load(); }
void ReportUnimplemented(uint32_t addr, const char* what) {
    g_unimpl.fetch_add(1);
    LOGW("unimplemented D3D9 0x%08x %s (total=%llu)", addr, what ? what : "?",
         (unsigned long long)g_unimpl.load());
}

} // namespace fh2::d3d9

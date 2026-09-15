#include "streaming.h"
#include <algorithm>

namespace fh2 {
namespace {
size_t g_pool = 128;
size_t g_resident = 0;
int g_lod = 2; // 0=low 1=med 2=high
} // namespace

void Streaming_Init(size_t pool_mb) { g_pool = pool_mb; g_resident = 0; g_lod = 2; }
void Streaming_Shutdown() { g_resident = 0; }

void Streaming_Update(float /*x*/, float /*y*/, float /*z*/, float speed_mps, float draw_mult) {
    // Heuristic v0: at speed, prefetch further but drop LOD to hold 30fps.
    // Real tile graph arrives with the recompiled world manager.
    float target = draw_mult;
    if (speed_mps > 55.f) target *= 0.8f;   // ~200km/h: trade distance for fps
    if (speed_mps > 80.f) target *= 0.85f;
    g_lod = target >= 1.0f ? 2 : (target >= 0.65f ? 1 : 0);
    size_t want = (size_t)(g_pool * 0.7 * target);
    if (want > g_pool) want = g_pool;
    g_resident = want;
}

void Streaming_OnTrim(int level) {
    if (level >= 15) { g_resident /= 2; if (g_lod > 0) g_lod--; }
}

void Streaming_Stats(size_t* resident_mb, int* lod) {
    if (resident_mb) *resident_mb = g_resident;
    if (lod) *lod = g_lod;
}

} // namespace fh2

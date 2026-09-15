// streaming.h — open-world tile streaming budgeted for mobile RAM.
#pragma once
#include <cstddef>

namespace fh2 {

void Streaming_Init(size_t pool_mb);
void Streaming_Shutdown();
// Called per frame with car position + speed; adjusts LOD + prefetch radius.
void Streaming_Update(float x, float y, float z, float speed_mps, float draw_mult);
// Frees aggressively on Android trim events.
void Streaming_OnTrim(int level);
void Streaming_Stats(size_t* resident_mb, int* lod);

} // namespace fh2

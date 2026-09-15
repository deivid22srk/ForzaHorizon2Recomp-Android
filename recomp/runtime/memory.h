// memory.h — guest memory pools sized for Android (no desktop assumptions).
#pragma once
#include <cstddef>
#include <cstdint>

namespace fh2 {

bool Memory_Init(size_t guest_mb, size_t stream_mb);
void Memory_Shutdown();
uint8_t* Memory_Base();
size_t Memory_Size();
// Called by the recompiled guest allocator hooks (pending R-1).
void Memory_MarkUsed(size_t bytes);
// Trim streaming pool under onTrimMemory pressure. Returns bytes freed.
size_t Memory_OnTrim(int level);
// Stats for status overlay.
void Memory_Stats(size_t* used, size_t* reserved);

} // namespace fh2

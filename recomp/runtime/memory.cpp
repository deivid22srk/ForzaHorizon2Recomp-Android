// memory.cpp — mmap-backed guest window + streaming ring buffer.
// Guest expects zeroed pages: MAP_ANONYMOUS gives them without a 256MB
// memset stall at boot. Falls back to malloc+memset off-Android (host tools).
#include "memory.h"
#include <android/log.h>
#include <cstdlib>
#include <cstring>
#ifdef __ANDROID__
#include <sys/mman.h>
#endif

#define LOG_TAG "FH2Mem"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace fh2 {
namespace {
uint8_t* g_base = nullptr;
bool g_mapped = false;
size_t g_reserved = 0;
// guest-used counter: updated by the recompiled guest allocator hooks
// (Memory_MarkUsed) once R-1 lands; 0 until then (honest, not estimated).
size_t g_used = 0;
uint8_t* g_stream = nullptr;
bool g_stream_mapped = false;
size_t g_stream_size = 0;
} // namespace

bool Memory_Init(size_t guest_mb, size_t stream_mb) {
    Memory_Shutdown();
    g_reserved = guest_mb * 1024u * 1024u;
#ifdef __ANDROID__
    g_base = (uint8_t*)mmap(nullptr, g_reserved ? g_reserved : 4096,
                            PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (g_base == MAP_FAILED) { g_base = nullptr; g_mapped = false; }
    else g_mapped = true;
#else
    g_base = nullptr; g_mapped = false;
#endif
    if (!g_base) {
        g_base = (uint8_t*)malloc(g_reserved ? g_reserved : 1);
        if (g_base) memset(g_base, 0, g_reserved);
    }
    if (!g_base) { LOGW("guest reserve %zuMB failed", guest_mb); return false; }
    g_used = 0;
    g_stream_size = stream_mb * 1024u * 1024u;
#ifdef __ANDROID__
    g_stream = (uint8_t*)mmap(nullptr, g_stream_size ? g_stream_size : 4096,
                              PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (g_stream == MAP_FAILED) { g_stream = nullptr; g_stream_mapped = false; }
    else g_stream_mapped = true;
#else
    g_stream = nullptr; g_stream_mapped = false;
#endif
    if (!g_stream && g_stream_size) {
        g_stream = (uint8_t*)malloc(g_stream_size);
        if (!g_stream) { LOGW("stream pool %zuMB failed, continuing without", stream_mb); g_stream_size = 0; }
    }
    LOGI("guest=%zuMB stream=%zuMB base=%p (%s)", guest_mb, stream_mb,
         (void*)g_base, g_mapped ? "mmap" : "malloc");
    return true;
}

void Memory_Shutdown() {
    if (g_base) {
#ifdef __ANDROID__
        if (g_mapped) munmap(g_base, g_reserved);
        else free(g_base);
#else
        free(g_base);
#endif
        g_base = nullptr;
    }
    if (g_stream) {
#ifdef __ANDROID__
        if (g_stream_mapped) munmap(g_stream, g_stream_size);
        else free(g_stream);
#else
        free(g_stream);
#endif
        g_stream = nullptr;
    }
    g_mapped = g_stream_mapped = false;
    g_reserved = 0; g_used = 0; g_stream_size = 0;
}

uint8_t* Memory_Base() { return g_base; }
size_t Memory_Size() { return g_reserved; }

void Memory_MarkUsed(size_t bytes) { g_used = bytes; }

size_t Memory_OnTrim(int level) {
    // Return streaming pages to the kernel instead of dirty-memset: the tile
    // manager re-faults them on demand (zero-fill, no I/O).
    if (!g_stream || level < 15) return 0;
    size_t drop = g_stream_size / 2;
#ifdef __ANDROID__
    madvise(g_stream, drop, MADV_DONTNEED);
#else
    memset(g_stream, 0, drop);
#endif
    return drop;
}

void Memory_Stats(size_t* used, size_t* reserved) {
    if (used) *used = g_used;
    if (reserved) *reserved = g_reserved;
}

} // namespace fh2

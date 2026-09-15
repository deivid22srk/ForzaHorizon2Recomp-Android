// memory.cpp — mmap-backed guest window + streaming ring buffer.
#include "memory.h"
#include <android/log.h>
#include <cstdlib>
#include <cstring>

#define LOG_TAG "FH2Mem"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace fh2 {
namespace {
uint8_t* g_base = nullptr;
size_t g_reserved = 0;
size_t g_used = 0;
uint8_t* g_stream = nullptr;
size_t g_stream_size = 0;
} // namespace

bool Memory_Init(size_t guest_mb, size_t stream_mb) {
    Memory_Shutdown();
    g_reserved = guest_mb * 1024u * 1024u;
    // Use malloc (portable, no overcommit surprises on mobile).
    g_base = (uint8_t*)malloc(g_reserved ? g_reserved : 1);
    if (!g_base) { LOGW("guest reserve %zuMB failed", guest_mb); return false; }
    memset(g_base, 0, g_reserved);
    g_used = 0;
    g_stream_size = stream_mb * 1024u * 1024u;
    g_stream = (uint8_t*)malloc(g_stream_size ? g_stream_size : 1);
    if (!g_stream) { LOGW("stream pool %zuMB failed, continuing without", stream_mb); g_stream_size = 0; }
    LOGI("guest=%zuMB stream=%zuMB base=%p", guest_mb, stream_mb, (void*)g_base);
    return true;
}

void Memory_Shutdown() {
    if (g_base) { free(g_base); g_base = nullptr; }
    if (g_stream) { free(g_stream); g_stream = nullptr; }
    g_reserved = 0; g_used = 0; g_stream_size = 0;
}

uint8_t* Memory_Base() { return g_base; }
size_t Memory_Size() { return g_reserved; }

size_t Memory_OnTrim(int level) {
    // level: Android ComponentCallbacks2 value. Free half the streaming pool
    // on critical pressure; the streaming manager re-faults tiles on demand.
    if (!g_stream || level < 15) return 0;
    size_t freed = g_stream_size / 2;
    memset(g_stream, 0, freed);
    return freed;
}

void Memory_Stats(size_t* used, size_t* reserved) {
    if (used) *used = g_used;
    if (reserved) *reserved = g_reserved;
}

} // namespace fh2

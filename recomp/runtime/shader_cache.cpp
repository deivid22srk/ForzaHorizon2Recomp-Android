// shader_cache.cpp — single-file hash store under app cache dir.
#include "shader_cache.h"
#include <cstdio>
#include <cstring>
#include <mutex>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>

namespace fh2 {
namespace {
std::string g_dir;
bool g_init = false;
size_t g_max = 256 * 1024 * 1024;
std::mutex g_mu;
std::unordered_map<uint64_t, std::vector<uint8_t>> g_mem;
size_t g_bytes = 0;

// mkdir -p equivalent (cache dir may not exist on first launch).
static void mkdirs(const std::string& path) {
    if (path.empty()) return;
    std::string cur;
    for (size_t i = 0; i < path.size(); i++) {
        cur += path[i];
        if (path[i] == '/' && cur.size() > 1) ::mkdir(cur.c_str(), 0755);
    }
    ::mkdir(path.c_str(), 0755);
}

std::string path_for(uint64_t key) {
    char b[64];
    snprintf(b, sizeof(b), "fh2_pso_%016llx.bin", (unsigned long long)key);
    return g_dir + "/" + b;
}
} // namespace

void ShaderCache_Init(const std::string& cache_dir, size_t max_mb) {
    std::lock_guard<std::mutex> l(g_mu);
    if (g_init && g_dir == cache_dir) return; // single owner (see fh2_jni)
    g_dir = cache_dir; g_max = max_mb * 1024 * 1024;
    g_mem.clear(); g_bytes = 0;
    mkdirs(g_dir);
    g_init = true;
}

void ShaderCache_Shutdown() {
    std::lock_guard<std::mutex> l(g_mu);
    g_mem.clear(); g_bytes = 0; g_init = false;
}

bool ShaderCache_IsInit() {
    std::lock_guard<std::mutex> l(g_mu);
    return g_init;
}

uint64_t ShaderCache_Hash(const void* data, size_t size) {
    // FNV-1a 64 (no external dep, stable across runs).
    const uint8_t* p = (const uint8_t*)data;
    uint64_t h = 1469598103934665603ull;
    for (size_t i = 0; i < size; i++) { h ^= p[i]; h *= 1099511628211ull; }
    return h;
}

bool ShaderCache_Lookup(uint64_t key, std::vector<uint8_t>* out) {
    std::lock_guard<std::mutex> l(g_mu);
    auto it = g_mem.find(key);
    if (it != g_mem.end()) { if (out) *out = it->second; return true; }
    if (g_dir.empty()) return false;
    FILE* f = fopen(path_for(key).c_str(), "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n <= 0 || (size_t)n > g_max) { fclose(f); return false; }
    std::vector<uint8_t> v((size_t)n);
    size_t r = fread(v.data(), 1, v.size(), f);
    fclose(f);
    if (r != v.size()) return false;
    if (g_bytes + v.size() < g_max) { g_bytes += v.size(); g_mem[key] = v; }
    if (out) *out = v;
    return true;
}

void ShaderCache_Store(uint64_t key, const void* data, size_t size) {
    if (!data || !size) return;
    std::lock_guard<std::mutex> l(g_mu);
    if (g_bytes + size >= g_max) return; // cap: eviction policy = keep-first (documented)
    std::vector<uint8_t> v((const uint8_t*)data, (const uint8_t*)data + size);
    g_mem[key] = v; g_bytes += size;
    if (!g_dir.empty()) {
        FILE* f = fopen(path_for(key).c_str(), "wb");
        if (f) { fwrite(data, 1, size, f); fflush(f); fsync(fileno(f)); fclose(f); }
    }
}

void ShaderCache_Stats(size_t* entries, size_t* bytes) {
    std::lock_guard<std::mutex> l(g_mu);
    if (entries) *entries = g_mem.size();
    if (bytes) *bytes = g_bytes;
}

} // namespace fh2

#include "xenos_shader.h"
#include "fh2_config.h"
#include "shader_cache.h"

#include <android/log.h>
#include <cstdio>
#include <dirent.h>
#include <vector>

#define LOG_TAG "FH2Xenos"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace fh2::xenos {
namespace { bool g_ok = false; }

bool Init(const std::string& cache_dir) {
    ShaderCache_Init(cache_dir, Config().mem.shader_cache_max_mb);
    g_ok = true;
    return true;
}
void Shutdown() { g_ok = false; }

bool GetSpirv(const void* microcode, size_t size, std::vector<uint8_t>* out) {
    if (!microcode || !size) return false;
    uint64_t k = ShaderCache_Hash(microcode, size);
    return ShaderCache_Lookup(k, out);
}

void ImportPrecompiled(uint64_t key, const void* spirv, size_t size) {
    ShaderCache_Store(key, spirv, size);
}

size_t ImportDir(const std::string& dir) {
    DIR* d = opendir(dir.c_str());
    if (!d) return 0;
    size_t n = 0;
    while (dirent* e = readdir(d)) {
        std::string name = e->d_name;
        if (name.size() != 20 || name.compare(16, 4, ".spv") != 0) continue;
        uint64_t key = 0;
        if (sscanf(name.c_str(), "%16llx", (unsigned long long*)&key) != 1) continue;
        std::string path = dir + "/" + name;
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) continue;
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (sz > 0 && sz < 8 * 1024 * 1024) {
            std::vector<uint8_t> v((size_t)sz);
            if (fread(v.data(), 1, v.size(), f) == v.size()) {
                // SPIR-V magic check (LE 0x07230203).
                uint32_t magic = v[0] | (v[1] << 8) | (v[2] << 16) | ((uint32_t)v[3] << 24);
                if (magic == 0x07230203) { ImportPrecompiled(key, v.data(), v.size()); n++; }
            }
        }
        fclose(f);
    }
    closedir(d);
    if (n) LOGI("imported %zu SPIR-V shaders from %s", n, dir.c_str());
    return n;
}

const char* Status() { return g_ok ? "xenos: cache ready (SPIR-V via XenosRecomp offline)" : "xenos: uninit"; }

} // namespace fh2::xenos

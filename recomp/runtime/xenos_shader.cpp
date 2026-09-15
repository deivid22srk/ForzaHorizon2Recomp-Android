#include "xenos_shader.h"
#include "shader_cache.h"

namespace fh2::xenos {
namespace { bool g_ok = false; }

bool Init(const std::string& cache_dir) {
    ShaderCache_Init(cache_dir, 256);
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

const char* Status() { return g_ok ? "xenos: cache ready (SPIR-V via XenosRecomp offline)" : "xenos: uninit"; }

} // namespace fh2::xenos

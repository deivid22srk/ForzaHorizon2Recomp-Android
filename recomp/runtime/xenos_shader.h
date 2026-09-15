// xenos_shader.h — XenosRecomp integration point.
// Offline: tools/run_recomp.sh runs XenosRecomp over the game's shader blobs
// (media/shaders/*) -> HLSL -> DXC -> SPIR-V cache shipped beside the APK
// build or downloaded at first run. Online: this module looks up SPIR-V in
// the persistent ShaderCache before compiling.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace fh2::xenos {

bool Init(const std::string& cache_dir);
void Shutdown();
// key = ShaderCache_Hash(xenos_microcode). Returns true if SPIR-V ready.
bool GetSpirv(const void* microcode, size_t size, std::vector<uint8_t>* spirv_out);
// Called by the offline XenosRecomp cache importer.
void ImportPrecompiled(uint64_t key, const void* spirv, size_t size);
const char* Status();

} // namespace fh2::xenos

// shader_cache.h — persistent pipeline cache (FH2 has 100s of variants).
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace fh2 {

void ShaderCache_Init(const std::string& cache_dir, size_t max_mb);
void ShaderCache_Shutdown();
bool ShaderCache_IsInit();
// Returns true on hit (out filled). Miss => caller compiles then Stores.
bool ShaderCache_Lookup(uint64_t key, std::vector<uint8_t>* out);
void ShaderCache_Store(uint64_t key, const void* data, size_t size);
uint64_t ShaderCache_Hash(const void* data, size_t size);
void ShaderCache_Stats(size_t* entries, size_t* bytes);

} // namespace fh2

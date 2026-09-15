// fh2_config.h — central runtime tuning, mobile-first defaults.
#pragma once
#include <cstddef>
#include <cstdint>

namespace fh2 {

enum class GfxApi : int { Auto = 0, Vulkan = 1, GLES = 2 };

// Memory budget for mid/high Android devices (world-streaming title).
// Xbox 360 has 512MB unified; we reserve less up-front and stream aggressively.
struct MemoryBudget {
    size_t guest_ram_reserve_mb = 256;   // XEX addressable window
    size_t streaming_pool_mb = 128;      // terrain/car LOD ring buffer
    size_t shader_cache_max_mb = 256;    // persistent disk cache cap
    int max_worker_threads = 4;          // scaled at runtime by CPU count
};

struct RuntimeConfig {
    GfxApi api = GfxApi::Auto;
    float resolution_scale = 0.85f;  // dynamic resolution base
    float resolution_min = 0.5f;
    float resolution_max = 1.0f;
    int target_fps = 30;
    float draw_distance = 1.0f;      // 0.4..1.5 multiplier
    int shadow_quality = 1;          // 0=off 1=med 2=high
    bool dynamic_resolution = true;
    MemoryBudget mem;
};

inline RuntimeConfig& Config() {
    static RuntimeConfig c;
    return c;
}

} // namespace fh2

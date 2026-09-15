// d3d9_translator.h — D3D9/Xenos call translation map (Android-first).
// The recompiled game calls these instead of raw D3D9. Each entry documents
// the Android backend path (Vulkan primary, GLES fallback).
#pragma once
#include <cstdint>

namespace fh2::d3d9 {

// Device/lifecycle (mapped to renderer.cpp EGL/Vulkan swapchain).
void OnCreateDevice();
void OnReset(uint32_t w, uint32_t h);
void OnLostDevice();

// Draw (translated to native draw; LOD bias applied for mobile).
void DrawIndexedPrimitive(uint32_t prim_type, uint32_t prim_count, int lod_bias);
void SetRenderState(uint32_t state, uint32_t value);
void SetTexture(uint32_t stage, uint32_t gpu_address);

// Terrain/car streaming hints from the guest (feed Streaming_Update).
void HintWorldPosition(float x, float y, float z, float speed_mps);

// Unimplemented-counter for backlog tracking (never silent).
uint64_t UnimplementedCount();
void ReportUnimplemented(uint32_t xex_address, const char* what);

} // namespace fh2::d3d9

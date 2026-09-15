// renderer.h — D3D9/Xenos -> Vulkan-or-GLES translation entry points.
// Android-first: no D3D12/Windows backend. GLES 3.1 bootstrap ships now;
// the Vulkan backend (renderer_vulkan.cpp, SPIR-V via XenosRecomp) is
// backlog G-1 — Backend() reports GLES until it lands (never Vulkan-early).
#pragma once
#include <string>

namespace fh2 {

enum class Backend { None, GLES, Vulkan };

void Renderer_SetApiPreference(int pref); // 0=auto 1=vulkan 2=gles (stored; Vulkan pending G-1)
bool Renderer_Init(void* android_native_window);
void Renderer_Shutdown();
void Renderer_SetSurface(void* window);
void Renderer_OnSurfaceDestroyed();
void Renderer_Resize(int w, int h);
void Renderer_SetResolutionScale(float s);
void Renderer_SetTargetFps(int fps);
void Renderer_OnPause();
void Renderer_OnResume();
// One frame; safe to call before game data exists (renders status clear).
void Renderer_Frame();
Backend Renderer_Backend();
std::string Renderer_Status();

} // namespace fh2

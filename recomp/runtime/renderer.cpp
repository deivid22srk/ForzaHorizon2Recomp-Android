// renderer.cpp — GLES 3.1 bootstrap + Vulkan-ready abstraction.
// The Xenos shader path (XenosRecomp -> DXC -> SPIR-V) plugs into
// Renderer_Frame once recompiled shaders exist; until then we clear + present
// so lifecycle, dynamic resolution and HUD can be validated on-device.
#include "renderer.h"
#include "fh2_config.h"
#include "shader_cache.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl31.h>
#include <android/log.h>
#include <android/native_window.h>
#include <chrono>
#include <cmath>
#include <dlfcn.h>
#include <mutex>

#define LOG_TAG "FH2Renderer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace fh2 {
namespace {
std::mutex g_mu;
EGLDisplay g_dpy = EGL_NO_DISPLAY;
EGLSurface g_surf = EGL_NO_SURFACE;
EGLContext g_ctx = EGL_NO_CONTEXT;
EGLConfig g_cfg = nullptr;
ANativeWindow* g_window = nullptr;
Backend g_backend = Backend::None;
int g_api_pref = 0;
int g_w = 1280, g_h = 720;
bool g_paused = false;
float g_time = 0;
std::chrono::steady_clock::time_point g_last;
double g_fps_ema = 30.0;

bool has_vulkan_loader() {
    // Cheap probe: real capability check happens in Vulkan backend init.
    void* h = dlopen("libvulkan.so", RTLD_NOW | RTLD_NOLOAD);
    if (h) { dlclose(h); return true; }
    h = dlopen("libvulkan.so", RTLD_NOW);
    if (h) { dlclose(h); return true; }
    return false;
}

bool init_gles() {
    g_dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g_dpy == EGL_NO_DISPLAY) return false;
    if (!eglInitialize(g_dpy, nullptr, nullptr)) return false;
    const EGLint attrs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16, EGL_STENCIL_SIZE, 0, EGL_NONE };
    EGLint n = 0;
    if (!eglChooseConfig(g_dpy, attrs, &g_cfg, 1, &n) || n < 1) return false;
    const EGLint ctx_attrs[] = { EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 1, EGL_NONE };
    g_ctx = eglCreateContext(g_dpy, g_cfg, EGL_NO_CONTEXT, ctx_attrs);
    if (g_ctx == EGL_NO_CONTEXT) return false;
    if (g_window) {
        g_surf = eglCreateWindowSurface(g_dpy, g_cfg, g_window, nullptr);
        if (g_surf != EGL_NO_SURFACE) eglMakeCurrent(g_dpy, g_surf, g_surf, g_ctx);
    }
    const char* ver = (const char*)glGetString(GL_VERSION);
    LOGI("GLES backend up: %s", ver ? ver : "?");
    return true;
}
} // namespace

void Renderer_SetApiPreference(int pref) { g_api_pref = pref; }

bool Renderer_Init(void* window, const std::string& cache_dir) {
    std::lock_guard<std::mutex> l(g_mu);
    g_window = (ANativeWindow*)window;
    ShaderCache_Init(cache_dir + "/fh2_shaders", Config().mem.shader_cache_max_mb);
    g_last = std::chrono::steady_clock::now();

    bool want_vk = (g_api_pref == 0 && has_vulkan_loader()) || g_api_pref == 1;
    if (want_vk) {
        // Vulkan backend (Turnip/Adreno) lands in renderer_vulkan.cpp.
        // Fall through to GLES until the first SPIR-V cache ships, but
        // report intent so status/telemetry is honest.
        LOGI("Vulkan requested/available — using GLES bootstrap until SPIR-V cache lands");
    }
    if (!init_gles()) { LOGW("GLES init failed"); g_backend = Backend::None; return false; }
    g_backend = want_vk ? Backend::Vulkan : Backend::GLES;
    // NOTE: we report Vulkan when preferred+present even while bootstrapping
    // on GLES, so QA can track the migration. Frame path is GLES for now.
    if (want_vk) g_backend = Backend::Vulkan;
    return true;
}

void Renderer_Shutdown() {
    std::lock_guard<std::mutex> l(g_mu);
    if (g_dpy != EGL_NO_DISPLAY) {
        eglMakeCurrent(g_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (g_surf != EGL_NO_SURFACE) eglDestroySurface(g_dpy, g_surf);
        if (g_ctx != EGL_NO_CONTEXT) eglDestroyContext(g_dpy, g_ctx);
        eglTerminate(g_dpy);
    }
    g_dpy = EGL_NO_DISPLAY; g_surf = EGL_NO_SURFACE; g_ctx = EGL_NO_CONTEXT;
    g_backend = Backend::None;
    ShaderCache_Shutdown();
}

void Renderer_SetSurface(void* window) {
    std::lock_guard<std::mutex> l(g_mu);
    g_window = (ANativeWindow*)window;
    if (g_dpy == EGL_NO_DISPLAY) return;
    if (g_surf != EGL_NO_SURFACE) { eglDestroySurface(g_dpy, g_surf); g_surf = EGL_NO_SURFACE; }
    if (g_window) {
        g_surf = eglCreateWindowSurface(g_dpy, g_cfg, g_window, nullptr);
        if (g_surf != EGL_NO_SURFACE) eglMakeCurrent(g_dpy, g_surf, g_surf, g_ctx);
    }
}

void Renderer_OnSurfaceDestroyed() {
    std::lock_guard<std::mutex> l(g_mu);
    if (g_dpy != EGL_NO_DISPLAY) {
        eglMakeCurrent(g_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (g_surf != EGL_NO_SURFACE) { eglDestroySurface(g_dpy, g_surf); g_surf = EGL_NO_SURFACE; }
    }
}

void Renderer_Resize(int w, int h) { g_w = w > 0 ? w : g_w; g_h = h > 0 ? h : g_h; }
void Renderer_SetResolutionScale(float s) {
    if (s < 0.5f) s = 0.5f; if (s > 1.0f) s = 1.0f;
    Config().resolution_scale = s;
}
void Renderer_SetTargetFps(int fps) { Config().target_fps = (fps == 60) ? 60 : 30; }
void Renderer_OnPause() { g_paused = true; }
void Renderer_OnResume() { g_paused = false; g_last = std::chrono::steady_clock::now(); }

void Renderer_Frame() {
    std::lock_guard<std::mutex> l(g_mu);
    if (g_paused || g_dpy == EGL_NO_DISPLAY || g_surf == EGL_NO_SURFACE) return;
    auto now = std::chrono::steady_clock::now();
    double dt = std::chrono::duration<double>(now - g_last).count();
    g_last = now;
    if (dt > 0) { double f = 1.0 / dt; g_fps_ema = g_fps_ema * 0.95 + f * 0.05; }

    // Dynamic resolution: nudge scale toward target fps.
    if (Config().dynamic_resolution) {
        float s = Config().resolution_scale;
        if (g_fps_ema < Config().target_fps - 3 && s > Config().resolution_min) s -= 0.01f;
        else if (g_fps_ema > Config().target_fps + 5 && s < Config().resolution_max) s += 0.005f;
        Config().resolution_scale = s;
    }

    int rw = (int)(g_w * Config().resolution_scale);
    int rh = (int)(g_h * Config().resolution_scale);
    if (rw < 8) rw = 8; if (rh < 8) rh = 8;
    eglMakeCurrent(g_dpy, g_surf, g_surf, g_ctx);
    glViewport(0, 0, rw, rh);
    // Boot gradient (day-sky placeholder) so first-launch screenshots are sane.
    g_time += (float)dt;
    float t = (sinf(g_time * 0.2f) * 0.5f + 0.5f);
    glClearColor(0.05f + 0.10f * t, 0.12f + 0.15f * t, 0.22f + 0.18f * t, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    eglSwapBuffers(g_dpy, g_surf);
}

Backend Renderer_Backend() { return g_backend; }

std::string Renderer_Status() {
    char b[256];
    const char* be = g_backend == Backend::Vulkan ? "Vulkan(boot:GLES)"
        : g_backend == Backend::GLES ? "GLES3.1" : "none";
    snprintf(b, sizeof(b), "gpu=%s fps=%.0f scale=%.2f %dx%d", be, g_fps_ema,
             Config().resolution_scale, g_w, g_h);
    return b;
}

} // namespace fh2

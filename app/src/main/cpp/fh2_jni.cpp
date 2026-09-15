// fh2_jni.cpp — JNI shell + render thread. Game code (recompiled PPC) runs
// behind Renderer_* / guest entry; without game data we run the boot loop so
// lifecycle, HUD and perf paths are testable on any device/CI.
#include <jni.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <chrono>

#include "fh2_config.h"
#include "memory.h"
#include "filesystem.h"
#include "renderer.h"
#include "audio.h"
#include "input.h"
#include "streaming.h"
#include "shader_cache.h"
#include "xenos_shader.h"
#include "d3d9_translator.h"

#define LOG_TAG "FH2JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace {
std::mutex g_mu;
std::atomic<bool> g_inited{false};
std::atomic<bool> g_running{false};
std::thread g_thread; // joinable; stopped by nativeShutdown
ANativeWindow* g_window = nullptr;
std::string g_cache_dir;

void render_loop() {
    using namespace std::chrono;
    while (g_running.load()) {
        auto t0 = steady_clock::now();
        fh2::Renderer_Frame();
        int fps = fh2::Config().target_fps;
        auto dt = steady_clock::now() - t0;
        long want_us = 1000000L / (fps > 0 ? fps : 30);
        long took_us = duration_cast<microseconds>(dt).count();
        if (took_us < want_us)
            std::this_thread::sleep_for(microseconds(want_us - took_us));
    }
}

std::string jstr(JNIEnv* env, jstring s) {
    if (!s) return "";
    const char* c = env->GetStringUTFChars(s, nullptr);
    std::string r = c ? c : "";
    if (c) env->ReleaseStringUTFChars(s, c);
    return r;
}
} // namespace

extern "C" {

JNIEXPORT void JNICALL
Java_com_deivid22srk_fh2recomp_NativeBridge_nativeInit(JNIEnv* env, jclass, jstring gamePath, jstring cacheDir, jint api) {
    std::lock_guard<std::mutex> l(g_mu);
    std::string gp = jstr(env, gamePath);
    std::string cd = jstr(env, cacheDir);
    if (g_inited.load()) {
        // Re-entry (e.g. rotation recreates the Activity): refresh config,
        // never leak a second thread or re-reserve guest memory.
        fh2::FS_SetGameRoot(gp);
        fh2::Config().api = (fh2::GfxApi)(int)api;
        fh2::Renderer_SetApiPreference((int)api);
        LOGI("re-init: config refreshed, runtime kept");
        return;
    }
    fh2::FS_SetGameRoot(gp);
    fh2::Config().api = (fh2::GfxApi)(int)api;
    fh2::Renderer_SetApiPreference((int)api);
    fh2::Memory_Init(fh2::Config().mem.guest_ram_reserve_mb, fh2::Config().mem.streaming_pool_mb);
    fh2::Streaming_Init(fh2::Config().mem.streaming_pool_mb);
    g_cache_dir = cd;
    // Single owner of ShaderCache_Init (xenos::Init reuses the same dir and
    // skips when already initialised).
    fh2::ShaderCache_Init(cd + "/fh2_shaders", fh2::Config().mem.shader_cache_max_mb);
    fh2::xenos::Init(cd + "/fh2_shaders");
    fh2::Audio_Init(48000);
    // Renderer gets its window in onSurfaceCreated; init display here so
    // status/telemetry works even before the first surface.
    fh2::Renderer_Init(nullptr);
    g_inited.store(true);
    if (!g_running.load()) {
        g_running.store(true);
        g_thread = std::thread(render_loop);
    }
    std::string why;
    bool ok = fh2::FS_ValidateGameFolder(&why);
    LOGI("init game='%s' valid=%d (%s)", gp.c_str(), ok, why.c_str());
}

JNIEXPORT void JNICALL Java_com_deivid22srk_fh2recomp_NativeBridge_nativeShutdown(JNIEnv*, jclass) {
    // Stop the render thread first (no detach leaks), then tear down.
    bool was_running = g_running.exchange(false);
    {
        // Drop the lock while joining: render_loop takes g_mu via Renderer_Frame.
        std::thread t;
        { std::lock_guard<std::mutex> l(g_mu); t = std::move(g_thread); }
        if (was_running && t.joinable()) t.join();
    }
    std::lock_guard<std::mutex> l(g_mu);
    if (g_window) { ANativeWindow_release(g_window); g_window = nullptr; }
    fh2::Renderer_Shutdown();
    fh2::Audio_Shutdown();
    fh2::xenos::Shutdown();
    fh2::ShaderCache_Shutdown();
    fh2::Streaming_Shutdown();
    fh2::Memory_Shutdown();
    g_inited.store(false);
    LOGI("shutdown complete");
}

JNIEXPORT void JNICALL Java_com_deivid22srk_fh2recomp_NativeBridge_nativeSetResolutionScale(JNIEnv*, jclass, jfloat s) {
    fh2::Renderer_SetResolutionScale(s);
}
JNIEXPORT void JNICALL Java_com_deivid22srk_fh2recomp_NativeBridge_nativeSetTargetFps(JNIEnv*, jclass, jint f) {
    fh2::Renderer_SetTargetFps(f);
}
JNIEXPORT void JNICALL Java_com_deivid22srk_fh2recomp_NativeBridge_nativeSetDrawDistance(JNIEnv*, jclass, jfloat v) {
    // Stored; consumed by the guest world manager once R-1 lands (see docs).
    fh2::Config().draw_distance = v;
}
JNIEXPORT void JNICALL Java_com_deivid22srk_fh2recomp_NativeBridge_nativeSetShadowQuality(JNIEnv*, jclass, jint q) {
    fh2::Config().shadow_quality = q;
}
JNIEXPORT void JNICALL Java_com_deivid22srk_fh2recomp_NativeBridge_nativeSetAssetValidated(JNIEnv* env, jclass, jboolean ok, jstring label) {
    fh2::FS_SetTreeValidated(ok == JNI_TRUE, jstr(env, label));
}

JNIEXPORT void JNICALL Java_com_deivid22srk_fh2recomp_NativeBridge_nativeOnSurfaceCreated(JNIEnv* env, jclass, jobject surf) {
    std::lock_guard<std::mutex> l(g_mu);
    ANativeWindow* w = surf ? ANativeWindow_fromSurface(env, surf) : nullptr;
    if (g_window) ANativeWindow_release(g_window);
    g_window = w;
    if (fh2::Renderer_Backend() == fh2::Backend::None) {
        fh2::Renderer_Init(w);
    } else {
        fh2::Renderer_SetSurface(w);
    }
}
JNIEXPORT void JNICALL Java_com_deivid22srk_fh2recomp_NativeBridge_nativeOnSurfaceChanged(JNIEnv*, jclass, jint w, jint h) {
    fh2::Renderer_Resize(w, h);
}
JNIEXPORT void JNICALL Java_com_deivid22srk_fh2recomp_NativeBridge_nativeOnSurfaceDestroyed(JNIEnv*, jclass) {
    fh2::Renderer_OnSurfaceDestroyed();
}
JNIEXPORT void JNICALL Java_com_deivid22srk_fh2recomp_NativeBridge_nativeOnPause(JNIEnv*, jclass) {
    fh2::Renderer_OnPause(); fh2::Audio_OnPause();
}
JNIEXPORT void JNICALL Java_com_deivid22srk_fh2recomp_NativeBridge_nativeOnResume(JNIEnv*, jclass) {
    fh2::Renderer_OnResume(); fh2::Audio_OnResume();
}
JNIEXPORT void JNICALL Java_com_deivid22srk_fh2recomp_NativeBridge_nativeOnTrimMemory(JNIEnv*, jclass, jint level) {
    fh2::Memory_OnTrim(level); fh2::Streaming_OnTrim(level);
}
JNIEXPORT void JNICALL Java_com_deivid22srk_fh2recomp_NativeBridge_nativePushDrivingInput(JNIEnv*, jclass, jint s, jint t, jint b, jint btn, jint src) {
    fh2::Input_PushSource(src, s, t, b, (uint32_t)btn);
}
JNIEXPORT jstring JNICALL Java_com_deivid22srk_fh2recomp_NativeBridge_nativeGetStatus(JNIEnv* env, jclass) {
    std::string why;
    bool ok = fh2::FS_ValidateGameFolder(&why);
    std::string game = ok ? (fh2::FS_TreeLabel().empty() ? "OK" : fh2::FS_TreeLabel()) : why;
    size_t entries = 0, bytes = 0;
    if (fh2::ShaderCache_IsInit()) fh2::ShaderCache_Stats(&entries, &bytes);
    size_t smb = 0; int lod = 0;
    fh2::Streaming_Stats(&smb, &lod);
    char buf[1024];
    snprintf(buf, sizeof(buf), "%s | game:%s | %s | unimpl_d3d9=%llu | shaders=%zu/%zukB | stream=%zumb lod=%d",
             fh2::Renderer_Status().c_str(), game.c_str(),
             fh2::xenos::Status(),
             (unsigned long long)fh2::d3d9::UnimplementedCount(),
             entries, bytes / 1024, smb, lod);
    return env->NewStringUTF(buf);
}

} // extern "C"

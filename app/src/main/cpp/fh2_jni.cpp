// fh2_jni.cpp — JNI shell + render thread. Game code (recompiled PPC) runs
// behind Renderer_* / guest entry; without game data we run the boot loop so
// lifecycle, HUD and perf paths are testable on any device/CI.
#include <jni.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <atomic>
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
std::atomic<bool> g_inited{false};
std::atomic<bool> g_running{false};
std::thread g_thread;
ANativeWindow* g_window = nullptr;

void render_loop() {
    using namespace std::chrono;
    while (g_running.load()) {
        auto t0 = steady_clock::now();
        fh2::Renderer_Frame();
        // Pace to target fps when idle (game thread will drive real pacing).
        int fps = fh2::Config().target_fps;
        auto dt = steady_clock::now() - t0;
        long want_us = 1000000L / (fps > 0 ? fps : 30);
        long took_us = duration_cast<microseconds>(dt).count();
        if (took_us < want_us)
            std::this_thread::sleep_for(microseconds(want_us - took_us));
        if (!g_inited.load()) break;
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
    std::string gp = jstr(env, gamePath);
    std::string cd = jstr(env, cacheDir);
    fh2::FS_SetGameRoot(gp);
    fh2::Config().api = (fh2::GfxApi)(int)api;
    fh2::Renderer_SetApiPreference((int)api);
    fh2::Memory_Init(fh2::Config().mem.guest_ram_reserve_mb, fh2::Config().mem.streaming_pool_mb);
    fh2::Streaming_Init(fh2::Config().mem.streaming_pool_mb);
    fh2::xenos::Init(cd + "/fh2_shaders");
    fh2::Audio_Init(48000);
    // Renderer needs a window; init is retried on surfaceCreated with the real one.
    fh2::Renderer_Init(nullptr, cd);
    g_inited.store(true);
    if (!g_running.load()) {
        g_running.store(true);
        g_thread = std::thread(render_loop);
        g_thread.detach();
    }
    std::string why;
    bool ok = fh2::FS_ValidateGameFolder(&why);
    LOGI("init game='%s' valid=%d (%s)", gp.c_str(), ok, why.c_str());
}

JNIEXPORT void JNICALL Java_com_deivid22srk_fh2recomp_NativeBridge_nativeSetResolutionScale(JNIEnv*, jclass, jfloat s) {
    fh2::Renderer_SetResolutionScale(s);
}
JNIEXPORT void JNICALL Java_com_deivid22srk_fh2recomp_NativeBridge_nativeSetTargetFps(JNIEnv*, jclass, jint f) {
    fh2::Renderer_SetTargetFps(f);
}
JNIEXPORT void JNICALL Java_com_deivid22srk_fh2recomp_NativeBridge_nativeSetDrawDistance(JNIEnv*, jclass, jfloat v) {
    fh2::Config().draw_distance = v;
}
JNIEXPORT void JNICALL Java_com_deivid22srk_fh2recomp_NativeBridge_nativeSetShadowQuality(JNIEnv*, jclass, jint q) {
    fh2::Config().shadow_quality = q;
}

JNIEXPORT void JNICALL Java_com_deivid22srk_fh2recomp_NativeBridge_nativeOnSurfaceCreated(JNIEnv* env, jclass, jobject surf) {
    ANativeWindow* w = surf ? ANativeWindow_fromSurface(env, surf) : nullptr;
    if (g_window) ANativeWindow_release(g_window);
    g_window = w;
    // (Re)bind surface; if Renderer never got a window, init now.
    if (fh2::Renderer_Backend() == fh2::Backend::None) {
        fh2::Renderer_Init(w, "/data/local/tmp");
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
JNIEXPORT void JNICALL Java_com_deivid22srk_fh2recomp_NativeBridge_nativePushDrivingInput(JNIEnv*, jclass, jint s, jint t, jint b, jint btn) {
    fh2::Input_Push(s, t, b, (uint32_t)btn);
}
JNIEXPORT jstring JNICALL Java_com_deivid22srk_fh2recomp_NativeBridge_nativeGetStatus(JNIEnv* env, jclass) {
    std::string why;
    bool ok = fh2::FS_ValidateGameFolder(&why);
    size_t entries = 0, bytes = 0;
    fh2::ShaderCache_Stats(&entries, &bytes);
    size_t smb = 0; int lod = 0;
    fh2::Streaming_Stats(&smb, &lod);
    char buf[1024];
    snprintf(buf, sizeof(buf), "%s | game:%s | %s | unimpl_d3d9=%llu | shaders=%zu/%zukB | stream=%zumb lod=%d",
             fh2::Renderer_Status().c_str(), ok ? "OK" : why.c_str(),
             fh2::xenos::Status(),
             (unsigned long long)fh2::d3d9::UnimplementedCount(),
             entries, bytes / 1024, smb, lod);
    return env->NewStringUTF(buf);
}

} // extern "C"

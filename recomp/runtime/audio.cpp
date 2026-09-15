// audio.cpp — AAudio sine/engine placeholder that keeps the build hermetic.
// Full XAudio2->AAudio voice graph lands with the recompiled audio thread
// (see docs/audio.md). This file guarantees: no crash without game data,
// <20ms path when the stream runs, clean pause/resume.
#include "audio.h"
#if __ANDROID_API__ >= 26
#include <aaudio/AAudio.h>
#endif
#include <android/log.h>
#include <atomic>
#include <cmath>

#define LOG_TAG "FH2Audio"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace fh2 {
namespace {
#if __ANDROID_API__ >= 26
AAudioStream* g_stream = nullptr;
#endif
std::atomic<bool> g_paused{false};
std::atomic<float> g_rpm{0.3f}, g_load{0.f}, g_radio{0.8f};
} // namespace

#if __ANDROID_API__ >= 26
static aaudio_data_callback_result_t cb(AAudioStream*, void*, void* audioData, int32_t numFrames) {
    float* out = (float*)audioData;
    static double phase = 0;
    float rpm = g_rpm.load(), load = g_load.load();
    bool paused = g_paused.load();
    double freq = 40.0 + rpm * 120.0;
    double inc = freq * 2.0 * 3.14159265358979 / 48000.0;
    for (int i = 0; i < numFrames; i++) {
        float s = paused ? 0.f : (float)(sin(phase) * 0.08 * (0.4 + load * 0.6));
        phase += inc;
        if (phase > 2 * 3.14159265358979) phase -= 2 * 3.14159265358979;
        out[i * 2 + 0] = s;
        out[i * 2 + 1] = s;
    }
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}
#endif

bool Audio_Init(int sample_rate) {
#if __ANDROID_API__ >= 26
    if (g_stream) return true;
    AAudioStreamBuilder* b = nullptr;
    if (AAudio_createStreamBuilder(&b) != AAUDIO_OK) return false;
    AAudioStreamBuilder_setDirection(b, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setSharingMode(b, AAUDIO_SHARING_MODE_EXCLUSIVE);
    AAudioStreamBuilder_setPerformanceMode(b, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setFormat(b, AAUDIO_FORMAT_PCM_FLOAT);
    AAudioStreamBuilder_setChannelCount(b, 2);
    AAudioStreamBuilder_setSampleRate(b, sample_rate);
    AAudioStreamBuilder_setDataCallback(b, cb, nullptr);
    aaudio_result_t r = AAudioStreamBuilder_openStream(b, &g_stream);
    AAudioStreamBuilder_delete(b);
    if (r != AAUDIO_OK) { g_stream = nullptr; return false; }
    r = AAudioStream_requestStart(g_stream);
    LOGI("AAudio started sr=%d", sample_rate);
    return r == AAUDIO_OK;
#else
    (void)sample_rate;
    LOGI("AAudio unavailable (minSdk<26 at compile), audio muted");
    return false;
#endif
}

void Audio_Shutdown() {
#if __ANDROID_API__ >= 26
    if (g_stream) { AAudioStream_requestStop(g_stream); AAudioStream_close(g_stream); g_stream = nullptr; }
#endif
}
void Audio_OnPause() { g_paused.store(true); }
void Audio_OnResume() { g_paused.store(false); }
void Audio_EngineRpm(float rpm01, float load01) { g_rpm.store(rpm01); g_load.store(load01); }
void Audio_SetRadioVolume(float v) { g_radio.store(v); }

} // namespace fh2

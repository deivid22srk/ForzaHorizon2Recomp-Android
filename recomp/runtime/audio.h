// audio.h — low-latency 3D mixer over AAudio (Oboe-compatible path later).
#pragma once

namespace fh2 {

bool Audio_Init(int sample_rate = 48000);
void Audio_Shutdown();
void Audio_OnPause();
void Audio_OnResume();
// Positional one-shots: engine loop is streamed separately by the game code.
void Audio_EngineRpm(float rpm01, float load01);
void Audio_SetRadioVolume(float v);

} // namespace fh2

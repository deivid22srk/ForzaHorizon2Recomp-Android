# Audio — AAudio (Oboe path reserved)

FH2 depends on positional audio: engine load, collisions, radio.

## v0 (shipped)

- `recomp/runtime/audio.cpp`: AAudio exclusive/low-latency stereo float
  stream at 48 kHz, engine-RPM placeholder tone, mute on pause.
- No external dependency (Oboe prefab intentionally not vendored yet) so CI
  stays hermetic on `ubuntu-latest` + NDK r27.

## Next (backlog A-1..A-3)

1. XMA -> FFmpeg decode to PCM (offline or cached at import).
2. XAudio2 voice graph -> AAudio streams: engine (loop+load), SFX (3D
   panner by camera), radio (compressed music channel).
3. Optional Oboe wrapper for route-change/quirk handling (AAudio stays the
   underlying API; Oboe adds no latency benefit on API 28+ beyond quirks).

Mixer target: <20 ms output latency on Snapdragon 7+/Dimensity 8000 class.

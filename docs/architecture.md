# Architecture — Android-first FH2 recompilation

All decisions assume **ARM64 Android** as the shipping target. There is no
Windows/D3D12 intermediate backend.

```
default.xex (user-owned, never committed)
   | tools/fetch_xex.sh (GH_TOKEN, XEX2 check)
   v
XenonAnalyse -> tools/recompiled_functions.toml (jump tables + manual stubs)
   |
XenonRecomp (tools/fh2recomp.toml + recomp/runtime/ppc_context.h)
   v
recomp/generated/*.cpp  (gitignored, CI-regenerated when GH_PAT exists)
   +
recomp/runtime/*        (committed: GLES/Vulkan, AAudio, input, fs, streaming)
   v
app/src/main/cpp/fh2_jni.cpp -> libfh2recomp.so (arm64-v8a, -O3/-flto, stripped)
   +
app/src/main/java/... (Setup SAF, driving HUD, gamepad, lifecycle)
   v
app-release.apk / app-debug.apk
```

Key constraints (open world + mobile):

- **Streaming first**: 256MB guest window + 128MB tile ring; LOD drops with
  speed; `onTrimMemory` halves the pool instead of crashing.
- **GPU**: Vulkan preferred (Adreno/Turnip), GLES 3.1 bootstrap guaranteed.
  Xenos shaders go XenosRecomp -> HLSL -> DXC -> SPIR-V offline, cached in
  `ShaderCache` on disk (no recompile per launch).
- **Audio**: AAudio low-latency stereo placeholder now; full XAudio2 voice
  graph + 3D mixer is tracked in `docs/audio.md`.
- **Input**: touch HUD is driving-shaped (steer stick + throttle/brake
  pedals + HB/camera/gears), gamepad mirrors Xbox 360 mapping incl. analog
  triggers.
- **Lifecycle**: pause/resume/surface-loss/trim are first-class (desktop
  ports lack these; Android kills you without them).

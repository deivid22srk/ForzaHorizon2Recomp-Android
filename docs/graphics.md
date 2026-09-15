# Graphics — D3D9/Xenos -> Vulkan (primary) / GLES 3.1 (fallback)

No D3D12/Windows path exists in this repo by design.

## Backend selection

`SetupActivity` stores `gfx_api`: Auto (default) = Vulkan if
`libvulkan.so` loads (Adreno stock or Turnip), else GLES 3.1+.
`Renderer_Init` probes and reports `gpu=Vulkan(boot:GLES)` while the SPIR-V
cache is being built, so telemetry stays honest during migration.

## FH2-specific load

- **Terrain streaming**: aggressive LOD (`Streaming_Update` scales with car
  speed; >55 m/s drops prefetch radius to hold frame budget).
- **Cars (PBR, many materials)**: material count is the draw-call driver;
  mobile path batches by shader key (XenosRecomp output) — see backlog G-2.
- **Realtime reflections / day-night**: reflection probes + shadow cascades
  scale with `draw_distance` (0.4..1.5) and `shadow_quality` (0/1/2).
- **Dynamic resolution**: 0.50..1.00 around base 0.85, servoed to 30/60 fps
  target (`Renderer_Frame`). Resolution scale + fps target are user options.
- **Shader cache**: `ShaderCache_*` persists PSO/SPIR-V under app cache,
  capped, FNV-hashed keys — no per-launch recompile of FH2's 100s of variants.

## Lifecycle (Android-only work)

`surfaceCreated/Changed/Destroyed`, `onPause/Resume`, `onTrimMemory` all
tear down / recreate EGL surfaces without leaking the context. Minimizing
mid-race must not lose the device (desktop ports skip this entirely).

# Graphics — D3D9/Xenos -> Vulkan (primary) / GLES 3.1 (fallback)

No D3D12/Windows path exists in this repo by design.

## Backend selection

`SetupActivity` stores `gfx_api`: Auto (default) = Vulkan if
`libvulkan.so` loads (Adreno stock or Turnip), else GLES 3.1+.
The preference is **stored, not yet honored**: `Renderer_Backend()` reports
`GLES3.1` and the status line shows `gpu=GLES3.1(auto)` until the Vulkan
backend lands (backlog G-1). The old `Vulkan(boot:GLES)` label was removed
for reporting a backend that does not execute yet.

## FH2-specific load

- **Terrain streaming**: aggressive LOD (`Streaming_Update` scales with car
  speed; >55 m/s drops prefetch radius to hold frame budget).
- **Cars (PBR, many materials)**: material count is the draw-call driver;
  mobile path batches by shader key (XenosRecomp output) — see backlog G-2.
- **Realtime reflections / day-night**: reflection probes + shadow cascades
  scale with `draw_distance` (0.4..1.5) and `shadow_quality` (0/1/2).
- **Dynamic resolution**: scale 0.50..1.00 around base 0.85, servoed to the
  30/60 fps target (`Renderer_Frame`). Without an FBO the scale is stored,
  reported and applied to geometry once it lands (backlog G-5); the full
  surface is always cleared so no garbage borders appear.
- **Draw distance / shadows**: `draw_distance` (0.4..1.5) and `shadow_quality`
  (0/1/2) are stored in `RuntimeConfig` and exposed to the guest world manager
  once R-1 lands; the streaming LOD heuristic already consumes the draw
  multiplier. Not silently dropped — pending consumer, tracked as G-3.
- **Shader cache**: `ShaderCache_*` persists PSO/SPIR-V under app cache,
  capped, FNV-hashed keys — no per-launch recompile of FH2's 100s of variants.

## Shader triage real (2026-09-15, `media/shaders`, 175 `.fxobj`)

Containers Xenos escaneados e convertidos **um a um em processo isolado**
(em vez do dir-mode tudo-ou-nada): 174 arquivos com containers → **2.918
containers únicos → 2.463 HLSL (84,4%)**; **455 falham** com segfault —
todos vertex-shaders (`kind 0x01`, concentrados em `Cars/`: rims,
`shaders_16car_v16`, `shaders_slod_v16`).

Causa raiz: `ShaderRecompiler::recompile(VertexFetchInstruction)` assume
`vertexElements.find(address) != end()` via `assert` — em Release o assert
some e vira segfault. Shaders de carro do FH2 usam vertex-fetch sem entrada
de declaração correspondente (padrão que o Unleashed nunca teve). 204 HLSL
adicionais exigem prelude Unleashed (`b129…`, `cubeMapData`) e falham no DXC
(gap G-4b). Pipeline completo medido: **2.918 containers → 2.463 HLSL
(84,4%) → 2.259 SPIR-V válidos via `dxc-linux -spirv` (77,4%)**, 27 MB,
chaveados por FNV-1a e importáveis por `xenos::ImportDir` (pasta
`spv_cache` no app). Evidência e lista de falhas: `tools/evidence_fh2.md`,
`tools/shader_failures.csv`.

## Lifecycle (Android-only work)

`surfaceCreated/Changed/Destroyed`, `onPause/Resume`, `onTrimMemory` all
tear down / recreate EGL surfaces without leaking the context. Minimizing
mid-race must not lose the device (desktop ports skip this entirely).

`surfaceCreated/Changed/Destroyed`, `onPause/Resume`, `onTrimMemory` all
tear down / recreate EGL surfaces without leaking the context. Minimizing
mid-race must not lose the device (desktop ports skip this entirely).

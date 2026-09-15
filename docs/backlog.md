# Backlog — tracked, never silently ignored

Format: `ID — title (owner/area) — status`. File one GitHub issue per ID when
the repo goes live; `unimpl` counters in-app (`unimpl_d3d9=N`) feed triage.

## Recompilation

- R-1 — XenonAnalyse full-toml from retail FH2 default.xex — **open**
  (bootstrap TOML committed; needs retail run, stub docs per function).
- R-2 — `Unimplemented instruction` catalog (per-mnemonic issues) — **open**.
- R-3 — Patched XEXP path (title update) if retail needs TU — **open**.

## GPU

- G-1 — Vulkan backend beyond GLES bootstrap (`renderer_vulkan.cpp`) — **open**
  (preference stored; reporting fixed to GLES until it executes).
- G-2 — Material batching for PBR car shaders — **open**.
- G-3 — Reflection-probe + cascade-shadow scalers wired to settings — **partial**
  (config exists, guest hooks pending; values stored, not dropped).
- G-4 — Full SPIR-V cache import from XenosRecomp (media/shaders/*) — **open**.
- G-5 — Dynamic-resolution FBO (scale applies to geometry; full clear meanwhile) — **open**.

## Audio / Input / Platform

- A-1 — XMA decode cache — **open**.
- A-2 — XAudio2 3D voice graph — **open**.
- A-3 — Oboe wrapper (optional) — **deferred** (AAudio suffices API 28+).
- I-1 — HUD editor (drag/resize/opacity per control) — **open**.
- P-1 — Device lab (SD 7 Gen / Dimensity / Mali fps matrix) — **open**.
- P-2 — Play integrity / scoped-storage hardening for Android 14+ — **open**.

## Fixed in review round 1 (2026-09-15, evaluator 4.0/10)

- JNI lifecycle: `nativeShutdown` (join, no detach leak), double-init guard,
  single `ShaderCache_Init` owner + `mkdirs` + `fsync`, no hardcoded
  `/data/local/tmp`, status loop cancelled in `onDestroy`.
- Honest GPU reporting (GLES until Vulkan executes); full-surface clear.
- Input fusion (touch/pad slots), analog preserved on button events,
  deadzone 0.08, B→handbrake, RZ/Z trigger fallback fixed.
- Real SAF validation (`DocumentsContract`: `media/`/`default.xex` required);
  `content://` never treated as POSIX path.
- Guest memory via `mmap` + `MADV_DONTNEED` trim; `used` honest-0 pending R-1.
- Res-scale drift fixed (0.5..1.0 everywhere); release APK documented unsigned.

## Known limitation (honest)

This scaffold boots to a status/HUD shell without game data and compiles the
full APK in CI. **Playable FH2 requires completing R-1..G-4 with a legal
dump** — estimated months of title-specific runtime work (same class as
Unleashed/Lost Odyssey Recompiled efforts). No ETA is promised.

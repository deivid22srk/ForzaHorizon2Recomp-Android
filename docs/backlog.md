# Backlog — tracked, never silently ignored

Format: `ID — title (owner/area) — status`. File one GitHub issue per ID when
the repo goes live; `unimpl` counters in-app (`unimpl_d3d9=N`) feed triage.

## R-2 REAL — Unrecognized-instruction catalog (retail FH2 default.xex)

4.249 instâncias, 64 mnemônicos, medidos no codegen de 2026-09-15 (100%).
Cada linha vira issue com EA de exemplo do log. (Log completo: 266 MB de
codegen local, fora do git.)

| # | mnemônico | ocorrências | classe |
|---|-----------|-------------|--------|
| 1 | stfsu | 865 | update-form float store |
| 2 | bdzf | 695 | decrement-and-branch |
| 3 | lfsu | 441 | update-form float load |
| 4 | sthu | 364 | update-form half store |
| 5 | lhzu | 224 | update-form half load |
| 6 | vsel128 | 147 | VMX128 select |
| 7 | eqv | 134 | equivalence |
| 8 | subfze | 106 | subtract-extended |
| 9 | lfsux | 78 | indexed update float load |
| 10 | bso | 65 | branch-on-overflow |
| 11 | stvlxl128 | 64 | VMX128 store |
| 12 | lvxl128 | 64 | VMX128 load |
| 13 | vaddsws | 63 | VMX128 saturating add |
| 14 | vpkswss128 | 62 | VMX128 pack |
| 15 | vslo128 | 60 | VMX128 shift-left-octet |
| 16 | vminub | 59 | VMX128 min |
| 17 | vsro128 | 57 | VMX128 shift-right-octet |
| 18 | vmaxub | 55 | VMX128 max |
| 19 | addc | 46 | add-carrying |
| 20 | sthux | 45 | indexed update half store |
| 21 | lhzux | 44 | indexed update half load |
| 22 | vmaxuh | 36 | VMX128 max half |
| 23 | vsrb | 35 | VMX128 shift-right-byte |
| 24 | addme | 32 | add-minus-one-extended |
| 25 | vpkuwum128 | 28 | VMX128 pack |
| 26 | vminuh | 28 | VMX128 min half |
| 27 | vcfpuxws128 | 28 | VMX128 convert |
| 28 | stfsux | 26 | indexed update float store |
| 29 | vaddsbs | 25 | VMX128 saturating add byte |
| 30 | vsubuwm | 24 | VMX128 subtract |
| 31 | vslo | 24 | VMX128 shift-left-octet |
| 32 | vsl | 24 | VMX128 shift-left |
| 33 | vrlh | 18 | VMX128 rotate half |
| 34 | vrlw128 | 15 | VMX128 rotate word |
| 35 | lbzux | 14 | indexed update byte load |
| 36 | vpkuhum128 | 13 | VMX128 pack |
| 37 | stbux | 13 | indexed update byte store |
| 38 | lhau | 13 | update algebraic half load |
| 39 | vsubuws | 12 | VMX128 subtract saturate |
| 40 | vcmpequh | 12 | VMX128 compare |
| 41 | lfdu | 9 | update double load |
| 42 | vminuw | 8 | VMX128 min word |
| 43 | vnor128 | 7 | VMX128 nor |
| 44 | lwzux | 7 | indexed update word load |
| 45 | vctuxs | 6 | VMX128 convert |
| 46 | stfdu | 6 | update double store |
| 47 | bns | 6 | branch-if-not-summary |
| 48 | vpkuwus128 | 5 | VMX128 pack saturate |
| 49 | stdux | 5 | indexed update dword store |
| 50 | vpkshss128 | 4 | VMX128 pack |
| 51 | lvehx | 4 | VMX load element |
| 52 | frsqrte | 4 | FP reciprocal-sqrt estimate |
| 53 | mullhwu | 3 | multiply-high |
| 54 | vsrh | 2 | VMX128 shift-right half |
| 55 | vspltish | 2 | VMX128 splat |
| 56 | vslh | 2 | VMX128 shift-left half |
| 57 | vcmpgtuw | 2 | VMX128 compare |
| 58 | ldux | 2 | indexed update dword load |
| 59 | bdnzt | 2 | decrement-branch |
| 60 | vpkuhus128 | 1 | VMX128 pack |
| 61 | vcmpgtsb | 1 | VMX128 compare |
| 62 | vandc | 1 | VMX128 and-complement |
| 63 | lhbrx | 1 | indexed byte-reverse load |
| 64 | dcbst | 1 | data-cache-store |
| + | data-in-code | 7 × `Unable to decode` @0x832F1A78+ | jump-table bytes (TOML) |

Leitura: o grosso (linhas 1–5, 9, 20–21, 28, 35–38, 44, 49, 58) são formas
*update/indexed* de load/store —tradução C++ direta, baixo risco. O bloco
VMX128 (linhas 6, 11–18, 22–33, 39–40, 42–43, 45, 48, 50–51, 54–57, 60–62) é
emitido como scalar fallback hoje (correto, lento) — otimizar depois com
NEON. `bdzf/bso/bns/bdnzt` (2, 10, 47, 59) afetam loops/branches — prioridade
de corretude. Sem essas implementações o jogo não passa do boot real (R-3).

## Recompilation

- R-1 — XenonAnalyse full-toml from retail FH2 default.xex — **done**
  (resultado real: 0 tabelas; switches exigem análise manual — G-6).
- R-2 — `Unimplemented instruction` catalog — **done (tabela acima)**.
- R-3 — Boot real do codegen (kernel-import runtime p/ 388 imports) — **open**
  (inventário 100% mapeado: `tools/imports_xboxkrnl.txt` 206 + `tools/imports_xam.txt` 182).
- R-4 — Patched XEXP path (title update) — **open** (só base analisada).

## GPU

- G-1 — Vulkan backend beyond GLES bootstrap (`renderer_vulkan.cpp`) — **open**
  (preference stored; reporting fixed to GLES until it executes).
- G-2 — Material batching for PBR car shaders — **open**.
- G-3 — Reflection-probe + cascade-shadow scalers wired to settings — **partial**
  (config exists, guest hooks pending; values stored, not dropped).
- G-4 — Full SPIR-V cache import from XenosRecomp (media/shaders/*) — **partial**
  (2.259/2.918 SPIR-V válidos offline + `xenos::ImportDir` implementado;
  faltam: 455 vertex-fetch (root-cause no assert) + 204 com prelude
  Unleashed `b129/cubeMapData` — gap G-4b).
- G-5 — Dynamic-resolution FBO (scale applies to geometry; full clear meanwhile) — **open**.
- G-6 — Manual switch-table analysis (XenonAnalyse: 0 tabelas p/ este binário) — **open**.

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

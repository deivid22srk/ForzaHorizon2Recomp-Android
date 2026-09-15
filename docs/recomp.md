# Recompilation (XenonRecomp + XenosRecomp)

## 0. Resultados reais (2026-09-15, retail default.xex) — LEIA PRIMEIRO

Pipeline executado de verdade sobre o `default.xex` retail (XEX2,
21.807.104 bytes, entry `0x82BF2CD0`, base `0x82000000`, imagem 23 MB,
criptografia NORMAL + compressão BASIC, imports só `xboxkrnl.exe` + `xam.xex`):

- **XenonAnalyse**: exit 0, **0 switch tables** (padrões do analisador não
  cobrem o compilador deste jogo) — ver `tools/recompiled_functions.toml`.
- **XenonRecomp**: exit 0, **100% das funções recompiladas** —
  **505 arquivos `ppc_recomp.*.cpp`, 266 MB**, `ppc_func_mapping.cpp` com
  **~128,5 mil funções**, `ppc_config.h`
  (`BASE 0x82000000 SIZE 0x1700000 CODE 0x823F0000+0xF0258C`).
- **TOML manual real** (`tools/fh2recomp.toml`): 4 tabelas CRT encontradas por
  decrypt+análise (`save/restgprlr_14` = `0x82C01080`/`0x82C010D0`,
  `save/restfpr_14` = `0x82C015A0`/`0x82C015EC`); com elas os 8 `ERROR`s de
  config caíram para 4 (só VMX, que comprovadamente não existe: 26 `stvx`).
- **Unrecognized**: 4.249 instâncias / 64 mnemônicos distintos (catálogo real
  em `docs/backlog.md`) + 7 `Unable to decode` (dados em código,
  `0x832F1A78+`, inofensivos, já no TOML como `invalid_instructions`).
- **TU compilada de verdade**: `ppc_recomp.0.cpp` → `.o` 268 KB com clang++
  (só warnings benignos de vetorização). Build completo (505 TUs) e link
  pendem do runtime de kernel-imports (inventário 100% mapeado em
  `tools/imports_*.txt`: 206 xboxkrnl + 182 xam, 0 ordinais desconhecidos).
- **XenosRecomp**: 174 `.fxobj` → 2.918 containers únicos → **2.463 HLSL
  (84,4%)**; 455 vertex-shaders quebram o tool (vertex-fetch sem decl —
  `assert` estourado em release vira segfault). Detalhes em
  `docs/graphics.md`.
- **Armadilhas do tooling encontradas** (documentadas para não morder de novo):
  1. paths do TOML são relativos ao dir do TOML (absoluto = segfault em
     `Image::ParseImage(nullptr,0)`);
  2. XenosRecomp só compila com Clang (GCC quebra em `anonymous struct`);
  3. XenosRecomp dir-mode é tudo-ou-nada (1 shader ruim mata o batch) —
     triar por container isolado;
  4. `ppc_context.h` gerado exige include do SIMDe (`thirdparty/simde`).

Nada acima é simulado: o código gerado (266 MB) fica fora do git
(`recomp/generated/*.cpp` ignorado, derivado do jogo); o repo guarda TOML,
inventários e números.

## 1. Fetch (authenticated, validated)

```bash
export GH_TOKEN=<PAT with Contents:Read on forza-horizon-2-xex>
./tools/fetch_xex.sh ./default.xex
```

Checks: size > 0, first 4 bytes `XEX2`. Local dump verified:
`default.xex` magic `58 45 58 32` ("XEX2"), 21,807,104 bytes.

## 2. Analyse

```bash
./tools/run_analyse.sh ./default.xex ./tools/recompiled_functions.toml
```

Review the TOML: every auto-missed function gets a manual stub
`0xADDR = { name = "sub_ADDR" }` with a comment (address source + reason).
Unresolved virtual-call targets go through the `ppc_config.h` hash region
(see XenonRecomp README `ppc_config.h` macros).

## 3. Recompile

```bash
./tools/run_recomp.sh ./default.xex
```

Consumes `tools/fh2recomp.toml` + `recomp/runtime/ppc_context.h`, writes
`recomp/generated/ppc_recomp.*.cpp` + `fh2_sources.cmake` (picked up by
`app/src/main/cpp/CMakeLists.txt` automatically).

## 4. Warnings policy

`Unimplemented instruction` warnings are **backlog issues, never ignored**:
file one GitHub issue per mnemonic with the faulting EA + context, link it
in `docs/backlog.md`. The build stays green; the status screen reports
`unimpl_d3d9=N` from `d3d9_translator`.

## 5. Shaders (XenosRecomp — mandatory for FH2)

FH2 lighting/weather/cars/terrain are shader-heavy. Flow:

```
media/shaders/* (user dump)
  -> XenosRecomp -> HLSL (+ shader_common.h)
  -> DXC -> SPIR-V (Vulkan) / recompiled GLES variants
  -> ImportPrecompiled() into ShaderCache (persistent, capped 256MB)
```

Runtime never compiles at draw time on the hot path: `GetSpirv()` hits the
disk cache first. See `docs/graphics.md`.

# Evidence — verifiable facts of the offline recompilation (2026-09-15)

No game-derived bytes are stored here (only addresses, names, counts,
hashes). Reproduce with your own legal dump via `tools/*.sh` + the method
in `docs/recomp.md`.

## Input binary

- File: `default.xex` (Forza Horizon 2, Xbox 360, retail)
- Size: 21.807.104 bytes
- SHA-256: `43943f9491a726c0f1589dc4aa6b402b032c8aad1c4e5e6e4d7efda5d1204625`
- Magic: `XEX2`, entry `0x82BF2CD0`, base `0x82000000`, image 23 MB
- Encryption NORMAL (retail key) + compression BASIC; imports:
  `xboxkrnl.exe` (206 functions) + `xam.xex` (182 functions), 0 unknown
  ordinals (see `tools/imports_*.txt`).

## XenonAnalyse (hedge-dev/XenonRecomp @ main 2026-09-15, Clang 18)

- Exit 0, **0 switch tables** (4 hardcoded patterns don't match this XDK).

## XenonRecomp (same build, +4 CRT addresses in TOML)

- Exit 0, `Recompiling functions... 100%`
- Output: **505 `ppc_recomp.*.cpp`, 266 MB**; `ppc_func_mapping.cpp`
  (~128,5 mil entradas, inclui `__save/restgprlr_14`, `__save/restfpr_14`
  nos endereços do TOML); `ppc_config.h`
  (`BASE 0x82000000 SIZE 0x1700000 CODE 0x823F0000+0xF0258C`)
- Config ERRORs: 8 → 4 (restam só os VMX, inexistentes: 26 `stvx` no binário)
- `Unrecognized instruction`: **4.249 instâncias / 64 mnemônicos**
  (catálogo em `docs/backlog.md`); `Unable to decode`: 7 (`0x832F1A78+`,
  dados em código, cobertos no TOML)
- Prova de compilação: `ppc_recomp.0.cpp` → `.o` 268 KB via clang++ 18
  (`-I thirdparty/simde`), só warnings de vetorização

## XenosRecomp (Clang rebuild; GCC quebra — `anonymous struct`)

- Scan: 175 `.fxobj` (22 MB) → 174 com containers → **2.918 únicos**
- HLSL: **2.463 (84,4%)**; 455 vertex-shaders com segfault
  (`VertexFetchInstruction` sem decl — lista em `tools/shader_failures.csv`)
- SPIR-V (DXC oficial `dxc-linux`, `-spirv`, perfil pelo kind do container):
  **2.259 válidos (77,4% do total; 91,7% do HLSL)**, 27 MB, magic verificado;
  204 HLSL exigem prelude Unleashed (`b129`/`cubeMapData`) — gap G-4b
- Cache offline nomeado por FNV-1a (mesma função do `ShaderCache`), pronto
  para `xenos::ImportDir` (dir `spv_cache` no app)

## Tooling gotchas (para reprodução)

1. TOML paths relativos ao dir do TOML (absoluto = segfault em `ParseImage`)
2. XenosRecomp: Clang apenas; dir-mode é tudo-ou-nada (triar isolado)
3. `ppc_context.h` gerado exige `-I <xenonrecomp>/thirdparty/simde`

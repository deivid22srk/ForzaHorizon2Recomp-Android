# Recompilation (XenonRecomp + XenosRecomp)

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

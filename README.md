# Forza Horizon 2 — Recompiled para Android (XenonRecomp, mobile-first)

> **Sem assets aqui.** Este repositório não distribui `default.xex`, texturas,
> áudio, mapas ou qualquer conteúdo do jogo. Você precisa fornecer sua própria
> cópia legalmente adquirida (dump do seu Xbox 360). Mesmo padrão de
> Sonic Unleashed Recompiled / Lost Odyssey Recompiled.

Projeto educacional/homebrew, **sem afiliação** com Microsoft, Playground
Games ou Turn 10 Studios.

## O que é

Port nativo para Android (ARM64) de Forza Horizon 2 (Xbox 360) via
**recompilação estática**: XenonRecomp (PPC -> C++) + XenosRecomp (shaders
Xenos -> SPIR-V), com runtime **construído para Android desde a linha 1**
— sem backend Windows/D3D12 intermediário.

- GPU: Vulkan preferido (Adreno/Turnip), fallback OpenGL ES 3.1+.
- Áudio: AAudio de baixa latência (caminho Oboe reservado).
- Input: HUD touch de direção (volante + pedais + HB/câmera/marchas) +
  gamepad físico com o mapeamento do Xbox 360 (triggers analógicos).
- Arquivos: Storage Access Framework na primeira execução, joga no lugar.
- Mundo aberto: streaming/LOD orçado p/ celular, resolução dinâmica,
  30/60 fps, shader cache em disco, pause/resume + perda de contexto.

## Status honesto

Scaffold **compilável que gera APK instalável em CI** (tela de setup + HUD +
status nativo). Jogabilidade completa exige a recompilação do seu
`default.xex` + cache SPIR-V (meses de trabalho title-specific, como todo
projeto XenonRecomp) — ver `docs/backlog.md`. Sem ETA prometido.

## Estrutura

```
app/                  # módulo Android (Gradle): Activities, SAF, HUD, JNI
  src/main/java/...   # SetupActivity, MainActivity, NativeBridge, gamepad
  src/main/cpp/       # fh2_jni.cpp + CMakeLists (puxa recomp/runtime)
  src/main/AndroidManifest.xml
recomp/
  generated/          # saída do XenonRecomp (não editar; gitignored)
  runtime/            # gráfico, áudio, input, fs, streaming p/ Android
tools/                # fetch_xex.sh, run_analyse.sh, run_recomp.sh, TOMLs
docs/                 # arquitetura, recomp, gráficos, áudio, input, backlog
.github/workflows/build.yml
```

## Como buildar (local)

```bash
# 1. Token p/ o repo privado do XEX (nunca commitar)
export GH_TOKEN=<seu PAT com Contents:Read>
./tools/fetch_xex.sh ./default.xex   # valida magic XEX2

# 2. Recompilação offline (opcional p/ o APK; obrigatória p/ jogar)
./tools/run_analyse.sh ./default.xex
# revise tools/recompiled_functions.toml, adicione stubs manuais
./tools/run_recomp.sh ./default.xex

# 3. APK (funciona mesmo sem o XEX — build runtime-only)
./gradlew assembleDebug
# -> app/build/outputs/apk/debug/app-debug.apk
```

## CI

`.github/workflows/build.yml`: a cada push/PR gera `app-debug` + `app-release`.
A release é **unsigned** (sem keystore commitado — assine com seus secrets
para distribuir). Se `secrets.GH_PAT` existir, o runner baixa o `default.xex`,
valida `XEX2` e tenta o codegen; sem o secret, o build continua em modo runtime-only (verde).
Nada protegido é commitado; o `.xex` vive só no runner.

Configure uma vez: Settings -> Secrets and variables -> Actions -> `GH_PAT`
(PAT com acesso ao repo privado `forza-horizon-2-xex`).

## Docs

- `docs/architecture.md` — visão geral mobile-first
- `docs/recomp.md` — pipeline XenonRecomp/XenosRecomp + validação XEX2
- `docs/graphics.md` — Vulkan/GLES, LOD, resolução dinâmica
- `docs/audio.md` — AAudio/3D
- `docs/input.md` — HUD de direção + gamepad 360
- `docs/streaming.md` — orçamentos de RAM/streaming
- `docs/backlog.md` — pendências rastreáveis (incl. `Unimplemented instruction`)

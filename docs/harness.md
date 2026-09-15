# Host boot harness — first execution of the recompiled guest

`harness/` builds a **host (x86_64 Linux) boot test** that links the
XenonRecomp output against a logging kernel shim, to prove the recompiled
PPC actually executes. It contains **no game bytes** (generated TUs come from
your own dump and are gitignored).

## Build / run (local)

```bash
cd harness
make PPC_DIR=<your codegen dir> SIMDE=<xenonrecomp>/thirdparty/simde -j$(nproc)
./fh2boot <your decrypted image>.bin
```

`PPC_DIR` must hold `ppc_recomp.*.cpp`, `ppc_func_mapping.cpp`, `ppc_config.h`,
`ppc_context.h`, `ppc_recomp_shared.h`.

## What had to be right for the guest to run

1. **Dispatch LUT.** Indirect calls read
   `*(PPCFunc**)(base + 0x82000000 + 0x1700000 + (va-0x823F0000)*2)`
   (`PPC_LOOKUP_FUNC` in `ppc_context.h`). The harness fills it from
   `PPCFuncMappings[]` (128 582 entries). Without it every virtual call hits
   NULL.
2. **Guest VAs are absolute.** The PE is mapped at `0x82000000` (not 0), so
   guest `r1`/stack and all stores are absolute too. The harness reserves a
   sparse 2.5 GB window and puts the stack at `0x9FFFFFF0`. A 64 MB window
   faults immediately.
3. **Skip `.reloc`.** Its VA (`0x1680200`) lands past `PPC_IMAGE_SIZE`
   (`0x1700000`) and clobbers the dispatch LUT.
4. **Null-safe indirect dispatch.** `ppc_context.h` is patched so
   `PPC_CALL_INDIRECT_FUNC` goes through `fh2_lookup_guest()`, which logs
   misses instead of hard-crashing.

## Observed boot (retail default.xex)

```
LUT entries=128582
BOOT _xstart guest=... size=0xa0000000 r1=0x9ffffff0
[indirect-miss 1] guest=0x00000000
[kernel] HalReturnToFirmware(1)
```

The guest enters `_xstart` (`0x82BF2CD0`), runs through the dispatch LUT and the
kernel shim (`NtAllocateVirtualMemory`, `KeGetCurrentProcessType`,
`RtlInitializeCriticalSection` all hit), then reaches a **sanity gate** in
`sub_82BFFCF0` -> `sub_82BFFC18`, which calls `RtlImageXexHeaderField`
(xboxkrnl ord `0x12b`, thunk `0x832F1074`). That reads a loader-owned global at
`0x8200076C`; with no XEX loader it is 0, so the title takes its
`HalReturnToFirmware` shutdown path. This is the **current frontier**: the next
step is a real XEX/kernel loader that sets up the module structure the CRT
expects — the same runtime work tracked as R-3 in `docs/backlog.md`.

## Status

- Recompiled guest **runs** on host (real execution, not a stub).
- Loader shim added (`RtlImageXexHeaderField` + seeded module/header). With
  correct absolute VAs the CRT now reaches its own launch gate
  (`sub_82BFFCF0` -> `sub_82BFFC18`) and, because `_xstart` overwrites the
  loader globals before the gate reads them, still takes the
  `HalReturnToFirmware(1)` abort path. Fixing this requires the kernel to
  populate those globals in response to the guest's own init calls (the R-3
  runtime), not pre-seeding.
- 375 kernel imports still logging stubs (`harness/stubs.cpp`, generated from
  `tools/imports_*.txt`); 13 have real shims (heap, critical sections, pool,
  firmware-return) in `harness/kernel_stubs.cpp`.
- Not yet playable on Android: needs the loader/runtime (R-3) and the Vulkan
  backend (G-1).
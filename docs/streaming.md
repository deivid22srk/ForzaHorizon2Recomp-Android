# Streaming & memory (open-world on mobile)

Xbox 360: 512 MB unified. Target phones: 4–12 GB shared with the OS, thermal
limits, no swap. FH2 streams terrain/cars/audio constantly.

Guest pages come from `mmap(MAP_ANONYMOUS)` (zero-fill, no 256 MB memset
stall); trim returns streaming pages with `MADV_DONTNEED` instead of
dirty-memset. `Memory_Stats.used` stays 0 until the guest allocator hooks
land (R-1) — reported honestly, not estimated.

## Budgets (`fh2_config.h`)

- Guest XEX window: 256 MB reserved (malloc, zeroed).
- Tile ring: 128 MB, LOD 0/1/2 selected by `Streaming_Update(x,y,z,speed,draw)`.
- Shader disk cache: 256 MB cap.
- Workers: min(4, cpu_count) — scaled at runtime, never desktop-sized pools.

## Policy

- Speed > 55 m/s (~200 km/h): prefetch radius *0.8, LOD may drop 2->1.
- `onTrimMemory >= TRIM_MEMORY_RUNNING_CRITICAL`: resident halved, LOD -1,
  shader mem-cache keeps disk copy (re-fault cheap).
- Draw distance multiplier (0.4..1.5) + shadow quality exposed in settings;
  CI default 1.0/med holds 30 fps on SD7-class (projected, pending device lab).

## Filesystem

`FS_SetGameRoot` takes the SAF tree URI (persisted) or POSIX path.
`content://` roots are validated in Java via `DocumentsContract` (children
must include `media/` or `default.xex`) and flagged to native with
`nativeSetAssetValidated`; POSIX roots resolve `media/...` directly. First
launch forces the SAF picker; nothing is copied (plays in place).

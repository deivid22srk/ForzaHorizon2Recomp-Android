# Streaming & memory (open-world on mobile)

Xbox 360: 512 MB unified. Target phones: 4–12 GB shared with the OS, thermal
limits, no swap. FH2 streams terrain/cars/audio constantly.

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
`content://` roots are served via Java `ContentResolver`; POSIX roots resolve
`media/...` directly. First launch forces the SAF picker; nothing is copied
(plays in place, Android 11+ `MANAGE_EXTERNAL_STORAGE` rationale in manifest
comment).

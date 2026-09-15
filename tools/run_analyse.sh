#!/usr/bin/env bash
# run_analyse.sh — XenonAnalyse: XEX -> jump-table TOML.
# Usage: ./tools/run_analyse.sh [default.xex] [tools/recompiled_functions.toml]
set -euo pipefail
XEX="${1:-./default.xex}"
OUT="${2:-./tools/recompiled_functions.toml}"
ANALYSE_BIN="${XENON_ANALYSE:-./thirdparty/XenonRecomp/build/XenonAnalyse}"

test -s "${XEX}" || { echo "Missing ${XEX}; run tools/fetch_xex.sh first." >&2; exit 1; }
if [[ ! -x "${ANALYSE_BIN}" ]]; then
  echo "XenonAnalyse not found at ${ANALYSE_BIN}" >&2
  echo "Build it: git clone --recursive https://github.com/hedge-dev/XenonRecomp thirdparty/XenonRecomp && cmake -S thirdparty/XenonRecomp -B thirdparty/XenonRecomp/build && cmake --build thirdparty/XenonRecomp/build" >&2
  exit 1
fi
mkdir -p "$(dirname "${OUT}")"
"${ANALYSE_BIN}" "${XEX}" "${OUT}"
echo "Wrote ${OUT}; review for missing functions (see docs/recomp.md)."

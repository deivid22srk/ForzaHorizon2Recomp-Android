#!/usr/bin/env bash
# fetch_xex.sh — authenticated download of default.xex from the private repo.
# Usage: GH_TOKEN=<pat> ./tools/fetch_xex.sh [out_path]
# Never hardcode tokens; CI passes GH_PAT via secrets.GH_PAT.
set -euo pipefail
OUT="${1:-./default.xex}"
# NOTE: the /raw/refs/heads/... github.com form returns 404 for private repos;
# raw.githubusercontent.com with the token header is the working form
# (validated: XEX2 magic, 21807104 bytes).
URL="https://raw.githubusercontent.com/deivid22srk/forza-horizon-2-xex/main/default.xex"

if [[ -z "${GH_TOKEN:-}" ]]; then
  echo "ERROR: GH_TOKEN is not set. Export a PAT with Contents:Read on forza-horizon-2-xex." >&2
  echo "  export GH_TOKEN=ghp_...; ./tools/fetch_xex.sh" >&2
  exit 2
fi

echo "Downloading default.xex (private repo)..."
curl -L --retry 3 --retry-delay 5 \
  -H "Authorization: token ${GH_TOKEN}" \
  -H "Accept: application/vnd.github.v3.raw" \
  -o "${OUT}" "${URL}"

test -s "${OUT}" || { echo "ERROR: default.xex empty/missing" >&2; exit 1; }
if ! head -c 4 "${OUT}" | grep -q "XEX2"; then
  echo "ERROR: invalid XEX2 magic (wrong file or HTML login page — check PAT scope)" >&2
  exit 1
fi
echo "OK: $(stat -c%s "${OUT}") bytes, XEX2 magic verified -> ${OUT}"

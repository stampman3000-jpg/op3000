#!/bin/bash
# Build & package OP-ARCADE (UID OPAR) via official Akai MPC Plugin Builder SDK.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MPC_SDK="${MPC_SDK:-}"

if [ ! -f "$MPC_SDK/tools/build.py" ]; then
  echo "ERROR: Set MPC_SDK to a checkout of https://github.com/inmusic-software/mpc-plugin-builder-sdk" >&2
  exit 1
fi
echo "== 1/3: Generating OP-ARCADE artwork & MPC skins =="
python3 "$ROOT/tools/gen_ui_assets.py"
python3 "$ROOT/tools/gen_skins.py"
python3 "$ROOT/tools/render_preview.py"

echo "== 2/3: Building OP-ARCADE (native + ARM32 + ARM64 + validate) =="
python3 "$MPC_SDK/tools/build.py" "$ROOT/plugins/op_arcade"

echo "== 3/3: Packaging deployable ZIP =="
mkdir -p "$ROOT/dist"
python3 "$MPC_SDK/tools/package.py" "$ROOT/plugins/op_arcade" --out "$ROOT/dist"
ls -lh "$ROOT/dist"/*.zip

#!/bin/bash
# Full validation suite for OP-ARCADE: silicon tuning, 32 presets, CPU benchmark, and official host_sim.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MPC_SDK="${MPC_SDK:-}"
TMP="$ROOT/build_tests"
mkdir -p "$TMP"

echo "== 1/3: Dual-Chip Silicon Tuning, FM Bite, PSG & 32-Preset Suite =="
clang++ -std=c++17 -O3 -Wall -Wextra "$ROOT/tests/test_op_arcade.cpp" -o "$TMP/test_op_arcade"
"$TMP/test_op_arcade"

echo ""
echo "== 2/3: Desktop CPU Benchmark =="
clang++ -std=c++17 -O3 -Wall -Wextra "$ROOT/tests/bench_op_arcade.cpp" -o "$TMP/bench_op_arcade"
"$TMP/bench_op_arcade"

if [ -x "$MPC_SDK/.sdk-tools/host_sim" ]; then
  echo ""
  echo "== 3/3: Official Akai SDK host_sim Verification =="
  "$MPC_SDK/.sdk-tools/host_sim" "$ROOT/plugins/op_arcade/build/native/op3000.dylib" test \
    --wav "$TMP/op_arcade_test.wav"
fi

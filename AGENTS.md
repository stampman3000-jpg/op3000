Global rules: also follow ~/agents/AGENTS.md (Johnny's shared rules for all agents).

# AGENTS.md

OP3000 is a public MPC instrument. UID `OPAR`, vendor Stampman3000, MIT. It is a dual-chip FM arcade synth: a YM2151-style core and a YM2612-style core, an SN76489-style PSG, a per-voice filter, chorus, and delay. `README.md` is the page for people. This file is the map and the rules.

## Layout

- `dsp/YamahaSiliconFm.h` — FM core (logsin and exp tables, envelopes, the eight algorithms).
- `dsp/OpArcadeEngine.h` — PSG, PCB tone, chorus, delay, and the per-voice filter.
- `bridge/OpArcadePresets.h` — the factory presets.
- `plugins/op_arcade/` — the SDK project. `plugin.json` is the identity: UID `OPAR`, version `1.0.1`, and the arm32 and arm64 filenames.
- `tools/gen_ui_assets.py`, `gen_skins.py`, `render_preview.py` — artwork. `tools/build_all.sh` and `tools/run_tests.sh` are the commands that are actually in the repo.
- `tests/test_op_arcade.cpp`, `tests/bench_op_arcade.cpp` — the DSP check and the CPU bench.
- `screenshots/`, `docs/previews/`, `tested.json`, `LICENSE`, `NOTICE.md`.

## Build and test

Set `MPC_SDK` to a checkout of the official Akai MPC Plugin Builder SDK (`inmusic-software/mpc-plugin-builder-sdk`). Both scripts stop if `$MPC_SDK/tools/build.py` is missing.

```bash
bash tools/build_all.sh
bash tools/run_tests.sh
```

`build_all.sh` draws the skins, builds native plus ARM32 plus ARM64 with `build.py`, then packs a zip with `package.py` into `dist/`. The folder the MPC wants is `Stampman3000 - VST - OP3000`.

`run_tests.sh` compiles the DSP test and the bench with `clang++` into `build_tests/`. If `$MPC_SDK/.sdk-tools/host_sim` is there, it also runs host_sim on `plugins/op_arcade/build/native/op3000.dylib`. When that simulator is missing, the script still exits after the two clang tests. A green run then does not mean the plugin was packaged.

## Gotchas

- Leave UID `OPAR` and the `.so` names in `plugin.json` alone. The MPC finds a saved plugin by that uid.
- Do not allocate, lock, or print on the audio path inside the engine headers.
- A wide-open filter is bypassed on purpose, so existing presets stay dry and the dry FM path stays cheap. Leave that bypass.
- `.gitignore` ignores `*.png` except `screenshots/*.png`. Skin PNGs under `plugins/` are build output.

## Release

Tags `v1.0.0` and `v1.0.1` are already published. `plugin.json` says `1.0.1`.

- Build both arm32 and arm64 with `tools/build_all.sh`. Do not commit `dist/`, `build/`, `build_tests/`, `*.zip`, `*.so`, or `*.dylib`.
- Never delete, replace, or re-upload a published release zip. A fix is a new version beside the old one, tagged on the commit that built it.
- A catalogue listing also follows the shared rules: full public source, documented build flags, and Gen2 registered in `pluginList-arm-64bit`.
- This repo is public. No home address, no personal email, no private files. Commits are `Stampman3000 <stampman3000@gmail.com>`. Do not rewrite history.

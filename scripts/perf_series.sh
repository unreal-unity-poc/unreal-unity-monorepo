#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FRAMES="${PERF_FRAMES:-300}"
STAMP="$(date -u +%Y%m%dT%H%M%SZ)"
LOG="${PERF_LOG:-$ROOT/perf/logs/perf-$STAMP.jsonl}"
BUILD_LOG="${LOG%.jsonl}.build.log"

mkdir -p "$(dirname "$LOG")"
: > "$LOG"
: > "$BUILD_LOG"

now_ms() {
  python3 -c 'import time; print(int(time.time() * 1000))'
}

json_escape() {
  python3 -c 'import json,sys; print(json.dumps(sys.argv[1]))' "$1"
}

log_event() {
  local target="$1"
  local phase="$2"
  local extra="${3:-}"
  local ts
  ts="$(now_ms)"
  if [[ -n "$extra" ]]; then
    printf '{"ts_unix_ms":%s,"target":"%s","phase":"%s",%s}\n' "$ts" "$target" "$phase" "$extra" >> "$LOG"
  else
    printf '{"ts_unix_ms":%s,"target":"%s","phase":"%s"}\n' "$ts" "$target" "$phase" >> "$LOG"
  fi
}

skip_target() {
  local target="$1"
  local reason="$2"
  log_event "$target" "skip" "\"reason\":$(json_escape "$reason")"
}

run_target() {
  local target="$1"
  shift
  log_event "$target" "series_start" "\"frames\":$FRAMES"
  if "$@" >> "$LOG" 2>> "$BUILD_LOG"; then
    log_event "$target" "series_end" "\"ok\":true"
  else
    local status=$?
    log_event "$target" "series_end" "\"ok\":false,\"exit_status\":$status"
  fi
}

cd "$ROOT"

echo "perf log: $LOG"
echo "build log: $BUILD_LOG"
echo "frames: $FRAMES"

log_event "series" "start" "\"frames\":$FRAMES"

./scripts/build_native_plugin.sh >> "$BUILD_LOG" 2>&1

cmake -S perf/native-ffi -B perf/native-ffi/build >> "$BUILD_LOG" 2>&1
cmake --build perf/native-ffi/build >> "$BUILD_LOG" 2>&1

cmake -S qt -B qt/build >> "$BUILD_LOG" 2>&1
cmake --build qt/build >> "$BUILD_LOG" 2>&1

cmake -S cef -B cef/build >> "$BUILD_LOG" 2>&1
cmake --build cef/build >> "$BUILD_LOG" 2>&1

cmake -S browserffi -B browserffi/build >> "$BUILD_LOG" 2>&1
cmake --build browserffi/build >> "$BUILD_LOG" 2>&1

cmake -S v8-blink -B v8-blink/build >> "$BUILD_LOG" 2>&1
cmake --build v8-blink/build >> "$BUILD_LOG" 2>&1

RUSTUP_STABLE_BIN="$HOME/.rustup/toolchains/stable-aarch64-apple-darwin/bin"
if [[ -d "$RUSTUP_STABLE_BIN" ]]; then
  PATH="$RUSTUP_STABLE_BIN:$PATH" wasm-pack build wasm --target web >> "$BUILD_LOG" 2>&1
else
  wasm-pack build wasm --target web >> "$BUILD_LOG" 2>&1
fi

run_target "rust-direct" cargo run --quiet --manifest-path rust-engine/Cargo.toml --example perf -- --target rust-direct --frames "$FRAMES"
run_target "native-ffi" ./perf/native-ffi/build/rust_native_ffi_perf --frames "$FRAMES"
run_target "qt-ffi" ./qt/build/rust_qt_perf --frames "$FRAMES"
run_target "cef-bridge-json" ./cef/build/rust_cef_bridge_perf --frames "$FRAMES"
run_target "browserffi-wkwebview" env BROWSERFFI_PERF_FRAMES="$FRAMES" ./browserffi/build/rust_browserffi_renderer.app/Contents/MacOS/rust_browserffi_renderer
run_target "wasm-worker-transfer" node wasm/perf-node-main.mjs --mode transfer --frames "$FRAMES"
run_target "wasm-worker-shared" node wasm/perf-node-main.mjs --mode shared --frames "$FRAMES"
run_target "v8-blink-concept" ./v8-blink/build/rust_v8_blink_concept

skip_target "unity-render" "Unity Hub installed, but Unity Editor download previously aborted; no headless Unity Player/Editor perf run available."
skip_target "godot-render" "Godot C# project builds, but this series does not launch the Godot editor/player headlessly yet."
skip_target "unreal-render" "Unreal Engine editor/runtime is not installed on this Mac; plugin scaffold only."
skip_target "o3de-render" "O3DE project/Gem scaffold is documented, but no local O3DE editor/runtime automation exists yet."
skip_target "flax-render" "Flax renderer scaffold is documented, but no local Flax editor/runtime automation exists yet."
skip_target "stride-render" "Stride renderer scaffold is documented, but no local Stride project/runtime automation exists yet."
skip_target "monogame-render" "MonoGame renderer scaffold is documented, but no local MonoGame project/runtime automation exists yet."
skip_target "bevy-render" "Bevy renderer scaffold is documented, but no local Bevy app/perf harness exists yet."
skip_target "defold-render" "Defold renderer scaffold is documented, but no local Defold project/native-extension automation exists yet."
skip_target "cocos2dx-render" "Cocos2d-x renderer scaffold is documented, but no local Cocos2d-x project/runtime automation exists yet."
skip_target "cryengine-render" "CryEngine editor/runtime is not available on this Mac; component scaffold only."
skip_target "webview2-render" "WebView2 is Windows-only and cannot run on this Mac."
skip_target "tauri-render" "Tauri renderer scaffold exists, but npm/Tauri runtime perf automation is not installed in this series."
skip_target "dioxus-render" "Dioxus renderer scaffold exists, but desktop/webview runtime perf automation is not installed in this series."
skip_target "leptos-render" "Leptos renderer scaffold exists, but browser/Trunk runtime perf automation is not installed in this series."

log_event "series" "summary" "\"log\":$(json_escape "$LOG"),\"build_log\":$(json_escape "$BUILD_LOG")"
log_event "series" "end" "\"frames\":$FRAMES"

echo "done: $LOG"

# Perf Harness

Run the serial performance suite:

```bash
PERF_FRAMES=300 ./scripts/perf_series.sh
```

The suite writes JSONL events to `perf/logs/perf-*.jsonl` and build output to a sibling `*.build.log`.

Every event includes:

- `ts_unix_ms`: wall-clock timestamp in milliseconds.
- `target`: benchmark target.
- `phase`: `start`, `frame`, `summary`, `skip`, or series wrapper phase.

Measured locally:

- `rust-direct`: Rust crate calls with no FFI.
- `native-ffi`: direct C ABI through `dlopen`.
- `qt-ffi`: Qt `QLibrary` bridge through the Rust C ABI.
- `cef-bridge-json`: C++ Rust FFI plus JSON payload build for a Chromium canvas bridge.
- `browserffi-wkwebview`: Rust FFI plus macOS `WKWebView` JavaScript submission.
- `wasm-worker-transfer`: Rust/WASM in a Worker, transferable `ArrayBuffer` frames to main.
- `wasm-worker-shared`: Rust/WASM in a Worker, `SharedArrayBuffer` frame reads on main.
- `v8-blink-concept`: concept binary that reports memory layout only.

Unavailable renderer targets emit `skip` events with reasons so the comparison log stays explicit.

OpenTelemetry is intentionally not used here. The logs are explicit JSONL instrumentation with no monkey patching or hidden runtime hooks.

Native FFI benchmarks register `rust_engine_set_event_callback` so they exercise both directions: host-to-Rust input/tick calls and Rust-to-host event callbacks.

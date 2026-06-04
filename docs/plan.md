# Rust Engine Renderer Comparison Plan

## Goal

Build a proof of concept where the Rust engine owns the authoritative simulation state and multiple UI/rendering technologies render the same earth-like globe.

The target behavior is the same everywhere:

- Blue earth/globe with green Rust-owned surface patches.
- Atmosphere shell or glow.
- Arrow-key rotation.
- Zoom in/out.
- Reset control.
- Input flows to Rust first; Rust decides state; renderer displays Rust state.

## Architecture Rules

1. Rust owns the simulation state.
2. Renderers send input intent into Rust before rendering.
3. Rust ticks and emits the next `EarthRenderState`.
4. Renderers draw the pushed state, with pull APIs kept for diagnostics and fallback.
5. Per-frame state should use native structs, pointers, flat buffers, shared memory, or WASM memory. JSON is only acceptable for low-frequency/debug paths.
6. Each renderer should log timestamped lifecycle, input, tick, callback, render, and frame summary events.
7. OpenTelemetry can be considered later, but only without monkey patching or hidden runtime instrumentation.

Preferred native loop:

```text
host input -> rust_engine_set_control_input
host tick  -> rust_engine_tick
Rust       -> registered EngineEvent callback
host UI    -> render callback state
```

## FFI Classification

True same-process native FFI:

- Unity: C# P/Invoke into `librust_engine.dylib`.
- Unreal Engine: C++ plugin loads/calls Rust C ABI directly.
- Godot C#: C# P/Invoke into `librust_engine.dylib`.
- Qt: C++/Qt loads/calls Rust C ABI directly.
- CryEngine: C++ entity component loads/calls Rust C ABI directly.
- CEF host: native C++ bridge calls Rust C ABI, then forwards state into Chromium.
- WebView2 host: native Windows C++ bridge calls Rust C ABI, then forwards state into WebView2.
- Browser FFI: native WebKit/WKWebView host calls Rust C ABI and injects render state into HTML canvas.
- Native perf harnesses: direct C ABI through `dlopen`/platform library loading.

Not native FFI, but still useful comparisons:

- Tauri: Rust backend can use `rust-engine` directly and send state into the webview.
- Dioxus: Rust UI can use `rust-engine` directly.
- Leptos: Rust UI/WASM path uses Rust-owned state but not native C FFI.
- WASM worker: Rust compiles to WebAssembly in a worker; main thread renders frames via transferable `ArrayBuffer` or `SharedArrayBuffer`.
- V8/Blink: lower-level embedding concept; target is Rust/shared memory into V8, Blink renders via GPU.

End users should not install Unity or Unreal editors. They should receive a packaged app/game that bundles the engine runtime and the Rust dynamic library. Editors are only needed for development/build machines.

## Current Status

Rust engine:

- Exposes `ControlInput`, `EarthRenderState`, `SurfacePatch`, `SurfacePatchView`, and callback-based `EngineEvent`.
- `rust_engine_tick` updates authoritative globe state and emits callback event kind `1`.
- `scripts/build_native_plugin.sh` builds and copies the Rust dylib into renderer folders.
- `cargo test --manifest-path rust-engine/Cargo.toml` has passed locally.

Unity:

- Unity Editor `6000.3.17f1` arm64 is installed at `/Applications/Unity/Hub/Editor/6000.3.17f1/Unity.app`.
- Standalone player builds and runs as `Rust Unity Renderer`.
- Rust dylib is bundled under `Contents/PlugIns/librust_engine.dylib`.
- Globe is rendering blue/green after adding project-owned unlit shaders.
- Next: verify controls manually, keep Unity-generated `.meta`/scene/project settings intentionally, and add Unity to perf automation.

Godot:

- Godot Mono project opens and renders the Rust-driven globe.
- Zoom was fixed via wheel/magnify/pan gesture input queued into Rust `ControlInput.zoom`.
- Next: add headless build/player automation and perf logging.

Qt:

- Qt renderer opens as `Rust Qt Earth Renderer`.
- Wheel zoom was fixed and routed through Rust input.
- Next: polish rendering parity and include UI smoke/perf checks.

Browser FFI:

- Native macOS WebKit host opens as `Rust Browser FFI Earth`.
- Keyboard/mouse beeps were fixed by consuming handled events.
- Flow is native input -> Rust C ABI -> Rust callback -> native JS injection -> HTML canvas.
- Next: add stronger smoke checks and perf measurements for JS bridge cost.

Tauri:

- Tauri app renders the globe after bridge fixes.
- Wheel zoom was added.
- Uses Rust engine directly rather than native C FFI.
- Next: decide whether to keep direct Rust crate usage or force the same C ABI for apples-to-apples comparison.

WASM:

- Rust/WASM worker path exists.
- Frame decoding was fixed for the current header layout.
- Uses transferable buffers and/or `SharedArrayBuffer`.
- Next: validate both transfer and shared modes in browser, not only node perf.

Dioxus:

- Dioxus renderer builds/runs.
- Uses Rust engine directly.
- Next: add equivalent zoom/key handling and performance instrumentation if not already present in the UI path.

Leptos:

- Leptos renderer runs through Trunk after control fixes.
- Global window key and wheel listeners were added.
- Frame rate was reduced to improve responsiveness.
- Next: investigate remaining sluggishness, keep renderer from blocking input, and add browser perf logs.

CEF:

- Native bridge/perf scaffolding exists.
- Current perf target measures Rust FFI plus JSON payload generation for the Chromium canvas bridge.
- Next: run a full CEF window smoke test with the globe and replace JSON on the hot path if needed.

WebView2:

- Scaffold exists for Windows.
- Cannot be fully run on this macOS machine.
- Next: test on Windows, verify native host -> Rust C ABI -> WebView2 rendering.

V8/Blink:

- Concept scaffold exists.
- Next: define a concrete minimum viable host: Rust/shared memory into V8, Blink renders to a GPU-backed surface, then log frame timings.

CryEngine:

- Component scaffold exists.
- Current macOS machine may not be a realistic CryEngine runtime target.
- Next: verify supported platform/toolchain, then build the native component around the same Rust ABI.

Unreal Engine:

- Plugin scaffold exists in `unreal/Plugins/RustEngine`.
- The plugin already loads the Rust library, resolves ABI symbols, registers a Rust event callback, sends `ControlInput`, ticks Rust, and reads `SurfacePatchView`.
- Current actor still needs rendering parity work: rename conceptual "player/orbit" pieces to globe/land, add blue/green materials, add atmosphere, add camera/light/bootstrap scene, and package a standalone app.
- Unreal Engine is not installed yet on this machine.

## Unreal Engine Plan

1. Install Unreal Engine in its own folder.
2. Confirm Apple Silicon/macOS support for the chosen UE version and required Xcode/toolchain.
3. Generate project files for `unreal/RustRenderer.uproject`.
4. Build the `RustEngine` plugin/module.
5. Fix dynamic library discovery so packaged builds find `librust_engine.dylib` reliably.
6. Add a default map or startup actor that spawns `ARustSimulationActor`.
7. Replace placeholder mesh naming with earth/globe terminology.
8. Add materials:
   - Blue ocean sphere.
   - Green instanced land patches from `SurfacePatchView`.
   - Faint atmosphere shell.
9. Add camera and directional light.
10. Route input to Rust:
    - Arrow keys rotate.
    - Mouse wheel / `+` / `-` zoom.
    - `R` reset.
11. Verify Rust-to-Unreal callback path is used after each tick.
12. Build and launch a standalone player titled `Rust Unreal Renderer`.
13. Add Unreal smoke/perf logging:
    - App start.
    - Library load.
    - Symbol resolution.
    - Input sample.
    - Rust tick start/end.
    - Rust callback timestamp.
    - Render apply timestamp.
    - Frame summary.
14. Compare Unreal against Unity:
    - Startup time.
    - Build/package size.
    - Input-to-render latency.
    - Per-frame FFI/callback cost.
    - Renderer-side CPU cost.
    - Packaging friction.

## Performance Plan

Update `scripts/perf_series.sh` so skip reasons match current reality. Unity is now installed and can build/run a player, so it should move from skipped to measured.

Per-target JSONL events should include:

- `ts_unix_ms`
- `target`
- `phase`
- `frame_index`
- `dt_seconds`
- `input_sample_ns`
- `rust_tick_ns`
- `callback_latency_ns`
- `render_apply_ns`
- `frame_total_ns`
- `state_bytes`
- `transport`

Targets to measure in series:

- `rust-direct`
- `native-ffi`
- `qt-ffi`
- `browserffi-wkwebview`
- `cef-bridge`
- `unity-render`
- `godot-render`
- `tauri-render`
- `dioxus-render`
- `leptos-render`
- `wasm-worker-transfer`
- `wasm-worker-shared`
- `v8-blink-concept`
- `unreal-render`
- `cryengine-render`
- `webview2-render`

Measurement order:

1. Build Rust native plugin.
2. Build all locally runnable native hosts.
3. Build all web/WASM hosts.
4. Run each target serially.
5. Write one JSONL log per series.
6. Write target-specific build logs.
7. Mark unavailable targets with explicit `skip` events and exact reasons.

## Packaging Plan

For native engines:

- Bundle Rust dynamic library inside the shipped app.
- Ensure runtime library paths are stable.
- Verify codesigning/notarization implications on macOS.
- Document editor/build-machine requirements separately from end-user requirements.

For webview/browser renderers:

- Decide whether each renderer uses direct Rust crate calls, C ABI calls, WASM worker memory, or a JS bridge.
- Keep the hot path structured and measurable.
- Avoid JSON for high-frequency frame state unless a target is intentionally measuring JSON bridge overhead.

For WASM:

- Keep the Rust engine in a worker.
- Compare transferable `ArrayBuffer` versus `SharedArrayBuffer`.
- Document COOP/COEP requirements for `SharedArrayBuffer` in browser deployments.

## Immediate Next Steps

1. Commit or intentionally ignore Unity-generated assets and metadata.
2. Update Unity README to mention installed version, build script, bundled shaders, and standalone player path.
3. Update `scripts/perf_series.sh` so Unity is no longer skipped.
4. Add a Unity perf/smoke launch mode that exits automatically after `PERF_FRAMES`.
5. Install Unreal Engine.
6. Build/open the Unreal project.
7. Fix Unreal rendering parity and package `Rust Unreal Renderer`.
8. Add Unreal to the serial UI launch flow.
9. Add Unreal to the serial perf flow.
10. Re-run the full UI sequence and perf sequence.

## Open Questions

- Should every desktop target be forced through the same C ABI, even when it is written in Rust and could call the crate directly?
- Should the comparison optimize for lowest-latency native state transfer, or for realistic production ergonomics per framework?
- Should the globe renderer stay deliberately simple, or should we add texture/lighting/detail once every target reaches parity?
- Which platform should own WebView2 and CryEngine validation if macOS cannot run them fully?
- Do we want a single launcher script that opens each renderer, waits for window close, then starts the next?

## Definition Of Done

This POC is complete when:

- Unity, Unreal, Godot, Qt, Browser FFI, Tauri, WASM, Dioxus, and Leptos can render the same Rust-owned globe state.
- Unreal has a working standalone player comparable to Unity.
- All supported local targets can be launched one by one.
- Controls go to Rust before the UI renders.
- Native FFI targets use bidirectional callbacks.
- WASM worker targets use transfer/shared memory instead of JSON for frame data.
- Performance logs are produced in a consistent JSONL schema.
- Documentation clearly says which targets are true native FFI, direct Rust, web worker/WASM, or platform-only scaffolds.

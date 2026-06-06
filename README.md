# Rust Unity Unreal POC

This repo is a proof of concept for a same-process Rust to Unity architecture:

- Rust owns simulation state.
- Renderers send input intent to Rust first.
- Rust updates the authoritative earth/globe state.
- Rust can call back into the renderer through a registered FFI callback after each tick.
- Renderers draw the state pushed by Rust, or fall back to pulling native `#[repr(C)]` structs / packed WASM frames.
- Per-frame data uses an FFI pointer plus count, not JSON.

## Layout

- `rust-engine/` is a Rust `cdylib` that exports the engine FFI.
- `unity/` is the Unity renderer project.
- `godot/` is the Godot renderer project.
- `unreal/` is reserved for the Unreal renderer comparison.
- `qt/` is reserved for the Qt renderer comparison.
- `cryengine/` is reserved for the CryEngine renderer comparison.
- `tauri/` is the Tauri webview renderer comparison.
- `cef/` is the Chromium Embedded Framework renderer comparison.
- `webview2/` is the Microsoft WebView2 renderer comparison.
- `dioxus/` is the Dioxus Rust UI renderer comparison.
- `leptos/` is the Leptos Rust UI renderer comparison.
- `wasm/` is the direct Rust WebAssembly plus JavaScript shared linear memory comparison.
- `electron-wasm/` is an Electron renderer using Rust-generated WASM in a Node worker.
- `electron-abi/` is an Electron renderer using a Node-API addon that calls the Rust C ABI.
- `v8-blink/` is the lower-level V8/Blink embedding comparison.
- `browserffi/` is a native WebKit browser window driven through the Rust C ABI.
- `scripts/build_native_plugin.sh` builds the Rust dynamic library and copies it into each renderer's native library folder.

## FFI Contract

Rust exposes:

- `rust_engine_create() -> *mut Engine`
- `rust_engine_destroy(*mut Engine)`
- `rust_engine_set_control_input(*mut Engine, ControlInput)`
- `rust_engine_tick(*mut Engine, dt_seconds: f32)`
- `rust_engine_set_event_callback(*mut Engine, RustEngineEventCallback, user_data)`
- `rust_engine_clear_event_callback(*mut Engine)`
- `rust_engine_render_state(*const Engine) -> EarthRenderState`
- `rust_engine_surface_patches(*const Engine) -> SurfacePatchView`

`EarthRenderState` contains the Rust-owned globe transform, camera distance, atmosphere radius, and light vector.

`EngineEvent` is the Rust-to-host callback payload. Its `kind = 1` event is emitted after each successful tick and carries the current `EarthRenderState`.

`SurfacePatchView` contains:

- `ptr: *const SurfacePatch`
- `len: usize`

The pointer is Rust-owned and remains valid until the next engine mutation or destroy call. Hosts read from it during the frame and do not free it.

The preferred native loop is bidirectional:

```text
host input -> rust_engine_set_control_input
host tick  -> rust_engine_tick
Rust       -> registered EngineEvent callback
host UI    -> render callback state
```

The explicit pull APIs remain useful as fallback diagnostics and for hosts that have not registered callbacks.

The C/C++ contract is mirrored in `rust-engine/include/rust_engine.h`.

## Build

Build and copy the native plugin to every renderer folder:

```bash
./scripts/build_native_plugin.sh
```

For an optimized native library:

```bash
./scripts/build_native_plugin.sh release
```

## Renderers

Each renderer should show the same Rust-owned state:

- Earth-like blue sphere/globe.
- Green surface patches from Rust-owned `SurfacePatch` data.
- Atmosphere glow.
- Arrow-key rotation and zoom inputs routed to Rust before rendering.

## Unity

Open `unity/` as the Unity project. The bootstrap script creates a camera, light, ground plane, and simulation object at play time.

Controls:

- `WASD` or arrow keys move the Rust-owned player.
- `Left Shift` boosts movement speed.

Unity Hub CLI docs: https://docs.unity.com/en-us/hub/hub-cli

## Godot

Open `godot/` with the C# capable Godot editor. The main scene creates a camera, light, ground plane, and simulation object at play time.

## Comparison Targets

The goal is to compare renderer integrations while keeping Rust as the authoritative simulation layer:

- Unity: C# P/Invoke into the Rust dynamic library.
- Godot: C# P/Invoke into the Rust dynamic library.
- Unreal: native C++ module/plugin calling the same Rust dynamic library.
- Qt: C++ Qt app calling the same Rust dynamic library.
- CryEngine: native C++ entity component calling the same Rust dynamic library.
- Tauri: Rust backend uses `rust-engine` directly, then serializes state into a webview canvas.
- CEF: C++ host calls the Rust dynamic library, then forwards state into Chromium.
- WebView2: Windows C++ host calls the Rust dynamic library, then forwards state into WebView2.
- Dioxus: Rust UI uses `rust-engine` directly and renders the state in a desktop webview.
- Leptos: Rust UI uses `rust-engine` directly and renders the same state in the browser/WASM path.
- WASM: Rust simulation compiles to WebAssembly and JavaScript reads entity structs from WASM linear memory.
- Electron WASM: Electron renderer draws HTML/canvas while a Node worker runs the Rust/WASM engine and sends frame buffers.
- Electron ABI: Electron renderer draws HTML/canvas while a Node-API native addon calls the Rust C ABI in-process.
- V8/Blink: native host embeds browser-engine pieces, shares native/WASM-style memory into V8, lets Blink/GPU render.
- Browser FFI: native host captures controls, calls Rust through FFI, and renders returned state in an HTML canvas.

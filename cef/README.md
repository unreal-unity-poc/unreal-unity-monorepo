# Chromium Embedded Framework Renderer

CEF renders the Rust-owned earth state through a native C++ host and a Chromium canvas.

Hot-path frame data flows as:

```text
CEF input -> C++ ControlInput -> Rust dynamic library tick -> Rust callback -> JS canvas bridge
```

This folder contains a reusable C++ Rust bridge plus web assets. A complete CEF app still needs a CEF SDK drop or package manager integration.

The C++ bridge registers a native callback with Rust and uses the callback state when building the browser payload.

Smoke-test the Rust bridge after staging the native library:

```bash
../scripts/build_native_plugin.sh
cmake -S . -B build
cmake --build build
./build/rust_cef_bridge_smoke
```

Expected output:

- Blue earth globe in the CEF canvas.
- Green Rust-owned surface patches.

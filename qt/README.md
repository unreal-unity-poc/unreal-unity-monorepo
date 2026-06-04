# Qt Renderer

Qt renders the Rust-owned simulation state through a small C++ Widgets app.

Planned shape:

- A Qt C++ application dynamically loads the Rust engine library.
- Qt sends input into Rust through the same C ABI.
- Qt renders `EarthRenderState` and `SurfacePatchView` as a projected globe with `QPainter`.

The key comparison point against Unity and Unreal is the lower engine overhead and more conventional desktop UI/tooling model.

Build after copying the native library:

```bash
../scripts/build_native_plugin.sh
cmake -S . -B build
cmake --build build
./build/rust_qt_renderer
```

Expected output:

- Blue projected earth globe.
- Green Rust-owned surface patches.
- Arrow-key rotation and zoom controlled by Rust state.

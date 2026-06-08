# Defold Renderer

Defold renders the Rust-owned earth state through a native extension that exposes
Rust state to Lua and the render script.

Hot-path frame data should flow as:

```text
Defold input -> Lua/native extension ControlInput -> Rust tick -> Rust callback -> Defold render script
```

The target integration shape is a C/C++ native extension that loads or links the
Rust dynamic library, resolves the C ABI from `rust-engine/include/rust_engine.h`,
and passes compact state into Defold's Lua/render pipeline.

Build the native library before wiring the extension into a Defold project:

```bash
../scripts/build_native_plugin.sh
```

Expected output:

- Blue projected or mesh-backed earth.
- Green Rust-owned surface patches.
- Atmosphere shell or glow where the render pipeline supports it.

Notes:

- This folder is currently a scaffold; Defold project and extension files are still to be added.
- Defold is 2D-first, so the initial renderer may use a projected globe before deeper 3D rendering.

Reference:

- Defold native extensions: https://defold.com/manuals/extensions/

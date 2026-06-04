# CryEngine Renderer

CryEngine renders the Rust-owned earth state through a native C++ entity component.

Hot-path frame data flows as:

```text
CryEngine input -> C++ ControlInput -> Rust tick -> Rust callback -> CryEngine aux geometry
```

CryEngine registers a native C++ callback with `rust_engine_set_event_callback` and renders the Rust-pushed `EarthRenderState`.

Build the native library before wiring the component into a CryEngine project:

```bash
../scripts/build_native_plugin.sh
```

Expected output:

- Blue debug globe.
- Green Rust-owned surface patches.

Notes:

- The official CryEngine system requirements are Windows-focused for the editor, so this folder is a source scaffold on this Mac.
- The first renderer uses `IRenderAuxGeom` debug drawing to prove the shared Rust state contract before introducing CryEngine meshes, materials, or entity spawning.

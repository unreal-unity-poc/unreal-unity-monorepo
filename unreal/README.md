# Unreal Renderer

Unreal renders the Rust-owned simulation state through a native C++ plugin.

Shape:

- A native Unreal C++ plugin/module loads the Rust dynamic library.
- Unreal sends input into Rust with C ABI calls.
- Unreal reads `EarthRenderState` and `SurfacePatchView` from Rust and maps them to a globe mesh plus instanced land patches.

The key comparison point against Unity is that Unreal can call the native Rust ABI directly from C++ without going through C# P/Invoke.

Build the native library before opening the `.uproject`:

```bash
../scripts/build_native_plugin.sh
```

Expected output:

- Blue earth mesh.
- Green Rust-owned surface patches.

# Godot Renderer

Godot renders the Rust-owned simulation state through C# P/Invoke.

Hot-path frame data flows as:

```text
Godot input -> C# ControlInput -> Rust tick -> Rust callback -> Godot globe
```

Godot registers a managed delegate with `rust_engine_set_event_callback`, pins the renderer instance with `GCHandle`, and applies the `EarthRenderState` Rust pushes back during `rust_engine_tick`.

Open this folder with the C# capable Godot editor after building the native plugin:

```bash
../scripts/build_native_plugin.sh
```

Expected output:

- Blue earth sphere with green land patches.
- Atmosphere shell.
- Arrow-key rotation and zoom controlled by Rust state.

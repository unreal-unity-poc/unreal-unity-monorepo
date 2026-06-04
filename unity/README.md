# Unity Renderer

Unity renders the Rust-owned simulation state through C# P/Invoke.

Hot-path frame data flows as:

```text
Unity input -> C# ControlInput -> Rust tick -> Rust callback -> Unity globe
```

Unity registers a managed delegate with `rust_engine_set_event_callback`, pins the renderer instance with `GCHandle`, and applies the `EarthRenderState` Rust pushes back during `rust_engine_tick`.

Open this folder as the Unity project after building the native plugin:

```bash
../scripts/build_native_plugin.sh
```

Expected output:

- Blue earth sphere with green land patches.
- Atmosphere shell.
- Arrow-key rotation and zoom controlled by Rust state.

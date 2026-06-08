# MonoGame Renderer

MonoGame renders the Rust-owned earth state through a C# game loop using
P/Invoke.

Hot-path frame data should flow as:

```text
MonoGame input -> C# ControlInput -> Rust tick -> Rust callback -> MonoGame draw calls
```

MonoGame is a useful lower-overhead C# game framework comparison. It will not
exercise a heavy editor pipeline, but it can measure managed P/Invoke, callback
handling, content loading, and draw-loop cost in a compact target.

Build the native library before running the MonoGame project:

```bash
../scripts/build_native_plugin.sh
```

Expected output:

- Blue projected or mesh-backed earth.
- Green Rust-owned surface patches.
- Atmosphere shell or glow.

Notes:

- This folder is currently a scaffold; MonoGame project files are still to be added.

Reference:

- MonoGame documentation: https://docs.monogame.net/

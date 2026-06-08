# Stride Renderer

Stride renders the Rust-owned earth state through C# scripts using P/Invoke.

Hot-path frame data should flow as:

```text
Stride input -> C# ControlInput -> Rust tick -> Rust callback -> Stride entity/material update
```

Stride is useful as a managed, open-source C# engine comparison against Unity
and Godot C#. The implementation should share as much managed ABI binding shape
as possible with the Unity and Godot targets while keeping Stride-specific
rendering code isolated.

Build the native library before opening the Stride project:

```bash
../scripts/build_native_plugin.sh
```

Expected output:

- Blue earth entity.
- Green Rust-owned surface patches.
- Atmosphere shell or glow.

Notes:

- This folder is currently a scaffold; Stride project files and scripts are still to be added.

Reference:

- Stride scripts: https://doc.stride3d.net/latest/en/manual/scripts/index.html

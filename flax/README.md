# Flax Renderer

Flax renders the Rust-owned earth state through a native C++ script/module or a
C# script using P/Invoke.

Preferred hot path:

```text
Flax input -> C++ ControlInput -> Rust tick -> Rust callback -> Flax actor/material update
```

The first implementation should use native C++ scripting when possible because
it gives direct engine API access while preserving the same Rust C ABI used by
Unreal, CryEngine, O3DE, and Cocos2d-x. A C# script path can be kept as a
managed comparison against Unity, Godot C#, Stride, and MonoGame.

Build the native library before opening the Flax project:

```bash
../scripts/build_native_plugin.sh
```

Expected output:

- Blue earth actor or mesh.
- Green Rust-owned surface patches.
- Atmosphere shell or glow.

Notes:

- This folder is currently a scaffold; Flax project files and scripts are still to be added.

Reference:

- Flax C++ scripting: https://docs.flaxengine.com/manual/scripting/cpp/index.html

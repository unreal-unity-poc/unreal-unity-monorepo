# O3DE Renderer

Open 3D Engine renders the Rust-owned earth state through a native C++ Gem.

Hot-path frame data should flow as:

```text
O3DE input component -> C++ ControlInput -> Rust tick -> Rust callback -> O3DE mesh/material update
```

The target integration shape is a runtime Gem that loads the staged Rust dynamic
library, resolves the C ABI from `rust-engine/include/rust_engine.h`, registers
`rust_engine_set_event_callback`, and updates an entity with globe mesh,
surface-patch instances, camera, and light.

Build the native library before wiring the Gem into an O3DE project:

```bash
../scripts/build_native_plugin.sh
```

Expected output:

- Blue earth mesh.
- Green Rust-owned surface patches.
- Atmosphere shell or glow.

Notes:

- O3DE is a strong fit because Gem modules are native C++ runtime modules.
- This folder is currently a scaffold; the actual Gem project files are still to be added.

Reference:

- O3DE Gem module system: https://docs.o3de.org/docs/user-guide/programming/gems/overview/

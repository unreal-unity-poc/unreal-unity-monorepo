# Bevy Renderer

Bevy renders the Rust-owned earth state through a Rust-native ECS/rendering path.

Preferred hot path:

```text
Bevy input system -> rust-engine crate tick -> Bevy systems/resources -> Bevy renderer
```

This target is not native C FFI by default. Its value is the Rust-to-Rust
comparison: no managed boundary, no dynamic library loading, and no C callback
shim unless an optional ABI comparison mode is added later.

Expected output:

- Blue earth mesh.
- Green Rust-owned surface patches.
- Atmosphere shell or glow.

Notes:

- This folder is currently a scaffold; Bevy `Cargo.toml` and systems are still to be added.
- If an apples-to-apples mode is needed later, add a second Bevy path that loads the C ABI dynamically.

Reference:

- Bevy plugins: https://bevy.org/learn/quick-start/getting-started/plugins/

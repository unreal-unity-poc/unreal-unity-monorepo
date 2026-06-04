# Leptos Renderer

Leptos renders the Rust-owned earth state through a Rust/WASM UI.

Hot-path frame data flows as:

```text
Browser timer -> Rust Engine tick in WASM -> EarthRenderState/SurfacePatch slice -> SVG DOM
```

This target uses the `rust-engine` crate directly and compiles the same simulation logic into the Leptos app. That is a different comparison point than C#/C++ FFI: same Rust source, browser/WASM execution.

Run shape:

```bash
trunk serve
```

Expected output:

- Blue earth globe.
- Green Rust-owned surface patches.

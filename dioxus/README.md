# Dioxus Renderer

Dioxus renders the Rust-owned earth state from a Rust UI app.

Hot-path frame data flows as:

```text
Dioxus app loop -> Rust Engine tick -> EarthRenderState/SurfacePatch slice -> SVG/desktop webview
```

This target uses the `rust-engine` crate directly. That avoids an FFI boundary for the Rust-to-Rust case and lets us compare UI rendering overhead rather than ABI cost.

Run shape:

```bash
cargo run
```

Expected output:

- Blue earth globe.
- Green Rust-owned surface patches.

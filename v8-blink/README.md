# V8 Blink Renderer

This target documents and scaffolds the lower-level browser engine comparison.

Conceptual hot path:

```text
Rust
   ↕ shared memory
V8
   ↕
Blink
   ↕
GPU
```

The intended shape is a native host that:

- Loads or embeds the Rust simulation.
- Exposes the earth frame buffer into V8 as an `ArrayBuffer`/external backing store or a WASM memory view.
- Runs JavaScript that reads the shared memory without JSON parsing.
- Lets Blink paint to the browser compositor and GPU pipeline.

This is deliberately lower level than CEF. CEF already packages Chromium process management and browser embedding; `v8-blink/` is where we compare the raw architecture and identify what a custom embedder would need.

Expected output:

- Blue earth globe.
- Green Rust-owned surface patches.

# Tauri Renderer

Tauri renders the Rust-owned earth state through a Rust backend and a webview canvas.

Hot-path frame data flows as:

```text
Webview input -> Tauri command -> Rust Engine tick -> serialized EarthRenderState/SurfacePatch DTOs -> Canvas
```

This target uses the `rust-engine` crate directly inside the Tauri backend. That means the simulation boundary is native Rust, while the browser rendering boundary is serialized through Tauri commands.

Run shape:

```bash
npm install
npm run tauri dev
```

Expected output:

- Blue earth globe.
- Green Rust-owned surface patches.

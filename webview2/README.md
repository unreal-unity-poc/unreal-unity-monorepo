# WebView2 Renderer

WebView2 renders the Rust-owned earth state through a Windows native host and a WebView2 canvas.

Hot-path frame data flows as:

```text
WebView2 input -> C++ ControlInput -> Rust dynamic library tick -> EarthRenderState/SurfacePatchView -> JS canvas bridge
```

This target is Windows-only. The folder contains the host bridge and web assets, but it cannot be built or launched on this Mac.

Expected output:

- Blue earth globe.
- Green Rust-owned surface patches.

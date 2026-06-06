# Electron ABI Renderer

Electron renderer owns the HTML/canvas window. Each animation frame calls a Node-API native addon. The addon loads `librust_engine.dylib`, sends input through the Rust C ABI, ticks Rust, receives the Rust callback state, flattens state into an `ArrayBuffer`, and returns it to the UI thread for canvas drawing.

```text
Electron renderer thread
  -> Node-API addon
  -> Rust C ABI: set input + tick
Rust
  -> EngineEvent callback
Node-API addon
  -> ArrayBuffer frame
Electron renderer thread
  -> canvas draw
```

Run:

```bash
npm install
npm start
```

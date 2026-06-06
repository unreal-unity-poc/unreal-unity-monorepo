# Electron WASM Renderer

Electron renderer owns the HTML/canvas window. The Rust engine is compiled to WebAssembly and runs inside a Node `worker_threads` worker created by Electron's main process. The worker sends flat binary frame buffers to the renderer UI thread, which decodes and draws the globe.

```text
Electron renderer thread
  -> input IPC
Electron main process
  -> input message
Node worker
  -> Rust/WASM engine tick
  -> ArrayBuffer frame
Electron main process
  -> frame IPC
Electron renderer thread
  -> canvas draw
```

Run:

```bash
npm install
npm start
```

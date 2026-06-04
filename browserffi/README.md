# Browser FFI Renderer

This renderer opens a native browser window that renders HTML while Rust remains the authority for state.

On macOS it uses `WKWebView`, which Apple documents as the platform-native WebKit view for embedding web content in an app UI. The host loads local HTML with `loadHTMLString` and pushes Rust-owned state into the page with `evaluateJavaScript`.

Hot-path frame data flows as:

```text
keyboard / scroll
        ↓
native host ControlInput
        ↓
Rust dylib through C ABI
        ↓
Rust callback with EarthRenderState + SurfacePatchView pull for static patches
        ↓
WKWebView JavaScript call
        ↓
HTML canvas
```

The browser layer does not own simulation state. It only draws the snapshot Rust returns.

Build and run:

```bash
../scripts/build_native_plugin.sh
cmake -S . -B build
cmake --build build
./build/rust_browserffi_renderer.app/Contents/MacOS/rust_browserffi_renderer
```

Controls:

- Arrow keys rotate the earth.
- `+`, `-`, Page Up, Page Down, or scroll zoom.
- `R` resets the Rust-owned camera and rotation state.

References:

- [Apple WKWebView](https://developer.apple.com/documentation/WebKit/WKWebView)
- [WKWebView loadHTMLString](https://developer.apple.com/documentation/webkit/wkwebview/loadhtmlstring%28_%3Abaseurl%3A%29?language=objc)
- [WKWebView evaluateJavaScript](https://developer.apple.com/documentation/webkit/wkwebview/evaluatejavascript%28_%3Ain%3Ain%3Acompletionhandler%3A%29?language=objc)

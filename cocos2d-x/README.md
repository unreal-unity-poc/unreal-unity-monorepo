# Cocos2d-x Renderer

Cocos2d-x renders the Rust-owned earth state through native C++ scene/layer code.

Hot-path frame data should flow as:

```text
Cocos2d-x input -> C++ ControlInput -> Rust tick -> Rust callback -> Cocos2d-x draw/update
```

Cocos2d-x is useful as a lightweight native C++ game-engine comparison. It can
exercise the same Rust C ABI path as Unreal, CryEngine, O3DE, and Qt while
staying closer to a compact game loop.

Build the native library before opening the Cocos2d-x project:

```bash
../scripts/build_native_plugin.sh
```

Expected output:

- Blue projected or mesh-backed earth.
- Green Rust-owned surface patches.
- Atmosphere shell or glow where the renderer supports it.

Notes:

- This folder is currently a scaffold; Cocos2d-x project files are still to be added.
- Cocos2d-x is 2D-first, so the first pass can render the globe as a projected view.

Reference:

- Cocos2d-x getting started: https://docs.cocos.com/cocos2d-x/manual/en/about/getting_started.html

#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROFILE="${1:-debug}"

if [[ "$PROFILE" == "release" ]]; then
  RELEASE_FLAG="--release"
  TARGET_DIR="release"
else
  RELEASE_FLAG=""
  TARGET_DIR="debug"
fi

cargo build --manifest-path "$ROOT/rust-engine/Cargo.toml" $RELEASE_FLAG

case "$(uname -s)" in
  Darwin)
    LIB_NAME="librust_engine.dylib"
    PLUGIN_DIRS=(
      "$ROOT/unity/Assets/Plugins/macOS"
      "$ROOT/godot/native/macos"
      "$ROOT/qt/native/macos"
      "$ROOT/unreal/Plugins/RustEngine/Source/ThirdParty/RustEngineLibrary/Mac"
      "$ROOT/o3de/native/macos"
      "$ROOT/flax/native/macos"
      "$ROOT/stride/native/macos"
      "$ROOT/monogame/native/macos"
      "$ROOT/bevy/native/macos"
      "$ROOT/defold/native/macos"
      "$ROOT/cocos2d-x/native/macos"
      "$ROOT/cryengine/native/macos"
      "$ROOT/tauri/src-tauri/native/macos"
      "$ROOT/cef/native/macos"
      "$ROOT/browserffi/native/macos"
      "$ROOT/electron-abi/native/macos"
      "$ROOT/dioxus/native/macos"
      "$ROOT/leptos/native/macos"
    )
    ;;
  Linux)
    LIB_NAME="librust_engine.so"
    PLUGIN_DIRS=(
      "$ROOT/unity/Assets/Plugins/Linux"
      "$ROOT/godot/native/linux"
      "$ROOT/qt/native/linux"
      "$ROOT/unreal/Plugins/RustEngine/Source/ThirdParty/RustEngineLibrary/Linux"
      "$ROOT/o3de/native/linux"
      "$ROOT/flax/native/linux"
      "$ROOT/stride/native/linux"
      "$ROOT/monogame/native/linux"
      "$ROOT/bevy/native/linux"
      "$ROOT/defold/native/linux"
      "$ROOT/cocos2d-x/native/linux"
      "$ROOT/cryengine/native/linux"
      "$ROOT/tauri/src-tauri/native/linux"
      "$ROOT/cef/native/linux"
      "$ROOT/browserffi/native/linux"
      "$ROOT/electron-abi/native/linux"
      "$ROOT/dioxus/native/linux"
      "$ROOT/leptos/native/linux"
    )
    ;;
  MINGW*|MSYS*|CYGWIN*)
    LIB_NAME="rust_engine.dll"
    PLUGIN_DIRS=(
      "$ROOT/unity/Assets/Plugins/Windows"
      "$ROOT/godot/native/windows"
      "$ROOT/qt/native/windows"
      "$ROOT/unreal/Plugins/RustEngine/Source/ThirdParty/RustEngineLibrary/Win64"
      "$ROOT/o3de/native/windows"
      "$ROOT/flax/native/windows"
      "$ROOT/stride/native/windows"
      "$ROOT/monogame/native/windows"
      "$ROOT/bevy/native/windows"
      "$ROOT/defold/native/windows"
      "$ROOT/cocos2d-x/native/windows"
      "$ROOT/cryengine/native/windows"
      "$ROOT/tauri/src-tauri/native/windows"
      "$ROOT/cef/native/windows"
      "$ROOT/webview2/native/windows"
      "$ROOT/browserffi/native/windows"
      "$ROOT/electron-abi/native/windows"
      "$ROOT/dioxus/native/windows"
      "$ROOT/leptos/native/windows"
    )
    ;;
  *)
    echo "Unsupported OS: $(uname -s)" >&2
    exit 1
    ;;
esac

for PLUGIN_DIR in "${PLUGIN_DIRS[@]}"; do
  mkdir -p "$PLUGIN_DIR"
  cp "$ROOT/target/$TARGET_DIR/$LIB_NAME" "$PLUGIN_DIR/$LIB_NAME"
  echo "Copied $LIB_NAME to $PLUGIN_DIR"
done

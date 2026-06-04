#include "rust_engine_bridge.h"

#include <windows.h>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    MessageBoxW(
        nullptr,
        L"WebView2 host scaffold is ready. Wire this bridge into a WebView2 controller and call window.chrome.webview.postMessage with RustEngineBridge::tick_json().",
        L"Rust WebView2 Renderer",
        MB_OK);
    return 0;
}


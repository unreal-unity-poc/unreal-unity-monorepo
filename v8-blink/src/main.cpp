#include "rust_engine.h"

#include <iostream>

int main()
{
    std::cout
        << "V8/Blink concept target:\n"
        << "  Rust owns EarthRenderState and SurfacePatch memory.\n"
        << "  V8 should receive that memory as an external ArrayBuffer or WASM memory view.\n"
        << "  Blink should render from JS/DOM/canvas/WebGPU without JSON on the hot path.\n";

    std::cout << "EarthRenderState stride bytes: " << sizeof(EarthRenderState) << '\n';
    std::cout << "SurfacePatch stride bytes: " << sizeof(SurfacePatch) << '\n';
    return 0;
}

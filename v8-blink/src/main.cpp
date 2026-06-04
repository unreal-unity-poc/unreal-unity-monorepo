#include "rust_engine.h"

#include <chrono>
#include <iostream>

int main()
{
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    std::cout
        << "{\"ts_unix_ms\":" << ts
        << ",\"target\":\"v8-blink-concept\""
        << ",\"phase\":\"summary\""
        << ",\"earth_state_stride_bytes\":" << sizeof(EarthRenderState)
        << ",\"surface_patch_stride_bytes\":" << sizeof(SurfacePatch)
        << ",\"note\":\"concept target only; no embedded V8/Blink runtime measured\""
        << "}\n";
    return 0;
}

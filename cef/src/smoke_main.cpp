#include "rust_engine_bridge.h"

#include <iostream>

int main()
{
    RustEngineBridge bridge;
    ControlInput input {};
    input.rotate_y = 1.0f;
    input.zoom = 0.25f;
    std::cout << bridge.tick_json(input, 1.0f / 60.0f) << '\n';
    return 0;
}

#include "rust_engine_bridge.h"

#include <sstream>
#include <stdexcept>
#include <windows.h>

RustEngineBridge::RustEngineBridge()
{
    library_handle = reinterpret_cast<void*>(LoadLibraryA(RUST_ENGINE_LIBRARY_PATH));
    if (!library_handle) {
        throw std::runtime_error("Could not load Rust engine library");
    }

    create = resolve<RustEngineCreate>("rust_engine_create");
    destroy = resolve<RustEngineDestroy>("rust_engine_destroy");
    set_control_input = resolve<RustEngineSetControlInput>("rust_engine_set_control_input");
    tick_engine = resolve<RustEngineTick>("rust_engine_tick");
    render_state = resolve<RustEngineRenderState>("rust_engine_render_state");
    surface_patches = resolve<RustEngineSurfacePatches>("rust_engine_surface_patches");
    engine = create();
}

RustEngineBridge::~RustEngineBridge()
{
    if (engine && destroy) {
        destroy(engine);
        engine = nullptr;
    }

    if (library_handle) {
        FreeLibrary(reinterpret_cast<HMODULE>(library_handle));
    }
}

std::string RustEngineBridge::tick_json(ControlInput input, float dt_seconds)
{
    set_control_input(engine, input);
    tick_engine(engine, dt_seconds);

    const EarthRenderState state = render_state(engine);
    const SurfacePatchView patches = surface_patches(engine);
    std::ostringstream json;
    json << "{\"state\":{"
         << "\"radius\":" << state.radius
         << ",\"atmosphereRadius\":" << state.atmosphere_radius
         << ",\"rotationX\":" << state.rotation_x
         << ",\"rotationY\":" << state.rotation_y
         << ",\"cloudRotationY\":" << state.cloud_rotation_y
         << ",\"cameraDistance\":" << state.camera_distance
         << ",\"lightX\":" << state.light_x
         << ",\"lightY\":" << state.light_y
         << ",\"lightZ\":" << state.light_z
         << "},\"patches\":[";

    for (size_t index = 0; index < patches.len; ++index) {
        const SurfacePatch& patch = patches.ptr[index];
        if (index != 0) {
            json << ',';
        }

        json << "{\"latDegrees\":" << patch.lat_degrees
             << ",\"lonDegrees\":" << patch.lon_degrees
             << ",\"radiusDegrees\":" << patch.radius_degrees
             << ",\"stretchX\":" << patch.stretch_x
             << ",\"stretchY\":" << patch.stretch_y
             << '}';
    }

    json << "]}";
    return json.str();
}

template <typename T>
T RustEngineBridge::resolve(const char* name)
{
    void* symbol = reinterpret_cast<void*>(
        GetProcAddress(reinterpret_cast<HMODULE>(library_handle), name));
    if (!symbol) {
        throw std::runtime_error(std::string("Missing Rust symbol: ") + name);
    }

    return reinterpret_cast<T>(symbol);
}


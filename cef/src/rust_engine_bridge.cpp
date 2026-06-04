#include "rust_engine_bridge.h"

#include <sstream>
#include <stdexcept>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace
{
void* load_library(const char* path)
{
#if defined(_WIN32)
    return reinterpret_cast<void*>(LoadLibraryA(path));
#else
    return dlopen(path, RTLD_NOW | RTLD_LOCAL);
#endif
}

void unload_library(void* handle)
{
    if (!handle) {
        return;
    }

#if defined(_WIN32)
    FreeLibrary(reinterpret_cast<HMODULE>(handle));
#else
    dlclose(handle);
#endif
}

void* resolve_symbol(void* handle, const char* name)
{
#if defined(_WIN32)
    return reinterpret_cast<void*>(GetProcAddress(reinterpret_cast<HMODULE>(handle), name));
#else
    return dlsym(handle, name);
#endif
}
}

RustEngineBridge::RustEngineBridge()
{
    library_handle = load_library(RUST_ENGINE_LIBRARY_PATH);
    if (!library_handle) {
        throw std::runtime_error("Could not load Rust engine library");
    }

    create = resolve<RustEngineCreate>("rust_engine_create");
    destroy = resolve<RustEngineDestroy>("rust_engine_destroy");
    set_control_input = resolve<RustEngineSetControlInput>("rust_engine_set_control_input");
    tick_engine = resolve<RustEngineTick>("rust_engine_tick");
    set_event_callback = resolve<RustEngineSetEventCallback>("rust_engine_set_event_callback");
    clear_event_callback = resolve<RustEngineClearEventCallback>("rust_engine_clear_event_callback");
    render_state = resolve<RustEngineRenderState>("rust_engine_render_state");
    surface_patches = resolve<RustEngineSurfacePatches>("rust_engine_surface_patches");
    engine = create();
    set_event_callback(engine, on_rust_event, this);
}

RustEngineBridge::~RustEngineBridge()
{
    if (engine && destroy) {
        if (clear_event_callback) {
            clear_event_callback(engine);
        }
        destroy(engine);
        engine = nullptr;
    }

    unload_library(library_handle);
}

std::string RustEngineBridge::tick_json(ControlInput input, float dt_seconds)
{
    set_control_input(engine, input);
    tick_engine(engine, dt_seconds);

    const EarthRenderState state = callback_count > 0 ? latest_state : render_state(engine);
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

void RustEngineBridge::on_rust_event(void* user_data, EngineEvent event)
{
    auto* bridge = static_cast<RustEngineBridge*>(user_data);
    bridge->latest_state = event.state;
    bridge->callback_count += 1;
}

template <typename T>
T RustEngineBridge::resolve(const char* name)
{
    void* symbol = resolve_symbol(library_handle, name);
    if (!symbol) {
        throw std::runtime_error(std::string("Missing Rust symbol: ") + name);
    }

    return reinterpret_cast<T>(symbol);
}

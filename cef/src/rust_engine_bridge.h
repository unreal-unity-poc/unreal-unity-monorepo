#pragma once

#include "rust_engine.h"

#include <string>

class RustEngineBridge final {
public:
    RustEngineBridge();
    ~RustEngineBridge();

    RustEngineBridge(const RustEngineBridge&) = delete;
    RustEngineBridge& operator=(const RustEngineBridge&) = delete;

    std::string tick_json(ControlInput input, float dt_seconds);

private:
    using RustEngineCreate = RustEngine* (*)();
    using RustEngineDestroy = void (*)(RustEngine*);
    using RustEngineSetControlInput = void (*)(RustEngine*, ControlInput);
    using RustEngineTick = void (*)(RustEngine*, float);
    using RustEngineSetEventCallback = void (*)(RustEngine*, RustEngineEventCallback, void*);
    using RustEngineClearEventCallback = void (*)(RustEngine*);
    using RustEngineRenderState = EarthRenderState (*)(const RustEngine*);
    using RustEngineSurfacePatches = SurfacePatchView (*)(const RustEngine*);

    template <typename T>
    T resolve(const char* name);

    static void on_rust_event(void* user_data, EngineEvent event);

    void* library_handle = nullptr;
    RustEngine* engine = nullptr;
    RustEngineCreate create = nullptr;
    RustEngineDestroy destroy = nullptr;
    RustEngineSetControlInput set_control_input = nullptr;
    RustEngineTick tick_engine = nullptr;
    RustEngineSetEventCallback set_event_callback = nullptr;
    RustEngineClearEventCallback clear_event_callback = nullptr;
    RustEngineRenderState render_state = nullptr;
    RustEngineSurfacePatches surface_patches = nullptr;
    EarthRenderState latest_state {};
    uint64_t callback_count = 0;
};

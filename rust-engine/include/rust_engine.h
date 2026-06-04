#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RustEngine RustEngine;

typedef struct ControlInput {
    float rotate_x;
    float rotate_y;
    float zoom;
    uint32_t reset;
} ControlInput;

typedef struct EarthRenderState {
    float radius;
    float atmosphere_radius;
    float rotation_x;
    float rotation_y;
    float cloud_rotation_y;
    float camera_distance;
    float light_x;
    float light_y;
    float light_z;
} EarthRenderState;

typedef struct SurfacePatch {
    float lat_degrees;
    float lon_degrees;
    float radius_degrees;
    float stretch_x;
    float stretch_y;
} SurfacePatch;

typedef struct SurfacePatchView {
    const SurfacePatch* ptr;
    size_t len;
} SurfacePatchView;

typedef struct EngineEvent {
    uint32_t kind;
    uint64_t frame_index;
    EarthRenderState state;
} EngineEvent;

typedef void (*RustEngineEventCallback)(void* user_data, EngineEvent event);

RustEngine* rust_engine_create(void);
void rust_engine_destroy(RustEngine* engine);
void rust_engine_set_control_input(RustEngine* engine, ControlInput input);
void rust_engine_tick(RustEngine* engine, float dt_seconds);
void rust_engine_set_event_callback(
    RustEngine* engine,
    RustEngineEventCallback callback,
    void* user_data);
void rust_engine_clear_event_callback(RustEngine* engine);
EarthRenderState rust_engine_render_state(const RustEngine* engine);
SurfacePatchView rust_engine_surface_patches(const RustEngine* engine);

#ifdef __cplusplus
}
#endif

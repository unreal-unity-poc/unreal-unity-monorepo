#include <node_api.h>

#include <cstdint>
#include <cstring>
#include <dlfcn.h>
#include <string>
#include <vector>

#include "rust_engine.h"

#ifndef NODE_GYP_MODULE_NAME
#define NODE_GYP_MODULE_NAME rust_engine_addon
#endif

namespace {

using RustEngineCreate = RustEngine* (*)();
using RustEngineDestroy = void (*)(RustEngine*);
using RustEngineSetControlInput = void (*)(RustEngine*, ControlInput);
using RustEngineTick = void (*)(RustEngine*, float);
using RustEngineSetEventCallback = void (*)(RustEngine*, RustEngineEventCallback, void*);
using RustEngineClearEventCallback = void (*)(RustEngine*);
using RustEngineRenderState = EarthRenderState (*)(const RustEngine*);
using RustEngineSurfacePatches = SurfacePatchView (*)(const RustEngine*);

void* library_handle = nullptr;
RustEngine* engine = nullptr;
RustEngineCreate create_engine = nullptr;
RustEngineDestroy destroy_engine = nullptr;
RustEngineSetControlInput set_control_input = nullptr;
RustEngineTick tick_engine = nullptr;
RustEngineSetEventCallback set_event_callback = nullptr;
RustEngineClearEventCallback clear_event_callback = nullptr;
RustEngineRenderState render_state = nullptr;
RustEngineSurfacePatches surface_patches = nullptr;
EarthRenderState latest_state {};
uint64_t latest_frame_index = 0;
bool has_latest_state = false;

void Throw(napi_env env, const char* message) {
  napi_throw_error(env, nullptr, message);
}

bool Check(napi_env env, napi_status status, const char* message) {
  if (status == napi_ok) {
    return true;
  }

  Throw(env, message);
  return false;
}

template <typename T>
bool LoadSymbol(napi_env env, T* target, const char* name) {
  *target = reinterpret_cast<T>(dlsym(library_handle, name));
  if (!*target) {
    std::string message = "Missing Rust engine symbol: ";
    message += name;
    Throw(env, message.c_str());
    return false;
  }

  return true;
}

void OnRustEngineEvent(void*, EngineEvent event) {
  if (event.kind != 1) {
    return;
  }

  latest_state = event.state;
  latest_frame_index = event.frame_index;
  has_latest_state = true;
}

void ShutdownEngine() {
  if (engine && destroy_engine) {
    if (clear_event_callback) {
      clear_event_callback(engine);
    }
    destroy_engine(engine);
  }

  engine = nullptr;
  has_latest_state = false;
  latest_frame_index = 0;

  if (library_handle) {
    dlclose(library_handle);
  }

  library_handle = nullptr;
  create_engine = nullptr;
  destroy_engine = nullptr;
  set_control_input = nullptr;
  tick_engine = nullptr;
  set_event_callback = nullptr;
  clear_event_callback = nullptr;
  render_state = nullptr;
  surface_patches = nullptr;
}

std::string ReadString(napi_env env, napi_value value) {
  size_t length = 0;
  if (!Check(env, napi_get_value_string_utf8(env, value, nullptr, 0, &length), "Expected string")) {
    return {};
  }

  std::vector<char> buffer(length + 1);
  if (!Check(env, napi_get_value_string_utf8(env, value, buffer.data(), buffer.size(), &length), "Failed to read string")) {
    return {};
  }

  return std::string(buffer.data(), length);
}

double ReadNumber(napi_env env, napi_value value, const char* message) {
  double number = 0.0;
  Check(env, napi_get_value_double(env, value, &number), message);
  return number;
}

napi_value Initialize(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1];
  if (!Check(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), "Failed to read arguments")) {
    return nullptr;
  }

  if (argc < 1) {
    Throw(env, "initialize(libraryPath) requires a Rust dynamic library path");
    return nullptr;
  }

  ShutdownEngine();
  const std::string library_path = ReadString(env, argv[0]);
  bool pending_exception = false;
  napi_is_exception_pending(env, &pending_exception);
  if (pending_exception) return nullptr;

  library_handle = dlopen(library_path.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (!library_handle) {
    std::string message = "Failed to load Rust engine library: ";
    message += dlerror();
    Throw(env, message.c_str());
    return nullptr;
  }

  if (!LoadSymbol(env, &create_engine, "rust_engine_create") ||
      !LoadSymbol(env, &destroy_engine, "rust_engine_destroy") ||
      !LoadSymbol(env, &set_control_input, "rust_engine_set_control_input") ||
      !LoadSymbol(env, &tick_engine, "rust_engine_tick") ||
      !LoadSymbol(env, &set_event_callback, "rust_engine_set_event_callback") ||
      !LoadSymbol(env, &clear_event_callback, "rust_engine_clear_event_callback") ||
      !LoadSymbol(env, &render_state, "rust_engine_render_state") ||
      !LoadSymbol(env, &surface_patches, "rust_engine_surface_patches")) {
    ShutdownEngine();
    return nullptr;
  }

  engine = create_engine();
  if (!engine) {
    ShutdownEngine();
    Throw(env, "rust_engine_create returned null");
    return nullptr;
  }

  set_event_callback(engine, &OnRustEngineEvent, nullptr);

  napi_value undefined;
  napi_get_undefined(env, &undefined);
  return undefined;
}

napi_value TickFrame(napi_env env, napi_callback_info info) {
  if (!engine || !set_control_input || !tick_engine || !render_state || !surface_patches) {
    Throw(env, "Rust engine is not initialized");
    return nullptr;
  }

  size_t argc = 5;
  napi_value argv[5];
  if (!Check(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr), "Failed to read arguments")) {
    return nullptr;
  }

  if (argc < 5) {
    Throw(env, "tickFrame(rotateX, rotateY, zoom, reset, dtSeconds) requires five arguments");
    return nullptr;
  }

  ControlInput input {};
  input.rotate_x = static_cast<float>(ReadNumber(env, argv[0], "rotateX must be a number"));
  input.rotate_y = static_cast<float>(ReadNumber(env, argv[1], "rotateY must be a number"));
  input.zoom = static_cast<float>(ReadNumber(env, argv[2], "zoom must be a number"));
  input.reset = static_cast<uint32_t>(ReadNumber(env, argv[3], "reset must be a number"));
  const float dt_seconds = static_cast<float>(ReadNumber(env, argv[4], "dtSeconds must be a number"));

  set_control_input(engine, input);
  tick_engine(engine, dt_seconds);

  EarthRenderState state = has_latest_state ? latest_state : render_state(engine);
  SurfacePatchView patches = surface_patches(engine);
  const size_t float_count = 9 + patches.len * 5;
  const size_t byte_length = 16 + float_count * sizeof(float);

  void* data = nullptr;
  napi_value array_buffer;
  if (!Check(env, napi_create_arraybuffer(env, byte_length, &data, &array_buffer), "Failed to create frame ArrayBuffer")) {
    return nullptr;
  }

  auto* header = reinterpret_cast<int32_t*>(data);
  auto* floats = reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(data) + 16);
  header[0] = static_cast<int32_t>(latest_frame_index);
  header[1] = static_cast<int32_t>(patches.len);
  header[2] = static_cast<int32_t>(float_count);
  header[3] = 0;

  floats[0] = state.radius;
  floats[1] = state.atmosphere_radius;
  floats[2] = state.rotation_x;
  floats[3] = state.rotation_y;
  floats[4] = state.cloud_rotation_y;
  floats[5] = state.camera_distance;
  floats[6] = state.light_x;
  floats[7] = state.light_y;
  floats[8] = state.light_z;

  for (size_t index = 0; index < patches.len; ++index) {
    const SurfacePatch& patch = patches.ptr[index];
    const size_t offset = 9 + index * 5;
    floats[offset + 0] = patch.lat_degrees;
    floats[offset + 1] = patch.lon_degrees;
    floats[offset + 2] = patch.radius_degrees;
    floats[offset + 3] = patch.stretch_x;
    floats[offset + 4] = patch.stretch_y;
  }

  return array_buffer;
}

napi_value Shutdown(napi_env env, napi_callback_info) {
  ShutdownEngine();
  napi_value undefined;
  napi_get_undefined(env, &undefined);
  return undefined;
}

void Cleanup(void*) {
  ShutdownEngine();
}

napi_value Init(napi_env env, napi_value exports) {
  napi_add_env_cleanup_hook(env, Cleanup, nullptr);

  napi_property_descriptor descriptors[] = {
    {"initialize", nullptr, Initialize, nullptr, nullptr, nullptr, napi_default, nullptr},
    {"tickFrame", nullptr, TickFrame, nullptr, nullptr, nullptr, napi_default, nullptr},
    {"shutdown", nullptr, Shutdown, nullptr, nullptr, nullptr, napi_default, nullptr},
  };
  napi_define_properties(env, exports, 3, descriptors);
  return exports;
}

} // namespace

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)

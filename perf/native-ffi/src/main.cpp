#include "rust_engine.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

using Clock = std::chrono::steady_clock;
using RustEngineCreate = RustEngine* (*)();
using RustEngineDestroy = void (*)(RustEngine*);
using RustEngineSetControlInput = void (*)(RustEngine*, ControlInput);
using RustEngineTick = void (*)(RustEngine*, float);
using RustEngineSetEventCallback = void (*)(RustEngine*, RustEngineEventCallback, void*);
using RustEngineClearEventCallback = void (*)(RustEngine*);
using RustEngineRenderState = EarthRenderState (*)(const RustEngine*);
using RustEngineSurfacePatches = SurfacePatchView (*)(const RustEngine*);

namespace
{
uint64_t now_ms()
{
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

void* load_library(const char* path)
{
#if defined(_WIN32)
    return reinterpret_cast<void*>(LoadLibraryA(path));
#else
    return dlopen(path, RTLD_NOW | RTLD_LOCAL);
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

template <typename T>
T resolve(void* handle, const char* name)
{
    void* symbol = resolve_symbol(handle, name);
    if (!symbol) {
        throw std::runtime_error(std::string("Missing Rust symbol: ") + name);
    }

    return reinterpret_cast<T>(symbol);
}

int frames_from_args(int argc, char** argv)
{
    for (int index = 1; index + 1 < argc; ++index) {
        if (std::string(argv[index]) == "--frames") {
            return std::max(1, std::stoi(argv[index + 1]));
        }
    }

    return 600;
}

ControlInput input_for_frame(int frame)
{
    ControlInput input {};
    input.rotate_x = frame % 180 < 90 ? 0.35f : -0.35f;
    input.rotate_y = frame % 240 < 120 ? 1.0f : -1.0f;
    input.zoom = frame % 300 < 150 ? 0.25f : -0.25f;
    return input;
}

struct CallbackStats {
    uint64_t count = 0;
    uint64_t last_frame_index = 0;
    EarthRenderState last_state {};
};

void rust_event_callback(void* user_data, EngineEvent event)
{
    auto* stats = static_cast<CallbackStats*>(user_data);
    stats->count += 1;
    stats->last_frame_index = event.frame_index;
    stats->last_state = event.state;
}

uint64_t percentile(std::vector<uint64_t>& sorted, double p)
{
    if (sorted.empty()) {
        return 0;
    }

    const auto index = static_cast<size_t>((sorted.size() - 1) * p + 0.5);
    return sorted[index];
}
}

int main(int argc, char** argv)
{
    const int frames = frames_from_args(argc, argv);
    void* library = load_library(RUST_ENGINE_LIBRARY_PATH);
    if (!library) {
        std::cerr << "Failed to load " << RUST_ENGINE_LIBRARY_PATH << '\n';
        return 1;
    }

    auto create = resolve<RustEngineCreate>(library, "rust_engine_create");
    auto destroy = resolve<RustEngineDestroy>(library, "rust_engine_destroy");
    auto set_control_input = resolve<RustEngineSetControlInput>(library, "rust_engine_set_control_input");
    auto tick = resolve<RustEngineTick>(library, "rust_engine_tick");
    auto set_event_callback = resolve<RustEngineSetEventCallback>(library, "rust_engine_set_event_callback");
    auto clear_event_callback = resolve<RustEngineClearEventCallback>(library, "rust_engine_clear_event_callback");
    auto render_state = resolve<RustEngineRenderState>(library, "rust_engine_render_state");
    auto surface_patches = resolve<RustEngineSurfacePatches>(library, "rust_engine_surface_patches");

    RustEngine* engine = create();
    CallbackStats callback_stats {};
    set_event_callback(engine, rust_event_callback, &callback_stats);
    std::vector<uint64_t> samples;
    samples.reserve(frames);

    std::cout << "{\"ts_unix_ms\":" << now_ms() << ",\"target\":\"native-ffi\",\"phase\":\"start\",\"frames\":" << frames << "}\n";

    for (int frame = 0; frame < frames; ++frame) {
        const auto started = Clock::now();
        set_control_input(engine, input_for_frame(frame));
        tick(engine, 1.0f / 60.0f);
        const EarthRenderState state = callback_stats.count > 0
            ? callback_stats.last_state
            : render_state(engine);
        const SurfacePatchView patches = surface_patches(engine);
        const auto elapsed = Clock::now() - started;
        const uint64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
        samples.push_back(ns);

        std::cout << "{\"ts_unix_ms\":" << now_ms()
                  << ",\"target\":\"native-ffi\",\"phase\":\"frame\",\"frame\":" << frame
                  << ",\"ffi_roundtrip_ns\":" << ns
                  << ",\"camera_distance\":" << state.camera_distance
                  << ",\"patch_count\":" << patches.len
                  << ",\"callback_count\":" << callback_stats.count
                  << ",\"callback_frame_index\":" << callback_stats.last_frame_index
                  << "}\n";
    }

    clear_event_callback(engine);
    destroy(engine);
    std::sort(samples.begin(), samples.end());
    uint64_t total = 0;
    for (const uint64_t sample : samples) {
        total += sample;
    }

    std::cout << "{\"ts_unix_ms\":" << now_ms()
              << ",\"target\":\"native-ffi\",\"phase\":\"summary\",\"frames\":" << frames
              << ",\"avg_ns\":" << (samples.empty() ? 0.0 : static_cast<double>(total) / samples.size())
              << ",\"p50_ns\":" << percentile(samples, 0.50)
              << ",\"p95_ns\":" << percentile(samples, 0.95)
              << ",\"max_ns\":" << (samples.empty() ? 0 : samples.back())
              << ",\"callback_count\":" << callback_stats.count
              << "}\n";

    return 0;
}

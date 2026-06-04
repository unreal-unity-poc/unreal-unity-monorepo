#include "rust_engine.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QLibrary>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

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

int frames_from_args(const QStringList& args)
{
    const int index = args.indexOf(QStringLiteral("--frames"));
    if (index >= 0 && index + 1 < args.size()) {
        return std::max(1, args[index + 1].toInt());
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
    if (!user_data || event.kind != 1) {
        return;
    }

    auto* stats = static_cast<CallbackStats*>(user_data);
    stats->count += 1;
    stats->last_frame_index = event.frame_index;
    stats->last_state = event.state;
}

template <typename T>
T resolve(QLibrary& library, const char* symbol)
{
    T fn = reinterpret_cast<T>(library.resolve(symbol));
    if (!fn) {
        throw std::runtime_error(QString("Missing Rust symbol: %1").arg(symbol).toStdString());
    }

    return fn;
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
    QCoreApplication app(argc, argv);
    const int frames = frames_from_args(app.arguments());

    QLibrary library(QStringLiteral(RUST_ENGINE_LIBRARY_PATH));
    if (!library.load()) {
        std::cerr << library.errorString().toStdString() << '\n';
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

    std::cout << "{\"ts_unix_ms\":" << now_ms() << ",\"target\":\"qt-ffi\",\"phase\":\"start\",\"frames\":" << frames << "}\n";

    QElapsedTimer timer;
    for (int frame = 0; frame < frames; ++frame) {
        timer.restart();
        set_control_input(engine, input_for_frame(frame));
        tick(engine, 1.0f / 60.0f);
        const EarthRenderState state = callback_stats.count > 0
            ? callback_stats.last_state
            : render_state(engine);
        const SurfacePatchView patches = surface_patches(engine);
        const uint64_t ns = timer.nsecsElapsed();
        samples.push_back(ns);

        std::cout << "{\"ts_unix_ms\":" << now_ms()
                  << ",\"target\":\"qt-ffi\",\"phase\":\"frame\",\"frame\":" << frame
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
              << ",\"target\":\"qt-ffi\",\"phase\":\"summary\",\"frames\":" << frames
              << ",\"avg_ns\":" << (samples.empty() ? 0.0 : static_cast<double>(total) / samples.size())
              << ",\"p50_ns\":" << percentile(samples, 0.50)
              << ",\"p95_ns\":" << percentile(samples, 0.95)
              << ",\"max_ns\":" << (samples.empty() ? 0 : samples.back())
              << ",\"callback_count\":" << callback_stats.count
              << ",\"callback_frame_index\":" << callback_stats.last_frame_index
              << "}\n";

    return 0;
}

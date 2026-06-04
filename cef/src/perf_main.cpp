#include "rust_engine_bridge.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

using Clock = std::chrono::steady_clock;

namespace
{
uint64_t now_ms()
{
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
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
    RustEngineBridge bridge;
    std::vector<uint64_t> samples;
    samples.reserve(frames);
    size_t bytes_total = 0;

    std::cout << "{\"ts_unix_ms\":" << now_ms() << ",\"target\":\"cef-bridge-json\",\"phase\":\"start\",\"frames\":" << frames << "}\n";

    for (int frame = 0; frame < frames; ++frame) {
        const auto started = Clock::now();
        const std::string payload = bridge.tick_json(input_for_frame(frame), 1.0f / 60.0f);
        const auto elapsed = Clock::now() - started;
        const uint64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
        samples.push_back(ns);
        bytes_total += payload.size();

        std::cout << "{\"ts_unix_ms\":" << now_ms()
                  << ",\"target\":\"cef-bridge-json\",\"phase\":\"frame\",\"frame\":" << frame
                  << ",\"bridge_json_ns\":" << ns
                  << ",\"payload_bytes\":" << payload.size()
                  << "}\n";
    }

    std::sort(samples.begin(), samples.end());
    uint64_t total = 0;
    for (const uint64_t sample : samples) {
        total += sample;
    }

    std::cout << "{\"ts_unix_ms\":" << now_ms()
              << ",\"target\":\"cef-bridge-json\",\"phase\":\"summary\",\"frames\":" << frames
              << ",\"avg_ns\":" << (samples.empty() ? 0.0 : static_cast<double>(total) / samples.size())
              << ",\"p50_ns\":" << percentile(samples, 0.50)
              << ",\"p95_ns\":" << percentile(samples, 0.95)
              << ",\"max_ns\":" << (samples.empty() ? 0 : samples.back())
              << ",\"avg_payload_bytes\":" << (frames == 0 ? 0.0 : static_cast<double>(bytes_total) / frames)
              << "}\n";

    return 0;
}


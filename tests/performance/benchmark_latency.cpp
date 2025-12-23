/**
 * @file benchmark_latency.cpp
 * @brief Performance benchmark for audio pass-through latency
 *
 * This benchmark measures the actual latency of the audio pass-through pipeline
 * and validates that it meets the <200ms requirement.
 *
 * Benchmark Metrics:
 * - Average latency across multiple iterations
 * - Min/Max latency to detect jitter
 * - Percentiles (p50, p95, p99) for consistency analysis
 *
 * Success Criteria:
 * - Average latency < 200ms
 * - p95 latency < 200ms
 * - p99 latency < 250ms (allowing some tolerance)
 */

#include <benchmark/benchmark.h>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <thread>
#include <chrono>

#include "core/Factory.h"
#include "utils/Logger.h"

using namespace audiobridge;

// Global test setup
static std::unique_ptr<IAudioEngine> g_engine;

// Helper: Select test devices
static bool SelectTestDevices(IAudioEngine* engine) {
    auto& enumerator = engine->GetDeviceEnumerator();

    // Get default devices
    int defaultInput = enumerator.GetDefaultInputDevice();
    int defaultOutput = enumerator.GetDefaultOutputDevice();

    if (defaultInput < 0 || defaultOutput < 0) {
        return false;
    }

    // If different, use them
    if (defaultInput != defaultOutput) {
        return engine->SelectInputDevice(defaultInput) &&
               engine->SelectOutputDevice(defaultOutput);
    }

    // Otherwise, find different devices
    auto inputDevices = enumerator.GetInputDevices();
    auto outputDevices = enumerator.GetOutputDevices();

    for (const auto& in : inputDevices) {
        for (const auto& out : outputDevices) {
            if (in.deviceId != out.deviceId) {
                if (engine->SelectInputDevice(in.deviceId) &&
                    engine->SelectOutputDevice(out.deviceId)) {
                    return true;
                }
            }
        }
    }

    return false;
}

// Benchmark: Measure pass-through latency
static void BM_PassThroughLatency(benchmark::State& state) {
    // Initialize engine once
    if (!g_engine) {
        Logger::GetInstance().Initialize();
        g_engine = CreateAudioEngine();

        if (!SelectTestDevices(g_engine.get())) {
            state.SkipWithError("Could not select test devices");
            return;
        }

        g_engine->SetPassThroughEnabled(true);

        if (!g_engine->Start()) {
            state.SkipWithError("Could not start engine");
            return;
        }

        // Warm up
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // Measure latency
    std::vector<double> latencies;
    latencies.reserve(1000);

    for (auto _ : state) {
        double latency = g_engine->GetCurrentLatency();
        latencies.push_back(latency);

        // Small delay between measurements
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Calculate statistics
    state.SetLabel("ms");

    // Store statistics for custom output
    size_t n = latencies.size();
    std::sort(latencies.begin(), latencies.end());

    double sum = std::accumulate(latencies.begin(), latencies.end(), 0.0);
    double mean = sum / n;

    double min = latencies.front();
    double max = latencies.back();
    double p50 = latencies[n * 50 / 100];
    double p95 = latencies[n * 95 / 100];
    double p99 = latencies[n * 99 / 100];

    // Calculate standard deviation
    double sq_sum = 0.0;
    for (double latency : latencies) {
        sq_sum += (latency - mean) * (latency - mean);
    }
    double stddev = std::sqrt(sq_sum / n);

    // Set custom counters
    state.counters["mean_ms"] = benchmark::Counter(mean, benchmark::Counter::kAvgIterations);
    state.counters["min_ms"] = benchmark::Counter(min, benchmark::Counter::kAvgIterations);
    state.counters["max_ms"] = benchmark::Counter(max, benchmark::Counter::kAvgIterations);
    state.counters["p50_ms"] = benchmark::Counter(p50, benchmark::Counter::kAvgIterations);
    state.counters["p95_ms"] = benchmark::Counter(p95, benchmark::Counter::kAvgIterations);
    state.counters["p99_ms"] = benchmark::Counter(p99, benchmark::Counter::kAvgIterations);
    state.counters["stddev_ms"] = benchmark::Counter(stddev, benchmark::Counter::kAvgIterations);

    // Print detailed statistics
    if (state.thread_index() == 0) {
        printf("\n=== Latency Benchmark Results ===\n");
        printf("Samples: %zu\n", n);
        printf("Mean:   %.2f ms\n", mean);
        printf("Min:    %.2f ms\n", min);
        printf("Max:    %.2f ms\n", max);
        printf("StdDev: %.2f ms\n", stddev);
        printf("P50:    %.2f ms\n", p50);
        printf("P95:    %.2f ms\n", p95);
        printf("P99:    %.2f ms\n", p99);
        printf("===============================\n");

        // Validate requirements
        if (mean >= 200.0) {
            printf("FAIL: Mean latency %.2f ms >= 200 ms threshold\n", mean);
        } else {
            printf("PASS: Mean latency %.2f ms < 200 ms threshold\n", mean);
        }

        if (p95 >= 200.0) {
            printf("WARNING: P95 latency %.2f ms >= 200 ms threshold\n", p95);
        } else {
            printf("PASS: P95 latency %.2f ms < 200 ms threshold\n", p95);
        }

        if (p99 >= 250.0) {
            printf("WARNING: P99 latency %.2f ms >= 250 ms threshold\n", p99);
        } else {
            printf("PASS: P99 latency %.2f ms < 250 ms threshold\n", p99);
        }
    }
}

// Benchmark: Sustained latency over time
static void BM_SustainedLatency(benchmark::State& state) {
    if (!g_engine) {
        state.SkipWithError("Engine not initialized");
        return;
    }

    // Measure latency over time
    auto startTime = std::chrono::steady_clock::now();
    size_t iteration = 0;

    for (auto _ : state) {
        double latency = g_engine->GetCurrentLatency();

        // Print latency every second
        if (iteration % 100 == 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - startTime).count();
            printf("[%ld s] Latency: %.2f ms\n", elapsed, latency);
        }

        iteration++;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// Benchmark: Latency under load
static void BM_LatencyUnderLoad(benchmark::State& state) {
    if (!g_engine) {
        state.SkipWithError("Engine not initialized");
        return;
    }

    // Simulate load by measuring latency more frequently
    for (auto _ : state) {
        double latency = g_engine->GetCurrentLatency();
        benchmark::DoNotOptimize(latency);

        // Minimal delay (higher frequency)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// Register benchmarks
BENCHMARK(BM_PassThroughLatency)
    ->Iterations(1000)
    ->Unit(benchmark::kMillisecond)
    ->Threads(1);

BENCHMARK(BM_SustainedLatency)
    ->Iterations(5000)
    ->Unit(benchmark::kMillisecond)
    ->Threads(1);

BENCHMARK(BM_LatencyUnderLoad)
    ->Iterations(10000)
    ->Unit(benchmark::kMillisecond)
    ->Threads(1);

// Cleanup
static void Cleanup() {
    if (g_engine) {
        g_engine->Stop();
        g_engine.reset();
        Logger::GetInstance().Flush();
    }
}

// Custom main to handle cleanup
int main(int argc, char** argv) {
    benchmark::Initialize(&argc, argv);

    if (benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }

    printf("\n");
    printf("==============================================\n");
    printf("  AudioBridge Pass-through Latency Benchmark\n");
    printf("==============================================\n");
    printf("This benchmark measures audio pass-through latency\n");
    printf("and validates the <200ms requirement.\n");
    printf("\n");

    // Run benchmarks
    benchmark::RunSpecifiedBenchmarks();

    // Cleanup
    Cleanup();

    printf("\n");
    printf("Benchmark completed.\n");
    printf("==============================================\n\n");

    return 0;
}

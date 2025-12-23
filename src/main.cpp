/**
 * @file main.cpp
 * @brief AudioBridge Demo Application
 *
 * This demo application showcases the complete audio I/O functionality:
 * - Device enumeration (list all available audio devices)
 * - Device selection (choose input and output devices)
 * - Audio pass-through (route input audio to output)
 * - Real-time level monitoring
 *
 * Usage:
 *   ./audioBridge                          # Use default devices
 *   ./audioBridge --list                   # List all devices
 *   ./audioBridge --input 0 --output 1     # Specify devices
 *   ./audioBridge --help                   # Show help
 */

#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <atomic>
#include <csignal>

#include "core/Factory.h"
#include "utils/Logger.h"

using namespace audiobridge;

// Global flag for graceful shutdown
static std::atomic<bool> g_running(true);

// Signal handler for Ctrl+C
void SignalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n\nReceived shutdown signal. Stopping..." << std::endl;
        g_running.store(false);
    }
}

// Print device information
void PrintDeviceInfo(const AudioDeviceInfo& info, int index) {
    std::cout << "  [" << index << "] " << info.name << "\n";
    std::cout << "      API: " << info.hostApi << "\n";
    std::cout << "      Channels: " << info.maxInputChannels << " in, "
              << info.maxOutputChannels << " out\n";
    std::cout << "      Sample Rate: " << info.defaultSampleRate << " Hz\n";

    if (info.isDefaultInput) {
        std::cout << "      [Default Input]\n";
    }
    if (info.isDefaultOutput) {
        std::cout << "      [Default Output]\n";
    }
}

// List all available devices
void ListDevices(IAudioEngine& engine) {
    auto& enumerator = engine.GetDeviceEnumerator();

    std::cout << "\n=== Available Audio Devices ===\n" << std::endl;

    // List all devices
    auto allDevices = enumerator.GetAllDevices();
    std::cout << "\nAll Devices (" << allDevices.size() << "):\n" << std::endl;
    for (size_t i = 0; i < allDevices.size(); ++i) {
        PrintDeviceInfo(allDevices[i], static_cast<int>(i));
    }

    // List input devices
    auto inputDevices = enumerator.GetInputDevices();
    std::cout << "\nInput Devices (" << inputDevices.size() << "):\n" << std::endl;
    for (size_t i = 0; i < inputDevices.size(); ++i) {
        std::cout << "  [" << inputDevices[i].deviceId << "] "
                  << inputDevices[i].name << "\n";
    }

    // List output devices
    auto outputDevices = enumerator.GetOutputDevices();
    std::cout << "\nOutput Devices (" << outputDevices.size() << "):\n" << std::endl;
    for (size_t i = 0; i < outputDevices.size(); ++i) {
        std::cout << "  [" << outputDevices[i].deviceId << "] "
                  << outputDevices[i].name << "\n";
    }

    std::cout << "\n===============================\n" << std::endl;
}

// Print level bars
void PrintLevelBar(float level, int width) {
    int filled = static_cast<int>(level * width);
    filled = std::max(0, std::min(filled, width));

    std::cout << "[";
    for (int i = 0; i < width; ++i) {
        if (i < filled) {
            std::cout << "=";
        } else {
            std::cout << " ";
        }
    }
    std::cout << "] " << std::fixed << std::setprecision(3) << level;
}

// Run audio pass-through with level monitoring
void RunPassThrough(IAudioEngine& engine, int inputDevice, int outputDevice) {
    // Select devices
    std::cout << "\nSelecting devices..." << std::endl;

    if (!engine.SelectInputDevice(inputDevice)) {
        std::cerr << "Error: Failed to select input device " << inputDevice << std::endl;
        return;
    }

    if (!engine.SelectOutputDevice(outputDevice)) {
        std::cerr << "Error: Failed to select output device " << outputDevice << std::endl;
        return;
    }

    std::cout << "Input device: " << inputDevice << std::endl;
    std::cout << "Output device: " << outputDevice << std::endl;

    // Set up level monitoring
    engine.SetLevelCallback([](const AudioLevels& levels) {
        std::cout << "\r";  // Move cursor to start of line

        // Print input levels
        std::cout << "IN  L: ";
        PrintLevelBar(levels.inputPeakL, 20);
        std::cout << "  R: ";
        PrintLevelBar(levels.inputPeakR, 20);

        std::cout << " | OUT L: ";
        PrintLevelBar(levels.outputPeakL, 20);
        std::cout << "  R: ";
        PrintLevelBar(levels.outputPeakR, 20);

        std::cout << "    " << std::flush;
    });

    // Enable pass-through
    engine.SetPassThroughEnabled(true);
    std::cout << "\n\nPass-through enabled" << std::endl;

    // Start engine
    if (!engine.Start()) {
        std::cerr << "Error: Failed to start audio engine" << std::endl;
        return;
    }

    std::cout << "\nAudio engine started" << std::endl;
    std::cout << "\nLevels (real-time):\n" << std::endl;
    std::cout << "Press Ctrl+C to stop...\n" << std::endl;

    // Run until interrupted
    auto lastUpdate = std::chrono::steady_clock::now();
    int iterations = 0;

    while (g_running.load()) {
        // Update latency display every second
        auto now = std::chrono::steady_clock::now();
        if ((now - lastUpdate) >= std::chrono::seconds(1)) {
            double latency = engine.GetCurrentLatency();
            std::cout << "\nLatency: " << std::fixed << std::setprecision(2)
                      << latency << " ms    " << std::endl;
            lastUpdate = now;
            iterations++;

            // Print status every 10 seconds
            if (iterations % 10 == 0) {
                std::cout << "Running... (" << iterations << "s)" << std::endl;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Stop engine
    std::cout << "\n\nStopping audio engine..." << std::endl;
    engine.Stop();
    std::cout << "Stopped" << std::endl;
}

// Print usage
void PrintUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [OPTIONS]\n" << std::endl;
    std::cout << "AudioBridge Demo - Audio Pass-through Application\n" << std::endl;
    std::cout << "Options:\n";
    std::cout << "  --list              List all available audio devices\n";
    std::cout << "  --input <id>        Select input device by ID\n";
    std::cout << "  --output <id>       Select output device by ID\n";
    std::cout << "  --help              Show this help message\n" << std::endl;
    std::cout << "Examples:\n";
    std::cout << "  " << programName << " --list\n";
    std::cout << "  " << programName << " --input 0 --output 1\n";
    std::cout << "  " << programName << "                    # Use default devices\n" << std::endl;
}

int main(int argc, char* argv[]) {
    // Initialize logger
    Logger::GetInstance().Initialize();

    std::cout << "\n====================================\n";
    std::cout << "  AudioBridge Demo Application\n";
    std::cout << "====================================\n" << std::endl;

    // Set up signal handler
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    // Parse command-line arguments
    bool listOnly = false;
    bool useDefaults = true;
    int inputDevice = -1;
    int outputDevice = -1;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            PrintUsage(argv[0]);
            return 0;
        }
        else if (arg == "--list" || arg == "-l") {
            listOnly = true;
        }
        else if (arg == "--input" || arg == "-i") {
            if (i + 1 < argc) {
                inputDevice = std::atoi(argv[++i]);
                useDefaults = false;
            } else {
                std::cerr << "Error: --input requires a device ID" << std::endl;
                return 1;
            }
        }
        else if (arg == "--output" || arg == "-o") {
            if (i + 1 < argc) {
                outputDevice = std::atoi(argv[++i]);
                useDefaults = false;
            } else {
                std::cerr << "Error: --output requires a device ID" << std::endl;
                return 1;
            }
        }
        else {
            std::cerr << "Error: Unknown option: " << arg << std::endl;
            PrintUsage(argv[0]);
            return 1;
        }
    }

    // Create audio engine
    auto engine = CreateAudioEngine();
    if (!engine) {
        std::cerr << "Error: Failed to create audio engine" << std::endl;
        return 1;
    }

    // List devices if requested
    if (listOnly) {
        ListDevices(*engine);
        return 0;
    }

    // Get default devices if using defaults
    if (useDefaults) {
        auto& enumerator = engine->GetDeviceEnumerator();
        inputDevice = enumerator.GetDefaultInputDevice();
        outputDevice = enumerator.GetDefaultOutputDevice();

        if (inputDevice < 0 || outputDevice < 0) {
            std::cerr << "Error: No default devices available" << std::endl;
            std::cerr << "Use --list to see available devices" << std::endl;
            return 1;
        }

        if (inputDevice == outputDevice) {
            std::cerr << "Error: Default input and output are the same device" << std::endl;
            std::cerr << "Please specify different devices using --input and --output" << std::endl;
            return 1;
        }

        std::cout << "Using default devices:" << std::endl;
        std::cout << "  Input:  " << inputDevice << std::endl;
        std::cout << "  Output: " << outputDevice << std::endl;
    }

    // Run pass-through
    RunPassThrough(*engine, inputDevice, outputDevice);

    // Cleanup
    std::cout << "\nExiting..." << std::endl;
    Logger::GetInstance().Flush();

    return 0;
}

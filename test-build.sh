#!/bin/bash
# Quick test build script for audioBridge-test

echo "================================"
echo "audioBridge Quick Test Build"
echo "================================"
echo ""

# Create a simple test directory
mkdir -p /tmp/audiobridge-test
cd /tmp/audiobridge-test

# Create a minimal test that doesn't require all dependencies
cat > audioBridge-test.cpp << 'EOF'
#include <iostream>
#include <csignal>
#include <atomic>

namespace {
    std::atomic<bool> g_interrupted{false};

    void signalHandler(int signal) {
        if (signal == SIGINT || signal == SIGTERM) {
            if (!g_interrupted.exchange(true)) {
                std::cout << "\n⚠ Interrupt signal received\n";
                std::cout << "Cleaning up...\n";
                std::cout << "✗ Test interrupted by user\n";
            }
            exit(130);
        }
    }
}

int main() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "================================\n";
    std::cout << "audioBridge-test (Minimal Build)\n";
    std::cout << "================================\n\n";

    if (argc < 2) {
        std::cout << "Usage: audioBridge-test <command>\n\n";
        std::cout << "Commands:\n";
        std::cout << "  list-devices      List all available audio devices\n";
        std::cout << "  setup-check       Verify system setup and dependencies\n";
        std::cout << "  validate          Validate audio file with metrics\n";
        std::cout << "  help              Show this help message\n\n";
        std::cout << "Note: This is a minimal test build.\n";
        std::cout << "      Full functionality requires PortAudio, ALSA, and libsndfile.\n";
        return 0;
    }

    std::string command = argv[1];

    if (command == "list-devices") {
        std::cout << "Device enumeration requires full build with PortAudio.\n";
        std::cout << "Please build the complete version with all dependencies.\n";
    } else if (command == "setup-check") {
        std::cout << "System Setup Check:\n";
        std::cout << "==================\n\n";

        // Check for snd-aloop
        if (system("lsmod | grep -q snd_aloop") == 0) {
            std::cout << "✓ snd-aloop module loaded\n";
        } else {
            std::cout << "✗ snd-aloop module NOT loaded\n";
            std::cout << "  Run: sudo modprobe snd-aloop\n";
        }

        // Check for PortAudio
        std::cout << "\n⚠ Full build requires:\n";
        std::cout << "  - portaudio19-dev\n";
        std::cout << "  - libasound2-dev\n";
        std::cout << "  - libsndfile1-dev\n";
    } else if (command == "validate") {
        if (argc < 3) {
            std::cerr << "Error: Audio file path required\n";
            return 1;
        }
        std::cout << "Audio validation requires full build with Gist library.\n";
        std::cout << "File: " << argv[2] << "\n";
    } else if (command == "help") {
        // Show help (already done above)
    } else {
        std::cerr << "Error: Unknown command '" << command << "'\n";
        return 1;
    }

    return 0;
}
EOF

echo "Creating minimal test executable..."
g++ -o audioBridge-test audioBridge-test.cpp -std=c++17

if [ $? -eq 0 ]; then
    echo "✓ Build successful!"
    echo ""
    echo "Test the executable:"
    echo "  ./audioBridge-test help"
    echo "  ./audioBridge-test setup-check"
else
    echo "✗ Build failed"
fi

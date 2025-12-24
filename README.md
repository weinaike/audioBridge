# audioBridge

Linux Virtual Audio Testing Infrastructure

## Overview

audioBridge is a C++ audio engine library with comprehensive virtual audio testing infrastructure for Linux. It enables automated audio testing without physical hardware using ALSA snd-aloop kernel module.

## Features

- **Virtual Audio Loopback Testing**: Test audio I/O without physical devices
- **Automated Test Execution**: Run single tests or test suites with one command
- **Audio Validation**: FFT-based frequency analysis, SNR/THD measurements, latency tracking
- **Cross-Platform Core**: Audio engine works on Linux, Windows, and macOS
- **Linux-Specific Tests**: Automated testing with PortAudio and ALSA integration
- **CI/CD Ready**: JSON output format for automated testing pipelines

## Quick Start

### Prerequisites

- Linux system (Ubuntu 20.04+, Fedora 33+, or Arch Linux)
- C++17 compiler (GCC 8+ or Clang 10+)
- CMake 3.12+
- PortAudio development headers
- ALSA development headers
- spdlog logging library

### Installation (30 minutes)

```bash
# 1. Install dependencies
sudo apt-get install portaudio19-dev libasound2-dev libspdlog-dev cmake build-essential

# 2. Load virtual audio loopback module
sudo modprobe snd-aloop

# 3. Build the project
mkdir build && cd build
cmake ..
make -j$(nproc)

# 4. Run tests
./tests/tools/audioBridge-test list-devices
./tests/tools/audioBridge-test setup-check
```

## Documentation

### Linux Virtual Audio Testing

Comprehensive documentation for Linux virtual audio testing is available in `specs/003-linux-virtual-audio-testing/`:

- **[Quick Start Guide](specs/003-linux-virtual-audio-testing/quickstart.md)** - 30-minute setup guide
- **[Ubuntu Setup](docs/linux-audio-testing/setup-ubuntu.md)** - Ubuntu-specific instructions
- **[Fedora Setup](docs/linux-audio-testing/setup-fedora.md)** - Fedora-specific instructions
- **[Arch Setup](docs/linux-audio-testing/setup-arch.md)** - Arch Linux-specific instructions
- **[Troubleshooting](docs/linux-audio-testing/troubleshooting.md)** - Common issues and solutions

## Usage

### List Available Audio Devices

```bash
./tests/tools/audioBridge-test list-devices
./tests/tools/audioBridge-test list-devices --type loopback
./tests/tools/audioBridge-test list-devices --json
```

### Run Audio Validation

```bash
# Basic validation
./tests/tools/audioBridge-test validate test-audio/1khz-sine.wav

# With custom thresholds
./tests/tools/audioBridge-test validate test-audio/1khz-sine.wav \
    --expected-frequency 1000 \
    --frequency-tolerance 5 \
    --min-snr 40

# With reference file comparison
./tests/tools/audioBridge-test validate test-audio/captured.wav \
    --reference-file test-audio/reference.wav

# Load thresholds from config file
./tests/tools/audioBridge-test validate test-audio/1khz-sine.wav \
    --config test-data/configs/default-loopback.json
```

### Run Loopback Tests

```bash
# Run single test
./tests/tools/audioBridge-test run test-audio/1khz-sine.wav

# Run test suite
./tests/tools/audioBridge-test run-suite default

# With custom devices
./tests/tools/audioBridge-test run test-audio/1khz-sine.wav \
    --devices hw:1,0,hw:1,1
```

## Test Audio Files

Test audio files are provided in `test-data/audio/`:

- `1khz-sine.wav` - 1kHz sine wave (5 seconds, 48kHz, mono)
- `440hz-tone.wav` - 440Hz musical A tone (5 seconds, 48kHz, mono)
- `white-noise.wav` - White noise (5 seconds, 48kHz, stereo)
- `frequency-sweep.wav` - 20Hz-20kHz frequency sweep (5 seconds, 48kHz, mono)

Generate custom test audio:

```bash
./scripts/utils/generate-test-audio.sh
```

## Architecture

```
audioBridge/
├── src/                    # Audio engine core
│   ├── core/              # Audio pipeline and processing
│   ├── adapters/          # PortAudio I/O adapters
│   └── processing/        # DSP and AI processing
├── tests/                  # Test infrastructure
│   ├── tools/             # Test tools (audioBridge-test)
│   ├── unit/              # Unit tests
│   ├── integration/       # Integration tests
│   └── performance/       # Performance tests
├── docs/                   # Documentation
├── scripts/                # Utility scripts
└── test-data/             # Test audio and configs
```

## Technical Stack

- **Language**: C++17
- **Build System**: CMake
- **Audio I/O**: PortAudio
- **Linux Audio**: ALSA (snd-aloop module)
- **Audio Analysis**: Gist library (FFT, frequency analysis)
- **Logging**: spdlog
- **Testing**: Custom test framework

## Validation Metrics

The test framework provides comprehensive audio validation:

- **Frequency Analysis**: FFT-based spectrum with peak detection
- **Signal Quality**: SNR (dB), THD (%), noise floor
- **Latency Measurement**: Round-trip latency with statistics
- **Reference Comparison**: Pearson correlation coefficient
- **Configurable Thresholds**: Via CLI or JSON config files

## CI/CD Integration

JSON output format for automated testing:

```bash
./tests/tools/audioBridge-test validate test-audio/1khz-sine.wav --json > results.json
```

Parse results in CI/CD pipelines to enforce quality thresholds.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

[Specify your license here]

## See Also

- [Spec: Linux Virtual Audio Testing](specs/003-linux-virtual-audio-testing/spec.md)
- [Implementation Plan](specs/003-linux-virtual-audio-testing/plan.md)
- [API Documentation](specs/003-linux-virtual-audio-testing/contracts/test-api.md)

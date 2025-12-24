# Implementation Plan: Linux Virtual Audio Testing

**Branch**: `003-linux-virtual-audio-testing` | **Date**: 2025-12-24 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/003-linux-virtual-audio-testing/spec.md`

## Summary

This feature implements Linux virtual audio loopback testing infrastructure for audioBridge, enabling automated audio testing without physical hardware. The technical approach uses ALSA snd-aloop kernel module for virtual audio devices, PortAudio for cross-platform audio I/O, Gist library for audio validation, and shell scripts for test orchestration. The implementation prioritizes developer productivity with quick setup (under 30 minutes), comprehensive documentation for three major Linux distributions, and seamless CI/CD integration.

## Technical Context

**Language/Version**: C++17
**Primary Dependencies**: PortAudio, spdlog, CMake, ALSA, Gist (audio analysis), SoX (audio generation)
**Storage**: File-based (JSON for metadata, WAV/FLAC for audio)
**Testing**: Custom audio test framework with unit, integration, and performance tests
**Target Platform**: Linux (Ubuntu, Fedora, Arch) - cross-platform core, Linux-specific test infrastructure
**Project Type**: single (C++ audio engine library + test tools)
**Performance Goals**: <50ms round-trip latency for loopback testing, real-time safe audio processing
**Constraints**: Real-time safety (no blocking I/O in audio threads), lock-free data structures only, pre-allocated memory, Linux-specific code isolated in test layer
**Scale/Scope**: Test infrastructure with virtual audio device support, automated test execution, and audio validation

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

### Real-Time Audio Safety ✅
- [x] No dynamic memory allocation in audio callback paths - Validation runs in separate thread
- [x] No blocking I/O operations in real-time threads - Test infrastructure isolated from audio threads
- [x] All audio thread operations are lock-free and predictable - Uses existing audioBridge architecture

### Adapter Abstraction Layer ✅
- [x] All audio I/O uses IAudioInput/IAudioOutput interfaces - Tests use existing PortAudio adapters
- [x] No direct dependencies on PortAudio, VB-Cable, or specific drivers in business logic - Linux-specific code in test layer only
- [x] Clear separation between adapter layer and core engine - Test infrastructure respects architecture boundaries

### Cross-Platform Compatibility ✅
- [x] Core audio engine compiles on Windows, macOS, Linux without platform-specific code - Linux code isolated in `tests/`
- [x] Platform-specific implementations isolated in adapter layer only - Test infrastructure in `tests/integration/test_linux_virtual_audio.cpp`
- [x] Use of CMake for cross-platform build system - Linux tests built conditionally

### Low Latency Requirements ✅
- [x] Design demonstrates <200ms end-to-end latency capability - Loopback latency measured <50ms
- [x] Buffer sizes and processing paths optimized for minimal delay - Uses 128-frame buffers at 48kHz
- [x] Real-time monitoring of latency metrics included - Latency measurement in every test

### Test-First Audio Development ✅
- [x] Unit test strategy for all audio components - Tests for device enumeration, audio capture
- [x] Integration tests for complete audio pipeline - End-to-end loopback tests
- [x] Performance tests for real-time constraints and latency verification - Latency consistency tests

**Constitution Compliance**: ✅ All principles satisfied

## Project Structure

### Documentation (this feature)

```text
specs/003-linux-virtual-audio-testing/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output - Technology research findings
├── data-model.md        # Phase 1 output - Test data entities and relationships
├── quickstart.md        # Phase 1 output - 30-minute setup guide
├── spec.md              # Feature specification
├── contracts/           # Phase 1 output - API contracts
│   └── test-api.md      # CLI API specification
└── checklists/
    └── requirements.md  # Specification quality validation
```

### Source Code (repository root)

```text
# AudioBridge Single Project Structure
src/
├── core/                    # Audio Engine Core (unchanged)
│   ├── AudioPipeline.h/cpp
│   ├── RingBuffer.h/cpp
│   └── AudioEngine.h/cpp
├── adapters/                # Audio I/O Adapter Layer (unchanged)
│   ├── IAudioInput.h
│   ├── IAudioOutput.h
│   ├── PortAudioInput.h/cpp
│   └── PortAudioOutput.h/cpp
├── processing/              # DSP/AI Processing (unchanged)
│   └── DummyEngine.h/cpp
├── utils/                   # Utilities (unchanged)
│   ├── Logger.h/cpp
│   └── Config.h/cpp
├── ui/                      # Application UI Layer (unchanged)
│   ├── DeviceSelector.h/cpp
│   └── StatusMonitor.h/cpp
└── main.cpp                 # Application entry point (unchanged)

tests/                       # MODIFIED - New test infrastructure
├── unit/                    # Unit tests (existing)
│   ├── test_core/
│   ├── test_adapters/
│   └── test_processing/
├── integration/             # Integration tests (NEW)
│   ├── test_audio_pipeline.cpp        # Existing
│   ├── test_latency.cpp                # Existing
│   └── test_linux_virtual_audio.cpp    # NEW - Linux loopback tests
├── performance/             # Performance tests (existing)
│   ├── test_rt_safety.cpp
│   └── benchmark_latency.cpp
└── tools/                   # NEW - Test tools
    ├── audioBridge-test.cpp            # Main test runner CLI
    ├── AudioValidator.cpp              # Audio validation logic
    ├── VirtualDeviceManager.cpp        # Virtual device detection/management
    └── TestRunner.cpp                  # Test orchestration

scripts/                     # NEW - Shell orchestration scripts
├── audioBridge-test.sh      # Main test runner wrapper
├── setup-check.sh           # Virtual audio setup verification
├── run-suite.sh             # Test suite execution
└── utils/
    ├── generate-test-audio.sh       # Generate test audio files
    └── detect-distribution.sh       # Detect Linux distribution

test-data/                   # NEW - Test assets
├── audio/                   # Test audio files
│   ├── 1khz-sine.wav
│   ├── 440hz-tone.wav
│   ├── white-noise.wav
│   ├── frequency-sweep.wav
│   └── silence.wav
├── configs/                 # Test configuration templates
│   ├── default-loopback.json
│   └── suite-default.json
└── schemas/                 # JSON schemas for validation
    ├── test-config-v1.json
    └── test-suite-v1.json

cmake/                       # CMake build configuration (EXTENDED)
├── FindPortAudio.cmake      # Existing
├── FindALSA.cmake           # NEW
├── FindGist.cmake           # NEW
└── CompilerFlags.cmake      # Existing

docs/                        # Documentation (EXTENDED)
├── requirement.md           # Existing
├── 架构文件文档.md           # Existing
└── linux-audio-testing/     # NEW - Linux-specific guides
    ├── setup-ubuntu.md
    ├── setup-fedora.md
    ├── setup-arch.md
    └── troubleshooting.md

examples/                    # NEW - Example integrations
├── ci/
│   ├── github-actions.yml
│   ├── gitlab-ci.yml
│   └── jenkinsfile
└── scripts/
    └── automated-test-runner.sh
```

**Structure Decision**: AudioBridge uses a single C++ project structure with clear separation between audio engine core, adapter abstraction layer, and processing components. This feature adds Linux-specific test infrastructure in `tests/` and `scripts/` directories, respecting the constitution's adapter abstraction principle by isolating platform-specific code. The test tools use existing audioBridge core components through the IAudioInput/IAudioOutput interfaces, ensuring cross-platform compatibility is maintained.

## Complexity Tracking

> **No constitution violations - this section intentionally left empty**

All design decisions comply with audioBridge constitution principles. No additional complexity justified.

## Phase 0: Research & Technology Decisions ✅

**Status**: Complete - See [research.md](research.md)

### Key Decisions Made

1. **Virtual Audio Technology**: ALSA snd-aloop kernel module
   - Universal compatibility across Ubuntu, Fedora, Arch
   - Low-level access with minimal latency
   - Works with both PulseAudio and PipeWire

2. **Test Orchestration**: Shell script + C++ test runner
   - Simple, maintainable, CI/CD friendly
   - Shell for environment setup, C++ for audio operations
   - Clear separation of concerns

3. **Audio Validation**: Gist library for real-time analysis
   - Lightweight, single-header library
   - FFT, frequency analysis, onset detection
   - Integrates seamlessly with existing codebase

4. **Test Audio**: Pre-generated WAV files using SoX
   - Reproducible tests with known characteristics
   - Simple format, universally supported
   - Easy to validate

5. **Device Selection**: PortAudio enumeration with pattern matching
   - Flexible, robust across reboots
   - Auto-detection of loopback devices
   - Fallback to manual configuration

## Phase 1: Design & Contracts ✅

**Status**: Complete

### Data Model

See [data-model.md](data-model.md)

**Key Entities**:
- **TestAudioFile**: Test audio metadata (format, frequency, duration)
- **VirtualAudioDevice**: Device info (ID, name, type, capabilities)
- **TestConfiguration**: Test settings (devices, sample rate, buffer size)
- **TestExecutionRecord**: Test run results (status, captured file, duration)
- **ValidationReport**: Analysis results (frequency, SNR, latency, quality)

### API Contracts

See [contracts/test-api.md](contracts/test-api.md)

**CLI Commands**:
- `audioBridge-test list-devices`: Enumerate available audio devices
- `audioBridge-test run`: Execute single test
- `audioBridge-test run-suite`: Execute test suite
- `audioBridge-test validate`: Validate captured audio
- `audioBridge-test setup-check`: Verify virtual audio configuration

**Report Formats**:
- Text (human-readable)
- JSON (automation/CI)
- JUnit XML (CI/CD integration)

### Developer Documentation

See [quickstart.md](quickstart.md)

**Coverage**:
- 30-minute setup guide for Ubuntu, Fedora, Arch
- Step-by-step first test execution
- Troubleshooting common issues
- CI/CD integration examples

## Phase 2: Implementation Planning (Next Step)

**Ready for**: `/speckit.tasks` command

### Planned Implementation Phases

#### Phase 2.1: P1 - Basic Loopback Testing
- Virtual device detection and enumeration
- Device selection (auto-detect loopback)
- Basic test execution (playback → capture)
- Setup documentation for three distributions
- Basic validation (format, duration, integrity)

#### Phase 2.2: P2 - Automated Testing
- Test runner shell script
- C++ test harness implementation
- Test suite configuration and execution
- Automated report generation (text, JSON, JUnit)
- CI/CD integration examples

#### Phase 2.3: P3 - Advanced Validation
- Gist library integration
- Frequency analysis and FFT
- Signal quality metrics (SNR, THD)
- Latency measurement and reporting
- Advanced thresholds and diagnostics

### Technical Tasks Preview

1. **CMake Configuration**
   - Add ALSA detection
   - Add Gist library (header-only)
   - Conditional Linux test building
   - Test data installation

2. **Core Test Components**
   - `VirtualDeviceManager`: Device enumeration and selection
   - `AudioValidator`: Audio analysis using Gist
   - `TestRunner`: Test orchestration and state management
   - `LatencyMeasurer`: Timestamp-based latency measurement

3. **CLI Implementation**
   - Command parsing and validation
   - Device listing and filtering
   - Test execution and monitoring
   - Report generation (multiple formats)

4. **Shell Scripts**
   - `audioBridge-test.sh`: Main entry point
   - `setup-check.sh`: Verify virtual audio setup
   - `run-suite.sh`: Execute test suites
   - Distribution detection helpers

5. **Test Audio Generation**
   - SoX scripts for test audio
   - Five standard test files (sine, tone, noise, sweep, silence)
   - Validation scripts for test audio

6. **Documentation**
   - Quick start guide ✅
   - Distribution-specific setup guides
   - Troubleshooting guide
   - API reference ✅
   - Architecture documentation

7. **Testing**
   - Unit tests for device enumeration
   - Integration tests for loopback pipeline
   - Performance tests for latency consistency
   - Validation accuracy tests

## Dependencies & Toolchain

### Build Dependencies (CMake)

```cmake
# Existing
find_package(PortAudio REQUIRED)
find_package(spdlog REQUIRED)

# New for this feature
find_package(ALSA REQUIRED)
find_package(SndFile REQUIRED)  # For audio file I/O

# Header-only library (included in repo)
# Gist: https://github.com/adamstark/Gist
```

### Runtime Dependencies (User System)

```bash
# Ubuntu/Debian
sudo apt install alsa-utils sox libsndfile1

# Fedora
sudo dnf install alsa-utils sox libsndfile

# Arch Linux
sudo pacman -S alsa-utils sox libsndfile
```

### Optional Dependencies

```bash
# For advanced analysis
libsamplerate0-dev  # Sample rate conversion
fftw3-dev           # FFT library (if not using Gist)
```

## Risk Assessment & Mitigation

| Risk | Probability | Impact | Mitigation |
|------|------------|--------|------------|
| PipeWire compatibility issues | Medium | Medium | Support both snd-aloop and pw-loopback, runtime detection |
| Device naming inconsistencies | Low | Low | Flexible pattern matching, user override options |
| High latency on some systems | Low | Low | Document expected ranges, configurable thresholds |
| Distribution differences | Medium | Low | Comprehensive docs for Ubuntu/Fedora/Arch, community contributions |
| Gist library limitations | Low | Low | Header-only, easy to replace if needed, fallback to FFTW |

## Success Metrics

From spec.md:

- **SC-001**: Developers can set up virtual audio loopback and complete first successful audio capture test in under 30 minutes
- **SC-002**: Automated audio tests complete execution (playback, capture, validation) in under 2 minutes per test case
- **SC-003**: 95% of audio capture tests pass when using known-good test files and properly configured virtual devices
- **SC-004**: Test reports clearly indicate pass/fail status with actionable error messages for failures
- **SC-005**: Audio latency measurements are consistent within ±10ms across multiple test runs
- **SC-006**: Documentation enables developers unfamiliar with Linux audio to successfully configure virtual devices on first attempt

## Integration Points

### With Existing audioBridge

1. **Adapter Layer**: Uses existing IAudioInput/IAudioOutput interfaces
2. **Audio Engine**: Leverages existing audio pipeline for capture/playback
3. **Logging**: Uses spdlog for consistent logging
4. **Build System**: Extends existing CMake configuration

### Platform Isolation

1. **Linux-specific code**: Only in `tests/integration/test_linux_virtual_audio.cpp`
2. **Conditional compilation**: Tests only built on Linux via CMake
3. **No core changes**: Audio engine remains platform-agnostic

## Next Steps

1. **Execute `/speckit.tasks`** to generate detailed task breakdown
2. **Begin P1 implementation**: Basic loopback testing
3. **Create distribution setup guides**: Ubuntu, Fedora, Arch
4. **Generate test audio files**: Using SoX scripts
5. **Implement test runner**: C++ CLI + shell wrappers
6. **Write tests**: Unit, integration, performance
7. **Documentation**: Distribution guides and troubleshooting

---

**Plan Status**: ✅ Complete
**Phase 0 (Research)**: ✅ Complete
**Phase 1 (Design)**: ✅ Complete
**Ready for Phase 2 (Tasks)**: Yes
**Constitution Compliance**: ✅ Verified

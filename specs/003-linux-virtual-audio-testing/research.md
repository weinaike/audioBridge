# Research: Linux Virtual Audio Testing

**Feature**: 003-linux-virtual-audio-testing
**Date**: 2025-12-24
**Status**: Complete

## Overview

This document consolidates research findings for implementing Linux virtual audio testing infrastructure in audioBridge. The research covers virtual audio device configuration, automated testing approaches, audio validation techniques, and integration with existing audioBridge architecture.

---

## 1. Virtual Audio Loopback Technologies

### Decision: ALSA snd-aloop + PulseAudio/PipeWire Integration

**Rationale**:
- **Universal compatibility**: ALSA snd-aloop kernel module works across all major Linux distributions (Ubuntu, Fedora, Arch)
- **Low-level access**: Provides direct PCM-level loopback without additional latency layers
- **PortAudio compatible**: PortAudio can enumerate and use ALSA loopback devices directly
- **Modern audio stack integration**: Works with both PulseAudio (traditional) and PipeWire (modern) through ALSA emulation layer

**Implementation Approach**:
1. **Kernel Module**: Load `snd-aloop` module to create virtual loopback devices
2. **Device Enumeration**: Use PortAudio's device API to discover loopback devices
3. **Audio Routing**: Configure playback to loopback output, capture from loopback input
4. **Cross-distro Support**: Provide distribution-specific setup instructions

**Alternatives Considered**:
- **PipeWire pw-loopback**: Rejected because it's PipeWire-specific and doesn't work with PulseAudio systems
- **JACK audio server**: Rejected due to complexity and requirement for professional audio setup
- **PulseAudio module-loopback**: Rejected because it only works with PulseAudio, not ALSA directly

---

## 2. Distribution-Specific Setup

### Ubuntu/Debian

**Setup Commands**:
```bash
# Load snd-aloop module temporarily
sudo modprobe snd-aloop

# Permanently enable on boot
echo "snd-aloop" | sudo tee -a /etc/modules-load.d/alsa-loopback.conf
```

**Verification**:
```bash
# List audio devices
aplay -l
arecord -l
# Look for "Loopback" cards in output
```

**Configuration File**: `/etc/modprobe.d/alsa-loopback.conf`
```bash
options snd-aloop enable=1,1 index=1,2
```

### Fedora

**Setup Commands**:
```bash
# Load snd-aloop module temporarily
sudo modprobe snd-aloop

# Permanently enable on boot
echo "snd-aloop" | sudo tee -a /etc/modules-load.d/alsa-loopback.conf
```

**Verification**:
```bash
aplay -l | grep -i loopback
```

**Alternative (systemd)**:
```bash
sudo systemctl enable --now snd-aloop.service
```

### Arch Linux

**Setup Commands**:
```bash
# Load snd-aloop module temporarily
sudo modprobe snd-aloop

# Permanently enable on boot
echo "snd-aloop" | sudo tee -a /etc/modules-load.d/alsa-loopback.conf
```

**Verification**:
```bash
cat /proc/asound/cards | grep -i loopback
```

### PipeWire-Specific Considerations (2024+)

**Issue**: Modern distributions using PipeWire (Ubuntu 22.04+, Fedora 35+, Arch 2021+) have compatibility issues with snd-aloop

**Solutions**:
1. **Use ALSA emulation layer**: PipeWire provides ALSA compatibility, snd-aloop devices appear as ALSA devices
2. **pw-loopback alternative**: For pure PipeWire systems, use `pw-loopback` command
3. **Device profile mapping**: May need to configure PipeWire to expose loopback devices correctly

**Reference**: [snd-aloop compatibility issues with PipeWire](https://bbs.archlinux.org/viewtopic.php?id=296536) (June 2024)

---

## 3. Automated Test Execution

### Decision: Shell Script + C++ Test Runner

**Rationale**:
- **Simplicity**: Shell scripts for test orchestration are easy to understand and modify
- **PortAudio integration**: C++ test runner can directly use PortAudio API for device control
- **CI/CD friendly**: Standard exit codes and output format for Jenkins, GitHub Actions, GitLab CI
- **Separation of concerns**: Shell handles environment setup, C++ handles audio operations

**Architecture**:
```
test_runner.sh (orchestration)
├── 1. Verify virtual device availability
├── 2. Start audioBridge in capture mode
├── 3. Play test audio (aplay/ffplay)
├── 4. Wait for capture completion
├── 5. Run validation (C++ validator)
└── 6. Generate test report (JSON/text)
```

**Alternatives Considered**:
- **Python with pytest**: Rejected due to additional dependency and slower execution
- **Pure C++ test framework**: Rejected because system-level operations (modprobe, device detection) easier in shell
- **Docker containers**: Rejected due to audio device access complexity in containers

---

## 4. Audio Validation & Analysis

### Decision: Gist Library for Real-Time Analysis

**Rationale**:
- **Lightweight**: Single-header library, minimal dependencies
- **Real-time capable**: Designed for real-time audio analysis
- **Feature set**: FFT, frequency analysis, onset detection suitable for basic validation
- **C++ native**: Integrates seamlessly with existing audioBridge codebase

**Validation Metrics**:
1. **Frequency Response**: FFT analysis to verify captured frequency matches expected
2. **Signal-to-Noise Ratio (SNR)**: Calculate noise floor vs signal level
3. **Latency Measurement**: Timestamp comparison between playback start and capture start
4. **Basic Integrity**: Format validation (sample rate, channels, duration)

**Implementation**:
```cpp
#include "Gist.h"

// FFT-based frequency analysis
Gist<float> gist(bufferSize, sampleRate);
gist.processAudioFrame(audioBuffer, bufferSize);
float magnitude = gist.getMagnitudeSpectrum();

// Peak detection for frequency verification
float peakFrequency = gist.getPeakFrequency();
```

**Alternatives Considered**:
- **Essentia**: Rejected due to AGPL license and large dependency footprint
- **AudioFlux**: Rejected because it's more focused on music analysis than basic validation
- **Custom FFT implementation**: Rejected to avoid reinventing wheel and potential numerical errors

---

## 5. Test Audio Generation

### Decision: Pre-generated WAV Files with Known Characteristics

**Rationale**:
- **Reproducibility**: Same test audio used across all test runs
- **Predictable**: Known frequency content enables automated validation
- **Simple**: No need for runtime audio synthesis
- **Format support**: WAV files universally supported by Linux audio tools

**Test Audio Suite**:
1. **1kHz Sine Wave** (48kHz, 16-bit, mono) - Basic frequency response test
2. **440Hz Tone** (48kHz, 16-bit, mono) - Musical pitch reference test
3. **White Noise** (48kHz, 16-bit, stereo) - Dynamic range and SNR test
4. **Frequency Sweep** (20Hz - 20kHz, 48kHz, 16-bit, mono) - Full spectrum response test
5. **Silence** (48kHz, 16-bit, stereo, 10 seconds) - Noise floor test

**Generation Tools**:
```bash
# Using SoX (Sound eXchange)
sox -n -r 48000 -b 16 test_1khz.wav synth 5 sine 1000
sox -n -r 48000 -b 16 test_noise.wav synth 5 noise
sox -n -r 48000 -b 16 test_sweep.wav synth 5 sine 20-20000
```

**Alternatives Considered**:
- **Runtime synthesis**: Rejected due to added complexity and potential for inconsistency
- **MP3/FLAC files**: Rejected because lossy compression introduces artifacts that could affect validation
- **Recording from physical sources**: Rejected because it violates the goal of hardware-independent testing

---

## 6. PortAudio Device Selection

### Decision: Device Enumeration by Name Pattern Matching

**Rationale**:
- **Flexible**: Works with any virtual loopback device regardless of index
- **Robust**: Device indices can change between reboots, names are more stable
- **Cross-platform**: PortAudio's device enumeration API works consistently across platforms

**Implementation Pattern**:
```cpp
// Enumerate all devices
int numDevices = Pa_GetDeviceCount();
for (int i = 0; i < numDevices; i++) {
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);
    std::string deviceName = deviceInfo->name;

    // Match loopback devices
    if (deviceName.find("Loopback") != std::string::npos ||
        deviceName.find("loopback") != std::string::npos) {
        // Found loopback device
        if (deviceInfo->maxInputChannels > 0) {
            // This is a loopback input device
        }
        if (deviceInfo->maxOutputChannels > 0) {
            // This is a loopback output device
        }
    }
}
```

**Device Naming Conventions** (observed in research):
- **ALSA**: "hw:Loopback,0,0" (output), "hw:Loopback,0,1" (input)
- **PulseAudio via ALSA**: "pulse", with loopback appearing as "alsa_output.pci-..."
- **PipeWire**: Similar to PulseAudio with "Loopback" substring

**Error Handling**:
- Device not found: Clear error message suggesting setup steps
- Device busy: Retry with exponential backoff
- Device parameters mismatch (sample rate, channels): Automatic resampling if supported

---

## 7. Latency Measurement Strategy

### Decision: Timestamp-Based Round-Trip Measurement

**Rationale**:
- **Accurate**: Measures actual system latency including all processing stages
- **Simple**: Requires only timestamp capture at two points
- **Real-world**: Reflects actual user-perceived latency

**Measurement Points**:
1. **T0**: Playback start timestamp (when audio written to output buffer)
2. **T1**: Capture start timestamp (when first audio frame received from input)
3. **Latency**: T1 - T0 (in milliseconds)

**Implementation**:
```cpp
// Playback start
auto playbackStart = std::chrono::high_resolution_clock::now();
Pa_WriteStream(stream, buffer, frames);

// Capture start (in audio callback)
auto captureStart = std::chrono::high_resolution_clock::now();

// Calculate latency
auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
    captureStart - playbackStart
).count();
```

**Expected Latency Ranges** (based on research):
- **Direct loopback (snd-aloop)**: < 10ms
- **With PulseAudio**: 20-50ms
- **With PipeWire**: 10-30ms
- **Success criterion**: ±10ms consistency across runs

**Alternatives Considered**:
- **Correlation-based latency detection**: Rejected due to computational complexity
- **External measurement tools**: Rejected because they don't measure application-internal latency
- **Periodic signal injection**: Rejected due to added complexity in test audio generation

---

## 8. Error Handling & Edge Cases

### Virtual Device Unavailable

**Detection**: PortAudio enumeration returns no loopback devices

**Action**:
- Provide clear error message: "Virtual audio loopback device not found"
- Suggest setup steps based on detected distribution
- Link to documentation for manual configuration

### Multiple Applications Competing for Device

**Detection**: PortAudio returns `paDeviceUnavailable` error

**Action**:
- Retry with exponential backoff (100ms, 200ms, 400ms, max 3 attempts)
- If all retries fail: Report "Device busy" with list of processes using audio device (using `lsof /dev/snd/*`)
- Suggest closing other audio applications

### Audio Format Mismatch

**Detection**: Captured audio sample rate differs from expected (48kHz)

**Action**:
- Log warning with detected vs expected sample rate
- Attempt automatic resampling using libsamplerate if available
- If resampling unavailable: Mark test as "WARNING - Sample rate mismatch" but continue

### Disk Space Insufficient

**Detection**: `std::ofstream` open fails or write fails during capture

**Action**:
- Before test: Check available disk space (require 2x expected file size)
- If insufficient: Fail test with clear error message
- During capture: Handle write errors gracefully, cleanup partial files

### Long Duration Test Audio

**Detection**: Test audio file > 10 minutes

**Action**:
- Issue warning: "Long duration test detected - this may take several minutes"
- Provide progress updates every 10% of playback
- Support cancellation via SIGINT (Ctrl+C) with cleanup

---

## 9. Integration with audioBridge Architecture

### Compliance with Constitution

**Adapter Abstraction Layer**:
- Virtual audio devices accessed through existing IAudioInput/IAudioOutput interfaces
- No direct PortAudio calls in business logic
- Test infrastructure isolated in `tests/` directory

**Real-Time Safety**:
- Audio capture and playback use existing real-time safe audio threads
- Validation analysis runs in separate non-real-time thread
- No blocking I/O in audio callback paths

**Cross-Platform Compatibility**:
- Linux-specific code isolated in `tests/integration/test_linux_virtual_audio.cpp`
- Core audio engine remains platform-agnostic
- Other platforms can skip Linux-specific tests via CMake conditionals

**Test-First Development**:
- Unit tests for virtual audio device enumeration
- Integration tests for complete loopback pipeline
- Performance tests for latency consistency

---

## 10. Dependencies & Toolchain

### Build Dependencies

```cmake
# Existing dependencies (from 002-audio-io-foundation)
find_package(PortAudio REQUIRED)
find_package(spdlog REQUIRED)

# New dependencies for this feature
find_package(ALSA REQUIRED)         # For ALSA device detection
find_package(Git)                   # For test script versioning
```

### Runtime Dependencies

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
# For advanced audio analysis (future P3 enhancements)
sudo apt install libfftw3-dev libsamplerate0-dev
```

---

## 11. Documentation Strategy

### Target Audience: Developers with basic Linux skills

**Documentation Components**:

1. **Quick Start Guide** (`quickstart.md`):
   - 5-minute setup for common distributions
   - Run first test
   - Troubleshooting common issues

2. **Distribution Guides** (`docs/linux-setup-{ubuntu,fedora,arch}.md`):
   - Distribution-specific setup steps
   - Package installation commands
   - Known issues and workarounds

3. **API Reference** (`contracts/test-api.md`):
   - Test runner command-line interface
   - Configuration file format
   - Test report format (JSON schema)

4. **Architecture Documentation** (`docs/virtual-audio-architecture.md`):
   - How loopback devices work
   - Integration with PortAudio
   - Latency measurement methodology

---

## Summary & Recommendations

### Technical Stack

- **Virtual Audio**: ALSA snd-aloop kernel module
- **Audio I/O**: PortAudio (existing)
- **Test Orchestration**: Bash shell scripts
- **Audio Validation**: Gist C++ library
- **Test Audio**: Pre-generated WAV files (SoX)
- **Logging**: spdlog (existing)
- **Build System**: CMake (existing)

### Implementation Phases

1. **Phase 1 (P1 - Basic Loopback Testing)**:
   - Setup documentation for three major distributions
   - Device enumeration and selection in audioBridge
   - Manual test execution instructions
   - Basic validation (integrity check)

2. **Phase 2 (P2 - Automated Testing)**:
   - Test runner shell script
   - C++ test harness with PortAudio integration
   - CI/CD integration examples
   - Automated report generation

3. **Phase 3 (P3 - Advanced Validation)**:
   - Gist library integration
   - Frequency analysis and SNR calculation
   - Latency measurement and reporting
   - Quality metrics and thresholds

### Risks & Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| PipeWire compatibility issues | Medium | Support both snd-aloop and pw-loopback, detect audio system at runtime |
| Device naming inconsistencies | Low | Flexible pattern matching for device names |
| High latency on some systems | Low | Document expected ranges, warn if thresholds exceeded |
| Test audio generation errors | Low | Include pre-generated test files in repository |

---

**Research Status**: ✅ Complete
**All Technical Clarifications Resolved**: Yes
**Ready for Phase 1 Design**: Yes

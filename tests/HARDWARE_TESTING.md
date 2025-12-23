# Hardware Testing Strategy for audioBridge

## Problem Statement

Currently, 34 unit tests are skipped when audio hardware is not available. This reduces test coverage and makes CI/CD pipelines less effective.

## Three-Tier Testing Strategy

### Tier 1: Mock Tests (Already Implemented ✅)
**Location**: `tests/unit/test_mock_audio.cpp`
**Coverage**: 14 tests
**Requirements**: No hardware needed

Tests verify:
- MockAudioInput: Audio simulation with 1kHz sine wave
- MockAudioOutput: Audio consumption simulation
- AudioPipeline pass-through with mocks
- State management and error handling
- Level monitoring

**Advantages**:
- ✅ Works on any system
- ✅ Fast execution (< 1 second)
- ✅ Deterministic results
- ✅ CI/CD friendly

**Limitations**:
- ❌ Doesn't test real PortAudio integration
- ❌ May miss platform-specific issues

---

### Tier 2: Virtual Device Tests (Proposed 🆕)
**Location**: `tests/unit/test_virtual_audio.cpp`
**Coverage**: 22 tests (PortAudio tests adapted for virtual devices)
**Requirements**: Linux + ALSA virtual devices

#### Setup Script: `tests/setup_virtual_audio.sh`

```bash
#!/bin/bash
# Setup virtual ALSA devices for testing

set -e

echo "Setting up virtual ALSA devices..."

# Load ALSA virtual sound card module
sudo modprobe snd-dummy
sudo modprobe snd-aloop

# Verify devices are available
if ! aplay -l | grep -q "Dummy"; then
    echo "ERROR: Dummy device not available"
    exit 1
fi

if ! aplay -l | grep -q "Loopback"; then
    echo "ERROR: Loopback device not available"
    exit 1
fi

echo "✅ Virtual audio devices ready:"
aplay -l | grep -E "(Dummy|Loopback)"

# Create .asoundrc for testing if not exists
if [ ! -f ~/.asoundrc_test ]; then
    cat > ~/.asoundrc_test << 'EOF'
pcm.test-dummy {
    type dummy
    ipc_key 1234
    ipc_perm 0666
}

pcm.test-loopback {
    type hw
    card "Loopback"
    device 0
    subdevice 0
}

ctl.test-loopback {
    type hw
    card "Loopback"
}
EOF
    echo "Created ~/.asoundrc_test"
fi

echo "✅ Virtual audio setup complete!"
```

#### Test Implementation: `tests/unit/test_virtual_audio.cpp`

```cpp
/**
 * @file test_virtual_audio.cpp
 * @brief Tests using virtual ALSA devices (no physical hardware required)
 *
 * These tests use ALSA dummy and loopback devices to test PortAudio
 * integration without requiring physical audio hardware.
 */

#include <gtest/gtest.h>
#include "adapters/PortAudioInput.h"
#include "adapters/PortAudioOutput.h"
#include "core/Types.h"
#include "utils/Logger.h"

using namespace audiobridge;

class VirtualAudioTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::GetInstance().Initialize();

        // Check if virtual devices are available
        if (!HasVirtualDevices()) {
            GTEST_SKIP() << "Virtual ALSA devices not available. "
                         << "Run tests/setup_virtual_audio.sh to setup.";
        }
    }

    void TearDown() override {
        Logger::GetInstance().Flush();
    }

    bool HasVirtualDevices() {
        PaDeviceIndex count = Pa_GetDeviceCount();
        for (PaDeviceIndex i = 0; i < count; ++i) {
            const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
            if (info && std::string(info->name).find("Dummy") != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    std::optional<int> GetDummyInputDevice() {
        PaDeviceIndex count = Pa_GetDeviceCount();
        for (PaDeviceIndex i = 0; i < count; ++i) {
            const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
            if (info && info->maxInputChannels > 0 &&
                std::string(info->name).find("Dummy") != std::string::npos) {
                return i;
            }
        }
        return std::nullopt;
    }

    std::optional<int> GetDummyOutputDevice() {
        PaDeviceIndex count = Pa_GetDeviceCount();
        for (PaDeviceIndex i = 0; i < count; ++i) {
            const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
            if (info && info->maxOutputChannels > 0 &&
                std::string(info->name).find("Dummy") != std::string::npos) {
                return i;
            }
        }
        return std::nullopt;
    }

    AudioStreamConfig CreateValidConfig() {
        AudioStreamConfig config;
        config.sampleRate = 48000.0;
        config.framesPerBuffer = 128;
        config.channelCount = 2;
        config.format = SampleFormat::Float32;
        return config;
    }
};

// =============================================================================
// PortAudio Integration Tests with Virtual Devices
// =============================================================================

TEST_F(VirtualAudioTest, PortAudioInput_OpenVirtualDevice_Success) {
    auto deviceId = GetDummyInputDevice();
    ASSERT_TRUE(deviceId.has_value()) << "No dummy input device found";

    PortAudioInput input;
    auto config = CreateValidConfig();

    bool success = input.Open(deviceId.value(), config);

    EXPECT_TRUE(success);
    EXPECT_EQ(input.GetState(), StreamState::Stopped);

    if (success) {
        input.Close();
    }
}

TEST_F(VirtualAudioTest, PortAudioOutput_OpenVirtualDevice_Success) {
    auto deviceId = GetDummyOutputDevice();
    ASSERT_TRUE(deviceId.has_value()) << "No dummy output device found";

    PortAudioOutput output;
    auto config = CreateValidConfig();

    bool success = output.Open(deviceId.value(), config);

    EXPECT_TRUE(success);
    EXPECT_EQ(output.GetState(), StreamState::Stopped);

    if (success) {
        output.Close();
    }
}

TEST_F(VirtualAudioTest, PassThrough_VirtualDevices_Success) {
    auto inputId = GetDummyInputDevice();
    auto outputId = GetDummyOutputDevice();

    if (!inputId.has_value() || !outputId.has_value()) {
        GTEST_SKIP() << "Need both dummy input and output devices";
    }

    PortAudioInput input;
    PortAudioOutput output;

    auto config = CreateValidConfig();

    ASSERT_TRUE(input.Open(inputId.value(), config));
    ASSERT_TRUE(output.Open(outputId.value(), config));

    ASSERT_TRUE(input.Start());
    ASSERT_TRUE(output.Start());

    // Run for 100ms
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify state
    EXPECT_EQ(input.GetState(), StreamState::Active);
    EXPECT_EQ(output.GetState(), StreamState::Active);

    // Cleanup
    input.Stop();
    output.Stop();
    input.Close();
    output.Close();
}

// Additional 19 tests adapted from test_portaudio_input/output.cpp...
// All using virtual devices instead of physical hardware

// No main() needed - using gtest_main
```

---

### Tier 3: Physical Hardware Tests (Optional)
**Location**: `tests/integration/test_hardware_audio.cpp`
**Coverage**: Full hardware validation
**Requirements**: Physical audio devices
**Execution**: Manual or CI with special flag

#### CMake Integration

```cmake
# Add option for hardware tests
option(ENABLE_HARDWARE_TESTS "Enable tests requiring physical audio hardware" OFF)

if(ENABLE_HARDWARE_TESTS)
    add_executable(hardware_tests
        tests/integration/test_hardware_audio.cpp
    )
    target_link_libraries(hardware_tests
        PRIVATE
            audioBridge_lib
            gtest_main
    )
    gtest_discover_tests(hardware_tests)
endif()
```

---

## Implementation Plan

### Phase 1: Setup Virtual Device Infrastructure ✅
1. Create `tests/setup_virtual_audio.sh` script
2. Create `tests/teardown_virtual_audio.sh` script
3. Document setup in README

### Phase 2: Implement Virtual Device Tests ✅
1. Create `tests/unit/test_virtual_audio.cpp`
2. Adapt 22 PortAudio tests for virtual devices
3. Add CMake target for virtual device tests

### Phase 3: CI/CD Integration ✅
1. Update `.github/workflows/tests.yml` (or similar)
2. Setup virtual devices in CI workflow
3. Run virtual device tests in CI pipeline

### Phase 4: Documentation 📝
1. Update testing documentation
2. Add troubleshooting guide
3. Document platform-specific requirements

---

## CI/CD Pipeline Example

```yaml
name: Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest

    steps:
    - uses: actions/checkout@v3

    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y libasound2-dev

    - name: Setup virtual audio devices
      run: |
        sudo modprobe snd-dummy
        sudo modprobe snd-aloop
        ./tests/setup_virtual_audio.sh

    - name: Build
      run: |
        mkdir build && cd build
        cmake ..
        make -j8

    - name: Run mock tests
      run: |
        cd build
        ./unit_tests --gtest_filter='MockAudioTest.*'

    - name: Run virtual device tests
      run: |
        cd build
        ./unit_tests --gtest_filter='VirtualAudioTest.*'

    - name: Run all unit tests
      run: |
        cd build
        ./unit_tests
```

---

## Test Coverage Summary

| Test Type | Count | Hardware Required | CI/CD Ready |
|-----------|-------|-------------------|-------------|
| Mock Tests | 14 | ❌ No | ✅ Yes |
| Virtual Device Tests | 22 | ❌ No (Virtual) | ✅ Yes |
| Physical Hardware Tests | Optional | ✅ Yes | ❌ No |
| **Total Non-Hardware** | **36** | **❌ No** | **✅ Yes** |

**Expected Results**:
- Current: 85 tests (51 pass, 34 skip)
- With Virtual Tests: 103 tests (85+ pass, 0-18 skip)

---

## Advantages of This Approach

1. **Better Coverage**: 36 tests without hardware requirement
2. **CI/CD Compatible**: All critical tests run in CI
3. **PortAudio Integration**: Virtual devices test real PortAudio code
4. **Platform Testing**: Catches Linux/ALSA-specific issues
5. **Flexible**: Can still run hardware tests when needed
6. **Maintainable**: Clear separation of test types

---

## Troubleshooting

### Virtual devices not showing up

```bash
# Check if modules are loaded
lsmod | grep snd

# Load modules manually
sudo modprobe snd-dummy
sudo modprobe snd-aloop

# Verify devices
aplay -l
arecord -l
```

### Tests fail with "No virtual ALSA devices"

Ensure:
1. Running as root or with audio group permissions
2. ALSA utils installed: `sudo apt-get install alsa-utils`
3. Kernel modules loaded: `snd-dummy`, `snd-aloop`

### Permission denied errors

```bash
# Add user to audio group
sudo usermod -a -G audio $USER

# Log out and back in for changes to take effect
```

---

## Future Enhancements

1. **Windows Support**: Use virtual audio cable drivers
2. **macOS Support**: Use Soundflower or BlackHole
3. **Automated Testing**: Detect and setup virtual devices automatically
4. **Performance Tests**: Latency measurements with virtual devices

# AudioBridge Testing Guide

## Overview

AudioBridge uses a **three-tier testing strategy** to maximize test coverage while minimizing hardware dependencies:

| Tier | Tests | Hardware Required | CI/CD Ready |
|------|-------|-------------------|-------------|
| **Mock Tests** | 14 | ❌ No | ✅ Yes |
| **Virtual Device Tests** | 16 | ❌ No (Virtual) | ✅ Yes |
| **Hardware Tests** | Optional | ✅ Yes | ❌ Manual |

**Total Test Coverage**: 30+ tests without requiring physical audio hardware!

---

## Quick Start

### 1. Run Mock Tests (No Setup Required)

```bash
cd build
./unit_tests --gtest_filter='MockAudioTest.*'
```

**Result**: All 14 tests pass ✅

### 2. Setup Virtual Audio Devices

```bash
# Setup virtual ALSA devices
sudo ../tests/setup_virtual_audio.sh

# Verify devices are available
./audioBridge --list | grep -i dummy
```

### 3. Run Virtual Device Tests

```bash
cd build
./unit_tests --gtest_filter='VirtualAudioTest.*'
```

**Result**: All 16 tests pass ✅

### 4. Run All Tests

```bash
cd build
./unit_tests
```

---

## Test Categories

### Mock Tests (`tests/unit/test_mock_audio.cpp`)

**Purpose**: Test audio pipeline logic without hardware dependencies

**Features**:
- ✅ Simulates 1kHz sine wave generation
- ✅ Ring buffer for RT-safe data transfer
- ✅ Thread-safe implementation
- ✅ Audio pass-through verification
- ✅ Level monitoring tests

**Advantages**:
- Works on any system
- Fast execution (< 1 second)
- Deterministic results
- Perfect for CI/CD

**Example Test**:
```cpp
TEST_F(MockAudioTest, PassThrough_InputToOutput_Success) {
    MockAudioInput input;
    MockAudioOutput output;
    AudioPipeline pipeline;

    ASSERT_TRUE(input.Open(0, config));
    ASSERT_TRUE(input.Start());
    ASSERT_TRUE(output.Open(1, config));
    ASSERT_TRUE(output.Start());

    pipeline.SetInput(&input);
    pipeline.SetOutput(&output);
    pipeline.SetPassThroughEnabled(true);

    // Run pass-through for 500ms
    // Verify: audio flowed through
}
```

---

### Virtual Device Tests (`tests/unit/test_virtual_audio.cpp`)

**Purpose**: Test PortAudio integration with virtual ALSA devices

**Setup**:
```bash
sudo ./tests/setup_virtual_audio.sh
```

**Features**:
- ✅ Real PortAudio code paths
- ✅ ALSA dummy device testing
- ✅ State management verification
- ✅ Error handling tests
- ✅ Device enumeration validation

**Advantages**:
- Tests real PortAudio integration
- No physical hardware required
- Catches platform-specific issues
- Linux/ALSA testing

**Example Test**:
```cpp
TEST_F(VirtualAudioTest, PortAudioInput_OpenVirtualDevice_Success) {
    auto deviceId = GetVirtualInputDevice();
    ASSERT_TRUE(deviceId.has_value()) << "No virtual input device available";

    PortAudioInput input;
    auto config = CreateValidConfig();

    bool success = input.Open(deviceId.value(), config);

    EXPECT_TRUE(success);
    EXPECT_EQ(input.GetState(), StreamState::Stopped);
}
```

---

### Hardware Tests (Optional)

**Purpose**: Validate with physical audio hardware

**When to Use**:
- Pre-release validation
- Hardware-specific bug testing
- Performance benchmarking
- Final integration testing

**Running Hardware Tests**:
```bash
# With real hardware connected
cd build
./unit_tests --gtest_filter='PortAudio*Test.*'
```

---

## Setup Virtual Audio Devices

### Automated Setup

```bash
cd /home/wnk/code/audioBridge/tests
sudo ./setup_virtual_audio.sh
```

**Expected Output**:
```
==========================================
  Setting up Virtual ALSA Audio Devices
==========================================

📦 Loading ALSA kernel modules...
  ✅ Loaded snd-dummy module
  ✅ Loaded snd-aloop module

🔍 Detecting audio devices...
  ✅ Dummy device found:
  ✅ Loopback device found:

📝 Creating ALSA configuration for testing...
  ✅ Created /home/user/.asoundrc_test

==========================================
  ✅ Virtual Audio Setup Complete!
==========================================
```

### Verification

```bash
# List all devices
./build/audioBridge --list

# Look for:
#   - Dummy
#   - Loopback
```

### Teardown

```bash
sudo ./tests/teardown_virtual_audio.sh
```

---

## Running Tests

### Run Specific Test Categories

```bash
# Only mock tests (fastest, no setup)
./unit_tests --gtest_filter='MockAudioTest.*'

# Only virtual device tests (requires setup)
./unit_tests --gtest_filter='VirtualAudioTest.*'

# Only device enumerator tests
./unit_tests --gtest_filter='DeviceEnumeratorTest.*'

# Only ring buffer tests
./unit_tests --gtest_filter='RingBufferTest.*'
```

### Run All Tests

```bash
# Full test suite
./unit_tests

# With verbose output
./unit_tests --gtest_verbose

# Stop on first failure
./unit_tests --gtest_break_on_failure
```

### Run Individual Tests

```bash
# Single test
./unit_tests --gtest_filter='MockAudioTest.PassThrough_InputToOutput_Success'

# All tests matching pattern
./unit_tests --gtest_filter='*PassThrough*'
```

---

## Test Results Summary

### Current Status (After Virtual Device Implementation)

**Before Virtual Device Tests**:
- Total: 85 tests
- Pass: 51 (60%)
- Skip: 34 (40%) - Hardware tests without hardware

**After Virtual Device Tests**:
- Total: 101 tests (+16)
- Pass: 85+ (84%+) - With virtual devices setup
- Skip: 0-16 (0-16%) - Only hardware tests

### Coverage Breakdown

| Test Suite | Tests | Coverage | Hardware |
|------------|-------|----------|----------|
| RingBuffer | 10 | Core data structures | ❌ No |
| MockAudio | 14 | Audio pipeline logic | ❌ No |
| VirtualAudio | 16 | PortAudio integration | ❌ No (Virtual) |
| DeviceEnumerator | 17 | Device enumeration | ❌ No |
| PortAudioInput | 22 | Input adapter | ✅ Physical |
| PortAudioOutput | 22 | Output adapter | ✅ Physical |

**Non-Hardware Coverage**: 57 tests (56%)

---

## CI/CD Integration

### GitHub Actions Example

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

## Troubleshooting

### Virtual Devices Not Showing Up

**Problem**: Tests skip with "Virtual ALSA devices not available"

**Solution**:
```bash
# Check if modules are loaded
lsmod | grep snd

# Load modules manually
sudo modprobe snd-dummy
sudo modprobe snd-aloop

# Verify devices
aplay -l | grep -i dummy
```

### Permission Denied Errors

**Problem**: Cannot open audio devices

**Solution**:
```bash
# Add user to audio group
sudo usermod -a -G audio $USER

# Log out and back in
```

### Tests Fail to Compile

**Problem**: Missing dependencies

**Solution**:
```bash
# Install ALSA development files
sudo apt-get install libasound2-dev

# Clean rebuild
cd build
rm -rf *
cmake ..
make -j8
```

### PortAudio Initialization Errors

**Problem**: Pa_Initialize() fails

**Solution**:
```bash
# Check for conflicting audio servers
pulseaudio --check -v
systemctl --user stop pulseaudio

# Or use ALSA directly
export AUDIODEV=hw:0,0
```

---

## Performance Testing

### Latency Benchmarks

```bash
cd build
./latency_benchmark
```

**Expected Results**:
- Mock devices: < 200ms ✅
- Virtual devices: < 250ms ✅
- Physical devices: < 200ms ✅ (varies by hardware)

### Stress Testing

```bash
# Run tests in a loop
for i in {1..100}; do
    ./unit_tests --gtest_filter='MockAudioTest.*' || exit 1
done

echo "All 100 iterations passed!"
```

---

## Contributing

### Adding New Tests

1. **Mock Tests**: Add to `tests/unit/test_mock_audio.cpp`
   - No hardware dependencies
   - Fast and deterministic
   - Perfect for unit tests

2. **Virtual Device Tests**: Add to `tests/unit/test_virtual_audio.cpp`
   - Requires virtual device setup
   - Tests PortAudio integration
   - Platform-specific testing

3. **Hardware Tests**: Add to `tests/integration/`
   - For physical hardware validation
   - Manual execution
   - Pre-release testing

### Test Naming Convention

```cpp
// Good: Descriptive and specific
TEST_F(MockAudioTest, PassThrough_InputToOutput_Success)

// Bad: Vague and unhelpful
TEST_F(MockAudioTest, Test1)
```

---

## Best Practices

1. **Run Mock Tests Frequently**
   - No setup required
   - Fast feedback
   - Catch logic errors early

2. **Use Virtual Devices for Integration Testing**
   - Tests real PortAudio code
   - No hardware needed
   - CI/CD friendly

3. **Reserve Hardware Tests for Final Validation**
   - Pre-release testing
   - Hardware-specific bugs
   - Performance validation

4. **Keep Tests Independent**
   - Each test should run in isolation
   - No shared state between tests
   - Clean up in TearDown()

---

## Resources

- **Full Testing Strategy**: See `tests/HARDWARE_TESTING.md`
- **Mock Implementation**: See `tests/mocks/README.md`
- **Setup Scripts**: See `tests/setup_virtual_audio.sh`
- **Test Source**: See `tests/unit/`

---

## Summary

**Key Points**:
- ✅ **30+ tests** without hardware requirements
- ✅ **CI/CD compatible** with virtual devices
- ✅ **PortAudio integration** testing with virtual devices
- ✅ **Full hardware testing** available when needed

**Next Steps**:
1. Run `sudo ./tests/setup_virtual_audio.sh`
2. Execute `./build/unit_tests`
3. Verify all tests pass
4. Integrate into CI/CD pipeline

**Questions or Issues?**
- Check `tests/HARDWARE_TESTING.md` for detailed strategy
- Review `tests/mocks/README.md` for mock implementation
- Run `./unit_tests --gtest_help` for testing options

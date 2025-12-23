# Mock Audio Adapters for Testing

## Overview

The mock audio adapters (`MockAudioInput` and `MockAudioOutput`) provide a way to test the audio pipeline functionality without requiring actual audio hardware. This is especially useful for:

- CI/CD environments without audio devices
- Automated testing on headless servers
- Development and debugging without hardware dependencies
- Performance and latency testing in controlled environments

## Features

### MockAudioInput
- ✅ Simulates 1kHz sine wave audio generation
- ✅ Real-time level monitoring
- ✅ Configurable sample rate, channels, and buffer size
- ✅ Thread-safe implementation using atomic operations
- ✅ Ring buffer for RT-safe data transfer

### MockAudioOutput
- ✅ Simulates audio playback with ring buffer
- ✅ Tracks total frames played for verification
- ✅ Real-time level monitoring
- ✅ Configurable sample rate, channels, and buffer size
- ✅ Thread-safe implementation

## Usage

### Running Mock Tests

```bash
# Run only mock tests (no hardware required)
./unit_tests --gtest_filter='MockAudioTest.*'

# Run all unit tests (includes hardware-dependent tests)
./unit_tests

# Run specific mock test
./unit_tests --gtest_filter='MockAudioTest.PassThrough_InputToOutput_Success'
```

### Example Test

```cpp
#include "mocks/MockAudioInput.h"
#include "mocks/MockAudioOutput.h"
#include "core/AudioPipeline.h"

TEST(MyTest, AudioPassThrough) {
    MockAudioInput input;
    MockAudioOutput output;
    AudioPipeline pipeline;

    AudioStreamConfig config;
    config.sampleRate = 48000;
    config.channelCount = 2;
    config.framesPerBuffer = 128;

    // Setup
    ASSERT_TRUE(input.Open(0, config));
    ASSERT_TRUE(input.Start());
    ASSERT_TRUE(output.Open(1, config));
    ASSERT_TRUE(output.Start());

    // Connect pipeline
    pipeline.SetInput(&input);
    pipeline.SetOutput(&output);
    pipeline.SetPassThroughEnabled(true);

    // Process audio
    for (int i = 0; i < 100; ++i) {
        pipeline.Process();
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    // Verify
    EXPECT_GT(output.GetTotalFramesPlayed(), 0u);

    // Cleanup
    input.Stop();
    input.Close();
    output.Stop();
    output.Close();
}
```

## Test Results Summary

**Total Unit Tests**: 83 (from 69)
- **RingBuffer**: 10 tests ✅
- **PortAudioInput**: 22 tests (requires hardware, some skipped)
- **PortAudioOutput**: 22 tests (requires hardware, some skipped)
- **DeviceEnumerator**: 15 tests (requires hardware, some skipped)
- **MockAudio**: 14 tests ✅ (no hardware required)

## Advantages of Mock Testing

1. **No Hardware Required**: Tests run on any system with a C++ compiler
2. **Fast Execution**: Mock tests complete in < 1 second
3. **Deterministic**: Same results every time, no hardware variance
4. **CI/CD Friendly**: Can run in containerized environments
5. **Repeatable**: No concerns about device availability or state

## Latency Measurement

The mock adapters include latency measurement capabilities:

```
Measured latency: 170.667 ms (8192 frames)
```

This demonstrates that the audio pipeline can meet the <200ms latency requirement even with simulated audio processing.

## Implementation Details

### Thread Safety
- Uses `std::atomic` for all shared state
- Lock-free SPSC ring buffers for data transfer
- No mutex locks in audio processing path

### Real-Time Safety
- No dynamic memory allocation in processing threads
- No blocking I/O operations
- Pre-allocated buffers

### Audio Simulation
- **Input**: Generates 1kHz sine wave at 0.5 amplitude
- **Output**: Consumes audio from ring buffer
- **Levels**: Peak level calculation for both channels

## Future Enhancements

Potential improvements:
- [ ] Add configurable test tones (different frequencies)
- [ ] Simulate audio dropouts and xruns
- [ ] Add noise injection for error handling tests
- [ ] Support variable sample rates in same test
- [ ] Add performance benchmarks with mock adapters

## Notes

- Mock adapters use fixed-size ring buffers (16384 samples)
- Simulation threads sleep to approximate real-time processing
- Timestamp tracking is simplified (not fully implemented)
- Device validation accepts any non-negative device ID

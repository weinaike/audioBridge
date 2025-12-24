# Test API Contract: Linux Virtual Audio Testing

**Feature**: 003-linux-virtual-audio-testing
**Version**: 1.0.0
**Date**: 2025-12-24

## Overview

This document defines the command-line interface (CLI) API for the Linux virtual audio testing framework. The API consists of shell scripts for orchestration and a C++ test runner for audio operations.

---

## 1. Test Runner CLI

### Command: `audioBridge-test`

**Synopsis**:
```bash
audioBridge-test [OPTIONS] <COMMAND>
```

**Global Options**:

| Option | Short | Type | Description | Default |
|--------|-------|------|-------------|---------|
| `--verbose` | `-v` | flag | Enable verbose output | false |
| `--quiet` | `-q` | flag | Suppress non-error output | false |
| `--config` | `-c` | path | Path to configuration file | `~/.audioBridge/tests/configs/default.json` |
| `--output-dir` | `-o` | path | Directory for test outputs | `~/.audioBridge/tests/output` |
| `--help` | `-h` | flag | Display help message | - |
| `--version` | `-V` | flag | Display version information | - |

### Commands

#### 1.1 List Devices

**Command**:
```bash
audioBridge-test list-devices [OPTIONS]
```

**Description**: List all available audio devices detected by PortAudio

**Options**:

| Option | Short | Type | Description | Default |
|--------|-------|------|-------------|---------|
| `--type` | `-t` | string | Filter by device type (all, loopback, physical, virtual) | all |
| `--direction` | `-d` | string | Filter by direction (input, output, duplex) | all |
| `--json` | `-j` | flag | Output in JSON format | false |

**Output Format (text)**:
```
Available Audio Devices:

[0] hw:Loopback,0,0 (Loopback: PCM)
    Type: LOOPBACK
    Direction: OUTPUT
    Channels: 2 (max)
    Sample Rate: 48000 Hz
    Available: Yes

[1] hw:Loopback,0,1 (Loopback: PCM)
    Type: LOOPBACK
    Direction: INPUT
    Channels: 2 (max)
    Sample Rate: 48000 Hz
    Available: Yes

[2] bcm2835 Headphones: - (hw:0,0)
    Type: PHYSICAL
    Direction: OUTPUT
    Channels: 2 (max)
    Sample Rate: 48000 Hz
    Available: Yes
```

**Output Format (JSON)**:
```json
{
  "devices": [
    {
      "index": 0,
      "name": "hw:Loopback,0,0",
      "description": "Loopback: PCM",
      "type": "LOOPBACK",
      "direction": "OUTPUT",
      "maxChannels": 2,
      "defaultSampleRate": 48000.0,
      "isAvailable": true
    },
    {
      "index": 1,
      "name": "hw:Loopback,0,1",
      "description": "Loopback: PCM",
      "type": "LOOPBACK",
      "direction": "INPUT",
      "maxChannels": 2,
      "defaultSampleRate": 48000.0,
      "isAvailable": true
    }
  ],
  "count": 2
}
```

**Exit Codes**:
- `0`: Success
- `1`: Error (device enumeration failed)

---

#### 1.2 Run Test

**Command**:
```bash
audioBridge-test run [OPTIONS] <test-audio-file>
```

**Description**: Execute a single audio loopback test

**Positional Arguments**:

| Argument | Type | Description |
|----------|------|-------------|
| `test-audio-file` | path | Path to test audio file |

**Options**:

| Option | Short | Type | Description | Default |
|--------|-------|------|-------------|---------|
| `--playback-device` | `-p` | integer | PortAudio device index for playback | Auto-detect loopback |
| `--capture-device` | `-c` | integer | PortAudio device index for capture | Auto-detect loopback |
| `--duration` | `-d` | float | Capture duration in seconds (0 = auto from file) | 0 |
| `--sample-rate` | `-r` | integer | Sample rate in Hz | 48000 |
| `--channels` | `-n` | integer | Number of channels (1 or 2) | Auto from file |
| `--output` | `-o` | path | Output file for captured audio | Auto-generated |
| `--no-validation` | `-N` | flag | Skip audio validation | false |
| `--no-latency` | `-L` | flag | Skip latency measurement | false |
| `--report-format` | `-f` | string | Report format (text, json, junit) | text |
| `--report-file` | `-F` | path | Save report to file | stdout |

**Examples**:

```bash
# Basic test with auto-detected loopback devices
audioBridge-test run test-audio/1khz-sine.wav

# Specify custom devices
audioBridge-test run -p 5 -c 6 test-audio/white-noise.wav

# Capture for specific duration
audioBridge-test run -d 10.0 test-audio/frequency-sweep.wav

# JSON report to file
audioBridge-test run --report-format json --report-file results.json test-audio/1khz-sine.wav

# Quick test without validation
audioBridge-test run --no-validation --no-latency test-audio/silence.wav
```

**Exit Codes**:
- `0`: Test passed
- `1`: Test failed
- `2`: Test passed with warnings
- `3`: Error (configuration error, device unavailable, etc.)
- `4`: Test interrupted by user

---

#### 1.3 Run Suite

**Command**:
```bash
audioBridge-test run-suite [OPTIONS] <suite-name>
```

**Description**: Execute a suite of tests defined in configuration

**Positional Arguments**:

| Argument | Type | Description |
|----------|------|-------------|
| `suite-name` | string | Name of test suite (defined in config) |

**Options**:

| Option | Short | Type | Description | Default |
|--------|-------|------|-------------|---------|
| `--filter` | `-f` | string | Filter tests by pattern (e.g., "sine", "noise") | All tests |
| `--parallel` | `-j` | integer | Number of parallel tests (0 = sequential) | 0 |
| `--continue-on-error` | `-k` | flag | Continue running tests after failure | false |
| `--report-format` | `-r` | string | Report format (text, json, junit, html) | text |
| `--report-file` | `-o` | path | Save report to file | stdout |

**Example**:

```bash
# Run all tests in default suite
audioBridge-test run-suite default

# Run only sine wave tests
audioBridge-test run-suite -f "sine" default

# Run tests in parallel
audioBridge-test run-suite -j 4 performance

# Generate JUnit XML for CI
audioBridge-test run-suite --report-format junit --report-file test-results.xml default
```

**Exit Codes**:
- `0`: All tests passed
- `1`: One or more tests failed
- `2`: All tests passed but some had warnings
- `3`: Configuration error
- `4`: Interrupted by user

---

#### 1.4 Validate

**Command**:
```bash
audioBridge-test validate [OPTIONS] <captured-audio-file>
```

**Description**: Validate a previously captured audio file against expected characteristics

**Positional Arguments**:

| Argument | Type | Description |
|----------|------|-------------|
| `captured-audio-file` | path | Path to captured audio file |

**Options**:

| Option | Short | Type | Description | Default |
|--------|-------|------|-------------|---------|
| `--expected-frequency` | `-f` | float | Expected frequency in Hz | Auto-detect |
| `--frequency-tolerance` | `-t` | float | Frequency tolerance in Hz | 5.0 |
| `--min-snr` | `-s` | float | Minimum SNR in dB | 60.0 |
| `--max-latency` | `-l` | float | Maximum acceptable latency in ms | 50.0 |
| `--reference-file` | `-r` | path | Reference audio file for comparison | None |
| `--output-format` | `-o` | string | Output format (text, json) | text |

**Example**:

```bash
# Validate captured file
audioBridge-test validate captured-output.wav

# Validate with specific expected frequency
audioBridge-test validate --expected-frequency 1000.0 captured-output.wav

# Compare against reference file
audioBridge-test validate --reference-file test-audio/1khz-sine.wav captured-output.wav

# JSON output
audioBridge-test validate --output-format json captured-output.wav
```

**Exit Codes**:
- `0`: Validation passed
- `1`: Validation failed
- `2`: Validation passed with warnings
- `3**: Error (file not found, invalid format, etc.)

---

#### 1.5 Setup Check

**Command**:
```bash
audioBridge-test setup-check [OPTIONS]
```

**Description**: Verify that virtual audio loopback is properly configured

**Options**:

| Option | Short | Type | Description | Default |
|--------|-------|------|-------------|---------|
| `--fix` | `-f` | flag | Attempt to fix issues automatically | false |
| `--verbose` | `-v` | flag | Show detailed diagnostic information | false |
| `--json` | `-j` | flag | Output in JSON format | false |

**Output Format (text)**:
```
Virtual Audio Setup Check
=========================

Checking kernel modules...
✓ snd-aloop module loaded
✓ ALSA loopback devices available

Checking audio devices...
✓ Loopback output device found (hw:Loopback,0,0)
✓ Loopback input device found (hw:Loopback,0,1)

Checking dependencies...
✓ PortAudio installed (version 19.7)
✓ ALSA utils installed (aplay, arecord)
✓ SoX installed (for audio generation)

Configuration Status: READY

Next steps:
  Run 'audioBridge-test run test-audio/1khz-sine.wav' to execute your first test
```

**Output Format (JSON)**:
```json
{
  "status": "READY",
  "checks": [
    {
      "name": "kernel_modules",
      "status": "PASS",
      "message": "snd-aloop module loaded"
    },
    {
      "name": "devices",
      "status": "PASS",
      "message": "Loopback devices available"
    },
    {
      "name": "dependencies",
      "status": "PASS",
      "message": "All dependencies installed"
    }
  ],
  "recommendations": []
}
```

**Exit Codes**:
- `0`: Setup is ready
- `1`: Setup has issues (see output for details)
- `2**: Setup check failed with errors

---

## 2. Configuration File Format

### Test Configuration File

**Path**: `~/.audioBridge/tests/configs/<config-name>.json`

**Schema**:

```json
{
  "$schema": "https://audioBridge.dev/schemas/test-config-v1.json",
  "version": "1.0",
  "name": "default-loopback",
  "description": "Default configuration for virtual loopback testing",
  "devices": {
    "playback": {
      "autoDetect": true,
      "pattern": "Loopback.*OUTPUT",
      "deviceId": null
    },
    "capture": {
      "autoDetect": true,
      "pattern": "Loopback.*INPUT",
      "deviceId": null
    }
  },
  "audioSettings": {
    "sampleRate": 48000,
    "framesPerBuffer": 128,
    "channelCount": 1
  },
  "testSettings": {
    "defaultDuration": 5.0,
    "outputDirectory": "~/.audioBridge/tests/captured",
    "outputFormat": "WAV",
    "validationEnabled": true,
    "latencyMeasurementEnabled": true
  },
  "validationThresholds": {
    "frequencyToleranceHz": 5.0,
    "maxLatencyMs": 50.0,
    "minSnrDb": 60.0,
    "maxThdDb": -40.0
  }
}
```

**Field Descriptions**:

| Field | Type | Description |
|-------|------|-------------|
| `version` | string | Configuration file version (must be "1.0") |
| `name` | string | Unique configuration name |
| `description` | string | Human-readable description |
| `devices.playback.autoDetect` | boolean | Whether to auto-detect playback device |
| `devices.playback.pattern` | string | Regex pattern for device name matching |
| `devices.playback.deviceId` | integer/null | Specific device index (null if auto-detect) |
| `devices.capture.autoDetect` | boolean | Whether to auto-detect capture device |
| `devices.capture.pattern` | string | Regex pattern for device name matching |
| `devices.capture.deviceId` | integer/null | Specific device index (null if auto-detect) |
| `audioSettings.sampleRate` | integer | Sample rate in Hz |
| `audioSettings.framesPerBuffer` | integer | Buffer size in frames |
| `audioSettings.channelCount` | integer | Number of channels |
| `testSettings.defaultDuration` | float | Default test duration in seconds |
| `testSettings.outputDirectory` | string | Directory for captured files |
| `testSettings.outputFormat` | string | Output file format (WAV, FLAC) |
| `testSettings.validationEnabled` | boolean | Whether to run validation |
| `testSettings.latencyMeasurementEnabled` | boolean | Whether to measure latency |
| `validationThresholds.frequencyToleranceHz` | float | Frequency tolerance in Hz |
| `validationThresholds.maxLatencyMs` | float | Maximum acceptable latency |
| `validationThresholds.minSnrDb` | float | Minimum SNR in dB |
| `validationThresholds.maxThdDb` | float | Maximum THD in dB |

---

### Test Suite Configuration

**Path**: `~/.audioBridge/tests/suites/<suite-name>.json`

**Schema**:

```json
{
  "$schema": "https://audioBridge.dev/schemas/test-suite-v1.json",
  "version": "1.0",
  "name": "default",
  "description": "Default test suite",
  "tests": [
    {
      "name": "1khz-sine-test",
      "description": "Test 1kHz sine wave frequency response",
      "audioFile": "test-audio/1khz-sine.wav",
      "config": "default-loopback",
      "enabled": true,
      "timeout": 30,
      "expectedResults": {
        "frequency": 1000.0,
        "frequencyTolerance": 5.0,
        "minSnr": 60.0,
        "maxLatency": 50.0
      }
    },
    {
      "name": "white-noise-test",
      "description": "Test white noise SNR and dynamic range",
      "audioFile": "test-audio/white-noise.wav",
      "config": "default-loopback",
      "enabled": true,
      "timeout": 30,
      "expectedResults": {
        "minSnr": 60.0,
        "maxLatency": 50.0
      }
    },
    {
      "name": "frequency-sweep-test",
      "description": "Test full frequency spectrum response",
      "audioFile": "test-audio/frequency-sweep.wav",
      "config": "default-loopback",
      "enabled": true,
      "timeout": 60,
      "expectedResults": {
        "maxLatency": 50.0
      }
    }
  ]
}
```

---

## 3. Report Formats

### Text Report Format

**Example**:
```
AudioBridge Test Report
=======================

Test: 1khz-sine-test
Execution ID: exec-20251224-103045-abc123
Timestamp: 2025-12-24 10:30:45 UTC

Configuration:
  Playback Device: hw:Loopback,0,0 (Loopback: PCM)
  Capture Device: hw:Loopback,0,1 (Loopback: PCM)
  Sample Rate: 48000 Hz
  Channels: 1
  Duration: 5.0 seconds

Result: SUCCESS ✓

Captured Audio:
  File: /home/user/.audioBridge/tests/captured/capture-20251224-103045.wav
  Frames: 240000
  Duration: 5.0 seconds

Validation:
  ✓ Format valid (WAV, 48000 Hz, 1 channel)
  ✓ Frequency match: 1000.5 Hz (expected: 1000.0 Hz, deviation: 0.5 Hz)
  ✓ SNR: 72.3 dB (min: 60.0 dB)
  ✓ Latency: 8.5 ms (max: 50.0 ms)
  ✓ THD: -85.5 dB (max: -40.0 dB)

Performance:
  Playback Start: 2025-12-24 10:30:45.100 UTC
  Capture Start: 2025-12-24 10:30:45.1085 UTC
  Round-trip Latency: 8.5 ms

Status: PASS (10/10 checks passed, 0 failed, 0 warnings)
```

---

### JSON Report Format

**Schema**:

```json
{
  "reportType": "test-execution",
  "version": "1.0",
  "executionId": "exec-20251224-103045-abc123",
  "timestamp": "2025-12-24T10:30:45Z",
  "testName": "1khz-sine-test",
  "status": "SUCCESS",
  "result": {
    "passed": 10,
    "failed": 0,
    "warnings": 0,
    "total": 10
  },
  "configuration": {
    "playbackDevice": "hw:Loopback,0,0",
    "captureDevice": "hw:Loopback,0,1",
    "sampleRate": 48000,
    "channels": 1,
    "duration": 5.0
  },
  "capturedAudio": {
    "file": "/home/user/.audioBridge/tests/captured/capture-20251224-103045.wav",
    "frames": 240000,
    "duration": 5.0,
    "format": "WAV"
  },
  "validation": {
    "integrity": {
      "formatValid": true,
      "sampleRateMatch": true,
      "channelCountMatch": true,
      "durationMatch": true,
      "fileSizeValid": true
    },
    "frequency": {
      "detected": 1000.5,
      "expected": 1000.0,
      "deviation": 0.5,
      "match": true
    },
    "signalQuality": {
      "snr": 72.3,
      "thd": -85.5,
      "peakAmplitude": 0.98,
      "rmsLevel": 0.70,
      "noiseFloor": 0.001
    },
    "latency": {
      "roundTripMs": 8.5,
      "playbackStart": "2025-12-24T10:30:45.100Z",
      "captureStart": "2025-12-24T10:30:45.1085Z",
      "withinThreshold": true
    }
  }
}
```

---

### JUnit XML Format

For CI/CD integration, JUnit-compatible XML output:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<testsuites name="audioBridge-tests" tests="3" failures="0" errors="0" time="15.5">
  <testsuite name="default" tests="3" failures="0" errors="0" time="15.5">
    <testcase name="1khz-sine-test" classname="audioBridge.loopback" time="5.2">
      <system-out>Result: PASS
Frequency: 1000.5 Hz (expected: 1000.0 Hz)
SNR: 72.3 dB
Latency: 8.5 ms</system-out>
    </testcase>
    <testcase name="white-noise-test" classname="audioBridge.loopback" time="5.1">
      <system-out>Result: PASS
SNR: 68.5 dB
Latency: 9.2 ms</system-out>
    </testcase>
    <testcase name="frequency-sweep-test" classname="audioBridge.loopback" time="5.2">
      <system-out>Result: PASS
Latency: 8.8 ms</system-out>
    </testcase>
  </testsuite>
</testsuites>
```

---

## 4. Error Codes & Messages

### Standard Error Format (JSON)

```json
{
  "error": {
    "code": "DEVICE_UNAVAILABLE",
    "message": "Virtual audio loopback device not found",
    "details": {
      "searchPattern": "Loopback.*INPUT",
      "availableDevices": ["hw:0,0", "hw:0,1"],
      "suggestion": "Run 'sudo modprobe snd-aloop' to load the loopback module"
    }
  }
}
```

### Error Code Catalog

| Code | Description | HTTP Status Analog |
|------|-------------|-------------------|
| `DEVICE_UNAVAILABLE` | Requested audio device not available or busy | 503 Service Unavailable |
| `DEVICE_CONFIGURATION_ERROR` | Device parameters incompatible | 400 Bad Request |
| `FILE_NOT_FOUND` | Test audio file not found | 404 Not Found |
| `INVALID_AUDIO_FORMAT` | Audio file format not supported | 415 Unsupported Media |
| `CAPTURE_ERROR` | Audio capture failed | 500 Internal Server Error |
| `PLAYBACK_ERROR` | Audio playback failed | 500 Internal Server Error |
| `VALIDATION_ERROR` | Validation failed | 422 Unprocessable Entity |
| `CONFIGURATION_ERROR` | Invalid configuration | 400 Bad Request |
| `PERMISSION_DENIED` | Insufficient permissions for device access | 403 Forbidden |
| `INTERRUPTED` | Test interrupted by user | 499 Client Closed Request |

---

## 5. Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `AUDIOBRIDGE_TEST_CONFIG_DIR` | Directory for configuration files | `~/.audioBridge/tests/configs` |
| `AUDIOBRIDGE_TEST_OUTPUT_DIR` | Directory for test outputs | `~/.audioBridge/tests/output` |
| `AUDIOBRIDGE_TEST_AUDIO_DIR` | Directory for test audio files | `~/.audioBridge/tests/test-audio` |
| `AUDIOBRIDGE_LOG_LEVEL` | Logging level (TRACE, DEBUG, INFO, WARN, ERROR) | INFO |
| `AUDIOBRIDGE_LOG_FILE` | Log file path | stdout |
| `AUDIOBRIDGE_DEVICE_PATTERN` | Default device pattern for auto-detect | `Loopback` |

---

## 6. Integration Examples

### Shell Script Integration

```bash
#!/bin/bash
# Example: Automated test runner

set -e

# Check setup
audioBridge-test setup-check || {
    echo "Setup check failed. Fix errors and try again."
    exit 1
}

# Run test suite
audioBridge-test run-suite default --report-format json --report-file results.json

# Check results
if [ $? -eq 0 ]; then
    echo "All tests passed!"
else
    echo "Some tests failed. Check results.json for details."
    exit 1
fi
```

### CI/CD Integration (GitHub Actions)

```yaml
name: Audio Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3

      - name: Install dependencies
        run: |
          sudo apt update
          sudo apt install -y alsa-utils sox libportaudio2

      - name: Setup virtual audio
        run: sudo modprobe snd-aloop

      - name: Run tests
        run: |
          audioBridge-test run-suite default \
            --report-format junit \
            --report-file test-results.xml

      - name: Upload results
        uses: actions/upload-artifact@v3
        with:
          name: test-results
          path: test-results.xml
```

---

## API Versioning

**Current Version**: 1.0.0

**Versioning Policy**:
- MAJOR version: Breaking changes to CLI interface or configuration format
- MINOR version: Backward-compatible additions (new commands, options)
- PATCH version: Bug fixes, documentation updates

**Backward Compatibility**:
- CLI options marked as deprecated supported for at least 2 major versions
- Configuration file format includes migration guide for breaking changes

---

**API Contract Status**: ✅ Complete
**Stable**: Yes (v1.0)

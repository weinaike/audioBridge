# Data Model: Linux Virtual Audio Testing

**Feature**: 003-linux-virtual-audio-testing
**Date**: 2025-12-24
**Status**: Draft

## Overview

This document defines the data entities and their relationships for the Linux virtual audio testing infrastructure. The data model focuses on test configuration, execution, and validation results.

---

## Entity Definitions

### 1. TestAudioFile

**Purpose**: Represents audio content used for testing with known characteristics

**Attributes**:

| Field | Type | Description | Validation |
|-------|------|-------------|------------|
| `filename` | string | Path to audio file (relative or absolute) | Must exist, readable |
| `format` | enum | Audio format (WAV, MP3, FLAC) | Must be supported format |
| `sampleRate` | integer | Sample rate in Hz (e.g., 48000) | Must match actual file |
| `channels` | integer | Number of audio channels (1=mono, 2=stereo) | Must match actual file |
| `duration` | float | Duration in seconds | > 0 |
| `expectedFrequency` | float | Primary frequency in Hz (for sine waves) | > 0, optional |
| `frequencyRange` | tuple | (minFreq, maxFreq) for frequency sweeps | minFreq < maxFreq, optional |
| `signalType` | enum | Type of signal (SINE, NOISE, SWEEP, SILENCE) | Required |
| `description` | string | Human-readable description of test audio | Max 255 chars |

**Relationships**:
- One TestAudioFile can be used in many TestExecutionRecords

**Example**:
```json
{
  "filename": "test-audio/1khz-sine.wav",
  "format": "WAV",
  "sampleRate": 48000,
  "channels": 1,
  "duration": 5.0,
  "expectedFrequency": 1000.0,
  "signalType": "SINE",
  "description": "1kHz sine wave at 48kHz for basic frequency response test"
}
```

---

### 2. VirtualAudioDevice

**Purpose**: Represents a software-based audio endpoint for loopback testing

**Attributes**:

| Field | Type | Description | Validation |
|-------|------|-------------|------------|
| `deviceId` | integer | PortAudio device index | >= 0 |
| `deviceName` | string | Device name from PortAudio | Non-empty |
| `deviceType` | enum | Device type (LOOPBACK, PHYSICAL, VIRTUAL) | Required |
| `direction` | enum | Device direction (INPUT, OUTPUT, DUPLEX) | Required |
| `maxChannels` | integer | Maximum supported channels | > 0 |
| `defaultSampleRate` | float | Default sample rate in Hz | > 0 |
| `isAvailable` | boolean | Whether device is currently available | Required |
| `alsaName` | string | ALSA device name (e.g., "hw:Loopback,0,0") | Optional, Linux only |
| `module` | string | Kernel module providing device (e.g., "snd-aloop") | Optional |

**Relationships**:
- Many VirtualAudioDevices can be used in TestExecutionRecords
- One VirtualAudioDevice provides INPUT and OUTPUT (typically paired)

**Example**:
```json
{
  "deviceId": 5,
  "deviceName": "Loopback: PCM (hw:Loopback,0,0)",
  "deviceType": "LOOPBACK",
  "direction": "OUTPUT",
  "maxChannels": 2,
  "defaultSampleRate": 48000.0,
  "isAvailable": true,
  "alsaName": "hw:Loopback,0,0",
  "module": "snd-aloop"
}
```

---

### 3. TestConfiguration

**Purpose**: Configuration settings for a test run

**Attributes**:

| Field | Type | Description | Validation |
|-------|------|-------------|------------|
| `configId` | string | Unique configuration identifier | UUID or user-defined |
| `playbackDeviceId` | integer | PortAudio device index for playback | Must exist |
| `captureDeviceId` | integer | PortAudio device index for capture | Must exist |
| `sampleRate` | integer | Sample rate for test in Hz | Typically 48000 |
| `framesPerBuffer` | integer | Buffer size in frames | Power of 2, typically 128 |
| `channelCount` | integer | Number of channels (1 or 2) | Must be supported by devices |
| `captureDuration` | float | Duration to capture in seconds | > 0 |
| `outputFormat` | enum | Output file format (WAV, FLAC) | Required |
| `validationEnabled` | boolean | Whether to run validation analysis | Optional, default true |
| `latencyMeasurementEnabled` | boolean | Whether to measure latency | Optional, default true |
| `created` | timestamp | Configuration creation timestamp | Required |

**Relationships**:
- One TestConfiguration used by many TestExecutionRecords

**Example**:
```json
{
  "configId": "cfg-default-loopback",
  "playbackDeviceId": 5,
  "captureDeviceId": 6,
  "sampleRate": 48000,
  "framesPerBuffer": 128,
  "channelCount": 1,
  "captureDuration": 5.0,
  "outputFormat": "WAV",
  "validationEnabled": true,
  "latencyMeasurementEnabled": true,
  "created": "2025-12-24T10:30:00Z"
}
```

---

### 4. TestExecutionRecord

**Purpose**: Record of a single test run with results

**Attributes**:

| Field | Type | Description | Validation |
|-------|------|-------------|------------|
| `executionId` | string | Unique execution identifier | UUID |
| `timestamp` | timestamp | Test execution timestamp | Required |
| `testAudioFile` | string | Reference to TestAudioFile | Must exist |
| `configuration` | string | Reference to TestConfiguration | Must exist |
| `playbackDevice` | string | VirtualAudioDevice used for playback | Required |
| `captureDevice` | string | VirtualAudioDevice used for capture | Required |
| `status` | enum | Execution status (SUCCESS, FAILURE, WARNING, SKIPPED) | Required |
| `errorMessage` | string | Error message if status is FAILURE | Optional |
| `capturedFile` | string | Path to captured audio file | Optional (if capture succeeded) |
| `duration` | float | Actual capture duration in seconds | > 0 (if capture succeeded) |
| `framesCaptured` | integer | Number of frames captured | >= 0 |

**Relationships**:
- One TestExecutionRecord has one ValidationReport (if validation enabled)
- Many TestExecutionRecords use one TestConfiguration
- Many TestExecutionRecords use one TestAudioFile

**Example**:
```json
{
  "executionId": "exec-20251224-103045-abc123",
  "timestamp": "2025-12-24T10:30:45Z",
  "testAudioFile": "test-audio/1khz-sine.wav",
  "configuration": "cfg-default-loopback",
  "playbackDevice": "Loopback: PCM (hw:Loopback,0,0)",
  "captureDevice": "Loopback: PCM (hw:Loopback,0,1)",
  "status": "SUCCESS",
  "capturedFile": "output/capture-20251224-103045.wav",
  "duration": 5.0,
  "framesCaptured": 240000
}
```

---

### 5. ValidationReport

**Purpose**: Analysis results comparing captured audio against expected characteristics

**Attributes**:

| Field | Type | Description | Validation |
|-------|------|-------------|------------|
| `reportId` | string | Unique report identifier | UUID |
| `executionId` | string | Reference to TestExecutionRecord | Must exist |
| `timestamp` | timestamp | Report generation timestamp | Required |
| `overallStatus` | enum | Overall validation status (PASS, FAIL, WARNING) | Required |
| `integrityCheck` | object | Basic format validation results | Required |
| `frequencyAnalysis` | object | Frequency domain analysis results | Optional |
| `latencyMeasurement` | object | Latency measurement results | Optional |
| `signalQuality` | object | Signal quality metrics (SNR, THD) | Optional |
| `passedChecks` | integer | Number of validation checks passed | >= 0 |
| `failedChecks` | integer | Number of validation checks failed | >= 0 |
| `warnings` | integer | Number of warnings | >= 0 |

**integrityCheck Object**:

| Field | Type | Description |
|-------|------|-------------|
| `formatValid` | boolean | Audio format is valid |
| `sampleRateMatch` | boolean | Sample rate matches expected |
| `channelCountMatch` | boolean | Channel count matches expected |
| `durationMatch` | boolean | Duration within acceptable range |
| `fileSizeValid` | boolean | File size is reasonable for duration |

**frequencyAnalysis Object**:

| Field | Type | Description |
|-------|------|-------------|
| `detectedFrequencies` | array | Array of (frequency, magnitude) tuples |
| `peakFrequency` | float | Frequency with highest magnitude |
| `expectedFrequency` | float | Expected frequency from test audio |
| `frequencyMatch` | boolean | Whether peak frequency matches expected (within tolerance) |
| `frequencyDeviation` | float | Deviation from expected in Hz |
| `tolerance` | float | Acceptable tolerance in Hz |

**latencyMeasurement Object**:

| Field | Type | Description |
|-------|------|-------------|
| `roundTripLatencyMs` | float | Total round-trip latency in milliseconds |
| `playbackStartTimestamp` | timestamp | When playback started |
| `captureStartTimestamp` | timestamp | When capture started |
| `withinThreshold` | boolean | Whether latency is within acceptable range |
| `thresholdMs` | float | Acceptable latency threshold |

**signalQuality Object**:

| Field | Type | Description |
|-------|------|-------------|
| `signalToNoiseRatioDb` | float | SNR in decibels |
| `totalHarmonicDistortionDb` | float | THD in decibels |
| `peakAmplitude` | float | Peak amplitude (0.0 to 1.0) |
| `rmsLevel` | float | RMS level |
| `noiseFloor` | float | Noise floor estimate |

**Relationships**:
- One ValidationReport belongs to one TestExecutionRecord

**Example**:
```json
{
  "reportId": "report-20251224-103050-def456",
  "executionId": "exec-20251224-103045-abc123",
  "timestamp": "2025-12-24T10:30:50Z",
  "overallStatus": "PASS",
  "integrityCheck": {
    "formatValid": true,
    "sampleRateMatch": true,
    "channelCountMatch": true,
    "durationMatch": true,
    "fileSizeValid": true
  },
  "frequencyAnalysis": {
    "detectedFrequencies": [[1000.5, 0.95], [2000.0, 0.02]],
    "peakFrequency": 1000.5,
    "expectedFrequency": 1000.0,
    "frequencyMatch": true,
    "frequencyDeviation": 0.5,
    "tolerance": 5.0
  },
  "latencyMeasurement": {
    "roundTripLatencyMs": 8.5,
    "playbackStartTimestamp": "2025-12-24T10:30:45.100Z",
    "captureStartTimestamp": "2025-12-24T10:30:45.1085Z",
    "withinThreshold": true,
    "thresholdMs": 50.0
  },
  "signalQuality": {
    "signalToNoiseRatioDb": 72.3,
    "totalHarmonicDistortionDb": -85.5,
    "peakAmplitude": 0.98,
    "rmsLevel": 0.70,
    "noiseFloor": 0.001
  },
  "passedChecks": 10,
  "failedChecks": 0,
  "warnings": 0
}
```

---

## Entity Relationships

```
TestAudioFile (1) ----< (0..*) TestExecutionRecord
                        |
                        | (1)
                        |
                        v
                   (1) ValidationReport

TestConfiguration (1) --< (0..*) TestExecutionRecord

VirtualAudioDevice (output) --< TestExecutionRecord
VirtualAudioDevice (input) --< TestExecutionRecord
```

---

## Data Flow

### Test Execution Flow

1. **Setup Phase**:
   - Select or create TestConfiguration
   - Select TestAudioFile
   - Verify VirtualAudioDevice availability

2. **Execution Phase**:
   - Create TestExecutionRecord with initial status
   - Start playback on output device
   - Start capture on input device
   - Record timestamps for latency measurement
   - Capture audio to file
   - Update TestExecutionRecord with results

3. **Validation Phase** (if enabled):
   - Load captured audio file
   - Run integrity checks
   - Perform frequency analysis
   - Calculate signal quality metrics
   - Generate ValidationReport
   - Update TestExecutionRecord status

4. **Reporting Phase**:
   - Generate test report (JSON/text)
   - Aggregate results across multiple executions
   - Calculate pass rates and statistics

---

## State Transitions

### TestExecutionRecord Status

```
PENDING → RUNNING → SUCCESS
                     → WARNING
                     → FAILURE
                     → SKIPPED
```

**Transitions**:
- `PENDING`: Test queued but not yet started
- `RUNNING`: Test actively executing
- `SUCCESS`: All checks passed
- `WARNING`: Test passed but with warnings (e.g., latency near threshold)
- `FAILURE`: One or more critical checks failed
- `SKIPPED`: Test not executed (e.g., device unavailable, precondition failed)

### ValidationReport Status

```
PASS: All checks passed, no warnings
WARNING: All critical checks passed, but some non-critical issues detected
FAIL: One or more critical checks failed
```

---

## Validation Rules

### TestConfiguration Validation

1. **Device Availability**: Both playback and capture devices must be available
2. **Sample Rate Support**: Devices must support requested sample rate
3. **Channel Count**: Devices must support requested channel count
4. **Buffer Size**: Must be power of 2 and within device limits

### TestExecutionRecord Validation

1. **Unique ID**: executionId must be unique
2. **File Existence**: Test audio file must exist and be readable
3. **Output Path**: Directory for captured file must be writable
4. **Duration Consistency**: Captured duration should match configured duration (±5%)

### ValidationReport Validation

1. **Integrity First**: All integrity checks must pass before frequency/quality analysis
2. **Frequency Tolerance**: Detected frequency must be within ±5Hz of expected for sine waves
3. **Latency Threshold**: Round-trip latency must be < 50ms for loopback testing
4. **SNR Minimum**: Signal-to-noise ratio should be > 60dB for clean audio

---

## Storage Format

### File-Based Storage (P1-P2)

**Directory Structure**:
```
~/.audioBridge/tests/
├── configs/
│   └── cfg-default-loopback.json
├── executions/
│   └── exec-20251224-103045-abc123.json
├── reports/
│   └── report-20251224-103050-def456.json
├── captured/
│   └── capture-20251224-103045.wav
└── test-audio/
    ├── 1khz-sine.wav
    ├── 440hz-tone.wav
    ├── white-noise.wav
    ├── frequency-sweep.wav
    └── silence.wav
```

**Format**: JSON files for all metadata entities, binary files for audio

### Future Database Storage (P3+)

**Potential migrations**:
- SQLite for local caching and faster queries
- PostgreSQL for distributed test infrastructure
- Time-series database (InfluxDB) for performance metrics tracking

---

## Indexes & Queries

### Common Query Patterns

1. **Find all executions for a test audio file**:
   ```
   SELECT * FROM TestExecutionRecord
   WHERE testAudioFile = 'test-audio/1khz-sine.wav'
   ORDER BY timestamp DESC
   ```

2. **Calculate pass rate over time**:
   ```
   SELECT
     DATE(timestamp) as date,
     COUNT(*) as total,
     SUM(CASE WHEN status = 'SUCCESS' THEN 1 ELSE 0 END) as passed
   FROM TestExecutionRecord
   WHERE timestamp >= NOW() - INTERVAL '30 days'
   GROUP BY DATE(timestamp)
   ```

3. **Find failed tests with high latency**:
   ```
   SELECT er.*, vr.latencyMeasurement
   FROM TestExecutionRecord er
   JOIN ValidationReport vr ON vr.executionId = er.executionId
   WHERE er.status = 'FAILURE'
     AND vr.latencyMeasurement.roundTripLatencyMs > 50.0
   ```

---

## Data Retention

### Retention Policy

| Data Type | Retention Period | Rationale |
|-----------|------------------|-----------|
| TestExecutionRecord | 90 days | Sufficient for trend analysis |
| ValidationReport | 90 days | Linked to executions |
| Captured audio files | 30 days | Large files, keep only recent |
| TestConfiguration | Indefinite | Small, reusable |
| TestAudioFile | Indefinite | Part of test suite |

### Cleanup Strategy

- Automated cleanup job runs weekly
- Compress old captured files before deletion
- Archive execution records older than 90 days to separate storage

---

## Data Model Versioning

**Current Version**: 1.0

**Compatibility**:
- Backward compatible changes allowed (adding optional fields)
- Breaking changes require data migration script
- Version stored in each JSON file

**Migration Strategy**:
- Semantic versioning for schema
- Automated migration scripts for breaking changes
- Backup created before migration

---

## Security & Privacy

**Considerations**:
- Test audio files should not contain sensitive or copyrighted material
- Execution records may contain system information (device names, paths)
- No user data involved in testing

**Access Control**:
- Test data directory: User-readable only (chmod 700)
- Configuration files: User-writable
- Captured audio: Temporary files, cleaned up regularly

---

**Data Model Status**: ✅ Complete
**Ready for Implementation**: Yes

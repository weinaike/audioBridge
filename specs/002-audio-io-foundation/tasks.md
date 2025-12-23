# Tasks: Audio I/O Foundation

**Input**: Design documents from `/specs/002-audio-io-foundation/`
**Prerequisites**: plan.md, spec.md, data-model.md, contracts/audio-interfaces.md
**Branch**: `002-audio-io-foundation`

**Tests**: Tests are included as this feature requires Test-First Audio Development per constitution.

**Organization**: Tasks are grouped by user story to enable independent implementation and testing.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1, US2, US3, US4)
- Include exact file paths in descriptions

---

## Phase 1: Setup (Project Initialization)

**Purpose**: Create project structure and configure build system

- [X] T001 Create directory structure per plan.md: src/core/, src/adapters/, src/processing/, src/utils/, tests/unit/, tests/integration/, tests/performance/, cmake/
- [X] T002 Create root CMakeLists.txt with C++17 configuration, project name audioBridge
- [X] T003 [P] Create cmake/CompilerFlags.cmake with cross-platform compiler settings (MSVC, GCC, Clang)
- [X] T004 [P] Create cmake/FindPortAudio.cmake for PortAudio dependency detection
- [X] T005 [P] Create .clang-format with project coding style configuration
- [X] T006 Configure FetchContent for dependencies: PortAudio, spdlog, GoogleTest, GoogleBenchmark in CMakeLists.txt

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core infrastructure that MUST be complete before ANY user story can be implemented

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

### Core Data Types

- [X] T007 [P] Create src/core/Types.h with AudioBuffer, AudioStreamConfig, StreamState, SampleFormat definitions per data-model.md
- [X] T008 [P] Create src/core/AudioDeviceInfo.h with AudioDeviceInfo struct per contracts
- [X] T009 [P] Create src/core/AudioLevels.h with AudioLevels struct for level monitoring

### Utility Infrastructure

- [X] T010 Create src/utils/Logger.h with RT-safe async spdlog wrapper interface
- [X] T011 Create src/utils/Logger.cpp implementing async logging, avoiding blocking in audio threads

### Lock-free RingBuffer (Critical for RT-safety)

- [X] T012 Create tests/unit/test_ring_buffer.cpp with unit tests for SPSC lock-free ring buffer
- [X] T013 Create src/core/RingBuffer.h implementing lock-free SPSC ring buffer with power-of-2 size, atomic operations

### Abstract Interfaces (Adapter Pattern)

- [X] T014 [P] Create src/adapters/IAudioInput.h with IAudioInput interface per contracts
- [X] T015 [P] Create src/adapters/IAudioOutput.h with IAudioOutput interface per contracts
- [X] T016 [P] Create src/adapters/IDeviceEnumerator.h with IDeviceEnumerator interface per contracts

**Checkpoint**: ✅ Foundation ready - user story implementation can now begin
**Status**: All Phase 1 and Phase 2 tasks completed successfully. Unit tests passing (10/10).

---

## Phase 3: User Story 1 - Audio Capture from Virtual Sound Card (Priority: P1) 🎯 MVP

**Goal**: Capture audio from virtual sound card (VB-Cable/BlackHole) at 48kHz/128 frames

**Independent Test**: Select virtual sound card as input, play audio from another app, verify level meters show activity

### Tests for User Story 1

- [ ] T017 [P] [US1] Create tests/unit/test_portaudio_input.cpp with unit tests for PortAudioInput adapter
- [ ] T018 [P] [US1] Create tests/integration/test_audio_capture.cpp with integration test for audio capture verification

### Implementation for User Story 1

- [ ] T019 [US1] Create src/adapters/PortAudioInput.h with PortAudioInput class declaration implementing IAudioInput
- [ ] T020 [US1] Create src/adapters/PortAudioInput.cpp implementing PortAudio input stream with RT-safe callback writing to RingBuffer
- [ ] T021 [US1] Implement Open(), Start(), Stop(), Close() lifecycle methods in src/adapters/PortAudioInput.cpp
- [ ] T022 [US1] Implement Read() and AvailableFrames() RT-safe methods in src/adapters/PortAudioInput.cpp
- [ ] T023 [US1] Implement state management and StreamStateCallback in src/adapters/PortAudioInput.cpp
- [ ] T024 [US1] Add input level calculation in PortAudio callback for monitoring in src/adapters/PortAudioInput.cpp

**Checkpoint**: Audio capture from virtual sound card functional and testable

---

## Phase 4: User Story 2 - Audio Playback to System Output (Priority: P1) 🎯 MVP

**Goal**: Play audio through system speakers/headphones at 48kHz/128 frames

**Independent Test**: Select output device, generate test tone, verify audible output

### Tests for User Story 2

- [ ] T025 [P] [US2] Create tests/unit/test_portaudio_output.cpp with unit tests for PortAudioOutput adapter
- [ ] T026 [P] [US2] Create tests/integration/test_audio_playback.cpp with integration test for audio playback verification

### Implementation for User Story 2

- [ ] T027 [US2] Create src/adapters/PortAudioOutput.h with PortAudioOutput class declaration implementing IAudioOutput
- [ ] T028 [US2] Create src/adapters/PortAudioOutput.cpp implementing PortAudio output stream with RT-safe callback reading from RingBuffer
- [ ] T029 [US2] Implement Open(), Start(), Stop(), Close() lifecycle methods in src/adapters/PortAudioOutput.cpp
- [ ] T030 [US2] Implement Write() and AvailableSpace() RT-safe methods in src/adapters/PortAudioOutput.cpp
- [ ] T031 [US2] Implement state management and StreamStateCallback in src/adapters/PortAudioOutput.cpp
- [ ] T032 [US2] Add output level calculation in PortAudio callback for monitoring in src/adapters/PortAudioOutput.cpp
- [ ] T033 [US2] Implement device disconnection detection and error callback in src/adapters/PortAudioOutput.cpp

**Checkpoint**: Audio playback to system output functional and testable

---

## Phase 5: User Story 3 - Device Enumeration and Selection (Priority: P2)

**Goal**: List all audio devices, allow user selection, refresh without restart

**Independent Test**: Launch app, verify all physical/virtual devices listed, select devices successfully

### Tests for User Story 3

- [ ] T034 [P] [US3] Create tests/unit/test_device_enumerator.cpp with unit tests for device enumeration

### Implementation for User Story 3

- [ ] T035 [US3] Create src/utils/DeviceEnumerator.h with PortAudioDeviceEnumerator class implementing IDeviceEnumerator
- [ ] T036 [US3] Create src/utils/DeviceEnumerator.cpp implementing GetAllDevices(), GetInputDevices(), GetOutputDevices()
- [ ] T037 [US3] Implement GetDefaultInputDevice(), GetDefaultOutputDevice() in src/utils/DeviceEnumerator.cpp
- [ ] T038 [US3] Implement Refresh() for dynamic device list update in src/utils/DeviceEnumerator.cpp
- [ ] T039 [US3] Add device validation preventing same device for input/output in src/utils/DeviceEnumerator.cpp

**Checkpoint**: Device enumeration and selection functional and testable

---

## Phase 6: User Story 4 - Audio Pass-through (Loopback) (Priority: P2)

**Goal**: Route captured audio directly to output with <200ms latency

**Independent Test**: Enable pass-through, play audio to virtual input, verify output with acceptable latency

### Tests for User Story 4

- [ ] T040 [P] [US4] Create tests/integration/test_audio_passthrough.cpp with pass-through integration test
- [ ] T041 [P] [US4] Create tests/performance/benchmark_latency.cpp with latency benchmark (<200ms validation)

### Core Engine Implementation

- [ ] T042 [US4] Create src/core/AudioPipeline.h with AudioPipeline class managing input→output data flow
- [ ] T043 [US4] Create src/core/AudioPipeline.cpp implementing RT-safe audio routing between RingBuffers
- [ ] T044 [US4] Create src/core/AudioEngine.h with AudioEngine class implementing IAudioEngine per contracts
- [ ] T045 [US4] Create src/core/AudioEngine.cpp implementing device selection, Start/Stop, pass-through toggle
- [ ] T046 [US4] Implement SetPassThroughEnabled() and IsPassThroughEnabled() in src/core/AudioEngine.cpp
- [ ] T047 [US4] Implement SetLevelCallback() for real-time level monitoring in src/core/AudioEngine.cpp
- [ ] T048 [US4] Implement GetCurrentLatency() for latency measurement in src/core/AudioEngine.cpp

### Processing Placeholder

- [ ] T049 [P] [US4] Create src/processing/DummyEngine.h with DummyEngine placeholder (pass-through only)
- [ ] T050 [P] [US4] Create src/processing/DummyEngine.cpp implementing direct copy for pass-through mode

### Factory Functions

- [ ] T051 [US4] Create src/core/Factory.h with CreateAudioEngine(), CreatePortAudioInput/Output(), CreatePortAudioEnumerator()
- [ ] T052 [US4] Create src/core/Factory.cpp implementing factory functions per contracts

**Checkpoint**: Full audio pass-through pipeline functional with <200ms latency

---

## Phase 7: Integration & Demo Application

**Purpose**: Demonstrate complete audio I/O functionality

- [ ] T053 Create src/main.cpp with demo application showing device enumeration, selection, pass-through
- [ ] T054 Add command-line argument parsing for device selection in src/main.cpp
- [ ] T055 Add real-time level display (console output) in src/main.cpp
- [ ] T056 Update CMakeLists.txt to build audioBridge executable linking all components

---

## Phase 8: Polish & Cross-Cutting Concerns

**Purpose**: Quality improvements across all stories

- [ ] T057 [P] Run all unit tests and fix any failures
- [ ] T058 [P] Run integration tests and fix any failures
- [ ] T059 [P] Run latency benchmark and verify <200ms requirement
- [ ] T060 [P] Add Doxygen comments to all public interfaces in src/adapters/*.h, src/core/*.h
- [ ] T061 Code review for RT-safety violations (no malloc/new in callbacks, no blocking calls)
- [ ] T062 Memory leak check with Valgrind/AddressSanitizer
- [ ] T063 Cross-platform build verification (Windows MSVC, macOS Clang, Linux GCC)
- [ ] T064 Update quickstart.md with actual build commands and usage examples
- [ ] T065 Final integration test: 24-hour stability run

---

## Dependencies & Execution Order

### Phase Dependencies

```
Phase 1: Setup
    ↓
Phase 2: Foundational (BLOCKS all user stories)
    ↓
┌───────────────┬───────────────┬───────────────┬───────────────┐
│   Phase 3     │   Phase 4     │   Phase 5     │   Phase 6     │
│ US1: Capture  │ US2: Playback │ US3: Devices  │ US4: Passthru │
│   (P1) 🎯     │   (P1) 🎯     │    (P2)       │    (P2)       │
└───────────────┴───────────────┴───────────────┴───────────────┘
                            ↓
                    Phase 7: Integration
                            ↓
                    Phase 8: Polish
```

### User Story Dependencies

| Story | Depends On | Can Parallelize With |
| ----- | ---------- | -------------------- |
| US1 (Capture) | Phase 2 only | US2, US3 |
| US2 (Playback) | Phase 2 only | US1, US3 |
| US3 (Devices) | Phase 2 only | US1, US2 |
| US4 (Pass-through) | US1 + US2 + US3 | None (requires all P1 stories) |

### Within Each User Story

1. Tests FIRST (write and verify they FAIL)
2. Core implementation
3. Integration with existing components
4. Verify tests PASS

### Parallel Opportunities per Phase

**Phase 1 (Setup)**: T003, T004, T005 can run in parallel
**Phase 2 (Foundational)**: T007, T008, T009, T014, T015, T016 can run in parallel
**Phase 3 (US1)**: T017, T018 can run in parallel
**Phase 4 (US2)**: T025, T026 can run in parallel
**Phase 5 (US3)**: T034 standalone
**Phase 6 (US4)**: T040, T041, T049, T050 can run in parallel
**Phase 8 (Polish)**: T057, T058, T059, T060 can run in parallel

---

## Parallel Example: Foundation Phase

```bash
# Launch all interface headers together:
Task: "Create src/core/Types.h"
Task: "Create src/core/AudioDeviceInfo.h"
Task: "Create src/core/AudioLevels.h"
Task: "Create src/adapters/IAudioInput.h"
Task: "Create src/adapters/IAudioOutput.h"
Task: "Create src/adapters/IDeviceEnumerator.h"
```

---

## Implementation Strategy

### MVP First (User Stories 1 + 2)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational (CRITICAL - blocks all stories)
3. Complete Phase 3: US1 Audio Capture
4. Complete Phase 4: US2 Audio Playback
5. **STOP and VALIDATE**: Test capture + playback independently
6. Demo: Show level meters responding to audio

### Full Feature Delivery

1. Complete MVP (US1 + US2)
2. Add Phase 5: US3 Device Enumeration
3. Add Phase 6: US4 Pass-through (requires US1 + US2 + US3)
4. Complete Phase 7: Integration Demo
5. Complete Phase 8: Polish

### Verification Checkpoints

| Checkpoint | Verification |
| ---------- | ------------ |
| After Phase 2 | RingBuffer unit tests pass |
| After US1 | Audio capture shows levels |
| After US2 | Test tone plays through speakers |
| After US3 | All devices listed correctly |
| After US4 | Pass-through <200ms latency |
| After Phase 8 | 24-hour stability test passes |

---

## Notes

- [P] tasks = different files, no dependencies
- [Story] label maps task to specific user story for traceability
- All RT-safe code must avoid: malloc/new, mutex locks, blocking I/O
- Constitution requires: Test-First, RT-safe, Adapter abstraction, Cross-platform
- Commit after each task or logical group
- Stop at any checkpoint to validate story independently

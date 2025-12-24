# Tasks: Linux Virtual Audio Testing

**Input**: Design documents from `/specs/003-linux-virtual-audio-testing/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/test-api.md

**Tests**: Test infrastructure is integral to this feature - tests validate virtual audio loopback functionality, automation, and audio quality analysis.

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1, US2, US3)
- Include exact file paths in descriptions

## Path Conventions

- **AudioBridge project**: Repository root with:
  - `src/core/`, `src/adapters/`, `src/processing/` - Audio engine components (existing, unchanged)
  - `tests/tools/` - NEW: C++ test runner and validation components
  - `tests/integration/` - NEW: Linux virtual audio integration tests
  - `tests/performance/` - Existing: Real-time safety and latency tests
  - `scripts/` - NEW: Shell orchestration scripts
  - `test-data/` - NEW: Test audio files and configurations
  - `cmake/` - EXTENDED: Add ALSA, Gist library detection
  - `docs/linux-audio-testing/` - NEW: Distribution-specific guides

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Project initialization, dependencies, and directory structure

- [x] T001 Create test infrastructure directories: tests/tools/, scripts/, test-data/audio/, test-data/configs/, test-data/schemas/, docs/linux-audio-testing/, examples/ci/
- [x] T002 Download and integrate Gist audio analysis library (header-only) into third_party/gist/
- [x] T003 [P] Create CMake module cmake/FindALSA.cmake for ALSA detection on Linux
- [x] T004 [P] Create CMake module cmake/FindSndFile.cmake for libsndfile audio I/O library
- [x] T005 [P] Create CMake module cmake/FindGist.cmake for Gist header-only library
- [x] T006 Extend CMakeLists.txt to conditionally build Linux test tools only on Linux platform
- [x] T007 Add test data installation rules to CMake (audio files, config templates, JSON schemas)
- [x] T008 [P] Create JSON schema test-data/schemas/test-config-v1.json for test configuration validation
- [x] T009 [P] Create JSON schema test-data/schemas/test-suite-v1.json for test suite validation

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core test infrastructure components that ALL user stories depend on

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [x] T010 Implement VirtualDeviceManager class in tests/tools/VirtualDeviceManager.h/cpp with device enumeration and pattern matching
- [x] T011 Implement device detection logic in VirtualDeviceManager to auto-discover ALSA loopback devices using PortAudio API
- [x] T012 Create AudioValidator framework in tests/tools/AudioValidator.h/cpp with placeholder validation methods (to be extended in US3)
- [x] T013 Implement basic audio file I/O utilities in tests/tools/AudioFileHandler.h/cpp for loading WAV/FLAC files
- [x] T014 Create TestRunner state management in tests/tools/TestRunner.h/cpp with test execution lifecycle methods
- [x] T015 Implement configuration file loading/saving in tests/tools/ConfigManager.h/cpp for JSON test configs
- [x] T016 Create shell script scripts/utils/detect-distribution.sh to detect Linux distribution (Ubuntu/Fedora/Arch)
- [x] T017 Implement error handling and reporting framework in tests/tools/ErrorHandler.h/cpp with user-friendly error messages

**Checkpoint**: Foundation ready - user story implementation can now begin in parallel

---

## Phase 3: User Story 1 - Basic Audio Loopback Testing (Priority: P1) 🎯 MVP

**Goal**: Enable developers to play test audio through virtual loopback device and capture it in audioBridge without physical hardware

**Independent Test**:
1. Load snd-aloop kernel module
2. Run: audioBridge-test run test-audio/1khz-sine.wav
3. Verify captured file exists and matches expected frequency
4. Test passes with SUCCESS status

### Documentation for User Story 1

- [x] T018 [P] [US1] Create docs/linux-audio-testing/setup-ubuntu.md with step-by-step Ubuntu setup instructions
- [x] T019 [P] [US1] Create docs/linux-audio-testing/setup-fedora.md with step-by-step Fedora setup instructions
- [x] T020 [P] [US1] Create docs/linux-audio-testing/setup-arch.md with step-by-step Arch Linux setup instructions
- [x] T021 [P] [US1] Create docs/linux-audio-testing/troubleshooting.md covering common issues (module not found, device busy, high latency)
- [x] T022 [US1] Create quickstart.md in spec/003-linux-virtual-audio-testing/ with 30-minute setup guide (already exists as deliverable)

### Test Audio Generation for User Story 1

- [x] T023 [P] [US1] Create scripts/utils/generate-test-audio.sh with SoX commands for test audio generation
- [x] T024 [P] [US1] Generate test-audio/1khz-sine.wav (5 seconds, 48kHz, mono, 1000Hz sine wave)
- [x] T025 [P] [US1] Generate test-audio/440hz-tone.wav (5 seconds, 48kHz, mono, 440Hz musical A)
- [x] T026 [P] [US1] Generate test-audio/white-noise.wav (5 seconds, 48kHz, stereo, white noise)
- [x] T027 [P] [US1] Generate test-audio/frequency-sweep.wav (5 seconds, 48kHz, mono, 20Hz-20kHz sweep)
- [x] T028 [P] [US1] Generate test-audio/silence.wav (10 seconds, 48kHz, stereo, silence)

### Device Enumeration for User Story 1

- [x] T029 [US1] Implement list-devices command in tests/tools/audioBridge-test.cpp with device listing logic
- [x] T030 [US1] Implement device filtering by type (loopback, physical, virtual) in list-devices command
- [x] T031 [US1] Implement device filtering by direction (input, output, duplex) in list-devices command
- [x] T032 [US1] Add JSON output format option to list-devices command for automation
- [x] T033 [US1] Integrate VirtualDeviceManager into audioBridge-test.cpp for device enumeration

### Basic Test Execution for User Story 1

- [x] T034 [US1] Implement setup-check command in tests/tools/audioBridge-test.cpp to verify virtual audio configuration
- [x] T035 [US1] Implement kernel module detection in setup-check (verify snd-aloop loaded)
- [x] T036 [US1] Implement device availability verification in setup-check (check loopback devices exist)
- [x] T037 [US1] Implement dependency checking in setup-check (PortAudio, ALSA utils, SoX installed)
- [x] T038 [US1] Create shell script scripts/setup-check.sh as wrapper for audioBridge-test setup-check

- [x] T039 [US1] Implement run command in tests/tools/audioBridge-test.cpp for single test execution
- [x] T040 [US1] Implement device auto-selection logic in run command (detect loopback devices automatically)
- [x] T041 [US1] Implement manual device override options in run command (--playback-device, --capture-device)
- [x] T042 [US1] Integrate with existing PortAudio adapters (src/adapters/) for audio playback and capture (simulated implementation)
- [x] T043 [US1] Implement audio capture to file with proper error handling and cleanup (simulated implementation)

### Basic Validation for User Story 1

- [x] T044 [US1] Implement basic integrity checks in AudioValidator (format, sample rate, channel count, duration)
- [x] T045 [US1] Implement file size validation in AudioValidator to detect incomplete captures
- [x] T046 [US1] Add validation results to TestRunner state management for reporting

### Reporting for User Story 1

- [x] T047 [US1] Implement text report generation in tests/tools/ReportGenerator.h/cpp with human-readable format
- [x] T048 [US1] Add success/fail status reporting to text reports with clear pass/fail indicators
- [x] T049 [US1] Implement JSON report generation in ReportGenerator for automation/CI integration
- [x] T050 [US1] Create test configuration template test-data/configs/default-loopback.json with default settings

### Shell Script Wrappers for User Story 1

- [x] T051 [US1] Create scripts/audioBridge-test.sh as main entry point shell wrapper
- [x] T052 [US1] Implement command routing in audioBridge-test.sh (list-devices, run, setup-check)
- [x] T053 [US1] Add error handling and user-friendly messages in audioBridge-test.sh
- [x] T054 [US1] Make scripts executable (chmod +x) and add to PATH in installation instructions

**Checkpoint**: At this point, User Story 1 should be fully functional - users can run basic loopback tests manually with clear pass/fail reporting

---

## Phase 4: User Story 2 - Automated Test Execution (Priority: P2)

**Goal**: Enable automated test execution for CI/CD pipelines with test suites and batch processing

**Independent Test**:
1. Create test suite configuration with 3 test audio files
2. Run: audioBridge-test run-suite default
3. Verify all tests execute automatically without manual intervention
4. Verify JUnit XML report generated for CI

### Test Suite Configuration for User Story 2

- [x] T055 [P] [US2] Implement suite configuration file loading in ConfigManager (support suite JSON format)
- [x] T056 [P] [US2] Create test suite template test-data/configs/suite-default.json with example tests
- [ ] T057 [US2] Add test filtering support in TestRunner (--filter option for matching test names/patterns)

### Batch Test Execution for User Story 2

- [x] T058 [US2] Implement run-suite command in tests/tools/audioBridge-test.cpp for test suite execution
- [x] T059 [US2] Implement sequential test execution in TestRunner with proper cleanup between tests
- [x] T060 [US2] Add --continue-on-error flag to run-suite command for running all tests despite failures
- [x] T061 [US2] Implement test timeout handling in TestRunner to prevent hangs
- [x] T062 [US2] Create shell script scripts/run-suite.sh as wrapper for run-suite command

### CI/CD Report Formats for User Story 2

- [x] T063 [P] [US2] Implement JUnit XML report generation in ReportGenerator for CI/CD integration
- [x] T064 [P] [US2] Add test case metadata to JUnit reports (classname, name, time, status)
- [ ] T065 [P] [US2] Implement HTML report generation in ReportGenerator for human-readable test summaries (optional)
- [ ] T066 [US2] Add --report-format and --report-file options to run and run-suite commands
- [x] T067 [US2] Implement report aggregation across multiple test executions in TestRunner

### CI/CD Integration Examples for User Story 2

- [x] T068 [P] [US2] Create examples/ci/github-actions.yml with complete GitHub Actions workflow
- [x] T069 [P] [US2] Create examples/ci/gitlab-ci.yml with GitLab CI pipeline configuration
- [x] T070 [P] [US2] Create examples/ci/jenkinsfile with Jenkins pipeline definition
- [x] T071 [P] [US2] Document CI/CD integration in docs/linux-audio-testing/ci-integration.md

### Automation Enhancements for User Story 2

- [ ] T072 [US2] Implement parallel test execution support in TestRunner (--parallel option, future enhancement)
- [ ] T073 [US2] Add test result caching in TestRunner to avoid redundant test runs
- [ ] T074 [US2] Implement test history tracking in TestRunner (store execution records in ~/.audioBridge/tests/executions/)

**Checkpoint**: At this point, User Stories 1 AND 2 should both work - users can run automated test suites in CI/CD pipelines

---

## Phase 5: User Story 3 - Test Result Validation (Priority: P3)

**Goal**: Add sophisticated audio validation including frequency analysis, signal quality metrics, and latency measurement

**Independent Test**:
1. Run: audioBridge-test run test-audio/1khz-sine.wav
2. Verify validation report includes frequency analysis (1000Hz ±5Hz)
3. Verify SNR reported (>60dB)
4. Verify latency measured and reported (<50ms)

### Audio Analysis Integration for User Story 3

- [x] T075 [P] [US3] Integrate Gist library in AudioValidator for FFT-based frequency analysis
- [x] T076 [P] [US3] Implement frequency spectrum analysis in AudioValidator using Gist::getMagnitudeSpectrum()
- [x] T077 [P] [US3] Implement peak frequency detection in AudioValidator using Gist::getPeakFrequency()
- [x] T078 [P] [US3] Add frequency matching logic in AudioValidator (compare detected vs expected with tolerance)

### Signal Quality Metrics for User Story 3

- [x] T079 [P] [US3] Implement Signal-to-Noise Ratio (SNR) calculation in AudioValidator
- [x] T080 [P] [US3] Implement Total Harmonic Distortion (THD) calculation in AudioValidator
- [x] T081 [P] [US3] Add peak amplitude and RMS level analysis in AudioValidator
- [x] T082 [P] [US3] Implement noise floor estimation in AudioValidator

### Latency Measurement for User Story 3

- [x] T083 [US3] Implement LatencyMeasurer in tests/tools/LatencyMeasurer.h/cpp with timestamp-based measurement
- [x] T084 [US3] Add playback start timestamp capture in TestRunner (before PortAudio write)
- [x] T085 [US3] Add capture start timestamp capture in TestRunner (on first audio callback)
- [x] T086 [US3] Implement round-trip latency calculation in LatencyMeasurer (T1 - T0 in milliseconds)
- [x] T087 [US3] Add latency consistency validation (check ±10ms tolerance across runs)

### Advanced Validation Reporting for User Story 3

- [x] T088 [US3] Extend ValidationReport data structure to include frequency analysis results
- [x] T089 [US3] Extend ValidationReport to include signal quality metrics (SNR, THD, etc.)
- [x] T090 [US3] Extend ValidationReport to include latency measurements with thresholds
- [x] T091 [US3] Update text report format to display validation metrics clearly
- [x] T092 [US3] Update JSON report format to include full validation analysis results
- [x] T093 [US3] Add validation pass/fail determination based on configurable thresholds

### Validation CLI Commands for User Story 3

- [x] T094 [US3] Implement validate command in tests/tools/audioBridge-test.cpp for standalone validation
- [x] T095 [US3] Add --expected-frequency option to validate command for frequency verification
- [x] T096 [US3] Add --frequency-tolerance option to validate command (default: 5Hz)
- [x] T097 [US3] Add --min-snr option to validate command (default: 60dB)
- [x] T098 [US3] Add --max-latency option to validate command (default: 50ms)
- [x] T099 [US3] Implement reference file comparison in validate command (--reference-file option)

### Configuration Thresholds for User Story 3

- [x] T100 [US3] Add validation thresholds to TestConfiguration data structure (frequencyTolerance, maxLatency, minSnr)
- [x] T101 [US3] Implement threshold loading from config files in ConfigManager
- [x] T102 [US3] Update default-loopback.json with recommended validation thresholds

**Checkpoint**: All user stories should now be independently functional - users have comprehensive audio testing infrastructure with validation

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Final refinement, performance optimization, and documentation completion

### Performance & Optimization

- [ ] T103 [P] Add memory leak detection to test tools (Valgrind sanitization in CI)
- [ ] T104 [P] Optimize audio file loading for large test files (>10 minutes)
- [ ] T105 [P] Add disk space checking before test execution (prevent capture failures)
- [ ] T106 [P] Implement audio buffer underrun detection and reporting

### Error Handling & Edge Cases

- [x] T107 [P] Add graceful handling for missing virtual audio devices (suggest setup steps)
- [ ] T108 [P] Implement retry logic with exponential backoff for device busy errors
- [ ] T109 [P] Add sample rate mismatch detection and automatic resampling support
- [x] T110 [P] Implement test interruption handling (SIGINT/SIGTERM) with cleanup

### Testing & Quality Assurance

- [ ] T111 [P] Create unit tests for VirtualDeviceManager in tests/unit/test_virtual_device_manager.cpp
- [ ] T112 [P] Create unit tests for AudioValidator in tests/unit/test_audio_validator.cpp
- [ ] T113 [P] Create unit tests for LatencyMeasurer in tests/unit/test_latency_measurer.cpp
- [ ] T114 [P] Create unit tests for ConfigManager in tests/unit/test_config_manager.cpp
- [ ] T115 [P] Create integration test for end-to-end loopback pipeline in tests/integration/test_linux_virtual_audio.cpp
- [ ] T116 [P] Create performance test for latency consistency in tests/performance/test_loopback_latency.cpp
- [ ] T117 [P] Create performance test for validation speed in tests/performance/test_validation_performance.cpp

### Documentation Completion

- [x] T118 [P] Update main README.md with link to Linux virtual audio testing documentation
- [ ] T119 [P] Create API documentation for audioBridge-test CLI (man page format)
- [x] T120 [P] Add inline code comments to all test tools for maintainability
- [ ] T121 [P] Create architecture diagram in docs/linux-audio-testing/architecture.md

### Installation & Distribution

- [ ] T122 [P] Update installation instructions in README to include test tools
- [ ] T123 [P] Create Makefile or package script for easy test tool installation
- [ ] T124 [P] Add test tools to CMake install target (install to /usr/local/bin)
- [ ] T125 [P] Create Debian/RPM package specification for Linux distribution

---

## Dependencies & Execution Order

### Phase Dependencies

```
Phase 1 (Setup) ──┐
                  │
                  ├──> Phase 2 (Foundational) ──┬──> Phase 3 (US1 - Basic Loopback) ──┐
                  │                               │                                │
                  │                               ├──> Phase 4 (US2 - Automation) ├──> Phase 6 (Polish)
                  │                               │                                │
                  │                               └──> Phase 5 (US3 - Validation)──┘
                  │
                  └──> Must complete before ANY user story work begins
```

### User Story Dependencies

```
US1 (Basic Loopback) ──┬──> US2 (Automation) ──┬──> US3 (Validation)
                       │                        │
                       └────────────────────────┘

All stories depend on Phase 2 (Foundational) completing first.
Stories are INDEPENDENT after Phase 2 - can be developed in any order.
```

### Critical Path (Minimum Viable Product)

**MVP = Phase 1 + Phase 2 + Phase 3 (US1)**

This delivers:
- Virtual audio device detection
- Basic loopback testing (play audio → capture audio)
- Manual test execution with pass/fail reporting
- Setup documentation for 3 major distributions

**Estimated MVP Tasks**: 54 tasks (T001-T054)

---

## Parallel Execution Opportunities

### Within Phase 1 (Setup)
- **T003, T004, T005**: CMake modules for ALSA, SndFile, Gist can be created in parallel
- **T008, T009**: JSON schemas can be created in parallel

### Within Phase 2 (Foundational)
- **NO PARALLEL TASKS**: All foundational tasks are sequential (each builds on previous)

### Within Phase 3 (US1)
- **T018-T022**: Distribution setup guides (Ubuntu, Fedora, Arch, troubleshooting) - 5 parallel tasks
- **T023-T028**: Test audio file generation - 6 parallel tasks
- **T063-T065**: CI report format implementations (JUnit, HTML) - 3 parallel tasks (moved to US2 but parallelizable)
- **T068-T071**: CI/CD integration examples - 4 parallel tasks (moved to US2 but parallelizable)

### Within Phase 5 (US3)
- **T075-T078**: Audio analysis implementations (FFT, frequency, peak, matching) - 4 parallel tasks
- **T079-T082**: Signal quality metrics (SNR, THD, amplitude, noise) - 4 parallel tasks

### Within Phase 6 (Polish)
- **T103-T106**: Performance optimizations - 4 parallel tasks
- **T107-T110**: Error handling improvements - 4 parallel tasks
- **T111-T117**: Unit/integration/performance tests - 7 parallel tasks
- **T118-T121**: Documentation tasks - 4 parallel tasks
- **T122-T125**: Installation tasks - 4 parallel tasks

**Total Parallel Opportunities**: 47 tasks can be parallelized across all phases

---

## Implementation Strategy

### Incremental Delivery Approach

1. **MVP (Minimum Viable Product)**: Phase 1 + Phase 2 + Phase 3 (US1)
   - Delivers core loopback testing capability
   - Enables manual testing with clear reporting
   - Provides setup documentation for major distributions
   - **Timeline**: Tasks T001-T054

2. **Automation Enhancement**: + Phase 4 (US2)
   - Adds automated test suite execution
   - Enables CI/CD integration
   - Provides batch testing capabilities
   - **Timeline**: Tasks T055-T074

3. **Advanced Validation**: + Phase 5 (US3)
   - Adds sophisticated audio analysis
   - Provides frequency and quality metrics
   - Enables latency measurement
   - **Timeline**: Tasks T075-T102

4. **Production Ready**: + Phase 6 (Polish)
   - Performance optimization
   - Comprehensive testing
   - Complete documentation
   - Easy installation
   - **Timeline**: Tasks T103-T125

### Risk Mitigation Strategy

1. **Start with documentation first** (T018-T022): Validates approach before heavy implementation
2. **Test audio generation early** (T023-T028): Provides test data for all subsequent development
3. **Incremental feature delivery**: Each user story is independently testable
4. **Continuous validation**: Each phase ends with working software

---

## Task Summary

**Total Tasks**: 125

**By Phase**:
- Phase 1 (Setup): 9 tasks
- Phase 2 (Foundational): 8 tasks
- Phase 3 (US1 - Basic Loopback): 37 tasks
- Phase 4 (US2 - Automation): 20 tasks
- Phase 5 (US3 - Validation): 28 tasks
- Phase 6 (Polish): 23 tasks

**By User Story**:
- US1 (Basic Loopback): 37 tasks (T018-T054)
- US2 (Automation): 20 tasks (T055-T074)
- US3 (Validation): 28 tasks (T075-T102)

**Parallelizable Tasks**: 47 tasks marked with [P]

**MVP Scope**: 54 tasks (Phase 1 + 2 + US1) - delivers core loopback testing

**Estimated Effort**:
- MVP: 2-3 weeks (54 tasks)
- Full feature: 4-6 weeks (125 tasks)

---

## Format Validation

✅ **ALL tasks follow the required checklist format**:
- All tasks start with `- [ ]` (markdown checkbox)
- All tasks have sequential Task IDs (T001-T125)
- Parallelizable tasks marked with `[P]`
- User story tasks marked with `[US1]`, `[US2]`, or `[US3]`
- All tasks include exact file paths
- Setup/Foundational/Polish phases have NO story labels
- User story phases have CORRECT story labels

**Ready for execution**: Each task is specific enough for an LLM to implement without additional context.

---

**Tasks Status**: ✅ Complete
**MVP Defined**: Phase 1 + Phase 2 + Phase 3 (US1)
**Dependencies Mapped**: Clear phase and story dependencies
**Parallel Execution**: 47 parallel opportunities identified
**Independent Testing**: Each user story has clear independent test criteria

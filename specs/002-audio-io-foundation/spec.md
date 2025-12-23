# Feature Specification: Audio I/O Foundation

**Feature Branch**: `002-audio-io-foundation`
**Created**: 2025-12-22
**Status**: Draft
**Input**: User description: "完成首先音频I/O的基础架构,完成拾音与播放的基本功能"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Audio Capture from Virtual Sound Card (Priority: P1)

As a user, I want to capture audio from a virtual sound card (VB-Cable on Windows or BlackHole on macOS) so that I can receive audio input from other applications for processing.

**Why this priority**: Audio capture is the fundamental input capability. Without the ability to capture audio, no downstream processing (AI translation, DSP, etc.) can occur. This is the entry point of the entire audio pipeline.

**Independent Test**: Can be fully tested by selecting a virtual sound card as input device, playing audio through another application routed to that virtual device, and verifying that audio data is being received and can be monitored (e.g., level meters showing activity).

**Acceptance Scenarios**:

1. **Given** the application is launched and a virtual sound card (VB-Cable/BlackHole) is available, **When** the user selects it as the input device, **Then** the system begins capturing audio from that device at 48kHz sample rate with 128-frame buffer size.
2. **Given** audio is being captured, **When** another application plays audio routed to the virtual sound card, **Then** the captured audio levels reflect the incoming signal in real-time.
3. **Given** no virtual sound card is available, **When** the user attempts to select an input device, **Then** the system displays available physical input devices as alternatives with appropriate guidance.

---

### User Story 2 - Audio Playback to System Output (Priority: P1)

As a user, I want to play processed audio through my system speakers or headphones so that I can hear the output of the audio processing pipeline.

**Why this priority**: Audio playback is equally fundamental as capture - it's the output of the entire system. Together with capture, these two capabilities form the complete I/O foundation required for any audio processing application.

**Independent Test**: Can be fully tested by selecting an output device (speaker/headphone), generating or passing through audio data, and verifying audible output through the selected device.

**Acceptance Scenarios**:

1. **Given** the application is running and system audio output devices are available, **When** the user selects a speaker or headphone as output, **Then** audio data sent to the output stream is played through that device.
2. **Given** audio playback is active, **When** the audio data contains valid PCM samples, **Then** the user hears the corresponding sound with latency under 200ms from input to output.
3. **Given** the selected output device becomes unavailable (disconnected), **When** the system detects this, **Then** it notifies the user and allows selection of an alternative device.

---

### User Story 3 - Device Enumeration and Selection (Priority: P2)

As a user, I want to see a list of all available audio input and output devices so that I can choose which devices to use for capture and playback.

**Why this priority**: Device selection enables user configuration and flexibility. While default device selection could work for basic scenarios, explicit device enumeration and selection is essential for users with multiple audio devices or specific routing requirements.

**Independent Test**: Can be fully tested by launching the application and verifying that all connected audio devices (physical and virtual) appear in selectable lists, and that selection persists correctly.

**Acceptance Scenarios**:

1. **Given** the application starts, **When** device enumeration is requested, **Then** all available input devices (including virtual sound cards) are listed with their names.
2. **Given** the application starts, **When** device enumeration is requested, **Then** all available output devices (speakers, headphones) are listed with their names.
3. **Given** a device list is displayed, **When** a new audio device is connected to the system, **Then** the device list can be refreshed to include the new device.
4. **Given** devices have been selected, **When** the application is configured, **Then** the selected input and output devices are used for audio I/O operations.

---

### User Story 4 - Audio Pass-through (Loopback) (Priority: P2)

As a user, I want to hear the captured audio directly through my output device (pass-through mode) so that I can verify the audio pipeline is working correctly before adding processing.

**Why this priority**: Pass-through provides essential verification that the entire I/O chain works correctly. This is critical for debugging and establishing baseline functionality before integrating AI/DSP processing.

**Independent Test**: Can be fully tested by selecting input and output devices, enabling pass-through mode, and verifying that audio from the input is heard through the output with acceptable latency.

**Acceptance Scenarios**:

1. **Given** input and output devices are selected and active, **When** pass-through mode is enabled, **Then** audio captured from input is immediately routed to output.
2. **Given** pass-through is active, **When** measuring end-to-end latency, **Then** the total delay from input to output is less than 200ms.
3. **Given** pass-through is active, **When** the user disables pass-through, **Then** audio is no longer routed from input to output.

---

### Edge Cases

- What happens when the selected input device has no signal (silence)? The system should continue operating normally, passing silence through the pipeline.
- What happens when audio buffer underrun or overrun occurs? The system should recover gracefully without crashing and log the event for diagnostics.
- How does the system handle sample rate mismatch between devices? The system uses a fixed 48kHz rate; devices not supporting this rate should be filtered from selection or resampled transparently.
- What happens when the user selects the same device for both input and output? The system should prevent this configuration to avoid feedback loops, displaying an appropriate warning.
- How does the system handle very high CPU load affecting audio processing? The system should prioritize audio thread stability, potentially dropping frames rather than blocking or crashing.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST enumerate all available audio input devices on the host system, including virtual sound cards (VB-Cable, BlackHole).
- **FR-002**: System MUST enumerate all available audio output devices on the host system, including speakers and headphones.
- **FR-003**: System MUST allow users to select one input device and one output device from the enumerated lists.
- **FR-004**: System MUST capture audio from the selected input device at 48kHz sample rate with 128-frame buffer size.
- **FR-005**: System MUST output audio to the selected output device at 48kHz sample rate with 128-frame buffer size.
- **FR-006**: System MUST use PCM float 32-bit format for all audio data throughout the pipeline.
- **FR-007**: System MUST provide a pass-through mode that routes captured audio directly to output for verification purposes.
- **FR-008**: System MUST notify users when a selected device becomes unavailable during operation.
- **FR-009**: System MUST provide audio level monitoring capability to indicate signal presence on input and output.
- **FR-010**: System MUST support operation on Windows, macOS, and Linux platforms.
- **FR-011**: System MUST prevent selection of the same device for both input and output when it would cause feedback.
- **FR-012**: System MUST log audio I/O events and errors for diagnostic purposes.
- **FR-013**: System MUST allow device list refresh without restarting the application.

### Key Entities

- **AudioDevice**: Represents a physical or virtual audio device with properties including name, device identifier, supported sample rates, channel count, and device type (input/output).
- **AudioStream**: Represents an active audio capture or playback session with properties including associated device, stream state (active/stopped/error), buffer configuration, and sample format.
- **AudioBuffer**: Represents a block of audio samples with properties including sample data (PCM float 32-bit), frame count, timestamp, and channel layout.
- **DeviceConfiguration**: Represents user-selected device settings including selected input device, selected output device, and pass-through mode state.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Users can select and activate audio input/output devices within 5 seconds of application launch.
- **SC-002**: End-to-end audio latency (input to output in pass-through mode) is less than 200ms under normal operating conditions.
- **SC-003**: System operates continuously for 24+ hours without audio dropouts, crashes, or memory leaks.
- **SC-004**: Audio capture and playback work correctly on all three supported platforms (Windows, macOS, Linux).
- **SC-005**: Users can identify and select virtual sound cards (VB-Cable/BlackHole) from the device list without manual configuration.
- **SC-006**: Audio level indicators accurately reflect signal presence within 100ms of signal change.
- **SC-007**: Device disconnection is detected and reported to the user within 2 seconds of occurrence.
- **SC-008**: 95% of users can successfully set up audio pass-through on first attempt using the device selection interface.

## Assumptions

- Virtual sound cards (VB-Cable on Windows, BlackHole on macOS) are pre-installed by the user; the application does not install them.
- The host system has functional audio hardware with appropriate drivers installed.
- Users have basic familiarity with audio device concepts (input, output, virtual sound cards).
- The target platforms have PortAudio-compatible audio subsystems (WASAPI on Windows, CoreAudio on macOS, ALSA/PulseAudio on Linux).
- Buffer size of 128 frames at 48kHz provides acceptable latency (~2.67ms per buffer) for real-time audio processing.

## Dependencies

- PortAudio library for cross-platform audio I/O abstraction.
- Operating system audio subsystems: WASAPI (Windows), CoreAudio (macOS), ALSA/PulseAudio (Linux).
- Third-party virtual sound card software: VB-Cable (Windows), BlackHole (macOS).

## Out of Scope

- AI/DSP audio processing (covered by DummyEngine placeholder, actual implementation is future work).
- Self-developed virtual sound card driver (planned for future phases).
- Audio file recording or playback from disk.
- Multi-channel audio beyond stereo.
- Sample rate conversion or resampling.
- GUI implementation (this spec focuses on core I/O functionality; UI is a separate concern).

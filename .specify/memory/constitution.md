# audioBridge Constitution
<!--
Sync Impact Report:
- Version change: N/A → 1.0.0 (NEW)
- Modified principles: None (new constitution)
- Added sections: Core Principles (5), Audio Engine Architecture (3), Development Standards (3), Governance (2)
- Removed sections: None (new constitution)
- Templates updated: ✅ plan-template.md, ✅ tasks-template.md, ✅ spec-template.md (validated)
- Follow-up TODOs: None - all placeholders filled with project-specific requirements
-->

## Core Principles

### I. Real-Time Audio Safety
All audio processing threads MUST be real-time safe. No dynamic memory allocation, no blocking I/O, no system calls that can cause priority inversion in the audio callback path. Audio threads shall only perform lock-free operations and predictable computation.

### II. Adapter Abstraction Layer
All audio I/O MUST go through the IAudioInput/IAudioOutput abstraction layer. Business logic shall never depend directly on PortAudio, VB-Cable, BlackHole, or any specific virtual audio driver implementation. This enables seamless migration from third-party virtual audio devices to custom drivers.

### III. Cross-Platform Compatibility
All core audio engine components MUST compile and run on Windows, macOS, and Linux without platform-specific code paths. Platform-specific implementations shall be isolated within the adapter layer only.

### IV. Low Latency Requirements
End-to-end audio latency (input → AI processing → output) MUST remain under 200ms for the initial implementation. All architectural decisions shall prioritize minimizing audio buffer sizes and processing delays while maintaining stability.

### V. Test-First Audio Development
All audio components MUST have comprehensive unit tests before implementation. Integration tests MUST validate the complete audio pipeline from virtual input through AI processing to physical output. Performance tests MUST verify real-time constraints and latency requirements.

## Audio Engine Architecture

### Module Separation
The audio engine core SHALL be cleanly separated into three distinct layers:
1. **Application Layer**: Device selection UI and status monitoring
2. **Audio Engine Core**: Pipeline management, ring buffers, DSP/AI processing
3. **Audio I/O Adapter**: Platform abstraction for system and virtual devices

### Data Format Standardization
All internal audio data MUST use PCM float32 format at 48kHz sampling rate with 128-frame buffers. No other formats shall be used within the core engine pipeline to ensure consistency and predictability.

### Thread Safety Model
Audio processing SHALL use a multi-threaded design with dedicated threads for:
- Input capture (RT-safe)
- AI/DSP processing (separate from audio threads)
- Output rendering (RT-safe)
Communication between threads SHALL use lock-free ring buffers only.

## Development Standards

### Performance Monitoring
All audio components MUST include structured logging for performance metrics, buffer overruns, and processing delays. Real-time monitoring SHALL be built into the engine core with configurable log levels.

### Memory Management
Audio threads SHALL pre-allocate all required memory during initialization. No heap allocations shall occur during real-time audio processing. Memory pools SHALL be used for dynamic buffer management when necessary.

### Error Handling
Audio callback functions SHALL handle errors gracefully without blocking. Error conditions SHALL be logged via lock-free mechanisms and deferred to non-real-time threads for recovery actions.

## Governance

This constitution supersedes all other project documentation and coding standards. All architectural decisions MUST demonstrate compliance with these principles.

### Amendment Process
- Amendments require documented justification and impact analysis
- Changes MUST be approved through the same review process as code changes
- All dependent templates and documentation MUST be updated simultaneously
- Version SHALL follow semantic versioning: MAJOR for breaking changes, MINOR for new principles, PATCH for clarifications

### Compliance Verification
All pull requests MUST pass automated checks for:
- Real-time safety violations
- Adapter layer compliance
- Cross-platform compilation
- Performance regression testing
- Constitution principle validation

**Version**: 1.0.0 | **Ratified**: 2025-12-22 | **Last Amended**: 2025-12-22

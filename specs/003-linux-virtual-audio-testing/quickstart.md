# Quick Start Guide: Linux Virtual Audio Testing

**Feature**: 003-linux-virtual-audio-testing
**Target Audience**: Developers with basic Linux skills
**Time to Complete**: 30 minutes

---

## Overview

This guide will help you set up and run your first virtual audio loopback test on Linux in under 30 minutes. Virtual audio loopback allows you to test audioBridge without physical microphones or speakers by routing audio output back to input through software.

---

## Prerequisites

Before you begin, ensure you have:

- ✅ **Linux distribution**: Ubuntu 20.04+, Fedora 35+, or Arch Linux
- ✅ **Basic Linux skills**: Comfortable running commands in terminal
- ✅ **sudo access**: Required for loading kernel modules
- ✅ **AudioBridge installed**: From feature 002 (audio I/O foundation)
- ✅ **Compiler**: GCC 7+ or Clang 5+
- ✅ **CMake**: Version 3.15+

---

## Step 1: Install Dependencies (5 minutes)

### Ubuntu/Debian

```bash
sudo apt update
sudo apt install -y \
    alsa-utils \
    sox \
    libportaudio2 \
    libportaudio-dev \
    libsndfile1 \
    build-essential \
    cmake
```

### Fedora

```bash
sudo dnf install -y \
    alsa-utils \
    sox \
    portaudio \
    portaudio-devel \
    libsndfile \
    gcc-c++ \
    cmake
```

### Arch Linux

```bash
sudo pacman -S --needed \
    alsa-utils \
    sox \
    portaudio \
    libsndfile \
    gcc \
    cmake
```

**Verification**:
```bash
# Check ALSA tools
aplay --version
arecord --version

# Check SoX
sox --version

# Check CMake
cmake --version
```

---

## Step 2: Setup Virtual Audio Loopback (5 minutes)

### Load the ALSA Loopback Module

**Temporary (current session only)**:
```bash
sudo modprobe snd-aloop
```

**Permanent (auto-load on boot)**:
```bash
echo "snd-aloop" | sudo tee -a /etc/modules-load.d/alsa-loopback.conf
```

### Verify Loopback Devices

```bash
# List all playback devices
aplay -l

# List all recording devices
arecord -l
```

**Expected Output**:
You should see "Loopback" devices in the list:
```
card 1: Loopback [Loopback], device 0: Loopback PCM [Loopback PCM]
  Subdevices: 8/8
  Subdevice #0: subdevice #0
```

### Troubleshooting

**If devices don't appear**:
```bash
# Check if module is loaded
lsmod | grep snd_aloop

# If empty, try loading manually
sudo modprobe snd-aloop

# Check kernel messages
dmesg | tail -20
```

---

## Step 3: Build audioBridge Test Tools (5 minutes)

### Checkout and Build

```bash
# Navigate to audioBridge directory
cd /path/to/audioBridge

# Create build directory
mkdir -p build && cd build

# Configure with CMake
cmake .. -DBUILD_TESTING=ON

# Build
make -j$(nproc)

# Install (optional)
sudo make install
```

### Verify Installation

```bash
# Check test runner
audioBridge-test --version

# List available devices
audioBridge-test list-devices
```

**Expected Output**:
```
audioBridge-test version 1.0.0

Available Audio Devices:
[0] hw:Loopback,0,0 (Loopback: PCM)
    Type: LOOPBACK
    Direction: OUTPUT
    ...
```

---

## Step 4: Generate Test Audio Files (3 minutes)

### Using SoX

```bash
# Create test audio directory
mkdir -p ~/.audioBridge/tests/test-audio
cd ~/.audioBridge/tests/test-audio

# Generate 1kHz sine wave (5 seconds)
sox -n -r 48000 -b 16 1khz-sine.wav synth 5 sine 1000

# Generate 440Hz tone (musical A, 5 seconds)
sox -n -r 48000 -b 16 440hz-tone.wav synth 5 sine 440

# Generate white noise (5 seconds)
sox -n -r 48000 -b 16 white-noise.wav synth 5 noise

# Generate frequency sweep (20Hz to 20kHz, 5 seconds)
sox -n -r 48000 -b 16 frequency-sweep.wav synth 5 sine 20-20000

# Generate silence (10 seconds)
sox -n -r 48000 -b 16 silence.wav synth 10 silence

# Verify files
ls -lh
```

**Expected Files**:
- `1khz-sine.wav` (~470 KB)
- `440hz-tone.wav` (~470 KB)
- `white-noise.wav` (~470 KB)
- `frequency-sweep.wav` (~470 KB)
- `silence.wav` (~940 KB)

---

## Step 5: Run Your First Test (2 minutes)

### Setup Check

```bash
audioBridge-test setup-check
```

**Expected Output**:
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
✓ PortAudio installed
✓ ALSA utils installed
✓ SoX installed

Configuration Status: READY
```

### Run Basic Test

```bash
audioBridge-test run ~/.audioBridge/tests/test-audio/1khz-sine.wav
```

**Expected Output**:
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

Status: PASS (10/10 checks passed, 0 failed, 0 warnings)
```

### Verify Captured Audio

```bash
# Play captured audio (optional)
aplay ~/.audioBridge/tests/captured/capture-*.wav

# Analyze with SoX
soxi ~/.audioBridge/tests/captured/capture-*.wav
```

---

## Step 6: Run Test Suite (Optional, 5 minutes)

### Create Test Suite Configuration

```bash
mkdir -p ~/.audioBridge/tests/suites
cat > ~/.audioBridge/tests/suites/default.json << 'EOF'
{
  "version": "1.0",
  "name": "default",
  "description": "Default test suite",
  "tests": [
    {
      "name": "1khz-sine-test",
      "description": "Test 1kHz sine wave frequency response",
      "audioFile": "~/.audioBridge/tests/test-audio/1khz-sine.wav",
      "enabled": true
    },
    {
      "name": "440hz-tone-test",
      "description": "Test 440Hz musical tone",
      "audioFile": "~/.audioBridge/tests/test-audio/440hz-tone.wav",
      "enabled": true
    },
    {
      "name": "white-noise-test",
      "description": "Test white noise SNR",
      "audioFile": "~/.audioBridge/tests/test-audio/white-noise.wav",
      "enabled": true
    }
  ]
}
EOF
```

### Run Suite

```bash
audioBridge-test run-suite default
```

### Generate JUnit Report (for CI)

```bash
audioBridge-test run-suite default \
    --report-format junit \
    --report-file test-results.xml
```

---

## Common Issues & Solutions

### Issue: "snd-aloop module not found"

**Solution**:
```bash
# Check if module exists
modinfo snd-aloop

# If not found, install linux-modules-extra
sudo apt install linux-modules-extra-$(uname -r)

# Then load module
sudo modprobe snd-aloop
```

---

### Issue: "Device busy" error

**Solution**:
```bash
# Check what's using the audio device
lsof /dev/snd/*

# Close other audio applications (PulseAudio, etc.)
pulseaudio --kill

# Retry test
audioBridge-test run test-audio/1khz-sine.wav
```

---

### Issue: "No loopback devices found"

**Solution**:
```bash
# Verify module loaded
lsmod | grep snd_aloop

# Check device list
cat /proc/asound/cards

# Reboot if devices appeared in past but not now
sudo reboot
```

---

### Issue: High latency (> 50ms)

**Solution**:
```bash
# Check for PulseAudio (adds latency)
pulseaudio --check -v

# Try using ALSA directly
export AUDIOBRIDGE_USE_ALSA=1

# Reduce buffer size
audioBridge-test run --frames-per-buffer 64 test-audio/1khz-sine.wav
```

---

### Issue: PipeWire compatibility (Ubuntu 22.04+, Fedora 35+)

**Solution**:
```bash
# Check if using PipeWire
pactl info | grep "Server Name"

# If PipeWire, ensure ALSA emulation is active
pactl load-module module-alsa-sink

# Alternative: Use pw-loopback
pw-loopback --help
```

---

## Next Steps

### Learn More

- 📖 **Full Documentation**: See `docs/linux-setup-{ubuntu,fedora,arch}.md` for distribution-specific guides
- 📖 **API Reference**: See `contracts/test-api.md` for complete CLI documentation
- 📖 **Architecture**: See `docs/virtual-audio-architecture.md` for technical details

### Advanced Usage

```bash
# Custom device selection
audioBridge-test run --playback-device 5 --capture-device 6 test-audio/1khz-sine.wav

# Specific duration
audioBridge-test run --duration 10.0 test-audio/frequency-sweep.wav

# JSON output for automation
audioBridge-test run --report-format json --report-file results.json test-audio/1khz-sine.wav

# Validation only (skip capture)
audioBridge-test validate captured-output.wav

# List devices in JSON
audioBridge-test list-devices --json
```

### CI/CD Integration

See example workflows in `examples/ci/`:
- `github-actions.yml`
- `gitlab-ci.yml`
- `jenkinsfile`

---

## Verification Checklist

Before considering setup complete, verify:

- [ ] `snd-aloop` module loaded successfully
- [ ] Loopback devices visible in `aplay -l` and `arecord -l`
- [ ] `audioBridge-test --version` shows correct version
- [ ] `audioBridge-test setup-check` passes all checks
- [ ] First test (1kHz sine) completes with SUCCESS status
- [ ] Captured audio file exists and is valid
- [ ] Frequency analysis shows expected frequency (1000 Hz ±5 Hz)
- [ ] Latency < 50ms
- [ ] SNR > 60dB

**Estimated Completion Time**: 25-30 minutes ✅

---

## Getting Help

### Debug Mode

```bash
# Enable verbose logging
audioBridge-test -v run test-audio/1khz-sine.wav

# Check log files
tail -f ~/.audioBridge/logs/audioBridge-test.log
```

### Community Resources

- 🐛 **Report Issues**: GitHub Issues
- 💬 **Discussion**: GitHub Discussions
- 📧 **Email**: support@audioBridge.dev

### Useful Commands

```bash
# System audio status
pacmd list-sinks
pacmd list-sources

# ALSA device details
cat /proc/asound/cards
cat /proc/asound/devices

# Audio process monitoring
lsof /dev/snd/*
fuser -v /dev/snd/*
```

---

## Summary

Congratulations! 🎉 You've successfully:

1. ✅ Installed all required dependencies
2. ✅ Configured virtual audio loopback
3. ✅ Built audioBridge test tools
4. ✅ Generated test audio files
5. ✅ Run your first loopback test
6. ✅ Validated captured audio

You're now ready to use virtual audio testing for automated audio validation in audioBridge!

**Next Recommended Step**: Integrate tests into your CI/CD pipeline using the JUnit XML output format.

---

**Quick Start Guide Status**: ✅ Complete
**Last Updated**: 2025-12-24

# Troubleshooting Guide for audioBridge Linux Testing

**Purpose**: Diagnose and resolve common issues with virtual audio loopback testing
**Scope**: Ubuntu, Fedora, Arch Linux, and other distributions

---

## Quick Diagnostic Flow

```
Issue Start
    ↓
Run: audioBridge-test setup-check
    ↓
Check output for ✓ marks
    ↓
Jump to relevant section below
```

---

## Category 1: snd-aloop Module Issues

### Issue: "modprobe: ERROR: could not insert 'snd_aloop'"

**Symptoms**:
```
sudo modprobe snd-aloop
modprobe: ERROR: could not insert 'snd_aloop': Module not found
```

**Diagnosis**:
```bash
# Check if module file exists
find /lib/modules/$(uname -r) -name "snd-aloop.ko*"

# Check kernel version
uname -r
```

**Solutions**:

#### Solution 1: Update module dependencies
```bash
sudo depmod -a
sudo modprobe snd-aloop
```

#### Solution 2: Install ALSA modules package (Ubuntu/Debian)
```bash
sudo apt install linux-modules-extra-$(uname -r)
sudo modprobe snd-aloop
```

#### Solution 3: Use LTS kernel (Arch)
```bash
sudo pacman -S linux-lts
sudo reboot
# Choose LTS kernel in boot menu
```

#### Solution 4: Rebuild kernel module (Advanced)
```bash
# Install kernel headers
sudo apt install linux-headers-$(uname -r) build-essential

# Download kernel sources
# Build snd-aloop module manually
# See: https://www.kernel.org/doc/html/latest/kbuild/
```

---

### Issue: Module loads but devices not visible

**Symptoms**:
```bash
lsmod | grep snd_aloop  # Shows module loaded
aplay -l | grep Loopback  # No output
```

**Diagnosis**:
```bash
# Check sound card status
cat /proc/asound/cards
```

**Solutions**:

#### Solution 1: Reload ALSA
```bash
sudo alsa force-reload
sudo modprobe -r snd-aloop
sudo modprobe snd-aloop
```

#### Solution 2: Create device nodes manually
```bash
# Check /dev/snd directory
ls -l /dev/snd/

# If device nodes missing, recreate
sudo udevadm control --reload-rules
sudo udevadm trigger
```

#### Solution 3: Check kernel messages
```bash
dmesg | grep -i snd_aloop
```

Look for errors or warnings in kernel log.

---

## Category 2: Device Access Issues

### Issue: "Device or resource busy"

**Symptoms**:
```
audioBridge-test run test.wav
Error: Device or resource busy
```

**Diagnosis**:
```bash
# Find process using audio device
sudo lsof /dev/snd/*

# Or use fuser
sudo fuser -v /dev/snd/*
```

**Solutions**:

#### Solution 1: Stop conflicting applications
```bash
# Common culprits:
# - PulseAudio/PipeWire
# - Other audio applications (VLC, Audacity, etc.)
# - Previous audioBridge-test run

# Kill specific process (replace PID)
sudo kill -9 <PID>

# Or stop audio server
systemctl --user stop pipewire pulseaudio
```

#### Solution 2: Retry after delay
```bash
# Wait 2-3 seconds and retry
sleep 3
./audioBridge-test run test.wav
```

#### Solution 3: Reboot system
```bash
sudo reboot
```

---

### Issue: "Permission denied" accessing device

**Symptoms**:
```
audioBridge-test run test.wav
Error: Permission denied (hw:1,0)
```

**Diagnosis**:
```bash
# Check user groups
groups

# Check device permissions
ls -l /dev/snd/*
```

**Solutions**:

#### Solution 1: Add user to audio group
```bash
sudo usermod -aG audio $USER

# Log out and log back in
# Or use newgrp (temporary)
newgrp audio
```

#### Solution 2: Fix device permissions
```bash
# Check udev rules
cat /lib/udev/rules.d/90-pulseaudio.rules

# Add custom rule if needed
sudo tee /etc/udev/rules.d/99-audiobridge.rules > /dev/null <<EOF
# Allow audio group to access audio devices
KERNEL=="pcmC*D0", SUBSYSTEM=="sound", GROUP="audio", MODE="0660"
EOF

# Reload udev
sudo udevadm control --reload-rules
sudo udevadm trigger
```

#### Solution 3: Use sudo (not recommended for security)
```bash
sudo ./audioBridge-test run test.wav
```

**Warning**: Using sudo is a temporary workaround. Fix group permissions instead.

---

## Category 3: Audio Quality Issues

### Issue: Captured audio has noise or artifacts

**Symptoms**:
- Static or crackling in captured audio
- Dropouts or glitches
- Poor signal-to-noise ratio

**Diagnosis**:
```bash
# Check buffer settings
cat /proc/asound/card1/pcm0p/sub0/hw_params

# Check system load
top

# Check interrupt rate
grep -A 5 snd /proc/interrupts
```

**Solutions**:

#### Solution 1: Increase buffer size
```bash
# Create modprobe configuration
sudo tee /etc/modprobe.d/snd-aloop.conf > /dev/null <<EOF
options snd-aloop pcm_substreams=8
options snd-pcm buffer_bytes_max=65536
EOF

# Reload module
sudo modprobe -r snd-aloop
sudo modprobe snd-aloop
```

#### Solution 2: Disable CPU frequency scaling
```bash
# Set governor to performance
sudo cpupower frequency-set -g performance

# Or for specific CPU
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

#### Solution 3: Increase process priority
```bash
# Run with real-time priority (if allowed)
sudo chrt -f 50 ./audioBridge-test run test.wav

# Or nice value
sudo nice -n -10 ./audioBridge-test run test.wav
```

#### Solution 4: Check for xruns
```bash
# Monitor xruns during capture
cat /proc/xenomai/registry/rtap/*
# Or for plain ALSA
watch -n 1 cat /proc/asnd/card1/pcm0p/xrun_debug
```

---

### Issue: Latency too high (>50ms)

**Symptoms**:
- Measured latency exceeds 50ms
- Audio delay noticeable
- Test reports high latency

**Diagnosis**:
```bash
# Check current latency
./audioBridge-test run test.wav --measure-latency

# Check buffer settings
cat /proc/asound/card1/pcm0p/sub0/prealloc
```

**Solutions**:

#### Solution 1: Reduce buffer size
```bash
# Set smaller buffer size
sudo tee /etc/modprobe.d/snd-aloop.conf > /dev/null <<EOF
options snd-aloop pcm_substreams=8
options snd-pcm buffer_bytes_max=16384
options snd-pcm period_bytes_min=1024
EOF

# Reload module
sudo modprobe -r snd-aloop
sudo modprobe snd-aloop
```

#### Solution 2: Use hw: devices instead of plughw:
```bash
# In test configuration
# Use: hw:1,0 (hardware direct)
# Not: plughw:1,0 (plugin layer adds latency)
```

#### Solution 3: Disable unnecessary audio processing
```bash
# Edit /etc/asound.conf or ~/.asoundrc
# Disable sample rate conversion, resampling, etc.
```

---

## Category 4: Dependency Issues

### Issue: CMake doesn't find ALSA

**Symptoms**:
```
cmake ..
-- ALSA not found
CMake Error: Could not find ALSA
```

**Diagnosis**:
```bash
# Check for ALSA libraries
ldconfig -p | grep libasound

# Check pkg-config
pkg-config --modversion alsa
```

**Solutions**:

#### Solution 1: Install development packages (Ubuntu/Debian)
```bash
sudo apt install libasound2-dev
```

#### Solution 2: Install development packages (Fedora)
```bash
sudo dnf install alsa-lib-devel
```

#### Solution 3: Install development packages (Arch)
```bash
sudo pacman -S alsa-lib
```

#### Solution 4: Set CMAKE_PREFIX_PATH
```bash
cmake -DCMAKE_PREFIX_PATH=/usr/local ..
```

---

### Issue: CMake doesn't find libsndfile

**Symptoms**:
```
cmake ..
-- SndFile not found
CMake Error: Could not find SndFile
```

**Diagnosis**:
```bash
# Check for libsndfile
ldconfig -p | grep libsndfile

# Check pkg-config
pkg-config --modversion sndfile
```

**Solutions**:

#### Solution 1: Install development packages (Ubuntu/Debian)
```bash
sudo apt install libsndfile1-dev
```

#### Solution 2: Install development packages (Fedora)
```bash
sudo dnf install libsndfile-devel
```

#### Solution 3: Install development packages (Arch)
```bash
sudo pacman -S libsndfile
```

---

### Issue: SoX commands not working

**Symptoms**:
```
sox: command not found
sox: FAIL formats: no handler for file type `wav`
```

**Diagnosis**:
```bash
# Check SoX installation
sox --version

# Check supported formats
sox -h | grep "AUDIO FILE FORMATS"
```

**Solutions**:

#### Solution 1: Install SoX (Ubuntu/Debian)
```bash
sudo apt install sox
sudo apt install libsox-fmt-all  # All format support
```

#### Solution 2: Install SoX (Fedora)
```bash
sudo dnf install sox
```

#### Solution 3: Install SoX (Arch)
```bash
sudo pacman -S sox
```

#### Solution 4: Generate test audio manually
If SoX unavailable, use ffmpeg:
```bash
ffmpeg -f lavfi -i "sine=frequency=1000:duration=5" -ar 48000 test.wav
```

---

## Category 5: Build/Compilation Issues

### Issue: Compilation errors

**Symptoms**:
```
make audioBridge-test
error: 'spdlog' not found
error: 'PortAudio.h' not found
```

**Diagnosis**:
```bash
# Check for spdlog
ldconfig -p | grep spdlog

# Check for PortAudio
pkg-config --modversion portaudio-2.0
```

**Solutions**:

#### Solution 1: Install missing dependencies
```bash
# Ubuntu/Debian
sudo apt install libspdlog-dev portaudio19-dev

# Fedora
sudo dnf install spdlog-devel portaudio-devel

# Arch
sudo pacman -S spdlog portaudio
```

#### Solution 2: Clean and rebuild
```bash
cd build
make clean
cmake ..
make audioBridge-test
```

#### Solution 3: Check compiler version
```bash
g++ --version
# Should be GCC 7+ or Clang 5+
```

---

### Issue: Linker errors

**Symptoms**:
```
/usr/bin/ld: cannot find -lasound
/usr/bin/ld: cannot find -lsndfile
```

**Diagnosis**:
```bash
# Check library paths
ldconfig -p | grep libasound
ldconfig -p | grep libsndfile
```

**Solutions**:

#### Solution 1: Update library cache
```bash
sudo ldconfig
```

#### Solution 2: Add library path manually
```bash
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

#### Solution 3: Reinstall libraries
```bash
# Ubuntu/Debian
sudo apt install --reinstall libasound2-dev libsndfile1-dev

# Fedora
sudo dnf reinstall alsa-lib-devel libsndfile-devel
```

---

## Category 6: Test Execution Issues

### Issue: Test fails with "Validation failed"

**Symptoms**:
```
audioBridge-test run test.wav
Status: FAILED ✗
Validation: FAILED
```

**Diagnosis**:
```bash
# Check captured file
ls -lh captured-*.wav

# Play captured file
aplay captured-*.wav

# Check file format
file captured-*.wav

# Check for corruption
hexdump -C captured-*.wav | head -20
```

**Solutions**:

#### Solution 1: Verify test audio file
```bash
# Check source file
file test-data/audio/1khz-sine.wav

# Regenerate test audio
cd scripts/utils
./generate-test-audio.sh
```

#### Solution 2: Check validation thresholds
```bash
# View test configuration
cat test-data/configs/default-loopback.json

# Adjust thresholds if needed
# Edit tolerance values in config
```

#### Solution 3: Manual validation
```bash
# Use external tools
sox test.wav -n stat
sox captured.wav -n stat
# Compare frequency, SNR, etc.
```

---

### Issue: Test hangs or freezes

**Symptoms**:
```
audioBridge-test run test.wav
Starting test...
[hangs forever]
```

**Diagnosis**:
```bash
# Check process state
ps aux | grep audioBridge-test

# Check if process is sleeping (D state)
top -b -n 1 | grep audioBridge-test

# Check for deadlocks
sudo strace -p <PID>
```

**Solutions**:

#### Solution 1: Use timeout
```bash
timeout 30 ./audioBridge-test run test.wav
```

#### Solution 2: Kill process and retry
```bash
# Find process
ps aux | grep audioBridge-test

# Kill process
kill -9 <PID>

# Retry
./audioBridge-test run test.wav
```

#### Solution 3: Check device state
```bash
# Reset audio devices
sudo alsa force-reload

# Reload snd-aloop
sudo modprobe -r snd-aloop
sudo modprobe snd-aloop
```

---

## Category 7: Distribution-Specific Issues

### Ubuntu/Debian: AppArmor restrictions

**Symptoms**:
```
audited: /usr/bin/audioBridge-test
apparmor="DENIED"
```

**Solutions**:
```bash
# Check AppArmor status
sudo aa-status

# Disable profile temporarily
sudo aa-disable /usr/bin/audioBridge-test

# Or create custom profile
sudo aa-complain /usr/bin/audioBridge-test
```

---

### Fedora: SELinux restrictions

**Symptoms**:
```
audited: AVC denial
type=AVC msg=audit(...): avc: denied {...}
```

**Solutions**:
```bash
# Check SELinux status
sestatus

# Set permissive mode temporarily
sudo setenforce 0

# Check audit log
sudo ausearch -m avc -ts recent | grep audioBridge

# Create policy (advanced)
sudo audit2allow -a -M audioBridge
sudo semodule -i audioBridge.pp
```

---

### Arch: PipeWire conflicts

**Symptoms**:
```
PipeWire error: Failed to create stream
```

**Solutions**:
```bash
# Stop PipeWire temporarily
systemctl --user stop pipewire pipewire-pulse wireplumber

# Or configure PipeWire to allow ALSA direct access
mkdir -p ~/.config/pipewire/pipewire.conf.d
cat > ~/.config/pipewire/pipewire.conf.d/99-alsa.conf <<EOF
context.objects = [
    {   factory = libpipewire-module-alsa-node
        args = {
            alsa.monitor = false
        }
    }
]
EOF

# Restart PipeWire
systemctl --user restart pipewire
```

---

## Diagnostic Commands Reference

### System Information

```bash
# Distribution and version
cat /etc/os-release

# Kernel version
uname -r

# Audio devices
aplay -l
arecord -l

# ALSA cards
cat /proc/asound/cards

# Kernel modules
lsmod | grep snd
```

### Module Status

```bash
# Check snd-aloop loaded
lsmod | grep snd_aloop

# Module information
modinfo snd-aloop

# Kernel messages
dmesg | grep -i snd_aloop
```

### Device Access

```bash
# Device permissions
ls -l /dev/snd/*

# Processes using devices
sudo lsof /dev/snd/*

# User groups
groups
```

### Dependencies

```bash
# Check libraries
ldconfig -p | grep -E "(asound|sndfile|portaudio|spdlog)"

# Check pkg-config
pkg-config --modversion alsa
pkg-config --modversion sndfile
pkg-config --modversion portaudio-2.0
```

### Process Status

```bash
# Running processes
ps aux | grep audioBridge

# Process status (check for D state - uninterruptible sleep)
top -b -n 1 | grep audioBridge

# System calls
sudo strace -p <PID>
```

---

## Getting Help

### Collect Diagnostic Information

```bash
# Run diagnostic script
./scripts/collect-diagnostic-info.sh

# Or manually collect:
{
  echo "=== System Info ==="
  uname -a
  cat /etc/os-release

  echo "=== Audio Devices ==="
  aplay -l
  arecord -l

  echo "=== Module Status ==="
  lsmod | grep snd_aloop

  echo "=== Dependencies ==="
  pkg-config --modversion alsa sndfile portaudio-2.0

  echo "=== Test Run ==="
  ./audioBridge-test setup-check

} > diagnostic.txt 2>&1
```

### Useful Resources

- [ALSA Project](https://www.alsa-project.org/wiki/Main_Page)
- [PortAudio Documentation](http://www.portaudio.com/docs/)
- [audioBridge Issues](https://github.com/your-username/audioBridge/issues)

### Report an Issue

When reporting issues, include:
1. Distribution and version
2. Kernel version (`uname -r`)
3. Output of `audioBridge-test setup-check`
4. Full error message
5. Diagnostic output (above)

---

**Last Updated**: 2025-12-24
**Maintained By**: audioBridge Development Team

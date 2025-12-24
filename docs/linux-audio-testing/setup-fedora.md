# Fedora Setup Guide for audioBridge Testing

**Purpose**: Configure Fedora Linux for virtual audio loopback testing with audioBridge
**Tested Versions**: Fedora 38, 39, 40
**Setup Time**: ~15 minutes

---

## Prerequisites

- Fedora 38 or later
- sudo access (for kernel module loading)
- Internet connection (for package installation)

---

## Step 1: Install System Dependencies

### Update System

```bash
sudo dnf update -y
```

### Install Required Packages

```bash
sudo dnf install -y \
    alsa-utils \
    sox \
    libsndfile \
    gcc-c++ \
    cmake \
    git \
    make
```

**Package Description**:
- `alsa-utils`: ALSA command-line tools (aplay, arecord, aconnect)
- `sox`: Sound eXchange - audio file manipulation and generation
- `libsndfile`: Audio file I/O library (WAV, FLAC, etc.)
- `gcc-c++`: C++ compiler
- `cmake`: Build system configuration
- `git`: Version control
- `make`: Build automation tool

### Verify Installation

```bash
# Check ALSA tools
aplay --version

# Check SoX
sox --version

# Check libsndfile
dnf info libsndfile | grep Version
```

Expected output: Version numbers displayed without errors.

---

## Step 2: Load snd-aloop Kernel Module

The `snd-aloop` module creates virtual loopback audio devices that route audio output back to input.

### Load Module Temporarily

```bash
sudo modprobe snd-aloop
```

### Verify Module Loaded

```bash
lsmod | grep snd_aloop
```

Expected output:
```
snd_aloop              24576  0
snd_pcm                114688  3 snd_aloop,...
```

### List Loopback Devices

```bash
aplay -l | grep -A 2 "Loopback"
```

Expected output:
```
card 1: Loopback [Loopback], device 0: Loopback PCM [Loopback PCM]
  Subdevices: 1/1
  Subdevice #0: subdevice #0
```

---

## Step 3: Auto-Load Module at Boot (Recommended)

### Create Module Configuration File

```bash
sudo tee /etc/modules-load.d/snd-aloop.conf > /dev/null <<EOF
snd-aloop
EOF
```

### Verify Configuration

```bash
cat /etc/modules-load.d/snd-aloop.conf
```

Expected output: `snd-aloop`

**Result**: Module will load automatically at system startup.

---

## Step 4: Verify Device Configuration

### Check All Audio Devices

```bash
# List playback devices
aplay -l

# List capture devices
arecord -l
```

### Identify Loopback Devices

Loopback devices appear as:
- **Playback**: `Loopback: Loopback PCM (hw:1,0)`
- **Capture**: `Loopback: Loopback PCM (hw:1,1)`

The device numbers (e.g., `hw:1,0`) may vary on your system.

### Test Device with aplay/arecord

```bash
# Generate test audio
sox -n -r 48000 -b 16 /tmp/test-sine.wav synth 3 sine 1000

# Play to loopback (replace hw:1,0 with your device)
aplay -D hw:1,0 /tmp/test-sine.wav

# In another terminal, capture from loopback (replace hw:1,1)
arecord -D hw:1,1 -f cd -d 3 /tmp/captured.wav

# Verify capture
aplay /tmp/captured.wav
```

Expected: You should hear the 1kHz tone in the captured file.

---

## Step 5: Configure User Permissions

### Add User to Audio Group

```bash
sudo usermod -aG audio $USER
```

### Apply Group Changes

```bash
newgrp audio
```

**Or log out and log back in** for group changes to take effect.

### Verify Group Membership

```bash
groups
```

Expected output should include `audio`.

---

## Step 6: Configure SELinux (If Enforcing)

Fedora ships with SELinux enabled by default. You may need to allow audio device access.

### Check SELinux Status

```bash
sestatus
```

If status is `enabled` and current mode is `enforcing`, you have two options:

#### Option A: Permissive Mode (Recommended for Development)

```bash
sudo setenforce 0
```

To make permanent:
```bash
sudo sed -i 's/SELINUX=enforcing/SELINUX=permissive/g' /etc/selinux/config
```

#### Option B: Create SELinux Policy (Advanced)

```bash
# Check audit log for denials
sudo ausearch -m avc -ts recent | grep audioBridge

# Create policy (advanced - requires policy development)
sudo audit2allow -a -M audioBridge
sudo semodule -i audioBridge.pp
```

**Most users**: Use Option A (permissive mode) for testing.

---

## Step 7: Build audioBridge Test Tools

### Clone Repository (if not already done)

```bash
git clone https://github.com/your-username/audioBridge.git
cd audioBridge
```

### Create Build Directory

```bash
mkdir -p build && cd build
```

### Configure with CMake

```bash
cmake ..
```

**Expected Output**:
```
-- ALSA found
-- SndFile found
-- Gist found
-- Linux test tools will be built
```

### Build Test Tools

```bash
make audioBridge-test
```

**Expected**: Executable created at `build/tests/tools/audioBridge-test`

### Verify Build

```bash
./tests/tools/audioBridge-test --help
```

Expected: Usage information displayed.

---

## Step 8: Run Basic Test

### List Devices

```bash
./tests/tools/audioBridge-test list-devices
```

Expected output: List of audio devices including loopback devices.

### Run Setup Check

```bash
./tests/tools/audioBridge-test setup-check
```

Expected output:
```
✓ Kernel module snd-aloop loaded
✓ Loopback devices found
✓ Dependencies installed
Setup check passed!
```

### Run First Test

```bash
./tests/tools/audioBridge-test run ../test-data/audio/1khz-sine.wav
```

Expected output:
```
Starting test: 1khz-sine.wav
Playback device: Loopback PCM (hw:1,0)
Capture device: Loopback PCM (hw:1,1)
Testing...
Captured 240000 frames
Validation: PASSED
Status: SUCCESS ✓
```

---

## Troubleshooting

### Issue: "modprobe: ERROR: could not insert 'snd_aloop'"

**Cause**: Module not available or kernel module path issue

**Solution**:
```bash
# Update module dependencies
sudo depmod -a

# Try loading again
sudo modprobe snd-aloop

# Check if kernel module exists
find /lib/modules/$(uname -r) -name "snd-aloop.ko*"
```

### Issue: "Device or resource busy"

**Cause**: Another application is using the audio device

**Solution**:
```bash
# Find process using audio device
sudo lsof /dev/snd/*

# Kill the process or close the application
# Then retry
```

### Issue: "Permission denied" accessing audio device

**Cause**: User not in audio group or SELinux blocking

**Solution**:
```bash
# Add user to audio group
sudo usermod -aG audio $USER

# Check SELinux audit log
sudo ausearch -m avc -ts recent | grep audio

# Set SELinux to permissive if needed
sudo setenforce 0
```

### Issue: Loopback devices not detected

**Cause**: snd-aloop module not loaded or device configuration issue

**Solution**:
```bash
# Verify module loaded
lsmod | grep snd_aloop

# If not loaded, load it
sudo modprobe snd-aloop

# Rebuild ALSA device cache
sudo alsa force-reload
```

### Issue: CMake doesn't find ALSA or SndFile

**Cause**: Development packages missing

**Solution**:
```bash
# Install development packages
sudo dnf install -y alsa-lib-devel libsndfile-devel

# Re-run CMake
cd build
cmake ..
```

### Issue: PipeWire conflicts (Fedora 35+)

**Cause**: Fedora uses PipeWire by default, which may conflict with ALSA direct access

**Solution**:
```bash
# Check if PipeWire is running
pactl info

# Temporarily stop PipeWire (NOT recommended for desktop systems)
# Instead, configure PipeWire to use ALSA loopback

# Create PipeWire configuration
mkdir -p ~/.config/pipewire
cat > ~/.config/pipewire/pipewire.conf.d/99-loopback.conf <<EOF
context.modules = [
  {   name = libpipewire-module-alsa-node
      args = {
          alsa.monitor = true
      }
  }
]
EOF

# Restart PipeWire
systemctl --user restart pipewire pipewire-pulse
```

---

## Advanced Configuration

### Adjust Loopback Buffer Size

Create `/etc/modprobe.d/snd-aloop.conf`:

```bash
sudo tee /etc/modprobe.d/snd-aloop.conf > /dev/null <<EOF
options snd-aloop pcm_substreams=8
EOF
```

Reload module:
```bash
sudo modprobe -r snd-aloop
sudo modprobe snd-aloop
```

### Configure ALSA Defaults

Create `~/.asoundrc` for loopback aliases:

```bash
cat > ~/.asoundrc <<EOF
pcm.loopback_play {
    type hw
    card 1
    device 0
}

pcm.loopback_capture {
    type hw
    card 1
    device 1
}
EOF
```

Now use aliases:
```bash
aplay -D loopback_play test.wav
arecord -D loopback_capture test.wav
```

---

## Verification Checklist

- [ ] All packages installed (alsa-utils, sox, libsndfile)
- [ ] snd-aloop module loaded
- [ ] Loopback devices visible in `aplay -l` and `arecord -l`
- [ ] User added to audio group
- [ ] SELinux configured (permissive or policy created)
- [ ] audioBridge-test builds successfully
- [ ] `audioBridge-test setup-check` passes
- [ ] Basic test runs successfully

---

## Next Steps

1. Read [quickstart.md](../../specs/003-linux-virtual-audio-testing/quickstart.md) for 30-minute guide
2. Run test suite: `./tests/tools/audioBridge-test run-suite default`
3. Integrate with CI/CD: See [examples/ci/](../../examples/ci/)
4. Advanced validation: See [troubleshooting.md](./troubleshooting.md)

---

## Additional Resources

- [ALSA Project](https://www.alsa-project.org/wiki/Main_Page)
- [SoX Documentation](http://sox.sourceforge.net/sox.html)
- [Fedora Audio Documentation](https://docs.fedoraproject.org/en-US/quick-docs/sound/)
- [audioBridge Documentation](../../README.md)

---

## Fedora-Specific Notes

### Package Manager Differences

Fedora uses `dnf` instead of `apt`:
- Install: `sudo dnf install <package>`
- Update: `sudo dnf update`
- Search: `sudo dnf search <keyword>`
- Info: `dnf info <package>`

### SELinux Considerations

Fedora has SELinux enabled by default. For development/testing:
- Use **permissive mode** (recommended)
- Or create custom policies for production
- See [SELinux Documentation](https://selinuxproject.org/page/Main_Page)

### PipeWire Integration

Fedora 35+ uses PipeWire as the default audio server:
- PipeWire provides ALSA compatibility layer
- Most ALSA applications work transparently
- For issues, see PipeWire configuration above

---

**Setup Complete!** 🎉

Your Fedora system is now configured for virtual audio loopback testing with audioBridge.

*Last Updated*: 2025-12-24
*Fedora Versions*: 38, 39, 40

# Test Audio Files Directory

This directory contains test audio files for virtual audio loopback testing.

## Required Files

The following test audio files should be present:

1. **1khz-sine.wav** - 5 seconds, 48kHz, mono, 1000Hz sine wave
2. **440hz-tone.wav** - 5 seconds, 48kHz, mono, 440Hz musical A
3. **white-noise.wav** - 5 seconds, 48kHz, stereo, white noise
4. **frequency-sweep.wav** - 5 seconds, 48kHz, mono, 20Hz-20kHz sweep
5. **silence.wav** - 10 seconds, 48kHz, stereo, silence

## Generating Test Audio Files

### Method 1: Using SoX (Recommended)

```bash
# Install SoX
sudo apt install sox  # Ubuntu/Debian
sudo dnf install sox  # Fedora
sudo pacman -S sox    # Arch

# Generate all files
cd /home/wnk/code/audioBridge
./scripts/utils/generate-test-audio.sh
```

### Method 2: Manual Generation with SoX

```bash
cd /home/wnk/code/audioBridge/test-data/audio

# 1kHz sine wave (5s, mono)
sox -n -r 48000 -b 16 1khz-sine.wav synth 5 sine 1000

# 440Hz tone (5s, mono)
sox -n -r 48000 -b 16 440hz-tone.wav synth 5 sine 440

# White noise (5s, stereo)
sox -n -r 48000 -b 16 white-noise.wav synth 5 noise

# Frequency sweep (5s, mono)
sox -n -r 48000 -b 16 frequency-sweep.wav synth 5 sine 20-20000

# Silence (10s, stereo)
sox -n -r 48000 -b 16 silence.wav synth 10 silence
```

### Method 3: Using ffmpeg

If SoX is unavailable, use ffmpeg:

```bash
cd /home/wnk/code/audioBridge/test-data/audio

# 1kHz sine wave
ffmpeg -f lavfi -i "sine=frequency=1000:duration=5" -ar 48000 -ac 1 -b 16 1khz-sine.wav

# 440Hz tone
ffmpeg -f lavfi -i "sine=frequency=440:duration=5" -ar 48000 -ac 1 -b 16 440hz-tone.wav

# White noise
ffmpeg -f lavfi -i "anullsrc=r=48000:cl=stereo" -filter "anoisenull=-90:0:0" -t 5 white-noise.wav

# Frequency sweep
ffmpeg -f lavfi -i "sine=frequency=20:duration=5" -af "asetrate=48000,volume=1" frequency-sweep.wav

# Silence
ffmpeg -f lavfi -i "anullsrc=r=48000:cl=stereo" -t 10 silence.wav
```

### Method 4: Download Pre-generated Files

You can download pre-generated test files from various sources:

- [Audio Check](https://www.audiocheck.net/) - High-quality test tones
- [Sonic Visualiser Test Tones](https://www.sonicvisualiser.org/) - Various test signals

Place downloaded files in this directory and rename them to match the expected names.

## File Specifications

All files should meet these specifications:

- **Sample Rate**: 48000 Hz
- **Bit Depth**: 16-bit
- **Format**: WAV (PCM)
- **Duration**: As specified above

## Verification

Verify generated files:

```bash
# List files
ls -lh

# Check file info
file 1khz-sine.wav

# Play sample
aplay 1khz-sine.wav

# Or use ffprobe
ffprobe 1khz-sine.wav
```

## Troubleshooting

### Issue: "sox: command not found"

**Solution**: Install SoX package (see Method 1 above)

### Issue: "ffmpeg: command not found"

**Solution**: Install ffmpeg package:
```bash
sudo apt install ffmpeg  # Ubuntu/Debian
sudo dnf install ffmpeg  # Fedora
sudo pacman -S ffmpeg    # Arch
```

### Issue: Generated files are corrupted

**Solution**: Verify SoX/ffmpeg version and try again:
```bash
sox --version
ffmpeg -version
```

---

**Note**: This directory is intentionally empty in the repository. Generate files using one of the methods above.

*Last Updated*: 2025-12-24

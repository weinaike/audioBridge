#!/bin/bash
# setup_virtual_audio.sh - Setup virtual ALSA devices for testing
#
# This script sets up virtual ALSA audio devices (dummy and loopback)
# to enable PortAudio testing without physical audio hardware.
#
# Usage:
#   sudo ./tests/setup_virtual_audio.sh
#
# Requirements:
#   - Linux with ALSA support
#   - Root/sudo privileges
#   - ALSA kernel modules (snd-dummy, snd-aloop)

set -e

echo "=========================================="
echo "  Setting up Virtual ALSA Audio Devices"
echo "=========================================="
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "⚠️  This script requires root privileges."
    echo "Please run with: sudo $0"
    exit 1
fi

# Check if ALSA utils are installed
if ! command -v aplay &> /dev/null; then
    echo "❌ ERROR: ALSA utils not installed."
    echo "Install with: sudo apt-get install alsa-utils"
    exit 1
fi

echo "📦 Loading ALSA kernel modules..."

# Load dummy sound card module
if lsmod | grep -q "^snd_dummy "; then
    echo "  ✅ snd-dummy already loaded"
else
    if modprobe snd-dummy 2>/dev/null; then
        echo "  ✅ Loaded snd-dummy module"
    else
        echo "  ❌ Failed to load snd-dummy module"
        exit 1
    fi
fi

# Load loopback sound card module
if lsmod | grep -q "^snd_aloop "; then
    echo "  ✅ snd-aloop already loaded"
else
    if modprobe snd-aloop 2>/dev/null; then
        echo "  ✅ Loaded snd-aloop module"
    else
        echo "  ⚠️  Failed to load snd-aloop module (may not be critical)"
    fi
fi

echo ""
echo "🔍 Detecting audio devices..."

# Wait a moment for devices to be registered
sleep 1

# Check for dummy device
if aplay -l 2>/dev/null | grep -q "Dummy"; then
    echo "  ✅ Dummy device found:"
    aplay -l 2>/dev/null | grep -A 2 "Dummy" | sed 's/^/     /'
else
    echo "  ⚠️  Dummy device not detected"
fi

# Check for loopback device
if aplay -l 2>/dev/null | grep -q "Loopback"; then
    echo "  ✅ Loopback device found:"
    aplay -l 2>/dev/null | grep -A 2 "Loopback" | sed 's/^/     /'
else
    echo "  ⚠️  Loopback device not detected"
fi

echo ""
echo "📝 Creating ALSA configuration for testing..."

# Create .asoundrc for testing if not exists
if [ ! -f "$HOME/.asoundrc_test" ]; then
    cat > "$HOME/.asoundrc_test" << 'EOF'
# ALSA configuration for audioBridge testing
# Usage: export ALSA_CONFIG_PATH=$HOME/.asoundrc_test

pcm.test-dummy {
    type dummy
    ipc_key 1234
    ipc_perm 0666
}

pcm.test-loopback {
    type hw
    card "Loopback"
    device 0
    subdevice 0
}

ctl.test-loopback {
    type hw
    card "Loopback"
}

# Default to dummy device for testing
pcm.!default {
    type asym
    playback.pcm "test-dummy"
    capture.pcm "test-dummy"
}

ctl.!default {
    type hw
    card "Dummy"
}
EOF
    echo "  ✅ Created $HOME/.asoundrc_test"
    echo ""
    echo "  💡 To use: export ALSA_CONFIG_PATH=$HOME/.asoundrc_test"
else
    echo "  ℹ️  $HOME/.asoundrc_test already exists"
fi

echo ""
echo "=========================================="
echo "  ✅ Virtual Audio Setup Complete!"
echo "=========================================="
echo ""
echo "Available audio devices:"
aplay -l 2>/dev/null | grep -E "(card|Device)" | head -20
echo ""
echo "Testing tips:"
echo "  • Run tests: ./build/unit_tests"
echo "  • List devices: ./build/audioBridge --list"
echo "  • Monitor logs: tail -f audioBridge.log"
echo ""
echo "To cleanup later, run: sudo ./tests/teardown_virtual_audio.sh"
echo ""

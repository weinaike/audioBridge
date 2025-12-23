#!/bin/bash
# teardown_virtual_audio.sh - Cleanup virtual ALSA devices
#
# This script removes the virtual ALSA audio devices setup by
# setup_virtual_audio.sh
#
# Usage:
#   sudo ./tests/teardown_virtual_audio.sh

set -e

echo "=========================================="
echo "  Teardown Virtual ALSA Audio Devices"
echo "=========================================="
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "⚠️  This script requires root privileges."
    echo "Please run with: sudo $0"
    exit 1
fi

echo "📦 Unloading ALSA kernel modules..."

# Unload modules (reverse order)
if lsmod | grep -q "^snd_aloop "; then
    if modprobe -r snd-aloop 2>/dev/null; then
        echo "  ✅ Unloaded snd-aloop module"
    else
        echo "  ⚠️  Failed to unload snd-aloop (may be in use)"
    fi
else
    echo "  ℹ️  snd-aloop not loaded"
fi

if lsmod | grep -q "^snd_dummy "; then
    if modprobe -r snd-dummy 2>/dev/null; then
        echo "  ✅ Unloaded snd-dummy module"
    else
        echo "  ⚠️  Failed to unload snd-dummy (may be in use)"
    fi
else
    echo "  ℹ️  snd-dummy not loaded"
fi

echo ""
echo "🧹 Cleaning up configuration..."

# Optionally remove test configuration
if [ -f "$HOME/.asoundrc_test" ]; then
    read -p "Remove $HOME/.asoundrc_test? (y/N) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        rm -f "$HOME/.asoundrc_test"
        echo "  ✅ Removed .asoundrc_test"
    else
        echo "  ℹ️  Kept .asoundrc_test"
    fi
else
    echo "  ℹ️  .asoundrc_test not found"
fi

echo ""
echo "=========================================="
echo "  ✅ Teardown Complete!"
echo "=========================================="
echo ""
echo "Note: If you still see audio issues, you may need to:"
echo "  1. Restart audio applications"
echo "  2. Reboot your system (last resort)"
echo ""

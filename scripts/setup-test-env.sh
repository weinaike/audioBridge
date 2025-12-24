#!/bin/bash
# Setup script for audioBridge test environment

echo "================================"
echo "audioBridge Test Environment Setup"
echo "================================"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "This script requires root privileges to load kernel modules."
    echo "Please run with sudo:"
    echo "  sudo $0"
    exit 1
fi

echo "[1/3] Loading snd-aloop kernel module..."
if modprobe snd-aloop; then
    echo "✓ snd-aloop module loaded successfully"
else
    echo "✗ Failed to load snd-aloop module"
    exit 1
fi

echo ""
echo "[2/3] Verifying module..."
if lsmod | grep -q snd_aloop; then
    echo "✓ snd-aloop is loaded"
    lsmod | grep snd_aloop
else
    echo "✗ snd-aloop not found in module list"
    exit 1
fi

echo ""
echo "[3/3] Checking for loopback devices..."
if aplay -l | grep -q "Loopback"; then
    echo "✓ Loopback devices detected:"
    aplay -l | grep -A 2 "Loopback"
else
    echo "⚠ No loopback devices found yet (may need to restart audio system)"
fi

echo ""
echo "================================"
echo "✓ Test environment ready!"
echo "================================"
echo ""
echo "Next steps:"
echo "  1. Build the project: cd /home/wnk/code/audioBridge && mkdir build && cd build && cmake .. && make"
echo "  2. List devices: ./tests/tools/audioBridge-test list-devices"
echo "  3. Run tests: ./tests/tools/audioBridge-test setup-check"

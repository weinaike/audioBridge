#!/bin/bash
# Quick test script for audioBridge

set -e

echo "================================"
echo "audioBridge Quick Test"
echo "================================"
echo ""

# Color codes
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check dependencies
echo "[1/5] Checking dependencies..."
missing=0

if ! dpkg -l | grep -q libsndfile1-dev; then
    echo -e "${RED}✗ libsndfile1-dev not found${NC}"
    echo "  Install with: sudo apt-get install -y libsndfile1-dev"
    missing=1
else
    echo -e "${GREEN}✓ libsndfile1-dev found${NC}"
fi

if ! dpkg -l | grep -q portaudio19-dev; then
    echo -e "${RED}✗ portaudio19-dev not found${NC}"
    missing=1
else
    echo -e "${GREEN}✓ portaudio19-dev found${NC}"
fi

if ! dpkg -l | grep -q libasound2-dev; then
    echo -e "${RED}✗ libasound2-dev not found${NC}"
    missing=1
else
    echo -e "${GREEN}✓ libasound2-dev found${NC}"
fi

if [ $missing -eq 1 ]; then
    echo ""
    echo -e "${YELLOW}Please install missing dependencies first${NC}"
    exit 1
fi

echo ""
echo "[2/5] Checking snd-aloop module..."
if lsmod | grep -q snd_aloop; then
    echo -e "${GREEN}✓ snd-aloop module loaded${NC}"
else
    echo -e "${YELLOW}⚠ snd-aloop not loaded${NC}"
    echo "  Load with: sudo modprobe snd-aloop"
    echo ""
    read -p "Load snd-aloop now? (y/n): " load_module
    if [ "$load_module" = "y" ]; then
        sudo modprobe snd-aloop
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}✓ snd-aloop loaded successfully${NC}"
        else
            echo -e "${RED}✗ Failed to load snd-aloop${NC}"
            exit 1
        fi
    else
        echo "Skipping (tests may not work without loopback devices)"
    fi
fi

echo ""
echo "[3/5] Building test tools..."
cd /home/wnk/code/audioBridge/build
cmake .. > /dev/null 2>&1
if make -j$(nproc) 2>&1 | grep -q "audioBridge-test"; then
    echo -e "${GREEN}✓ Build successful${NC}"
else
    echo -e "${YELLOW}⚠ Build may have issues${NC}"
    echo "  Check: ls tests/tools/audioBridge-test"
fi

echo ""
echo "[4/5] Checking for test executable..."
if [ -f tests/tools/audioBridge-test ]; then
    echo -e "${GREEN}✓ Test tool found${NC}"
    ls -lh tests/tools/audioBridge-test
else
    echo -e "${RED}✗ Test tool not found${NC}"
    echo "  Build may have failed. Check CMake output above."
    exit 1
fi

echo ""
echo "[5/5] Running quick tests..."
echo ""

# Test 1: Help command
echo "Test 1: Help command"
./tests/tools/audioBridge-test help > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ PASS${NC}: help command works"
else
    echo -e "${RED}✗ FAIL${NC}: help command failed"
fi

# Test 2: Setup check
echo ""
echo "Test 2: Setup check"
./tests/tools/audioBridge-test setup-check 2>&1 | head -20

echo ""
echo "================================"
echo -e "${GREEN}✓ Quick test complete!${NC}"
echo "================================"
echo ""
echo "Next steps:"
echo "  1. List devices: ./tests/tools/audioBridge-test list-devices"
echo "  2. Validate audio: ./tests/tools/audioBridge-test validate test-audio/1khz-sine.wav"
echo "  3. Full test suite: ./tests/tools/audioBridge-test run-suite default"
echo ""
echo "For more information, see: specs/003-linux-virtual-audio-testing/TESTING_READINESS.md"

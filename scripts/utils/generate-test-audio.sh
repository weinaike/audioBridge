#!/bin/bash

###
# generate-test-audio.sh
#
# Purpose: Generate test audio files for audioBridge virtual loopback testing
# Requirements: SoX (Sound eXchange) installed
# Usage: ./generate-test-audio.sh [output_directory]
###

set -e  # Exit on error

# Configuration
OUTPUT_DIR="${1:-test-data/audio}"
SAMPLE_RATE=48000
BIT_DEPTH=16

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

###
# Function: print_header
# Description: Print script header
###
print_header() {
    echo "========================================"
    echo "  audioBridge Test Audio Generator"
    echo "========================================"
    echo ""
}

###
# Function: check_dependencies
# Description: Check if SoX is installed
###
check_dependencies() {
    echo -n "Checking SoX installation... "

    if ! command -v sox &> /dev/null; then
        echo -e "${RED}FAILED${NC}"
        echo ""
        echo "ERROR: SoX is not installed!"
        echo ""
        echo "Install SoX:"
        echo "  Ubuntu/Debian: sudo apt install sox"
        echo "  Fedora:        sudo dnf install sox"
        echo "  Arch:          sudo pacman -S sox"
        echo ""
        exit 1
    fi

    echo -e "${GREEN}OK${NC}"
    SOX_VERSION=$(sox --version | head -n 1)
    echo "  Found: $SOX_VERSION"
    echo ""
}

###
# Function: create_output_directory
# Description: Create output directory if it doesn't exist
###
create_output_directory() {
    echo -n "Creating output directory: $OUTPUT_DIR... "

    if mkdir -p "$OUTPUT_DIR"; then
        echo -e "${GREEN}OK${NC}"
    else
        echo -e "${RED}FAILED${NC}"
        echo "ERROR: Cannot create directory: $OUTPUT_DIR"
        exit 1
    fi
    echo ""
}

###
# Function: generate_file
# Arguments: $1=filename, $2=duration, $3=description, $4=sox_args
###
generate_file() {
    local filename="$1"
    local duration="$2"
    local description="$3"
    local sox_args="$4"

    local output_path="$OUTPUT_DIR/$filename"

    echo -n "Generating $filename... "
    echo "($description)"

    # Generate audio file
    if eval "sox -n -r $SAMPLE_RATE -b $BIT_DEPTH \"$output_path\" $sox_args" 2>/dev/null; then
        # Verify file created
        if [ -f "$output_path" ]; then
            local filesize=$(ls -lh "$output_path" | awk '{print $5}')
            echo -e "  ${GREEN}DONE${NC} (${filesize})"
        else
            echo -e "  ${RED}FAILED${NC} - File not created"
            return 1
        fi
    else
        echo -e "  ${RED}FAILED${NC} - SoX error"
        return 1
    fi

    echo ""
}

###
# Function: generate_all_files
# Description: Generate all test audio files
###
generate_all_files() {
    echo "Generating test audio files:"
    echo "  Sample Rate: $SAMPLE_RATE Hz"
    echo "  Bit Depth:   $BIT_DEPTH bit"
    echo "  Directory:   $OUTPUT_DIR"
    echo ""

    # T024: 1kHz sine wave (5 seconds, mono)
    generate_file \
        "1khz-sine.wav" \
        5 \
        "1kHz sine wave, 5s, mono" \
        "synth 5 sine 1000"

    # T025: 440Hz tone (5 seconds, mono) - Musical A (A4)
    generate_file \
        "440hz-tone.wav" \
        5 \
        "440Hz musical A, 5s, mono" \
        "synth 5 sine 440"

    # T026: White noise (5 seconds, stereo)
    generate_file \
        "white-noise.wav" \
        5 \
        "White noise, 5s, stereo" \
        "synth 5 noise synth 5 sine 0"  # Noise on both channels

    # T027: Frequency sweep (5 seconds, mono, 20Hz-20kHz)
    generate_file \
        "frequency-sweep.wav" \
        5 \
        "Frequency sweep 20Hz-20kHz, 5s, mono" \
        "synth 5 sine 20-20000"

    # T028: Silence (10 seconds, stereo)
    generate_file \
        "silence.wav" \
        10 \
        "Silence, 10s, stereo" \
        "synth 10 silence"
}

###
# Function: verify_files
# Description: Verify all generated files
###
verify_files() {
    echo "========================================"
    echo "Verifying generated files:"
    echo "========================================"
    echo ""

    local expected_files=(
        "1khz-sine.wav"
        "440hz-tone.wav"
        "white-noise.wav"
        "frequency-sweep.wav"
        "silence.wav"
    )

    local all_ok=true

    for file in "${expected_files[@]}"; do
        local filepath="$OUTPUT_DIR/$file"

        if [ -f "$filepath" ]; then
            local size=$(ls -lh "$filepath" | awk '{print $5}')
            local duration=$(soxi -D "$filepath" 2>/dev/null || echo "unknown")
            local channels=$(soxi -c "$filepath" 2>/dev/null || echo "unknown")
            local rate=$(soxi -r "$filepath" 2>/dev/null || echo "unknown")

            echo -e "${GREEN}✓${NC} $file"
            echo "    Size: $size | Duration: ${duration}s | Channels: $channels | Rate: ${rate}Hz"
        else
            echo -e "${RED}✗${NC} $file - MISSING"
            all_ok=false
        fi
    done

    echo ""

    if [ "$all_ok" = true ]; then
        echo -e "${GREEN}All files generated successfully!${NC}"
        return 0
    else
        echo -e "${RED}Some files are missing!${NC}"
        return 1
    fi
}

###
# Function: print_summary
# Description: Print generation summary
###
print_summary() {
    echo ""
    echo "========================================"
    echo "Summary:"
    echo "========================================"
    echo "  Directory:  $OUTPUT_DIR"
    echo "  Files:      5 test audio files"
    echo "  Total Size: $(du -sh "$OUTPUT_DIR" | awk '{print $1}')"
    echo ""
    echo "Next steps:"
    echo "  1. Verify files: ls -lh $OUTPUT_DIR"
    echo "  2. Play sample: aplay $OUTPUT_DIR/1khz-sine.wav"
    echo "  3. Run test:    ./audioBridge-test run $OUTPUT_DIR/1khz-sine.wav"
    echo ""
    echo "========================================"
}

###
# Main execution
###
main() {
    # Print header
    print_header

    # Check dependencies
    check_dependencies

    # Create output directory
    create_output_directory

    # Generate all files
    generate_all_files

    # Verify files
    if verify_files; then
        print_summary
        exit 0
    else
        echo ""
        echo -e "${RED}ERROR: File generation failed!${NC}"
        echo "Check SoX installation and try again."
        exit 1
    fi
}

# Run main function
main "$@"

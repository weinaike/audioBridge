#!/bin/bash

###
# audioBridge-test.sh
#
# Main shell wrapper for audioBridge Linux virtual audio testing
#
# This script provides convenient access to audioBridge-test functionality
# with additional error handling and user-friendly messages
#
# Usage:
#   ./audioBridge-test.sh <command> [options]
###

set -e  # Exit on error

# Script configuration
SCRIPT_NAME="$(basename "$0")"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
EXECUTABLE="$BUILD_DIR/tests/tools/audioBridge-test"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
BOLD='\033[1m'
NC='\033[0m' # No Color

###
# Function: print_header
# Description: Print script header
###
print_header() {
    echo "========================================"
    echo "  audioBridge Linux Audio Testing Tool"
    echo "========================================"
    echo ""
}

###
# Function: print_error
# Arguments: $1=error_message
###
print_error() {
    echo -e "${RED}ERROR: $1${NC}" >&2
}

###
# Function: print_warning
# Arguments: $1=warning_message
###
print_warning() {
    echo -e "${YELLOW}WARNING: $1${NC}"
}

###
# Function: print_success
# Arguments: $1=success_message
###
print_success() {
    echo -e "${GREEN}$1${NC}"
}

###
# Function: print_info
# Arguments: $1=info_message
###
print_info() {
    echo -e "${BLUE}$1${NC}"
}

###
# Function: check_executable
# Description: Verify audioBridge-test executable exists and is built
###
check_executable() {
    if [ ! -f "$EXECUTABLE" ]; then
        print_error "audioBridge-test executable not found!"
        echo ""
        echo "Expected location: $EXECUTABLE"
        echo ""
        echo "Please build the project first:"
        echo "  cd $PROJECT_ROOT"
        echo "  mkdir -p build && cd build"
        echo "  cmake .."
        echo "  make audioBridge-test"
        echo ""
        exit 1
    fi

    if [ ! -x "$EXECUTABLE" ]; then
        print_error "audioBridge-test is not executable!"
        echo ""
        echo "Run: chmod +x $EXECUTABLE"
        echo ""
        exit 1
    fi
}

###
# Function: check_dependencies
# Description: Verify system dependencies are installed
###
check_dependencies() {
    local missing_deps=()

    # Check for snd-aloop module
    if ! lsmod | grep -q snd_aloop; then
        missing_deps+=("snd-aloop kernel module (run: sudo modprobe snd-aloop)")
    fi

    # Check for ALSA utilities
    if ! command -v aplay &> /dev/null; then
        missing_deps+=("alsa-utils")
    fi

    # Check for SoX (optional but recommended)
    if ! command -v sox &> /dev/null; then
        print_warning "SoX not installed (optional but recommended)"
        print_info "Install: sudo apt install sox  # Ubuntu/Debian"
    fi

    if [ ${#missing_deps[@]} -gt 0 ]; then
        print_error "Missing dependencies:"
        echo ""
        for dep in "${missing_deps[@]}"; do
            echo "  - $dep"
        done
        echo ""
        print_info "Run: ./audioBridge-test.sh setup-check"
        echo ""
        exit 1
    fi
}

###
# Function: show_usage
# Description: Display usage information
###
show_usage() {
    cat << EOF
Usage: $SCRIPT_NAME <command> [options]

Commands:
  list-devices      List all available audio devices
  setup-check       Verify system setup and dependencies
  run               Run single test
  run-suite         Run test suite
  help              Show this help message

Options for list-devices:
  --type <type>     Filter by device type (loopback, physical, virtual)
  --direction <dir> Filter by direction (input, output, duplex)
  --json            Output in JSON format
  --verbose         Verbose output

Options for run:
  --devices <pb,cap> Specify playback and capture devices (e.g., hw:1,0,hw:1,1)

Examples:
  $SCRIPT_NAME list-devices
  $SCRIPT_NAME list-devices --type loopback
  $SCRIPT_NAME list-devices --json
  $SCRIPT_NAME setup-check
  $SCRIPT_NAME run test-data/audio/1khz-sine.wav
  $SCRIPT_NAME run test-data/audio/1khz-sine.wav --devices hw:1,0,hw:1,1

For more information:
  See: $PROJECT_ROOT/docs/linux-audio-testing/
  Or: $PROJECT_ROOT/specs/003-linux-virtual-audio-testing/quickstart.md

EOF
}

###
# Function: run_setup_check
# Description: Run setup check with enhanced output
###
run_setup_check() {
    print_info "Running system setup check..."
    echo ""

    # Run the actual check
    "$EXECUTABLE" setup-check "$@"
    local result=$?

    echo ""

    if [ $result -eq 0 ]; then
        print_success "Setup check passed!"
        echo ""
        echo "Your system is ready for audioBridge testing."
        echo ""
        echo "Next steps:"
        echo "  1. List devices: $SCRIPT_NAME list-devices"
        echo "  2. Run a test:   $SCRIPT_NAME run test-data/audio/1khz-sine.wav"
        echo ""
    else
        print_error "Setup check failed!"
        echo ""
        echo "Please fix the issues above before running tests."
        echo ""
        echo "For help, see: $PROJECT_ROOT/docs/linux-audio-testing/troubleshooting.md"
        echo ""
    fi

    return $result
}

###
# Function: run_list_devices
# Description: List devices with enhanced output
###
run_list_devices() {
    print_info "Enumerating audio devices..."
    echo ""

    "$EXECUTABLE" list-devices "$@"
    local result=$?

    echo ""

    # Provide guidance if no devices found
    if [ $result -ne 0 ] || ! "$EXECUTABLE" list-devices "$@" 2>/dev/null | grep -q "\["; then
        print_warning "No audio devices found or error occurred"
        echo ""
        echo "Troubleshooting:"
        echo "  1. Check if snd-aloop module is loaded: lsmod | grep snd_aloop"
        echo "  2. Load module: sudo modprobe snd-aloop"
        echo "  3. Verify devices: aplay -l | grep Loopback"
        echo "  4. Run setup check: $SCRIPT_NAME setup-check"
        echo ""
    fi

    return $result
}

###
# Function: run_test
# Description: Run single test with enhanced output
###
run_test() {
    local test_file="$1"

    if [ -z "$test_file" ]; then
        print_error "Missing test audio file"
        echo ""
        echo "Usage: $SCRIPT_NAME run <test-file.wav>"
        echo ""
        echo "Available test files:"
        if [ -d "$PROJECT_ROOT/test-data/audio" ]; then
            ls -1 "$PROJECT_ROOT/test-data/audio"/*.wav 2>/dev/null | while read file; do
                echo "  - $(basename "$file")"
            done
        fi
        echo ""
        echo "Generate test audio: $PROJECT_ROOT/scripts/utils/generate-test-audio.sh"
        echo ""
        exit 1
    fi

    # Check if test file exists
    if [ ! -f "$test_file" ]; then
        # Try relative to project root
        if [ -f "$PROJECT_ROOT/$test_file" ]; then
            test_file="$PROJECT_ROOT/$test_file"
        else
            print_error "Test file not found: $test_file"
            echo ""
            exit 1
        fi
    fi

    print_info "Running test with: $(basename "$test_file")"
    echo ""

    # Run the test
    "$EXECUTABLE" run "$test_file" "${@:2}"
    local result=$?

    return $result
}

###
# Function: show_installation_instructions
# Description: Show installation and setup instructions
###
show_installation_instructions() {
    cat << EOF
${BOLD}audioBridge Linux Audio Testing Tool${NC}

Installation Instructions:
---------------------------

1. Install Dependencies:
   Ubuntu/Debian:
     sudo apt update
     sudo apt install alsa-utils sox libsndfile1 build-essential cmake git

   Fedora:
     sudo dnf install alsa-utils sox libsndfile gcc-c++ cmake git

   Arch:
     sudo pacman -S alsa-utils sox libsndfile gcc cmake make base-devel

2. Load snd-aloop Kernel Module:
   sudo modprobe snd-aloop

   Enable auto-load at boot:
     echo "snd-aloop" | sudo tee /etc/modules-load.d/snd-aloop.conf

3. Build audioBridge:
   cd $PROJECT_ROOT
   mkdir -p build && cd build
   cmake ..
   make audioBridge-test

4. Verify Setup:
   ./scripts/audioBridge-test.sh setup-check

5. Run First Test:
   ./scripts/audioBridge-test.sh run test-data/audio/1khz-sine.wav

For detailed setup guides, see:
  - $PROJECT_ROOT/docs/linux-audio-testing/setup-ubuntu.md
  - $PROJECT_ROOT/docs/linux-audio-testing/setup-fedora.md
  - $PROJECT_ROOT/docs/linux-audio-testing/setup-arch.md

For troubleshooting, see:
  - $PROJECT_ROOT/docs/linux-audio-testing/troubleshooting.md

For quickstart guide, see:
  - $PROJECT_ROOT/specs/003-linux-virtual-audio-testing/quickstart.md

EOF
}

###
# Main execution
###
main() {
    # Parse command
    local command="$1"
    shift || true  # Remove command from arguments

    # Handle no command
    if [ -z "$command" ]; then
        print_header
        show_installation_instructions
        exit 0
    fi

    # Handle help command
    if [ "$command" = "help" ] || [ "$command" = "--help" ] || [ "$command" = "-h" ]; then
        show_usage
        exit 0
    fi

    # Check if executable exists
    check_executable

    # Route to command handler
    case "$command" in
        list-devices)
            run_list_devices "$@"
            ;;
        setup-check)
            run_setup_check "$@"
            ;;
        run)
            run_test "$@"
            ;;
        run-suite)
            print_info "Running test suite..."
            echo ""
            "$EXECUTABLE" run-suite "$@"
            ;;
        *)
            print_error "Unknown command: $command"
            echo ""
            show_usage
            exit 1
            ;;
    esac
}

# Run main function
main "$@"

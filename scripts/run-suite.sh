#!/bin/bash
#
# audioBridge Test Suite Runner
#
# This script provides a user-friendly wrapper for running automated test suites.
# It handles environment setup, error checking, and result aggregation.
#
# Usage:
#   ./scripts/run-suite.sh [suite-name] [options]
#
# Examples:
#   ./scripts/run-suite.sh default
#   ./scripts/run-suite.sh default --continue-on-error
#   ./scripts/run-suite.sh comprehensive --verbose
#

set -e

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
RESET='\033[0m'

# Default values
SUITE_NAME="default"
CONTINUE_ON_ERROR=false
VERBOSE=false
SHOW_HELP=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            SHOW_HELP=true
            shift
            ;;
        -s|--suite)
            SUITE_NAME="$2"
            shift 2
            ;;
        -c|--continue-on-error)
            CONTINUE_ON_ERROR=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -*)
            echo -e "${RED}Unknown option: $1${RESET}"
            echo "Use --help for usage information"
            exit 1
            ;;
        *)
            # Positional argument is treated as suite name
            SUITE_NAME="$1"
            shift
            ;;
    esac
done

# Show help message
show_help() {
    cat << EOF
${CYAN}audioBridge Test Suite Runner${RESET}

${YELLOW}Usage:${RESET}
  $0 [suite-name] [options]

${YELLOW}Arguments:${RESET}
  suite-name              Test suite to run (default: 'default')
                          Available: default, quick, comprehensive

${YELLOW}Options:${RESET}
  -h, --help              Show this help message
  -s, --suite <name>      Specify test suite name
  -c, --continue-on-error Continue running tests even after failures
  -v, --verbose           Enable verbose output

${YELLOW}Examples:${RESET}
  $0 default                          # Run default suite
  $0 quick                            # Run quick test suite
  $0 comprehensive --continue-on-error # Run comprehensive suite, don't stop on errors
  $0 default -v -c                    # Run with verbose output, continue on errors

${YELLOW}Available Test Suites:${RESET}
  default            Standard test suite (5 tests)
  quick              Quick validation tests (2 tests)
  comprehensive      Full test suite with extended validation (10+ tests)

${YELLOW}Test Results:${RESET}
  Results are saved to: build/test-results/
  JUnit XML reports are generated for CI/CD integration

${YELLOW}Environment:${RESET}
  - Requires snd-aloop kernel module loaded
  - Requires audioBridge-test binary built
  - Test suite config must exist in test-data/configs/

${YELLOW}For more information:${RESET}
  See docs/linux-audio-testing/README.md

EOF
}

if [ "$SHOW_HELP" = true ]; then
    show_help
    exit 0
fi

# Print header
print_header() {
    echo -e "${CYAN}========================================${RESET}"
    echo -e "${CYAN}  audioBridge Test Suite Runner${RESET}"
    echo -e "${CYAN}========================================${RESET}"
    echo ""
    echo -e "${BLUE}Test Suite:${RESET} ${SUITE_NAME}"
    echo -e "${BLUE}Continue on Error:${RESET} ${CONTINUE_ON_ERROR}"
    echo -e "${BLUE}Verbose Mode:${RESET} ${VERBOSE}"
    echo ""
}

# Print section header
print_section() {
    echo -e "${MAGENTA}>>> $1${RESET}"
}

# Check if audioBridge-test binary exists
check_binary() {
    print_section "Checking Build"

    if [ ! -f "${PROJECT_ROOT}/build/tests/tools/audioBridge-test" ]; then
        echo -e "${RED}✗ audioBridge-test binary not found${RESET}"
        echo ""
        echo "Please build the project first:"
        echo "  mkdir -p build && cd build"
        echo "  cmake .."
        echo "  make audioBridge-test"
        echo ""
        exit 1
    fi

    echo -e "${GREEN}✓ Binary found${RESET}"
}

# Check if snd-aloop module is loaded
check_kernel_module() {
    print_section "Checking Kernel Module"

    if ! lsmod | grep -q "^snd_aloop "; then
        echo -e "${YELLOW}⚠ snd-aloop module not loaded${RESET}"
        echo "Attempting to load module..."
        echo ""

        if sudo modprobe snd-aloop; then
            echo -e "${GREEN}✓ Module loaded successfully${RESET}"
        else
            echo -e "${RED}✗ Failed to load snd-aloop module${RESET}"
            echo ""
            echo "Please load the module manually:"
            echo "  sudo modprobe snd-aloop"
            echo ""
            exit 1
        fi
    else
        echo -e "${GREEN}✓ snd-aloop module is loaded${RESET}"
    fi
}

# Check if test suite configuration exists
check_suite_config() {
    print_section "Checking Test Suite Configuration"

    local config_file="${PROJECT_ROOT}/test-data/configs/suite-${SUITE_NAME}.json"

    if [ ! -f "$config_file" ]; then
        echo -e "${RED}✗ Test suite configuration not found:${RESET}"
        echo "  ${config_file}"
        echo ""
        echo "Available test suites:"
        for f in "${PROJECT_ROOT}"/test-data/configs/suite-*.json; do
            if [ -f "$f" ]; then
                local basename=$(basename "$f" .json)
                local suite_name=${basename#suite-}
                echo "  - ${suite_name}"
            fi
        done
        echo ""
        exit 1
    fi

    echo -e "${GREEN}✓ Configuration found${RESET}"

    if [ "$VERBOSE" = true ]; then
        echo ""
        echo "Test suite: ${SUITE_NAME}"
        local test_count=$(grep -c '"name"' "$config_file" || echo "0")
        echo "Number of tests: ${test_count}"
    fi
}

# Generate test audio if needed
check_test_audio() {
    print_section "Checking Test Audio"

    local audio_dir="${PROJECT_ROOT}/test-data/audio"

    if [ ! -d "$audio_dir" ]; then
        echo -e "${YELLOW}⚠ Test audio directory not found${RESET}"
        echo "Creating directory..."
        mkdir -p "$audio_dir"
    fi

    # Check for at least one test audio file
    if ! ls "${audio_dir}"/*.wav 1> /dev/null 2>&1; then
        echo -e "${YELLOW}⚠ No test audio files found${RESET}"
        echo "Attempting to generate test audio..."
        echo ""

        local generator_script="${PROJECT_ROOT}/scripts/utils/generate-test-audio.sh"

        if [ -f "$generator_script" ]; then
            chmod +x "$generator_script"
            if bash "$generator_script"; then
                echo -e "${GREEN}✓ Test audio generated${RESET}"
            else
                echo -e "${YELLOW}⚠ Test audio generation failed (continuing anyway)${RESET}"
            fi
        else
            echo -e "${YELLOW}⚠ Generator script not found${RESET}"
            echo "You may need to create test audio files manually"
        fi
    else
        local audio_count=$(ls "${audio_dir}"/*.wav 2>/dev/null | wc -l)
        echo -e "${GREEN}✓ Test audio files found (${audio_count} files)${RESET}"
    fi
}

# Run setup check
run_setup_check() {
    print_section "Running Setup Check"

    cd "${PROJECT_ROOT}/build"

    if ./tests/tools/audioBridge-test setup-check; then
        echo -e "${GREEN}✓ Setup check passed${RESET}"
    else
        echo -e "${YELLOW}⚠ Setup check reported warnings${RESET}"
    fi

    echo ""
}

# List available devices
list_devices() {
    if [ "$VERBOSE" = true ]; then
        print_section "Available Audio Devices"

        cd "${PROJECT_ROOT}/build"
        ./tests/tools/audioBridge-test list-devices
        echo ""
    fi
}

# Run the test suite
run_test_suite() {
    print_section "Running Test Suite"

    cd "${PROJECT_ROOT}/build"

    local cmd="./tests/tools/audioBridge-test run-suite ${SUITE_NAME}"

    if [ "$CONTINUE_ON_ERROR" = true ]; then
        cmd="${cmd} --continue-on-error"
    fi

    if [ "$VERBOSE" = true ]; then
        cmd="${cmd} --verbose"
    fi

    echo -e "${BLUE}Command:${RESET} ${cmd}"
    echo ""

    # Run the test suite and capture exit code
    if $cmd; then
        local exit_code=$?
        echo ""
        echo -e "${GREEN}========================================${RESET}"
        echo -e "${GREEN}✓ Test Suite PASSED${RESET}"
        echo -e "${GREEN}========================================${RESET}"
        return 0
    else
        local exit_code=$?
        echo ""
        echo -e "${RED}========================================${RESET}"
        echo -e "${RED}✗ Test Suite FAILED${RESET}"
        echo -e "${RED}========================================${RESET}"
        return $exit_code
    fi
}

# Show test results location
show_results() {
    print_section "Test Results"

    local results_dir="${PROJECT_ROOT}/build/test-results"

    if [ -d "$results_dir" ]; then
        echo -e "${GREEN}Results saved to:${RESET} ${results_dir}"

        # List generated files
        echo ""
        echo "Generated files:"
        find "$results_dir" -type f -name "*.xml" -o -name "*.json" -o -name "*.txt" | while read -r file; do
            local basename=$(basename "$file")
            local size=$(du -h "$file" | cut -f1)
            echo "  - ${basename} (${size})"
        done
    else
        echo -e "${YELLOW}No results directory found${RESET}"
    fi
}

# Cleanup on exit
cleanup() {
    # Optional: Unload kernel module (commented out for safety)
    # sudo modprobe -r snd-aloop 2>/dev/null || true
    :
}

# Trap to ensure cleanup runs
trap cleanup EXIT

# Main execution
main() {
    print_header

    # Pre-flight checks
    check_binary
    check_kernel_module
    check_suite_config
    check_test_audio
    echo ""

    # Optional setup verification
    run_setup_check
    list_devices

    # Run the test suite
    if run_test_suite; then
        local result=0
    else
        local result=$?
    fi

    echo ""

    # Show results location
    show_results

    echo ""

    exit $result
}

# Run main function
main

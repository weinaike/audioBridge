#!/bin/bash

###############################################################################
# detect-distribution.sh
#
# Detects the Linux distribution (Ubuntu, Fedora, Arch, etc.)
#
# Outputs: Distribution name (lowercase)
#          ubuntu, fedora, arch, debian, centos, rhel, or "unknown"
###############################################################################

set -e

# Function to detect distribution
detect_distro() {
    if [ -f /etc/os-release ]; then
        # Modern systems with /etc/os-release
        . /etc/os-release

        # Extract ID and convert to lowercase
        DISTRO=$(echo "$ID" | tr '[:upper:]' '[:lower:]')

        # Handle common distributions
        case "$DISTRO" in
            ubuntu)
                echo "ubuntu"
                ;;
            fedora)
                echo "fedora"
                ;;
            arch|archlinux|manjaro|endeavouros)
                echo "arch"
                ;;
            debian)
                echo "debian"
                ;;
            centos|rhel|rocky|almalinux)
                echo "rhel"
                ;;
            *)
                echo "unknown"
                ;;
        esac
    elif [ -f /etc/redhat-release ]; then
        echo "rhel"
    elif [ -f /etc/debian_version ]; then
        echo "debian"
    elif [ -f /etc/arch-release ]; then
        echo "arch"
    elif [ -f /etc/ubuntu-release ]; then
        echo "ubuntu"
    else
        echo "unknown"
    fi
}

# Main execution
DISTRO=$(detect_distro)
echo "$DISTRO"

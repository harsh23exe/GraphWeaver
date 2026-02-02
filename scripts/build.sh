#!/bin/bash
# Build script for doc-scraper

set -e

# Default values
BUILD_TYPE="${BUILD_TYPE:-Debug}"
BUILD_DIR="${BUILD_DIR:-build}"
JOBS="${JOBS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)}"

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --clean)
            echo "Cleaning build directory..."
            rm -rf "${BUILD_DIR}"
            shift
            ;;
        --sanitizers)
            EXTRA_CMAKE_FLAGS="-DBUILD_WITH_SANITIZERS=ON"
            shift
            ;;
        -j*)
            JOBS="${1#-j}"
            shift
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

echo "=== Building doc-scraper ==="
echo "Build type: ${BUILD_TYPE}"
echo "Build directory: ${BUILD_DIR}"
echo "Parallel jobs: ${JOBS}"
echo ""

# Create build directory
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Configure
echo "Configuring..."
cmake .. \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    ${EXTRA_CMAKE_FLAGS}

# Build
echo "Building..."
cmake --build . -j"${JOBS}"

echo ""
echo "=== Build complete ==="
echo "Executable: ${BUILD_DIR}/crawler"

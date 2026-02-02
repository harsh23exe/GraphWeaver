#!/bin/bash
# Test script for doc-scraper

set -e

BUILD_DIR="${BUILD_DIR:-build}"

# Check if build directory exists
if [[ ! -d "${BUILD_DIR}" ]]; then
    echo "Build directory not found. Running build first..."
    ./scripts/build.sh
fi

cd "${BUILD_DIR}"

echo "=== Running tests ==="
ctest --output-on-failure --verbose "$@"

echo ""
echo "=== All tests passed ==="

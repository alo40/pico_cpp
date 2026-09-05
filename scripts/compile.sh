#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
FIRMWARE_BUILD_DIR="$PROJECT_ROOT/build"
UNIT_TEST_SOURCE_DIR="$PROJECT_ROOT/unit test"
UNIT_TEST_BUILD_DIR="$UNIT_TEST_SOURCE_DIR/build"

if [[ "${1:-}" == "clean" ]]; then
    echo "Removing build directories..."
    cmake -E remove_directory "$FIRMWARE_BUILD_DIR"
    cmake -E remove_directory "$UNIT_TEST_BUILD_DIR"
elif [[ $# -gt 0 ]]; then
    echo "Usage: $0 [clean]" >&2
    exit 2
fi

echo "Configuring Pico firmware..."
cmake -S "$PROJECT_ROOT" -B "$FIRMWARE_BUILD_DIR"

echo "Building Pico firmware..."
cmake --build "$FIRMWARE_BUILD_DIR"

echo "Configuring parser unit tests..."
cmake -S "$UNIT_TEST_SOURCE_DIR" -B "$UNIT_TEST_BUILD_DIR"

echo "Building parser unit tests..."
cmake --build "$UNIT_TEST_BUILD_DIR"

echo "Running parser unit tests..."
ctest --test-dir "$UNIT_TEST_BUILD_DIR" --output-on-failure

echo "Firmware build and parser unit tests completed successfully."

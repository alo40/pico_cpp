#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
UF2_FILE="$PROJECT_ROOT/build/pico_app.uf2"

if [[ $# -gt 1 || ( $# -eq 1 && "$1" != "clean" ) ]]; then
    echo "Usage: $0 [clean]" >&2
    exit 2
fi

if [[ $# -eq 1 ]]; then
    "$SCRIPT_DIR/compile.sh" clean
else
    "$SCRIPT_DIR/compile.sh"
fi

if [[ ! -f "$UF2_FILE" ]]; then
    echo "ERROR: UF2 file not found: $UF2_FILE" >&2
    exit 1
fi

if ! command -v picotool >/dev/null 2>&1; then
    echo "ERROR: picotool is not installed or not available in PATH." >&2
    exit 1
fi

echo "Flashing $UF2_FILE..."
picotool load -f -x "$UF2_FILE"

echo "Build, tests, and flash completed successfully."

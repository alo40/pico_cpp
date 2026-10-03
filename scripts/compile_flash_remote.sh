#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

UF2_FILE="$PROJECT_ROOT/build/pico_app.uf2"

REMOTE_HOST="raspi"
REMOTE_UF2="/tmp/pico_app.uf2"

if [[ $# -gt 1 || ( $# -eq 1 && "$1" != "clean" ) ]]; then
    echo "Usage: $0 [clean]" >&2
    exit 2
fi

echo "========================================"
echo "Building firmware and running tests..."
echo "========================================"

if [[ $# -eq 1 ]]; then
    "$SCRIPT_DIR/compile.sh" clean
else
    "$SCRIPT_DIR/compile.sh"
fi

if [[ ! -f "$UF2_FILE" ]]; then
    echo "ERROR: UF2 file not found: $UF2_FILE" >&2
    exit 1
fi

echo
echo "========================================"
echo "Checking Raspberry Pi connection..."
echo "========================================"

if ! ssh "$REMOTE_HOST" 'true'; then
    echo "ERROR: Could not connect to Raspberry Pi: $REMOTE_HOST" >&2
    exit 1
fi

echo
echo "========================================"
echo "Checking picotool on Raspberry Pi..."
echo "========================================"

if ! ssh "$REMOTE_HOST" 'command -v picotool >/dev/null 2>&1'; then
    echo "ERROR: picotool is not installed on Raspberry Pi." >&2
    exit 1
fi

echo
echo "========================================"
echo "Copying UF2 to Raspberry Pi..."
echo "========================================"

scp "$UF2_FILE" "${REMOTE_HOST}:${REMOTE_UF2}"

echo
echo "========================================"
echo "Flashing Pico remotely..."
echo "========================================"

ssh "$REMOTE_HOST" "picotool load -f -x '$REMOTE_UF2'"

echo
echo "========================================"
echo "Cleaning temporary UF2 on Raspberry Pi..."
echo "========================================"

ssh "$REMOTE_HOST" "rm -f '$REMOTE_UF2'"

echo
echo "========================================"
echo "SUCCESS"
echo "========================================"
echo "Build, tests, transfer, and remote flash completed successfully."
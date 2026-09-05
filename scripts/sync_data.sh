#!/usr/bin/env bash

set -euo pipefail

usage() {
    echo "Usage: $0 [--dry-run]" >&2
}

dry_run=false

case "${1:-}" in
    "")
        ;;
    --dry-run)
        dry_run=true
        ;;
    *)
        usage
        exit 2
        ;;
esac

if (( $# > 1 )); then
    usage
    exit 2
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd "$script_dir/.." && pwd)"
data_dir="$project_root/data"
raw_destination="$data_dir/raw/"
processed_destination="$data_dir/processed/"

raw_source="raspi:~/pico_cpp/data/raw/"
processed_source="raspi:~/pico_cpp/data/processed/"

mkdir -p "$raw_destination" "$processed_destination"

rsync_options=(-rtv)
if [[ "$dry_run" == true ]]; then
    rsync_options+=(--dry-run)
    echo "Dry run: local dataset files and data/.last_sync will not be changed."
fi

sync_directory() {
    local label="$1"
    local source="$2"
    local destination="$3"

    echo "Synchronizing $label data"
    echo "  Source:      $source"
    echo "  Destination: $destination"
    rsync "${rsync_options[@]}" "$source" "$destination"
}

sync_directory "raw" "$raw_source" "$raw_destination"
sync_directory "processed" "$processed_source" "$processed_destination"

if [[ "$dry_run" == false ]]; then
    date '+%Y-%m-%dT%H:%M:%S%z' > "$data_dir/.last_sync"
    echo "Last successful sync: $(<"$data_dir/.last_sync")"
fi

echo "Synchronization completed successfully."

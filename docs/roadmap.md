# Project Roadmap

This checklist summarizes implemented project capabilities. Active work is
tracked in [`next_steps.md`](next_steps.md), while operational details and
verification evidence are documented in [`data_logging.md`](data_logging.md).

## Pico Acquisition

- [x] Configure VE.Direct UART for 19200 baud, 8N1, with no flow control.
- [x] Receive UART data through an interrupt-driven ring buffer.
- [x] Keep VE.Direct parsing outside the interrupt handler.
- [x] Extract the parser into a hardware-independent module.
- [x] Parse battery voltage, panel voltage, battery current, and panel power.
- [x] Validate complete VE.Direct blocks using the modulo-256 checksum.
- [x] Publish measurements only after receiving a complete valid block.
- [x] Reject invalid, incomplete, malformed, and overlong blocks.
- [x] Recover parsing after rejected input.
- [x] Track received, valid, checksum-invalid, and incomplete block counts.
- [x] Assign one sequence number to each published measurement snapshot.

## Display And USB Output

- [x] Display current measurements and voltage history on the OLED screens.
- [x] Display parser-health counters for acquisition diagnostics.
- [x] Emit one USB CSV row for each validated measurement snapshot.
- [x] Include a CSV header and publication sequence number.
- [x] Prevent incomplete or checksum-invalid measurements from reaching CSV output.

## Automated Validation

- [x] Build the Pico firmware with CMake and the Pico SDK.
- [x] Test the production parser independently of Pico hardware.
- [x] Cover valid, corrupted, truncated, malformed, repeated, and recovery cases.
- [x] Test parser counters and binary checksum-byte handling.
- [x] Test logger parsing, file handling, rotation, and restart behavior.
- [x] Provide scripts for building firmware, running tests, and optional flashing.

## Persistent Logging

- [x] Validate the original direct Pico-to-Mac logging workflow.
- [x] Preserve received serial lines in raw log files.
- [x] Write validated rows to processed CSV files.
- [x] Add timezone-aware logger-host timestamps.
- [x] Detect and report sequence discontinuities.
- [x] Create aligned daily raw and processed files.
- [x] Append safely after a same-day logger restart.
- [x] Avoid duplicate CSV headers.
- [x] Rotate files at the Raspberry Pi local date boundary.
- [x] Verify daily rotation through automated tests and real hardware operation.

## Raspberry Pi Service

- [x] Use a stable `/dev/serial/by-id/` Pico device path.
- [x] Run the logger through `vedirect-logger.service`.
- [x] Store persistent data under the Raspberry Pi project data directory.
- [x] Configure controlled SIGINT shutdown and journald diagnostics.
- [x] Configure automatic restart after unexpected failure.
- [x] Enable service startup for normal multi-user boot.
- [x] Verify controlled service start, stop, and restart behavior.
- [x] Verify logging continues after the administering SSH session ends.
- [x] Verify logging continues while synchronization runs.

## Data Synchronization

- [x] Synchronize raw and processed data separately with `rsync`.
- [x] Use SSH host `raspi` without hard-coding its IP address.
- [x] Resolve Mac destinations relative to the project.
- [x] Provide a non-modifying synchronization dry run.
- [x] Keep synchronization non-destructive.
- [x] Record successful synchronization time in `data/.last_sync`.
- [x] Verify full, incremental, and actively growing file transfers.
- [x] Keep synchronization independent from Jupyter notebooks.

## Local Analysis

- [x] Analyze synchronized local CSV files without network access.
- [x] Require explicit selection of one processed CSV.
- [x] Reject absolute, escaping, missing, and non-CSV dataset paths.
- [x] Validate the processed CSV schema and measurement values.
- [x] Preserve timezone-aware logger-host timestamps.
- [x] Report dataset identity, duration, and timing integrity.
- [x] Classify sequence gaps, duplicates, and restarts separately.
- [x] Convert stored measurement units for presentation.
- [x] Produce descriptive statistics and four time-series plots.
- [x] Save a clean top-to-bottom notebook execution.

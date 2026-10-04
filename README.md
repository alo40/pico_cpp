# VE.Direct Solar Monitoring Project

This repository implements an end-to-end solar-monitoring workflow around a Victron SmartSolar MPPT and a Raspberry Pi Pico.

The Pico acquires and validates VE.Direct data, publishes validated measurements over USB serial, and drives the local OLED display. A Raspberry Pi runs the persistent logger and browser dashboard. Measurement data can be synchronized to a Mac for offline Jupyter analysis.

## System overview

```text
Physical solar system
  -> Victron SmartSolar MPPT
  -> VE.Direct
  -> Raspberry Pi Pico
  -> USB serial
  -> Raspberry Pi
       -> persistent raw and processed data
       -> live and historical browser dashboard
       -> synchronization to Mac
  -> Mac
       -> synchronized local data
       -> Jupyter analysis
```

The main architectural boundary is:

```text
acquisition -> storage -> synchronization -> analysis
```

The browser dashboard is a read-only consumer of processed CSV data. It does not access the Pico serial device and does not write measurement files.

## Repository layout

```text
analysis/notebooks/     Offline Jupyter analysis
data/raw/               Raw serial logs
data/processed/         Validated processed CSV data
docs/PROJECT.md         Canonical architecture, status, verification, and active work
docs/COMMANDS.md        Operational command reference
examples/               Firmware and toolchain examples/experiments
include/                Public/reusable firmware headers
scripts/                Build, logging, dashboard, and synchronization tools
src/                    Pico firmware implementation and private headers
systemd/                Raspberry Pi service units
unit test/              Production-focused automated tests
tests/                  Compiler/linking experiments
openspec/               Change proposals/specifications
```

## Firmware modules

The active Pico firmware is built from:

```text
src/main.c
src/dashboard.c
src/ssd1306.c
src/vedirect_parser.c
src/vedirect_uart.c
```

Important ownership rules:

- `src/vedirect_uart.c` owns Pico UART transport and parser polling.
- `src/vedirect_parser.c` contains hardware-independent VE.Direct parser logic.
- `include/vedirect_parser.h` is the parser's public API.
- `src/dashboard.c` owns Pico/OLED dashboard state and rendering.
- `src/ssd1306.c` and `include/ssd1306.h` contain SSD1306 display support.
- Keep Pico SDK and hardware-specific dependencies out of the VE.Direct parser.

## Processed data schema

Processed CSV files use:

```csv
timestamp,sequence,battery_mv,panel_mv,battery_ma,panel_w
```

Semantics:

- `timestamp`: Raspberry Pi logger local receive/processing timestamp with timezone offset.
- `sequence`: Pico publication sequence; monotonic only within one firmware execution.
- `battery_mv`: battery voltage in mV.
- `panel_mv`: panel voltage in mV.
- `battery_ma`: battery current in mA; positive means charging and negative means discharging.
- `panel_w`: panel power in W.

The project does not directly measure solar irradiance, ambient temperature, panel temperature, or accurate battery state of charge.

## Quick start

Build the firmware from the repository root:

```sh
cmake -S . -B build -G Ninja
cmake --build build
```

Run the documented test workflow:

```sh
cmake -S "unit test" -B "unit test/build"
cmake --build "unit test/build"
ctest --test-dir "unit test/build" --output-on-failure
```

Synchronize Raspberry Pi data to the Mac:

```sh
./scripts/sync_data.sh --dry-run
./scripts/sync_data.sh
```

See `docs/COMMANDS.md` for operational commands and `docs/PROJECT.md` for the canonical technical project state.

## Documentation

- `README.md`: entry point and quick orientation.
- `AGENTS.md`: repository rules for coding agents.
- `docs/PROJECT.md`: canonical architecture, implemented capabilities, verification status, limitations, and active work.
- `docs/COMMANDS.md`: concise operational command reference.

When implementation status changes, update `docs/PROJECT.md` only after the corresponding implementation and tests/verification are complete.

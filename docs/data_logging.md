# macOS VE.Direct Data Logging

The Pico validates VE.Direct measurements and sends CSV rows over USB. The Mac
logger preserves the raw serial stream and creates a timestamped CSV for later
analysis.

The acquisition system should be developed in service of the analysis. New
sensors, VE.Direct fields, and firmware features should be introduced when they
answer a concrete engineering question or improve data reliability.

## Data flow

```text
Physical solar system
  -> Victron MPPT
  -> Pico acquisition and validation
  -> USB CSV
  -> Mac logger
  -> raw and processed files
  -> Python analysis
  -> engineering conclusions
```

The Pico acquisition layer owns VE.Direct reception, checksum validation,
atomic snapshots, sequence assignment, and USB transport. The Mac storage layer
owns raw capture, local timestamps, structured CSV, and persistent files. The
analysis layer will own integrity checks, visualization, derived quantities,
and engineering interpretation.

## Setup

Find the Pico manually if needed:

```sh
ls /dev/cu.usbmodem*
```

Direct USB rows have been verified on the current hardware with:

```sh
cat /dev/cu.usbmodem101
```

This produced real rows such as:

```csv
23,13740,23310,-100,2
24,13740,23290,-100,2
25,13740,23310,-100,2
```

`screen` is not required; it failed in the tested macOS setup. A real saved
session through `log_vedirect.py` is still unverified.

Install the serial dependency:

```sh
python3 -m pip install pyserial
```

## Start logging

With exactly one Pico USB serial device connected, run:

```sh
python3 scripts/log_vedirect.py
```

To select a specific device, run:

```sh
python3 scripts/log_vedirect.py --port /dev/cu.usbmodemXXXX
```

Press Ctrl+C to stop cleanly. Each session creates two files relative to the
repository, even when the script is launched from another working directory:

```text
data/raw/vedirect_<session>.log
data/processed/vedirect_<session>.csv
```

The raw log preserves what was received from the Pico for traceability,
debugging, and recovery. The processed CSV accepts only Pico payloads containing
exactly five integer fields, prepends a local ISO-8601 timestamp, and writes the
resulting six-column record. It is the intended input for Python analysis:

```csv
timestamp,sequence,battery_mv,panel_mv,battery_ma,panel_w
2026-09-05T12:45:31.421+02:00,1,13241,18470,840,15
```

Malformed lines remain available in the raw log but are excluded from the
processed CSV. Sequence gaps produce terminal warnings without stopping the
logger.

The Pico sequence is monotonic only within one firmware execution. A reset or
power cycle restarts the counter, so analysis should distinguish normal
progression (`102 -> 103`), a gap (`102 -> 104`), and a Pico restart
(`523 -> 1`). A restart must not be reported as hundreds of missing samples.
Reset classification is analysis work that has not yet been implemented.

## Current fields and limits

The processed columns and units are:

* `timestamp` — Mac local receive timestamp with timezone. It represents when
  the logger processed the Pico row, not the exact physical measurement time at
  the MPPT. It is neither an MPPT hardware timestamp nor a Pico acquisition
  timestamp.
* `sequence` — Pico publication sequence, monotonic within one firmware
  execution and restarted by a Pico reset or power cycle
* `battery_mv` — battery voltage in mV
* `panel_mv` — PV voltage in mV
* `battery_ma` — battery current in mA; positive means battery charging and
  negative means battery discharging
* `panel_w` — PV power in W

Analysis may convert mV to V and mA to A for display. The dataset does not
directly measure solar irradiance, ambient temperature, panel temperature, or
accurate battery state of charge. Analysis must not treat those quantities as
directly measured.

Differences between adjacent Mac receive timestamps can contain small timing
jitter from USB transport, macOS scheduling, Python execution, and serial
buffering. The timestamps remain useful for time-series analysis, but they must
not be treated as a precision hardware acquisition clock. The timing error has
not yet been measured.

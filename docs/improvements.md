# Solar Data Acquisition and Analysis Backlog

The acquisition foundation is now in place: VE.Direct reception, checksum
validation, atomic snapshots, parser-health diagnostics, sequenced USB CSV, and
basic logging and analysis infrastructure. Direct Pico-to-Mac logging proved
the initial workflow. The operational architecture now uses Raspberry Pi for
persistent storage and keeps the Mac optional during acquisition.

> The acquisition system should be developed in service of the analysis. New
> sensors, VE.Direct fields, and firmware features should be introduced when
> they answer a concrete engineering question or improve data reliability.

The project now has four deliberately separate layers:

* **Acquisition:** Pico, VE.Direct, checksum validation, atomic snapshots,
  sequence numbers, and USB transport.
* **Storage:** Raspberry Pi persistent raw serial logs and processed timestamped
  CSV, operating independently of the Mac.
* **Synchronization:** selected data copied over SSH from host `raspi` into a
  project-relative Mac `data/` working copy from `~/pico_cpp/data/` on
  Raspberry Pi.
* **Analysis:** Mac/Jupyter integrity checks, visualization, derived quantities,
  and engineering interpretation using local CSV files only.

```text
acquisition -> storage -> synchronization -> analysis
```

## 0. Current Design — Keep

* [x] Keep UART reception interrupt-driven.
* [x] Keep the UART ISR limited mainly to receiving bytes and filling the RX buffer.
* [x] Keep VE.Direct parsing outside the interrupt handler.
* [x] Keep the current VE.Direct UART configuration:

  * 19200 baud
  * 8 data bits
  * no parity
  * 1 stop bit
  * no flow control
* [x] Keep separate functions for display rendering, parsing, history handling, and UART reception.
* [x] Keep one history sample per validated VE.Direct block (~1 second).

---

## 1. VE.Direct Reliability

### 1.1 Implement real checksum validation

* [x] Calculate the VE.Direct checksum over the complete received block.
* [x] Accept a block only when the modulo-256 sum is valid.
* [x] Distinguish between:

  * received blocks
  * valid blocks
  * invalid/checksum-error blocks
* [x] Track validated blocks explicitly with `valid_blocks`.

**Status:** Completed reliability foundation

---

### 1.2 Detect UART RX buffer overflow

* [ ] Add an RX overflow counter.
* [ ] Increment it whenever the ring buffer is full and a byte must be discarded.
* [ ] Optionally display the overflow count on OLED 3.

**Priority:** Priority 6 — after the new host/sync workflow and before serious long-duration acquisition

---

### 1.3 Improve VE.Direct block handling

* [x] Build a temporary representation of the current VE.Direct block.
* [x] Store received fields in that temporary block.
* [x] Verify the checksum at the end of the block.
* [x] Publish the measurements only after the complete block is valid.
* [x] Avoid exposing partially updated measurements to the rest of the application.

**Status:** Completed reliability foundation

---

## 2. OLED Refresh / Main Loop Performance

### 2.1 Reduce OLED refresh frequency

* [ ] Stop redrawing all four OLEDs every `20 ms`.
* [ ] Define a sensible display refresh period.
* [ ] Consider refreshing displays only when new VE.Direct data is available.
* [ ] Keep UART processing responsive independently from OLED refresh timing.

**Priority:** Deferred unless display work compromises acquisition reliability

---

### 2.2 Separate application timing

* [ ] Avoid using one `sleep_ms()` as the timing mechanism for everything.
* [ ] Introduce independent timing for:

  * VE.Direct processing
  * OLED refresh
  * USB/CSV output
* [ ] Prefer timestamp-based scheduling over long blocking delays.

**Priority:** Deferred maintenance

---

## 3. Measurement Data Model

### 3.1 Group MPPT values

Current individual variables:

* `battery_mv`
* `panel_mv`
* `battery_ma`
* `panel_w`

Tasks:

* [x] Create a coherent parser measurement structure.
* [x] Store related parser values together by VE.Direct block.
* [x] Make it clear which parser values belong to the same validated block.
* [ ] Consider making one application snapshot the common data source for OLEDs and CSV output.

**Priority:** Deferred maintenance

---

### 3.2 Analysis-driven VE.Direct expansion

Currently parsed:

* `V`
* `VPV`
* `I`
* `PPV`
* `Checksum`

Add fields only when they answer an analysis question or improve reliability.

Time-varying measurements and state:

* [ ] `CS` — identify Off, Bulk, Absorption, Float, and Fault charger states.
  This is likely the first additional field worth adding.
* [ ] `MPPT` — determine whether the tracker is off, limited, or actively
  tracking maximum power.
* [ ] `ERR` — correlate measurement anomalies with charger faults.
* [ ] `H19`–`H23` — compare Victron yield history with independently calculated
  daily energy and peak-power statistics.
* [ ] `LOAD` / `IL` — analyze load-side behavior if it enters project scope.

Device metadata:

* [ ] `PID` / `FW` / `SER#` — identify the device and firmware. Prefer storing
  these once as metadata rather than repeating them at 1 Hz if the architecture
  permits.

**Priority:** Analysis-driven; `CS` is the leading candidate

---

## 4. Voltage History

### 4.1 Keep current history implementation initially

* [x] Current `memmove()` implementation is acceptable for 128 samples at ~1 sample/s.
* [x] No immediate optimization is required.

---

### 4.2 Convert history to a circular buffer

* [ ] Replace array shifting with a circular/ring history buffer.
* [ ] Track the newest sample index.
* [ ] Adapt graph drawing to chronological circular-buffer data.
* [ ] Preserve the current 128-sample display behavior.

**Priority:** Deferred maintenance

---

## 5. Code Organization

### 5.1 VE.Direct module extraction

Move VE.Direct responsibilities out of `main.c`.

Current parser files:

```text
src/
├── main.c
└── vedirect_parser.c

include/
└── vedirect_parser.h
```

Tasks:

* [ ] Move UART RX buffer handling if future maintenance justifies it.
* [ ] Move the UART interrupt handler if future maintenance justifies it.
* [x] Move the VE.Direct parser out of `main.c`.
* [x] Move checksum handling into the parser module.
* [x] Move VE.Direct measurement structures into the parser API.
* [x] Expose a small hardware-independent API to `main.c`.

**Priority:** Remaining UART extraction is deferred maintenance

---

### 5.2 Extract display logic

Potential future structure:

```text
src/
├── main.c
├── vedirect_parser.c
└── display.c

include/
├── vedirect_parser.h
└── display.h
```

Tasks:

* [ ] Move OLED graph functions.
* [ ] Move MPPT screen rendering.
* [ ] Move counter/debug screen rendering.
* [ ] Keep hardware-specific display switching outside the VE.Direct module.

**Priority:** Deferred maintenance

---

### 5.3 Simplify `main()`

Target responsibility of `main()`:

* [ ] initialize hardware
* [ ] initialize modules
* [ ] process incoming data
* [ ] schedule display updates
* [ ] schedule data transmission

Desired conceptual loop:

```text
initialize

while (true)
    process VE.Direct
    update application state
    refresh displays when required
    send logging data when required
```

**Priority:** Deferred maintenance

---

## 6. USB Logging, Raspberry Pi Storage, and Mac Synchronization

### 6.1 Send data from Pico over USB

* [x] Define the serial output format.
* [x] Select the initial VE.Direct fields to transmit.
* [x] Send only complete, validated measurement snapshots.
* [x] Add a monotonically increasing sequence number.
* [x] Compile and flash the data-logging firmware.
* [x] Detect the directly connected Pico as `/dev/cu.usbmodem*` on macOS.
* [x] Verify real USB serial rows with `cat /dev/cu.usbmodem101`.

**Status:** Implemented and verified on directly connected hardware

The direct Mac connection was the original validation topology. In the current
architecture, the Pico USB endpoint is Raspberry Pi.

---

### 6.2 Define CSV format

Pico output format:

```text
sequence,battery_mv,panel_mv,battery_ma,panel_w
```

Processed logger format:

```text
timestamp,sequence,battery_mv,panel_mv,battery_ma,panel_w
```

Tasks:

* [x] Decide the initial CSV columns.
* [x] Print one line per validated VE.Direct block.
* [x] Add a CSV header.
* [x] Ensure incomplete and checksum-invalid blocks cannot be emitted as valid
  Pico CSV rows; parser tests cover rejection behavior.

**Status:** Implemented and verified with a short real saved session

---

### 6.3 Historical direct-to-Mac logger validation

* [x] Identify the Pico USB serial device on macOS.
* [x] Verify raw serial output in Terminal using `cat` (`screen` did not work on
  this macOS setup).
* [x] Create a Mac-side logging method.
* [x] Implement raw-log and processed-CSV storage.
* [x] Add Mac-side ISO-8601 timestamps and sequence-gap detection.
* [x] Run the logger against the real flashed firmware.
* [x] Verify a real raw log and processed CSV.
* [ ] Verify long-duration logging after UART RX overflow detection is in place.

**Status:** Short-session persistence verified; long-duration verification remains

This completed work proved the logger and file formats. It is superseded as the
operational topology because logging must continue without the Mac.

---

### 6.4 Raspberry Pi persistent logger

* [x] Use the stable Pico device under `/dev/serial/by-id/` on Raspberry Pi.
* [x] Deploy the logger as system service `vedirect-logger.service`.
* [x] Document the persistent Raspberry Pi data root as `~/pico_cpp/data/`.
* [x] Define unattended logger startup and lifecycle management with systemd.
* [x] Verify active raw and processed storage on Raspberry Pi through real
  synchronization.
* [x] Verify Raspberry Pi acquisition continues while the Mac synchronizes.
* [x] Verify logging continues after its administering SSH session ends.
* [x] Verify clean stop/start and controlled restart with same-day append.
* [x] Rotate aligned raw and processed files at local date boundaries inside the
  continuously running Python logger.
* [x] Write one CSV header for a new/empty daily file and avoid duplicate
  headers after same-day restart.
* [x] Preserve historical timestamped session files without migration.
* [x] Verify daily naming and same-day restart behavior on Raspberry Pi.
* [x] Observe a real midnight file rollover on Raspberry Pi.
* [x] Enable boot startup through `multi-user.target`.
* [ ] Verify automatic recovery with a deliberate failure test.
* [ ] Verify boot startup through a real Raspberry Pi reboot.
* [ ] Verify logging continues with the Mac disconnected.

**Status:** systemd lifecycle operational and daily file rotation verified on
the real Raspberry Pi; fault injection, reboot, and full Mac-disconnection
verification remain pending

---

### 6.5 Raspberry Pi-to-Mac synchronization

* [x] Use project-relative `data/` as the synchronized Mac data root.
* [x] Implement a separate initial `rsync` pull using SSH host `raspi`.
* [x] Implement separate raw `.log` and processed `.csv` transfers.
* [x] Implement and remotely verify a non-modifying `--dry-run`.
* [x] Verify full and incremental transfers on the real system.
* [x] Implement `data/.last_sync` after successful real synchronization.
* [x] Verify freshness metadata during the first real transfer.
* [x] Verify that a later sync updates actively growing files.
* [x] Keep all SSH and transfer behavior outside Jupyter notebooks.

**Priority:** Immediately after Raspberry Pi logging is persistent

---

## 7. Diagnostic Improvements

OLED 3 now shows VE.Direct parser health:

* received blocks
* valid blocks
* checksum-invalid blocks
* incomplete blocks

Status and possible improvements:

* [x] Replace low-level IRQ and raw RX-byte diagnostics with parser-health counters.
* [x] Display checksum error count.
* [x] Display valid block count.
* [x] Display incomplete block count.
* [ ] Add RX overflow count.
* [ ] Decide which counters are useful for permanent diagnostics versus temporary debugging.

**Priority:** Parser-health display is maintenance; RX overflow is Priority 6

---

## 8. Numeric Formatting / Embedded Optimization

Current code uses floating-point formatting such as:

```text
battery_mv / 1000.0
%.2f
```

Tasks:

* [ ] Decide whether floating-point `snprintf()` is acceptable for this project.
* [ ] Optionally replace voltage/current formatting with integer arithmetic.
* [ ] Compare code size after the change.
* [ ] Do this only after the functional architecture is stable.

**Priority:** Deferred maintenance

---

# Current Dataset and Analytical Constraints

The processed dataset schema contains:

* `timestamp` — logger-host local receive timestamp with timezone; under the
  new architecture this is generated on Raspberry Pi when its logger processes
  the Pico row, not at the MPPT or Pico
* `sequence` — Pico publication sequence, monotonic only within one firmware
  execution and restarted by a Pico reset or power cycle
* `battery_mv` — battery voltage in mV
* `panel_mv` — PV voltage in mV
* `battery_ma` — battery current in mA; positive means charging and negative
  means discharging
* `panel_w` — PV power in W

Analysis may convert mV to V and mA to A for presentation. The stored values
retain their acquisition units.

Adjacent logger receive timestamps can contain unmeasured timing jitter from
USB transport, host scheduling, Python execution, and serial buffering. They
are useful for analysis but are not a precision hardware acquisition clock.
The existing first dataset is historical direct-to-Mac data and therefore has
Mac-generated receive timestamps.

Sequence analysis must distinguish normal progression (`102 -> 103`), a gap
(`102 -> 104`), and a Pico restart (`523 -> 1`). Restarts should eventually be
classified separately rather than counted as large numbers of missing samples.

The current dataset does **not** directly measure solar irradiance, ambient
temperature, panel temperature, or accurate battery state of charge. Future
analysis must not present those quantities as measured or infer them without an
explicit model and appropriate supporting data.

---

# Current Priority Order

The first direct-to-Mac dataset, integrity analysis, plots, Raspberry Pi
systemd service, and manual synchronization are complete. The next open
architecture-level validation is the full disconnect -> acquire -> reconnect
-> synchronize -> analyze workflow.

## Priority 1 — Confirm Raspberry Pi USB acquisition

* [x] Identify and use the stable Pico serial device on Raspberry Pi.
* [x] Verify the CSV header and rows on Raspberry Pi.
* [x] Confirm device access and permissions for user `fori`.

## Priority 2 — Deploy persistent Raspberry Pi logging

* [x] Deploy the existing logger as `vedirect-logger.service`.
* [x] Use `~/pico_cpp/data/` as the persistent Raspberry Pi data root.
* [x] Confirm active raw and processed output on the target host.
* [x] Define and verify systemd stop/start/restart lifecycle management.
* [x] Implement and verify same-day append to daily raw/processed files.
* [x] Enable systemd startup for normal boot.
* [ ] Verify enabled startup with a real reboot.
* [ ] Fault-inject an unexpected logger failure if additional recovery evidence
  is required.
* [x] Confirm acquisition continues while synchronization runs.
* [ ] Verify logging continues while the Mac is disconnected.

## Priority 3 — Define Mac synchronization destinations

* [x] Use project-relative `data/` as the local synchronized Mac data root.
* [x] Preserve the raw/processed distinction in both locations.
* [x] Define synchronization freshness as Mac completion time in
  `data/.last_sync`.

## Priority 4 — Implement separate synchronization

* [x] Implement a simple `rsync` pull using SSH host `raspi`.
* [x] Select `.log` and `.csv` source directories without hard-coded IP
  addresses.
* [x] Verify initial and incremental synchronization on the real system.
* [x] Keep synchronization outside the notebook.

## Priority 5 — Verify synchronized local analysis

* [ ] Point the notebook at the agreed local data location only if adaptation
  is required.
* [ ] Re-run integrity checks and plots against synchronized data.
* [ ] Test disconnect -> acquire -> reconnect -> synchronize -> analyze.

## Priority 6 — Add UART RX overflow detection

* [ ] Add an overflow counter where the UART ring buffer drops a byte.
* [ ] Expose the count in diagnostics where useful.
* [ ] Use overflow evidence when judging whether a dataset is trustworthy.

Dropped UART bytes can invalidate or lose VE.Direct blocks. Complete this
before serious multi-hour or daylight-cycle acquisition.

## Priority 7 — Longer datasets

* [ ] Capture several hours on Raspberry Pi without the Mac connected.
* [ ] Capture a complete daylight cycle.
* [ ] Later capture multi-day data.
* [ ] Keep raw logs for traceability and processed CSV for analysis.

## Priority 8 — Derived engineering quantities

* [ ] Integrate PV power over time to estimate generated energy.
* [ ] Calculate Wh per recording and later per hour/day.
* [ ] Find maximum PV power and its timestamp.
* [ ] Calculate average PV power during active production.
* [ ] Estimate charging and discharging durations.
* [ ] Report battery-voltage range.

## Priority 9 — Analysis-driven VE.Direct expansion

* [ ] Decide whether `CS` is required for the first charger-state analysis.
* [ ] Later evaluate `MPPT`, `ERR`, `H19`–`H23`, `LOAD`, and `IL` against
  concrete analysis questions.
* [ ] Treat `PID`, `FW`, and `SER#` as metadata rather than ordinary 1 Hz
  measurements where practical.

## Priority 10 — Remaining embedded maintenance

* [ ] Defer OLED refresh and scheduling optimization unless measurements show
  that display work harms acquisition.
* [ ] Defer remaining module extraction and `main()` simplification.
* [ ] Defer circular history buffers and numeric formatting optimization.

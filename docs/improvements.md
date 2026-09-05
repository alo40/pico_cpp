# Solar Data Acquisition and Analysis Backlog

The acquisition foundation is now in place: VE.Direct reception, checksum
validation, atomic snapshots, parser-health diagnostics, sequenced USB CSV, and
basic macOS logging infrastructure. Direct Pico USB output has been verified on
hardware. The next project goal is to verify persistence and begin analyzing
real solar-system behavior.

> The acquisition system should be developed in service of the analysis. New
> sensors, VE.Direct fields, and firmware features should be introduced when
> they answer a concrete engineering question or improve data reliability.

The project has three layers:

* **Acquisition:** Pico, VE.Direct, checksum validation, atomic snapshots,
  sequence numbers, and USB transport.
* **Storage:** raw serial logs for traceability and processed timestamped CSV
  for analysis.
* **Analysis:** Python integrity checks, visualization, derived quantities, and
  engineering interpretation.

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

**Priority:** Priority 5 — after first plots and before serious long-duration acquisition

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

## 6. USB / Mac Data Logging

### 6.1 Send data from Pico to Mac

* [x] Define the serial output format.
* [x] Select the initial VE.Direct fields to transmit.
* [x] Send only complete, validated measurement snapshots.
* [x] Add a monotonically increasing sequence number.
* [x] Compile and flash the data-logging firmware.
* [x] Detect the directly connected Pico as `/dev/cu.usbmodem*` on macOS.
* [x] Verify real USB serial rows with `cat /dev/cu.usbmodem101`.

**Status:** Implemented and verified on directly connected hardware

---

### 6.2 Define CSV format

Pico output format:

```text
sequence,battery_mv,panel_mv,battery_ma,panel_w
```

Processed Mac-side format:

```text
timestamp,sequence,battery_mv,panel_mv,battery_ma,panel_w
```

Tasks:

* [x] Decide the initial CSV columns.
* [x] Print one line per validated VE.Direct block.
* [x] Add a CSV header.
* [x] Ensure incomplete and checksum-invalid blocks cannot be emitted as valid
  Pico CSV rows; parser tests cover rejection behavior.

**Status:** Implemented; processed files still require real-session verification

---

### 6.3 Mac-side logger

* [x] Identify the Pico USB serial device on macOS.
* [x] Verify raw serial output in Terminal using `cat` (`screen` did not work on
  this macOS setup).
* [x] Create a Mac-side logging method.
* [x] Implement raw-log and processed-CSV storage.
* [x] Add Mac-side ISO-8601 timestamps and sequence-gap detection.
* [ ] Run the logger against the real flashed firmware.
* [ ] Verify a real raw log and processed CSV.
* [ ] Verify long-duration logging after UART RX overflow detection is in place.
* [ ] Later consider file rotation beyond one file pair per session.

**Priority:** Highest immediate priority: verify end-to-end persistence

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

**Priority:** Parser-health display is maintenance; RX overflow is Priority 5

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

The processed dataset currently contains:

* `timestamp` — Mac local receive timestamp with timezone; it records when the
  logger processed the Pico row, not the exact MPPT measurement time or a Pico
  acquisition time
* `sequence` — Pico publication sequence, monotonic only within one firmware
  execution and restarted by a Pico reset or power cycle
* `battery_mv` — battery voltage in mV
* `panel_mv` — PV voltage in mV
* `battery_ma` — battery current in mA; positive means charging and negative
  means discharging
* `panel_w` — PV power in W

Analysis may convert mV to V and mA to A for presentation. The stored values
retain their acquisition units.

Adjacent Mac receive timestamps can contain unmeasured timing jitter from USB
transport, macOS scheduling, Python execution, and serial buffering. They are
useful for analysis but are not a precision hardware acquisition clock.

Sequence analysis must distinguish normal progression (`102 -> 103`), a gap
(`102 -> 104`), and a Pico restart (`523 -> 1`). Restarts should eventually be
classified separately rather than counted as large numbers of missing samples.

The current dataset does **not** directly measure solar irradiance, ambient
temperature, panel temperature, or accurate battery state of charge. Future
analysis must not present those quantities as measured or infer them without an
explicit model and appropriate supporting data.

---

# New Priority Order

## Priority 1 — Verify real end-to-end logger persistence

* [ ] Run `scripts/log_vedirect.py` against the real flashed firmware.
* [ ] Confirm the raw log contains the real USB lines.
* [ ] Confirm the processed CSV contains valid timestamped rows.
* [ ] Confirm clean Ctrl+C shutdown and valid files afterward.

## Priority 2 — Verify the first short dataset

* [ ] Complete a short real recording.
* [ ] Inspect the first and last processed rows.
* [ ] Confirm column structure, timestamps, units, and plausible values.
* [ ] Check sequence progression.
* [ ] Confirm the saved CSV is usable as analysis input.

## Priority 3 — Establish data quality and integrity analysis

Create the first analysis layer for processed CSV before attempting physical
interpretation:

* [ ] Report sample count and recording duration.
* [ ] Report first and last timestamps.
* [ ] Calculate sampling-interval statistics.
* [ ] Detect missing and duplicate sequence numbers.
* [ ] Classify Pico resets separately from missing sequence numbers.
* [ ] Detect malformed or missing values.
* [ ] Report minimum, mean, and maximum for each measured quantity.

## Priority 4 — Basic time-series visualization

* [ ] Plot battery voltage against timestamp in V.
* [ ] Plot PV voltage against timestamp in V.
* [ ] Plot battery current against timestamp in A.
* [ ] Plot PV power against timestamp in W.

## Priority 5 — Add UART RX overflow detection

* [ ] Add an overflow counter where the UART ring buffer drops a byte.
* [ ] Expose the count in diagnostics where useful.
* [ ] Use overflow evidence when judging whether a dataset is trustworthy.

Dropped UART bytes can invalidate or lose VE.Direct blocks. Complete this
before serious multi-hour or daylight-cycle acquisition.

## Priority 6 — Longer datasets

* [ ] Capture several hours.
* [ ] Capture a complete daylight cycle.
* [ ] Later capture multi-day data.
* [ ] Keep raw logs for traceability and use processed CSV for analysis.

## Priority 7 — Derived engineering quantities

Plan, but do not yet implement:

* [ ] Integrate PV power over time to estimate generated energy.
* [ ] Calculate Wh per recording and later per hour/day.
* [ ] Find maximum PV power and its timestamp.
* [ ] Calculate average PV power during active production.
* [ ] Estimate charging and discharging durations.
* [ ] Report battery-voltage range.

## Priority 8 — Analysis-driven VE.Direct expansion

* [ ] Decide whether `CS` is required for the first charger-state analysis.
* [ ] Later evaluate `MPPT`, `ERR`, `H19`–`H23`, `LOAD`, and `IL` against
  concrete analysis questions.
* [ ] Treat `PID`, `FW`, and `SER#` as metadata rather than ordinary 1 Hz
  measurements where practical.

## Priority 9 — Remaining embedded maintenance

* [ ] Defer OLED refresh and scheduling optimization unless measurements show
  that display work harms acquisition.
* [ ] Defer remaining module extraction and `main()` simplification.
* [ ] Defer circular history buffers and numeric formatting optimization.

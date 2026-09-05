# `main.c` Improvement Checklist

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
* [x] Keep one history sample per completed VE.Direct block (~1 second).

---

## 1. VE.Direct Reliability

### 1.1 Implement real checksum validation

* [ ] Calculate the VE.Direct checksum over the complete received block.
* [ ] Accept a block only when the modulo-256 sum is valid.
* [ ] Distinguish between:

  * received blocks
  * valid blocks
  * invalid/checksum-error blocks
* [ ] Update `ve_block_count` semantics so it represents validated blocks, or rename it appropriately.

**Priority:** High

---

### 1.2 Detect UART RX buffer overflow

* [ ] Add an RX overflow counter.
* [ ] Increment it whenever the ring buffer is full and a byte must be discarded.
* [ ] Optionally display the overflow count on OLED 3.

**Priority:** High

---

### 1.3 Improve VE.Direct block handling

* [ ] Build a temporary representation of the current VE.Direct block.
* [ ] Store received fields in that temporary block.
* [ ] Verify the checksum at the end of the block.
* [ ] Publish the measurements only after the complete block is valid.
* [ ] Avoid exposing partially updated measurements to the rest of the application.

**Priority:** Medium–High

---

## 2. OLED Refresh / Main Loop Performance

### 2.1 Reduce OLED refresh frequency

* [ ] Stop redrawing all four OLEDs every `20 ms`.
* [ ] Define a sensible display refresh period.
* [ ] Consider refreshing displays only when new VE.Direct data is available.
* [ ] Keep UART processing responsive independently from OLED refresh timing.

**Priority:** High

---

### 2.2 Separate application timing

* [ ] Avoid using one `sleep_ms()` as the timing mechanism for everything.
* [ ] Introduce independent timing for:

  * VE.Direct processing
  * OLED refresh
  * future USB/CSV output
* [ ] Prefer timestamp-based scheduling over long blocking delays.

**Priority:** Medium

---

## 3. Measurement Data Model

### 3.1 Group MPPT values

Current individual variables:

* `battery_mv`
* `panel_mv`
* `battery_ma`
* `panel_w`

Tasks:

* [ ] Create a coherent MPPT measurement structure.
* [ ] Store related values together.
* [ ] Make it clear which values belong to the same VE.Direct block.
* [ ] Make this structure the common data source for OLEDs and future CSV output.

**Priority:** Medium–High

---

### 3.2 Expand VE.Direct fields

Currently parsed:

* `V`
* `VPV`
* `I`
* `PPV`
* `Checksum`

Possible future fields:

* [ ] `PID`
* [ ] `FW`
* [ ] `SER#`
* [ ] `CS`
* [ ] `MPPT`
* [ ] `ERR`
* [ ] `LOAD`
* [ ] `IL`
* [ ] `H19` — total yield
* [ ] `H20` — yield today
* [ ] `H21` — maximum power today
* [ ] `H22` — yield yesterday
* [ ] `H23` — maximum power yesterday
* [ ] Decide which additional fields are actually useful before implementing them.

**Priority:** Later

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

**Priority:** Low

---

## 5. Code Organization

### 5.1 Extract VE.Direct module

Move VE.Direct responsibilities out of `main.c`.

Potential files:

```text
src/
├── main.c
├── vedirect.c
└── vedirect.h
```

Tasks:

* [ ] Move UART RX buffer handling.
* [ ] Move UART interrupt handler.
* [ ] Move VE.Direct parser.
* [ ] Move checksum handling.
* [ ] Move VE.Direct measurement structures.
* [ ] Expose a small API to `main.c`.

**Priority:** High, preferably before adding major new features

---

### 5.2 Extract display logic

Potential future structure:

```text
src/
├── main.c
├── vedirect.c
├── vedirect.h
├── display.c
└── display.h
```

Tasks:

* [ ] Move OLED graph functions.
* [ ] Move MPPT screen rendering.
* [ ] Move counter/debug screen rendering.
* [ ] Keep hardware-specific display switching outside the VE.Direct module.

**Priority:** Medium

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

**Priority:** Medium

---

## 6. USB / Mac Data Logging

### 6.1 Send data from Pico to Mac

* [ ] Define the serial output format.
* [ ] Decide which VE.Direct fields to transmit.
* [ ] Send only complete/validated measurement snapshots.
* [ ] Add timestamp or sequence information if useful.
* [ ] Test USB serial output on macOS.

**Priority:** Next major feature after parser reliability

---

### 6.2 Define CSV format

Possible starting format:

```text
battery_mv,panel_mv,battery_ma,panel_w
```

Later:

```text
timestamp,battery_mv,panel_mv,battery_ma,panel_w,...
```

Tasks:

* [ ] Decide CSV columns.
* [ ] Print one line per validated VE.Direct block.
* [ ] Add a CSV header.
* [ ] Ensure incomplete blocks are never logged.

**Priority:** Medium–High

---

### 6.3 Mac-side logger

* [ ] Identify the Pico USB serial device on macOS.
* [ ] Verify raw serial output in Terminal.
* [ ] Create a Mac-side logging method.
* [ ] Redirect/store received data into a `.csv` file.
* [ ] Verify long-duration logging.
* [ ] Later consider automatic timestamps and file rotation.

**Priority:** After Pico-side output works

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

**Priority:** Medium

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

**Priority:** Low

---

# Recommended Implementation Order

* [ ] **1 — Reduce unnecessary OLED refreshes**
* [ ] **2 — Add UART RX overflow detection**
* [ ] **3 — Implement VE.Direct checksum validation**
* [ ] **4 — Make measurements atomic per validated VE.Direct block**
* [ ] **5 — Extract `vedirect.c` / `vedirect.h`**
* [ ] **6 — Add Pico → Mac serial data output**
* [ ] **7 — Define and output CSV records**
* [ ] **8 — Implement Mac-side CSV logging**
* [ ] **9 — Extract display functionality if useful**
* [ ] **10 — Convert history arrays to circular buffers**
* [ ] **11 — Optimize numeric formatting if necessary**
* [ ] **12 — Expand support for additional VE.Direct fields**

---

# Current Next-Task Candidates

* [ ] **A. OLED refresh optimization**
* [ ] **B. UART buffer overflow detection**
* [ ] **C. VE.Direct checksum validation**
* [ ] **D. VE.Direct data structure / atomic snapshot**
* [ ] **E. Split VE.Direct code into its own module**
* [ ] **F. Start USB serial output toward the Mac**

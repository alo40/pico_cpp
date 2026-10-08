# VE.Direct Solar Monitoring Project

This is the canonical technical project document. It consolidates system architecture, component responsibilities, implemented capabilities, verification status, known limitations, and active work.

This is a test input.

## 1. Purpose

The project acquires VE.Direct data from a Victron SmartSolar MPPT, validates and publishes measurements through a Raspberry Pi Pico, stores them continuously on a Raspberry Pi, exposes a local browser dashboard, synchronizes retained data to a Mac, and analyzes synchronized data in Jupyter.

Development should remain driven by analysis and data reliability. New sensors, VE.Direct fields, firmware features, or infrastructure should be introduced when they answer a concrete engineering question or improve the trustworthiness of the acquired dataset.

## 2. System architecture

The operational architecture is:

```text
Physical solar system
  -> Victron MPPT
  -> Pico acquisition and validation
  -> USB serial
  -> Raspberry Pi persistent logging and storage
       -> live/historical browser dashboard
       -> synchronization over the network
  -> Mac local data copy
  -> Jupyter analysis and visualization
  -> engineering conclusions
```

The long-term module boundary is:

```text
acquisition -> storage -> synchronization -> analysis
```

The Mac is not required for continuous acquisition.

## 3. Component responsibilities

### 3.1 Raspberry Pi Pico — acquisition

The Pico:

- receives VE.Direct serial data,
- validates complete VE.Direct blocks,
- publishes only valid complete measurement snapshots,
- assigns publication sequence numbers,
- emits validated rows over USB serial,
- renders local OLED information.

It does not own persistent file storage or engineering analysis.

Key firmware modules:

- `src/main.c`: active Pico application.
- `src/vedirect_uart.c`: UART transport and parser polling.
- `src/vedirect_parser.c`: hardware-independent parser.
- `src/dashboard.c`: Pico/OLED dashboard state and rendering.
- `src/ssd1306.c`: SSD1306 display driver/support.

The parser must remain independent of Pico SDK headers and hardware-specific code.

### 3.2 Raspberry Pi — logging and storage

The Raspberry Pi is the always-on edge/data-logging host.

It:

- receives Pico USB serial output,
- runs `scripts/log_vedirect.py`,
- stores raw serial input,
- stores validated processed CSV rows,
- rotates files on the Raspberry Pi local calendar-day boundary,
- runs independently of the Mac.

Persistent data root:

```text
~/pico_cpp/data/
├── raw/
└── processed/
```

The logger uses the stable Pico path under `/dev/serial/by-id/` rather than `/dev/ttyACM0`.

### 3.3 Browser dashboard — read-only visualization

`scripts/dashboard.py` reads processed CSV data after the logger writes it.

It:

- shows current battery voltage, panel voltage, battery current, and panel power,
- shows current-day graphs,
- marks old live data as stale,
- handles the no-data state,
- supports retained daily CSV history,
- shows a selected historical day as a static view,
- returns cleanly to `Today (live)`.

It does not:

- open the Pico serial device,
- modify raw or processed measurement files,
- own the acquisition lifecycle.

The dashboard and logger run as separate systemd services.

### 3.4 Mac — synchronization and analysis host

The Mac:

- is not required for continuous acquisition,
- administers the Raspberry Pi through SSH,
- pulls data from Raspberry Pi to the local repository,
- runs Jupyter analysis against synchronized local files.

Use the SSH configuration host `raspi`; scripts and documentation must not depend on a hard-coded current IP address.

### 3.5 Jupyter — offline/local analysis

Jupyter notebooks:

- read synchronized local CSV files only,
- do not perform SSH/SCP/SFTP/rsync operations,
- explicitly select the intended input dataset,
- preserve stored timezone offsets by default,
- perform integrity checks, statistics, visualization, derived quantities, and engineering interpretation.

Synchronization and analysis remain separate operations.

## 4. Data flow

```text
VE.Direct Text-mode frame
        |
        v
Pico UART transport
        |
        v
hardware-independent parser
        |
        v
validated measurement snapshot
        |
        +-> OLED display
        |
        v
USB CSV row
        |
        v
Raspberry Pi logger
        |
        +-> data/raw/*.log
        |
        +-> data/processed/*.csv
                |
                +-> browser dashboard
                |
                +-> rsync -> Mac -> Jupyter
```

Raw files preserve received Pico text for traceability and debugging. Processed CSV files are the analysis input.

## 5. Data model

Processed schema:

```csv
timestamp,sequence,battery_mv,panel_mv,battery_ma,panel_w
```

Field meaning:

- `timestamp`: Raspberry Pi logger local receive/processing timestamp with timezone.
- `sequence`: Pico publication sequence number.
- `battery_mv`: battery voltage in mV.
- `panel_mv`: panel voltage in mV.
- `battery_ma`: battery current in mA.
- `panel_w`: panel power in W.

Positive battery current means charging; negative current means discharging.

The timestamp is not an MPPT hardware timestamp or a Pico acquisition timestamp. It may contain jitter from USB transport, Raspberry Pi scheduling, Python execution, and serial buffering.

The Pico sequence is monotonic only during one firmware execution. Analysis must distinguish:

```text
102 -> 103   normal progression
102 -> 104   gap
102 -> 102   duplicate
523 -> 1     firmware restart/reset
```

Daily files:

```text
data/raw/vedirect_YYYY-MM-DD.log
data/processed/vedirect_YYYY-MM-DD.csv
```

The logger rotates files at the Raspberry Pi local date boundary without restarting the serial connection or Python process. Same-day service restart appends to the existing daily files without duplicating the CSV header.

## 6. Runtime and deployment

### 6.1 Logger service

Version-controlled unit:

```text
systemd/vedirect-logger.service
```

Installed unit:

```text
/etc/systemd/system/vedirect-logger.service
```

The service uses controlled SIGINT shutdown and is configured to restart after unexpected failure.

A second manual logger must not be run while systemd owns the Pico serial device.

### 6.2 Dashboard service

Version-controlled unit:

```text
systemd/vedirect-dashboard.service
```

The dashboard is intended for trusted local-network use.

It is currently unauthenticated and unencrypted. Port 8000 must not be exposed to the public internet without appropriate firewalling and, if remote exposure is intended, authentication and TLS.

### 6.3 Synchronization

Synchronization is a non-destructive Raspberry Pi-to-Mac pull:

```text
raspi:~/pico_cpp/data/raw/       -> <project-root>/data/raw/
raspi:~/pico_cpp/data/processed/ -> <project-root>/data/processed/
```

`scripts/sync_data.sh` does not use `--delete` and does not push Mac files to the Raspberry Pi.

A successful real synchronization updates:

```text
data/.last_sync
```

A dry run or failed synchronization does not update that file.

## 7. Implemented capabilities

### Pico acquisition

- [x] VE.Direct UART at 19200 baud, 8N1, no flow control.
- [x] Interrupt-driven UART RX ring buffer.
- [x] Parsing outside the interrupt handler.
- [x] Hardware-independent VE.Direct parser.
- [x] Battery voltage, panel voltage, battery current, and panel power parsing.
- [x] Modulo-256 VE.Direct checksum validation.
- [x] Publication only after a complete valid block.
- [x] Rejection/recovery for invalid, incomplete, malformed, and overlong blocks.
- [x] Parser-health counters.
- [x] Publication sequence number.

### Display and USB output

- [x] OLED current measurements and voltage history.
- [x] Parser-health diagnostics on display.
- [x] One USB CSV row per validated measurement snapshot.
- [x] CSV header and sequence number.
- [x] Invalid/incomplete frames prevented from reaching CSV output.

### Automated validation

- [x] Pico firmware builds with CMake and Pico SDK.
- [x] Production parser tested independently of Pico hardware.
- [x] Parser valid/corrupt/truncated/malformed/recovery coverage.
- [x] Parser counter and binary-checksum handling tests.
- [x] Logger parsing, file handling, rotation, and restart tests.
- [x] Dashboard automated tests are present.
- [x] Build/test and optional-flashing scripts are present.

### Persistent logging

- [x] Original direct Pico-to-Mac logging workflow validated.
- [x] Raw serial input preserved.
- [x] Validated processed CSV written.
- [x] Timezone-aware logger-host timestamp.
- [x] Sequence discontinuity reporting.
- [x] Aligned daily raw and processed files.
- [x] Safe same-day append after logger restart.
- [x] No duplicate CSV headers.
- [x] Raspberry Pi local-day file rotation.
- [x] Daily rotation verified through automated tests and real hardware operation.

### Raspberry Pi service

- [x] Stable `/dev/serial/by-id/` Pico path.
- [x] Logger systemd service.
- [x] Persistent data under Raspberry Pi project directory.
- [x] Controlled SIGINT shutdown and journald diagnostics.
- [x] Automatic restart configured after unexpected failure.
- [x] Service enabled for normal multi-user boot.
- [x] Controlled start/stop/restart behavior verified.
- [x] Logging continues after SSH administration session ends.
- [x] Logging continues while synchronization runs.

### Data synchronization

- [x] Separate raw and processed rsync pulls.
- [x] SSH host `raspi` used without hard-coded IP.
- [x] Mac destinations resolved relative to repository.
- [x] Non-modifying dry run.
- [x] Non-destructive synchronization.
- [x] Successful sync timestamp stored in `data/.last_sync`.
- [x] Full, incremental, and actively growing file transfers verified.
- [x] Synchronization kept outside notebooks.

### Local analysis

- [x] Synchronized local CSV analysis without network access.
- [x] Explicit single-file dataset selection.
- [x] Unsafe/invalid dataset paths rejected.
- [x] Processed CSV schema/value validation.
- [x] Timezone-aware logger timestamp preserved.
- [x] Dataset identity, duration, and timing integrity reported.
- [x] Sequence gaps, duplicates, and restarts classified separately.
- [x] Stored units converted for presentation.
- [x] Descriptive statistics and four time-series plots.
- [x] Clean top-to-bottom notebook execution saved.

### Browser dashboard

- [x] Read-only live browser dashboard implemented.
- [x] Current values displayed from processed daily CSV.
- [x] Full-day graphs implemented.
- [x] Incremental live updates implemented.
- [x] Stale-data indication implemented.
- [x] No-data behavior implemented.
- [x] Historical retained daily CSV browsing implemented.
- [x] Static historical graph/final-sample status implemented.
- [x] Clean return from historical view to `Today (live)`.
- [x] Separate dashboard systemd unit present.

## 8. Verification status

Verified on the real Mac/Raspberry Pi system:

- logger runs independently of the SSH administration session,
- controlled service stop/start/restart,
- daily naming and same-day append behavior,
- real midnight rollover without restarting the logger process,
- full and incremental synchronization,
- synchronization while files are actively growing,
- notebook explicit local dataset workflow.

Still open:

- real reboot verification of automatic logger startup,
- deliberate unexpected logger failure/recovery test,
- complete disconnect -> acquire -> reconnect -> synchronize -> analyze workflow against a selected Raspberry Pi daylight daily file,
- explicit multi-hour acquisition assessment using RX-overflow evidence,
- target installation and end-to-end real-device verification of `vedirect-dashboard.service`,
- confirmation that dashboard port 8000 is unreachable outside the trusted local network.

## 9. Active work

### Priority 1 — validate unattended operation

- [ ] Reboot the Raspberry Pi and verify `vedirect-logger.service` starts.
- [ ] Force an unexpected logger failure and verify automatic recovery.
- [ ] Disconnect the Mac, acquire daylight data, reconnect, synchronize, and analyze the explicitly selected Raspberry Pi daily CSV.

### Priority 2 — detect acquisition loss

- [ ] Count bytes discarded when the Pico UART RX ring buffer is full.
- [ ] Expose the overflow count through diagnostics.
- [ ] Use overflow evidence when assessing dataset reliability.
- [ ] Build firmware, run unit tests, and verify multi-hour logging.

### Priority 3 — analyze the data

- [ ] Estimate generated energy in Wh.
- [ ] Report peak power, its timestamp, and average active-production power.
- [ ] Analyze charging/discharging periods and battery-voltage range.
- [ ] Compare results across longer recordings.

### Priority 4 — validate live dashboard

- [ ] Install and start `vedirect-dashboard.service` on Raspberry Pi.
- [ ] Open `http://raspi:8000` from a trusted-LAN device and verify current values, full-day graphs, incremental updates, stale indication, and the no-data state.
- [ ] Select a retained daily CSV, verify its static graph and final-sample status, then return to `Today (live)`.
- [ ] Confirm port 8000 is not reachable outside the trusted local network.

## 10. Known limitations

The current processed dataset does not directly measure:

- solar irradiance,
- ambient temperature,
- panel temperature,
- accurate battery state of charge.

These quantities must not be presented as directly measured or silently inferred.

The Raspberry Pi logger timestamp is useful for analysis but is not a precision acquisition clock.

The dashboard is intended only for a trusted LAN in its current unauthenticated/unencrypted form.

## 11. Historical architecture note

The first working workflow was:

```text
Victron MPPT -> Pico -> USB serial -> Mac logging and analysis
```

That workflow validated the initial Pico CSV format, logger, and notebook, but acquisition depended on the Mac remaining connected and awake.

It was superseded by the current Raspberry Pi persistent-logging architecture so that acquisition continues when the Mac is off, asleep, or disconnected.

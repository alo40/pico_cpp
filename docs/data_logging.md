# Raspberry Pi VE.Direct Logging and Mac Analysis

The acquisition system should be developed in service of the analysis. New
sensors, VE.Direct fields, and firmware features should be introduced when they
answer a concrete engineering question or improve data reliability.

## Architecture change

The first working workflow connected the Pico directly to the Mac. The Mac ran
the serial logger, stored raw and processed files, and ran the analysis:

```text
Victron MPPT -> Pico -> USB serial -> Mac logging and analysis
```

That workflow proved the Pico CSV format, logger, and first analysis notebook,
but made acquisition depend on the Mac remaining connected and awake. It is now
superseded as the operational architecture.

The Raspberry Pi now remains with the solar system and becomes the persistent
logging and storage host:

```text
Physical solar system
  -> Victron MPPT
  -> Pico acquisition and validation
  -> USB serial
  -> Raspberry Pi persistent logging and storage
  -> synchronization over the network
  -> Mac local data copy
  -> Jupyter analysis and visualization
  -> engineering conclusions
```

The goal is continuous logging even when the Mac is switched off, asleep, or
disconnected. The long-term module boundary is:

```text
acquisition -> storage -> synchronization -> analysis
```

## Responsibilities

### Raspberry Pi Pico — acquisition

* Receives and parses VE.Direct frames.
* Validates complete snapshots and assigns publication sequence numbers.
* Sends the acquired data over USB serial.
* Remains an embedded acquisition device; it does not own file storage or
  analysis.

### Raspberry Pi — persistent storage

* Receives the Pico USB serial output.
* Runs the persistent logging workflow independently of the Mac.
* Stores raw serial logs and processed CSV data.
* Acts as the always-on edge/data-logging host.

The Pico serial device on Raspberry Pi still needs confirmation. The persistent
Raspberry Pi data root is:

```text
~/pico_cpp/data/
├── raw/
└── processed/
```

### Mac — synchronization and analysis host

* Is not required for continuous acquisition.
* Administers the Raspberry Pi remotely through SSH.
* Synchronizes selected logs from Raspberry Pi into a local working copy.
* Runs Jupyter notebooks and visualization against that local copy.

The synchronized Mac data root is the project-relative `data/` directory, with
the same `raw/` and `processed/` separation. The sync script resolves this from
its own location rather than the caller's working directory.

### Jupyter — local analysis only

* Reads local CSV files only.
* Does not initiate SSH, SCP, SFTP, or other Raspberry Pi connections.
* Does not depend on Raspberry Pi or network availability while running.
* Remains focused on integrity checks, statistics, visualization, derived
  quantities, and engineering interpretation.

Separating synchronization from notebooks preserves offline analysis,
reproducibility, simple tests, and clear ownership. The tradeoff is that the
analyst must synchronize data before analyzing it.

## SSH access from the Mac

Use the SSH configuration host name `raspi` as the stable logical endpoint.
Automation must not depend on the interactive shell alias and should not
hard-code the current IP address.

```sh
ssh raspi
./scripts/sync_data.sh
./scripts/sync_data.sh --dry-run
```

The shell alias `raspi='ssh raspi'` is convenient for interactive use, but
scripts and documented transfer commands use the SSH host directly.

## Logging and synchronization workflow

1. The Pico emits validated rows over USB serial.
2. The Raspberry Pi logger preserves raw input and writes timestamped processed
   CSV records.
3. Logging continues without the Mac.
4. When the Mac is available, run `./scripts/sync_data.sh` to pull `.log` and
   `.csv` files with `rsync`.
5. The notebook loads the synchronized local CSV; it never fetches the file.

The exact transfers are:

```text
raspi:~/pico_cpp/data/raw/       -> <project-root>/data/raw/
raspi:~/pico_cpp/data/processed/ -> <project-root>/data/processed/
```

The operation is a non-destructive Raspberry Pi-to-Mac pull. It does not use
`--delete`, remove remote files, or send Mac files to Raspberry Pi. Run
`./scripts/sync_data.sh --dry-run` to preview transfers without changing local
dataset files.

After both real transfers succeed, the script writes `data/.last_sync`. Its
value is the Mac's local completion time in `YYYY-MM-DDTHH:MM:SS±HHMM` form.
The file is not updated by a dry-run or failed synchronization and is not sent
to Raspberry Pi.

Files being actively appended on Raspberry Pi may be copied. Such a local file
represents the remote file at synchronization time; a later run updates it.
This is acceptable for the initial manual workflow. Immutable snapshots or
completed-session handling can be considered later if analysis requires them.

### Verification status

The manual workflow has been verified on the real Mac/Raspberry Pi system:

* `--dry-run` connected through `raspi` and previewed both source directories.
* The first real run copied raw and processed sessions into the project without
  removing older local datasets and created `data/.last_sync`.
* A second run transferred only the active raw and processed files that had
  grown; unchanged sessions were skipped.
* The Raspberry Pi logger continued appending acquisition data while the Mac
  synchronized it.

This verifies full and incremental manual synchronization, freshness metadata,
and the documented active-file behavior. It does not yet verify unattended
logger startup/recovery, long-duration acquisition, or the complete
disconnect -> acquire -> reconnect -> synchronize -> analyze workflow.

The target processed schema remains:

```csv
timestamp,sequence,battery_mv,panel_mv,battery_ma,panel_w
```

Under the new architecture, `timestamp` is intended to be the Raspberry Pi
logger's local receive/processing timestamp with timezone. It is not an MPPT
hardware timestamp or a Pico acquisition timestamp. Adjacent values may contain
unmeasured jitter from USB transport, Raspberry Pi scheduling, Python
execution, and serial buffering, so they are not a precision hardware clock.

The Pico sequence remains monotonic only within one firmware execution. A reset
or power cycle restarts it, so analysis must distinguish normal progression
(`102 -> 103`), a gap (`102 -> 104`), a duplicate (`102 -> 102`), and a restart
(`523 -> 1`).

Raw logs preserve received Pico text for traceability, debugging, and recovery.
Processed CSV contains validated structured records and is the analysis input.

## Historical direct-to-Mac verification

The former direct workflow was successfully tested using a macOS device such
as `/dev/cu.usbmodem101`, `cat`, and `scripts/log_vedirect.py`. It produced the
first real processed dataset and validated the initial notebook. This remains
useful development evidence, but it is no longer the intended unattended
logging topology.

## Current fields and limits

The processed data contains `timestamp`, `sequence`, `battery_mv`, `panel_mv`,
`battery_ma`, and `panel_w`. Voltage is stored in mV, current in mA, and power in
W. Positive battery current means charging; negative current means discharging.

It does not directly measure solar irradiance, ambient temperature, panel
temperature, or accurate battery state of charge. Analysis must not present
those quantities as directly measured.

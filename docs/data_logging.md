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

Raspberry Pi uses the confirmed stable Pico device under `/dev/serial/by-id/`.
The persistent Raspberry Pi data root is:

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

## Unattended Raspberry Pi logging with systemd

Raspberry Pi runs the logger as the system service
`vedirect-logger.service`. This removes the active SSH-terminal dependency;
SSH is now administration-only.

The version-controlled unit is `systemd/vedirect-logger.service` and is
installed as `/etc/systemd/system/vedirect-logger.service`. It runs as the
normal user `fori` with:

```text
WorkingDirectory=/home/fori/pico_cpp
ExecStart=/usr/bin/python3 /home/fori/pico_cpp/scripts/log_vedirect.py --port /dev/serial/by-id/usb-Raspberry_Pi_Pico_E6609103C37E4023-if00
```

The stable `/dev/serial/by-id/` path is used instead of `/dev/ttyACM0`.
`KillSignal=SIGINT` lets the logger follow its existing Ctrl+C shutdown path,
flush and close its files, and report its final sample count. Unexpected
failures are configured for retry with `Restart=on-failure` and
`RestartSec=5`; fault recovery has not been deliberately injected.

Common administration commands are:

```sh
sudo systemctl status vedirect-logger.service --no-pager
sudo systemctl start vedirect-logger.service
sudo systemctl stop vedirect-logger.service
sudo systemctl restart vedirect-logger.service
journalctl -u vedirect-logger.service --no-pager
```

The service is enabled for `multi-user.target`, but startup after a real reboot
has not yet been tested. A manual stop remains stopped. Do not run a second
manual logger while systemd owns the Pico serial device.

### Daily measurement files

The systemd service lifetime and measurement-file lifetime are independent.
The Python process may run continuously for days or weeks while the logger
rotates both output files at each Raspberry Pi local calendar-day boundary:

```text
data/raw/vedirect_YYYY-MM-DD.log
data/processed/vedirect_YYYY-MM-DD.csv
```

Rotation occurs when the next serial line is received after the local date
changes. The serial port and Python process remain open; only the previous
raw/processed files are closed and the new date's pair is opened. One host-local
time value determines both the file date and a valid row's timestamp.

A same-day service restart reopens the same daily files in append mode. Existing
content is preserved, and the processed CSV header is written only when the
file is new or empty. A restart on another date naturally selects that date's
files. Historical timestamped session files such as
`vedirect_2026-09-05_230319.csv` remain untouched and are not migrated.

Service lifecycle and diagnostic messages go to the systemd journal.
Measurement data remains under:

```text
/home/fori/pico_cpp/data/raw/
/home/fori/pico_cpp/data/processed/
```

On the real target, the manual logger was stopped cleanly with SIGINT before
the service started. New files were observed growing, logging continued after
the SSH verification session ended, and controlled service stop/start behavior
was verified.

Daily naming and same-day append behavior were also verified on Raspberry Pi.
A controlled same-day restart reused `vedirect_2026-09-05.log` and
`vedirect_2026-09-05.csv`, preserved existing rows, appended new measurements,
and retained exactly one CSV header. Real midnight rollover was subsequently
observed from 2026-09-05 to 2026-09-06: `vedirect-logger.service` remained
continuously active with the same Python PID (4829), and the first received
serial line after midnight opened:

```text
/home/fori/pico_cpp/data/raw/vedirect_2026-09-06.log
/home/fori/pico_cpp/data/processed/vedirect_2026-09-06.csv
```

The Python process and serial connection were not restarted, acquisition
continued, and the first processed timestamp was
`2026-09-06T00:00:00.892+02:00`. This confirms that the filename date and CSV
timestamp use the same Raspberry Pi local calendar date. Daily rollover is now
verified both by automated tests and on the real target.

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
During the day this is normally the current daily file; after midnight, rsync
will update the new day's active file while the prior daily file normally stops
changing. This is acceptable for the initial manual workflow.

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
and the documented active-file behavior. It does not yet verify fault-injected
automatic recovery, boot startup after a real reboot, long-duration
acquisition, or the complete disconnect -> acquire -> reconnect -> synchronize
-> analyze workflow.

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

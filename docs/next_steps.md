# Current Workflow: Raspberry Pi Persistent Acquisition

> The acquisition system should be developed in service of the analysis. New
> sensors, VE.Direct fields, and firmware features should be introduced when
> they answer a concrete engineering question or improve data reliability.

The direct Pico-to-Mac workflow proved USB CSV logging and the first analysis
notebook. It is now superseded operationally by this modular path:

```text
acquisition -> storage -> synchronization -> analysis
Pico           Raspberry Pi  network/SSH      Mac/Jupyter
```

The Mac must not be required for continuous acquisition, and notebooks must
analyze synchronized local files rather than access Raspberry Pi directly.

## Completed foundation

- [x] Validate complete VE.Direct snapshots and reject invalid blocks.
- [x] Assign one publication sequence number per valid snapshot.
- [x] Emit Pico USB CSV.
- [x] Verify direct USB output and a short saved dataset on Mac.
- [x] Implement raw/processed logging behavior.
- [x] Create and execute the first local data-inspection notebook.
- [x] Produce integrity statistics and four basic time-series plots.

These results remain valid. Raspberry Pi logging is now managed by systemd and
synchronization is operating. Fault-injected recovery and boot-after-reboot
verification remain pending.

## Phase 3: Raspberry Pi logging host

### 3.1 Confirm the Pico connection

- [x] Connect the Pico to Raspberry Pi over USB.
- [x] Use stable serial device
  `/dev/serial/by-id/usb-Raspberry_Pi_Pico_E6609103C37E4023-if00`.
- [x] Confirm the Pico CSV header and rows can be read on Raspberry Pi.
- [x] Confirm user `fori` has serial access through `plugdev`.

### 3.2 Deploy persistent logging

- [x] Deploy `vedirect-logger.service` as a system service on Raspberry Pi.
- [x] Document the persistent Raspberry Pi data root as `~/pico_cpp/data/`.
- [x] Confirm active raw `.log` and processed `.csv` files are stored there.
- [x] Confirm Raspberry Pi acquisition continues while the Mac synchronizes.
- [x] Define unattended startup and lifecycle management with systemd.
- [x] Enable the service for `multi-user.target` boot startup.
- [x] Verify logging survives termination of the administering SSH session.
- [x] Verify clean manual stop/start and controlled service restart behavior.
- [x] Implement one aligned raw/processed file pair per local calendar day.
- [x] Verify a same-day service restart appends to the same daily files.
- [x] Verify existing content is preserved and the CSV header is not duplicated.
- [x] Unit-test file-only midnight rollover without restarting Python or the
  serial connection.
- [x] Observe a real midnight rollover on Raspberry Pi.
- [ ] Verify automatic recovery through a deliberate unexpected-failure test.
- [ ] Verify service startup after a real Raspberry Pi reboot.
- [ ] Verify logging continues while the Mac is disconnected.

Automatic recovery is configured with `Restart=on-failure` and `RestartSec=5`,
but has not been fault-injected. Boot startup is configured but has not been
verified by rebooting the Raspberry Pi.

## Phase 4: Separate synchronization

- [x] Use project-relative `data/` as the synchronized Mac data root.
- [x] Implement an initial `rsync`-based pull using SSH host `raspi`.
- [x] Do not hard-code the Raspberry Pi IP or depend on the shell alias.
- [x] Keep the implementation independent of the IP, user, key path, shell
  alias, and caller's working directory.
- [x] Implement separate raw `.log` and processed `.csv` synchronization.
- [x] Implement `--dry-run` and verify it against `raspi`.
- [x] Verify the first full transfer while preserving older local datasets.
- [x] Verify a later incremental transfer updates growing files and skips
  unchanged sessions.
- [x] Preserve raw logs for traceability and processed CSV for analysis.
- [x] Implement `data/.last_sync` after both real transfers succeed.
- [x] Verify `data/.last_sync` during the first real transfer.
- [x] Verify synchronization of an actively growing session.

Synchronization is a distinct operation. It must not be embedded in the
Jupyter notebook.

The initial sync may copy the current daily file while it is still being
appended. That local copy represents the source at synchronization time and
will be updated by a later sync.

## Phase 5: Analyze synchronized local data

- [ ] Confirm the notebook can select the agreed local synchronized directory.
- [ ] Adapt the notebook only if the final local directory requires it.
- [ ] Keep the notebook free of SSH, SCP, SFTP, and Raspberry Pi dependencies.
- [ ] Re-run integrity checks and plots against a synchronized dataset.
- [ ] Test the complete disconnect -> acquire -> reconnect -> synchronize ->
  analyze workflow.

## Acquisition reliability before long datasets

- [ ] Add UART RX overflow detection.
- [ ] Use the overflow count when assessing dataset reliability.
- [ ] Verify long-duration Raspberry Pi logging after overflow detection is in
  place.
- [ ] Record several hours, then a complete daylight cycle, then multiple days.

Dropped UART bytes can invalidate or lose complete VE.Direct blocks, so
overflow detection remains important before serious long-duration acquisition.

## Later analysis work

- [ ] Integrate PV power over time to estimate generated energy.
- [ ] Calculate total Wh, peak power and its timestamp, and charging/discharging
  periods.
- [ ] Compare results across longer sessions.
- [ ] Add `CS`, then consider `MPPT`, `ERR`, `H19`–`H23`, `LOAD`, and `IL` only
  when a concrete analysis question requires them.

## Additional robustness validation — non-blocking

- [ ] If needed, perform an end-to-end malformed/checksum-invalid rejection
  experiment.

Deliberate corrupted-block testing belongs primarily in parser/unit tests and
does not block Raspberry Pi deployment or synchronization work.

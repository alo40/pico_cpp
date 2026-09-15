# Next Steps

Operational details and completed verification are documented in
[`data_logging.md`](data_logging.md). Implemented capabilities are summarized
in [`roadmap.md`](roadmap.md).

## 1. Validate unattended operation

- [ ] Reboot the Raspberry Pi and verify `vedirect-logger.service` starts.
- [ ] Force an unexpected logger failure and verify automatic recovery.
- [ ] Disconnect the Mac, acquire daylight data, reconnect, synchronize, and
  analyze the explicitly selected Raspberry Pi daily CSV.

## 2. Detect acquisition loss

- [ ] Count bytes discarded when the Pico UART RX ring buffer is full.
- [ ] Expose the overflow count through diagnostics.
- [ ] Use overflow evidence when assessing dataset reliability.
- [ ] Build firmware, run unit tests, and verify multi-hour logging.

## 3. Capture useful datasets

- [x] Capture a complete daylight cycle.
- [x] Capture multiple days.
- [x] Preserve raw logs and processed CSV for every recording.

## 4. Analyze the data

- [ ] Estimate generated energy in Wh.
- [ ] Report peak power, its timestamp, and average active-production power.
- [ ] Analyze charging/discharging periods and battery-voltage range.
- [ ] Compare results across longer recordings.

## 5. Validate live dashboard

- [ ] Install and start `vedirect-dashboard.service` on Raspberry Pi.
- [ ] Open `http://raspi:8000` from a trusted-LAN device and verify current
  values, full-day graphs, incremental updates, stale indication, and the
  no-data state.
- [ ] Confirm port 8000 is not reachable outside the trusted local network.

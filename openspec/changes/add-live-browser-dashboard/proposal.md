## Why

The OLEDs provide local live readings and Jupyter provides offline analysis,
but neither offers a convenient live view of the solar system from another
device on the trusted local network. A browser dashboard should expose current
measurements and the full current-day trend without changing acquisition.

## What Changes

- Add a read-only browser dashboard hosted by the Raspberry Pi on the trusted
  LAN.
- Display current battery voltage, panel voltage, battery current, and panel
  power.
- Graph the full current local calendar day, from midnight through the latest
  recorded sample.
- Refresh the dashboard as the existing daily processed CSV grows and reset for
  the new daily CSV at local midnight.
- Indicate the latest sample time and when incoming data is stale or missing.
- Add a systemd service so the dashboard starts independently of the logger.

## Capabilities

### New Capabilities
- `live-browser-dashboard`: Read-only LAN dashboard for current VE.Direct data
  and today-so-far measurement graphs.

### Modified Capabilities
- None.

## Impact

- Adds a small Python dashboard server and a systemd unit.
- Reads existing processed CSV files under `data/processed/` without modifying
  the logger, Pico firmware, or CSV schema.
- Exposes an unauthenticated HTTP service only for the trusted local network.
- Requires no new runtime dependency, database, charting library, or frontend
  build tooling.

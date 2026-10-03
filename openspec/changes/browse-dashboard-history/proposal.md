## Why

The browser dashboard only exposes the current day's CSV, despite the Raspberry
Pi retaining daily processed files for later inspection. Selecting a recorded
day in the same dashboard makes recent solar behavior accessible without
Jupyter, a database, or a separate viewer.

## What Changes

- Add a date selector containing the daily processed CSV files available on the
  Raspberry Pi.
- Keep `Today (live)` as the default selection, with its existing incremental
  updates and local-midnight rollover behavior.
- Render selected historical days as static full-day views, showing the selected
  date and final sample time instead of a live freshness warning.
- Reset the browser dataset when changing days so sequence numbers from one CSV
  cannot suppress samples from another.

## Capabilities

### New Capabilities
- `dashboard-history`: Select and inspect retained daily processed CSV files in
  the browser dashboard.

### Modified Capabilities
- None.

## Impact

- Extends `scripts/dashboard.py` with read-only day listing and selected-day
  data responses, plus the dashboard selector and historical presentation.
- Extends `unit test/test_dashboard.py` to cover day discovery and snapshot
  behavior for historical files.
- Updates dashboard documentation; no logger, Pico firmware, CSV schema,
  runtime dependency, or Raspberry Pi deployment change is required.

## 1. Dashboard Data Service

- [x] 1.1 Add a standard-library Python dashboard server that selects, validates,
  and read-only serves the current local day's processed CSV.
- [x] 1.2 Add snapshot and incremental-update responses that return only valid
  rows, ignore incomplete appends, and replace the client dataset after daily
  rollover.
- [x] 1.3 Add unit tests for current-day selection, missing or empty data,
  malformed final rows, incremental rows, and rollover responses.

## 2. Browser Dashboard

- [x] 2.1 Serve a responsive page with current measurement cards, latest-sample
  age, and clear unavailable or stale states.
- [x] 2.2 Render four native-canvas today-so-far graphs that retain the full
  midnight-to-latest-sample span while reducing data to display-width buckets.
- [x] 2.3 Load a full-day snapshot on page open, poll for incremental updates,
  and replace browser data when the server reports a new day.

## 3. Deployment And Verification

- [x] 3.1 Add a systemd service for the dashboard that starts independently of
  the logger and listens on trusted-LAN port 8000.
- [x] 3.2 Document dashboard installation, trusted-LAN access, service control,
  and the no-internet-exposure constraint.
- [x] 3.3 Run the logger tests and dashboard tests, then manually verify the
  local dashboard's full-day graph, stale state, and no-data state.
- [ ] 3.4 Install and validate the dashboard on Raspberry Pi, including live
  CSV append updates and trusted-LAN access.

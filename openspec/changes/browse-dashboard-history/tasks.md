## 1. Historical Data Service

- [x] 1.1 List valid dated processed CSV files in newest-first order for the
  dashboard date selector.
- [x] 1.2 Serve a full read-only snapshot for a selected historical day while
  preserving incremental responses only for the current local day.

## 2. Dashboard Day Selection

- [x] 2.1 Add a native date selector with `Today (live)` as the default and
  available historical days as options.
- [x] 2.2 Replace the browser dataset on selection, keep historical views static,
  and restore safe live polling and rollover when Today is selected.
- [x] 2.3 Present historical status and graph end labels from the selected day
  without stale-data warnings.

## 3. Verification And Documentation

- [x] 3.1 Add unit tests for date discovery, historical snapshots, and
  current-day incremental behavior.
- [x] 3.2 Run dashboard tests and manually verify a historical selection followed
  by a return to Today using local processed CSV files.
- [x] 3.3 Update dashboard architecture and next-step documentation after tests
  pass, keeping Raspberry Pi installation validation in the original change.

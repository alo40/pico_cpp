## Context

See proposal.md for motivation and `specs/dashboard-history/spec.md` for the
behavior contract. The dashboard already reads one current-day processed CSV and
uses a full snapshot followed by sequence-based incremental updates. Daily files
are named `vedirect_YYYY-MM-DD.csv` in `data/processed/`; prior files are
immutable after logger rollover.

## Goals / Non-Goals

**Goals:**
- Reuse retained processed CSVs as the sole historical source.
- Keep `Today (live)` efficient while making historical selections deterministic.
- Prevent a sequence number from one day affecting another day's display.

**Non-Goals:**
- Arbitrary date entry, cross-day aggregation, database indexing, CSV download,
  or changes to the logger and systemd deployment.
- Refreshing a selected historical view after it is loaded.

## Decisions

### List only daily processed-file dates

The server will expose the dates from filenames matching the existing daily CSV
format, sorted newest first. The browser uses that list to populate one native
select control, always including `Today (live)`.

Dates alone are sufficient for selection and avoid parsing every retained file
to compute optional metadata such as row counts or energy totals. A filesystem
directory listing is rejected because it could expose non-daily or unrelated
files.

### Serve any selected date as a full snapshot

The existing data response will accept a requested date. A date other than the
server's local current day always returns a complete snapshot from that file;
incremental `after` handling remains limited to the current day.

This matches immutable historical files and preserves the existing small live
updates. A separate history endpoint is rejected because the response schema is
already suitable for both cases.

### Keep live and historical browser modes explicit

`Today (live)` is the only mode that polls for sample rows and follows midnight
rollover. Selecting another date clears the buffered data and sequence cursor,
loads one snapshot, and stops live refresh. The historical status names the
selected date and final sample time; it never evaluates staleness. Graph labels
use the selected file's final sample time rather than claiming "now".

Clearing the cursor on selection is required because Pico sequence values are
not comparable between CSVs. Continuing to poll history was rejected because
daily files are expected to be complete after rollover and it adds requests with
no user-visible benefit.

## Risks / Trade-offs

- [Many retained daily files create a long selector] -> List dates only; add
  pagination or retention controls only if the list becomes unusable.
- [A selected file is absent, unreadable, or empty] -> Return the established
  empty-data response and let the browser show unavailable data.
- [The current day rolls over while history is selected] -> Preserve the explicit
  historical selection until the user selects `Today (live)`.

## Migration Plan

1. Deploy the updated dashboard script through the existing service workflow.
2. Open the dashboard on the trusted LAN and select a retained date, then return
   to `Today (live)` while the logger continues appending.
3. Roll back by restoring the previous dashboard script and restarting only
   `vedirect-dashboard.service`; the logger and all CSV files remain untouched.

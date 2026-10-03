## Context

See proposal.md for motivation and `specs/live-browser-dashboard/spec.md` for
the behavior contract. The logger already writes and flushes a dated processed
CSV after every valid sample, normally about once per second. A complete daily
file reaches about 86,400 rows and 4.4 MB. The Raspberry Pi runs the logger as
a systemd service and the processed CSV is the established source for local
analysis.

## Goals / Non-Goals

**Goals:**
- Serve a responsive live dashboard from the Raspberry Pi with no added runtime
  package.
- Represent the full current local day while keeping network updates small.
- Run independently from, and never block or alter, the logger.

**Non-Goals:**
- Long-term storage, historical browsing, remote access, authentication, TLS,
  dashboard configuration, or logger control.
- Replacing the existing OLED display or Jupyter notebook.

## Decisions

### Use one standard-library Python HTTP server

Add one dashboard script that serves the page and read-only JSON endpoints,
using Python's standard library. Add a dedicated systemd unit to launch it
under the same project working directory and user as the logger.

This is smaller than adding Flask, a JavaScript build, Grafana, or a database;
the existing daily CSV is adequate for the required one-day view. The server
will listen on port 8000 on the trusted LAN. Internet exposure is explicitly
out of scope because unauthenticated HTTP is appropriate only on that network.

### Treat the current daily processed CSV as the source of truth

The server selects `data/processed/vedirect_YYYY-MM-DD.csv` using Raspberry Pi
local time, validates its established header and numeric fields, and ignores a
partial or malformed final row while the logger is writing it. It does not
write, lock, rename, or change the logger's rotation behavior.

This reuses the deployed data contract and avoids a duplicate real-time data
pipeline. Reading the Pico serial device directly was rejected because it would
compete with the logger for the device and undermine persistent logging.

### Bootstrap the full day, then request only new rows

On page load, the browser requests all valid rows from the current daily CSV,
which establishes the required midnight-to-now history. Later requests include
the most recently received sequence and date; the server returns only rows
newer than that sequence for the same day. A date mismatch returns a new-day
snapshot so the browser replaces its buffer at rollover.

This prevents repeated multi-megabyte transfers while preserving a complete
day after refresh. A server-side database or streaming connection was rejected:
the approximately one-row-per-second source rate does not justify either.

### Draw graphs in the browser with native canvas

The served page contains its own HTML, CSS, and JavaScript. It renders four
responsive canvas graphs and current-value cards. For each graph width, it
groups samples into horizontal buckets and retains the bucket's extrema so short
power or current events are not hidden. Axis labels identify the midnight-to-now
time range; the underlying browser buffer continues to contain all received
valid samples.

Native canvas avoids a chart dependency and bundle. SVG and a third-party chart
library were rejected because this fixed four-series dashboard needs only lines
and labels.

### Determine freshness in the browser

The browser computes sample age from the latest timestamp and uses a fixed
two-minute stale threshold. The threshold is intentionally not configurable;
the logger normally writes every second and configuration would add unsupported
surface area. The dashboard refresh loop periodically recalculates freshness,
even when no rows arrive.

## Risks / Trade-offs

- [Unauthenticated HTTP is reachable on the LAN] -> Bind only for trusted-LAN
  use; add authentication and TLS before any internet exposure.
- [A CSV append can be read mid-write] -> Ignore incomplete and invalid rows;
  accept them on the next poll once complete.
- [A full day has more points than display pixels] -> Keep all rows in memory
  and draw per-pixel extrema without shortening the time interval.
- [The dashboard starts before the logger creates today's file] -> Serve an
  explicit no-data state and retry on the next poll.
- [A process restart loses its in-memory cache] -> Rebuild from the current CSV
  on the next client snapshot request.

## Migration Plan

1. Install the dashboard script and systemd unit on the Raspberry Pi.
2. Enable and start the dashboard service after confirming the logger remains
   active and today's processed CSV is readable.
3. Verify the dashboard from a trusted-LAN device at port 8000, including live
   updates and the no-data state.
4. Roll back by stopping and disabling only the dashboard service; existing
   acquisition, CSV files, and logger service are unaffected.

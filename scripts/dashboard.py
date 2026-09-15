#!/usr/bin/env python3
"""Serve a read-only live view of the current VE.Direct daily CSV."""

import argparse
import csv
from datetime import datetime
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
from urllib.parse import parse_qs, urlparse


PROJECT_ROOT = Path(__file__).resolve().parent.parent
PROCESSED_HEADER = (
    "timestamp",
    "sequence",
    "battery_mv",
    "panel_mv",
    "battery_ma",
    "panel_w",
)


def local_now():
    return datetime.now().astimezone()


def current_csv_path(project_root, day):
    return project_root / "data" / "processed" / f"vedirect_{day.isoformat()}.csv"


def read_samples(path):
    """Return only complete, schema-valid measurement rows from *path*."""
    if not path.is_file():
        return []

    try:
        with path.open(encoding="utf-8", newline="") as source:
            reader = csv.DictReader(source)
            if tuple(reader.fieldnames or ()) != PROCESSED_HEADER:
                return []

            samples = []
            for row in reader:
                try:
                    timestamp = datetime.fromisoformat(row["timestamp"])
                    samples.append(
                        {
                            "timestamp": timestamp.isoformat(),
                            "sequence": int(row["sequence"]),
                            "battery_mv": int(row["battery_mv"]),
                            "panel_mv": int(row["panel_mv"]),
                            "battery_ma": int(row["battery_ma"]),
                            "panel_w": int(row["panel_w"]),
                        }
                    )
                except (KeyError, TypeError, ValueError):
                    continue
            return samples
    except OSError:
        return []


def dashboard_payload(project_root=PROJECT_ROOT, now_provider=local_now,
                      client_day=None, after_sequence=None):
    """Return a full snapshot or rows appended after a client's last sequence."""
    day = now_provider().date()
    samples = read_samples(current_csv_path(project_root, day))
    snapshot = client_day != day.isoformat()

    if not snapshot and after_sequence is not None:
        try:
            after_sequence = int(after_sequence)
        except ValueError:
            snapshot = True
        else:
            # A Pico restart lowers the sequence; replace instead of losing rows.
            if samples and samples[-1]["sequence"] < after_sequence:
                snapshot = True
            else:
                samples = [sample for sample in samples
                           if sample["sequence"] > after_sequence]
    else:
        snapshot = True

    return {
        "day": day.isoformat(),
        "snapshot": snapshot,
        "samples": samples,
    }


PAGE = r"""<!doctype html>
<html lang="en">
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Solar Monitor</title>
<style>
:root { color-scheme: dark; font-family: system-ui, sans-serif; background: #101818; color: #e6f2ec; }
body { max-width: 1100px; margin: auto; padding: 1rem; }
header, .card { background: #182421; border: 1px solid #2d443c; border-radius: .6rem; }
header { display: flex; justify-content: space-between; align-items: baseline; padding: .75rem 1rem; }
h1 { font-size: 1.25rem; margin: 0; } #status { color: #9eb7aa; font-size: .9rem; }
.cards { display: grid; grid-template-columns: repeat(4, 1fr); gap: .75rem; margin: .75rem 0; }
.card { padding: .75rem; } .label { color: #9eb7aa; font-size: .8rem; } .value { font-size: 1.5rem; margin-top: .25rem; }
.charts { display: grid; grid-template-columns: repeat(2, 1fr); gap: .75rem; }
figure { margin: 0; background: #182421; border: 1px solid #2d443c; border-radius: .6rem; padding: .75rem; }
figcaption { font-size: .9rem; margin-bottom: .4rem; } canvas { display: block; width: 100%; height: 180px; }
.stale { color: #ffc66d !important; } .error { color: #ff9a9a !important; }
@media (max-width: 640px) { .cards, .charts { grid-template-columns: repeat(2, 1fr); } canvas { height: 150px; } }
</style>
<header><h1>Solar Monitor</h1><span id="status">Loading...</span></header>
<main>
<section class="cards">
  <div class="card"><div class="label">Battery</div><div class="value" id="battery">--</div></div>
  <div class="card"><div class="label">Panel</div><div class="value" id="panel">--</div></div>
  <div class="card"><div class="label">Battery current</div><div class="value" id="current">--</div></div>
  <div class="card"><div class="label">Panel power</div><div class="value" id="power">--</div></div>
</section>
<section class="charts">
  <figure><figcaption>Battery voltage, today</figcaption><canvas id="battery_mv"></canvas></figure>
  <figure><figcaption>Panel voltage, today</figcaption><canvas id="panel_mv"></canvas></figure>
  <figure><figcaption>Battery current, today</figcaption><canvas id="battery_ma"></canvas></figure>
  <figure><figcaption>Panel power, today</figcaption><canvas id="panel_w"></canvas></figure>
</section>
</main>
<script>
const fields = [
  ['battery_mv', 'Battery voltage', 'V', 1000],
  ['panel_mv', 'Panel voltage', 'V', 1000],
  ['battery_ma', 'Battery current', 'A', 1000],
  ['panel_w', 'Panel power', 'W', 1],
];
let samples = [], day = null;

function ageText(sample) {
  const seconds = Math.max(0, Math.round((Date.now() - Date.parse(sample.timestamp)) / 1000));
  return seconds < 60 ? `${seconds}s ago` : `${Math.floor(seconds / 60)}m ago`;
}
function updateSummary() {
  const status = document.querySelector('#status');
  if (!samples.length) {
    status.textContent = 'No live measurement available'; status.className = 'error';
    for (const id of ['battery', 'panel', 'current', 'power']) document.querySelector('#' + id).textContent = '--';
    return;
  }
  const latest = samples.at(-1), age = Date.now() - Date.parse(latest.timestamp), stale = age >= 120000;
  status.textContent = `Latest sample ${ageText(latest)}${stale ? ' (stale)' : ''}`;
  status.className = stale ? 'stale' : '';
  document.querySelector('#battery').textContent = (latest.battery_mv / 1000).toFixed(2) + ' V';
  document.querySelector('#panel').textContent = (latest.panel_mv / 1000).toFixed(2) + ' V';
  document.querySelector('#current').textContent = (latest.battery_ma / 1000).toFixed(2) + ' A';
  document.querySelector('#power').textContent = latest.panel_w + ' W';
}
function draw(field, label, unit, divisor) {
  const canvas = document.querySelector('#' + field), width = canvas.clientWidth, height = canvas.clientHeight;
  const ratio = devicePixelRatio || 1; canvas.width = width * ratio; canvas.height = height * ratio;
  const context = canvas.getContext('2d'); context.scale(ratio, ratio); context.clearRect(0, 0, width, height);
  if (!samples.length) return;
  let min = Infinity, max = -Infinity;
  for (const sample of samples) { min = Math.min(min, sample[field]); max = Math.max(max, sample[field]); }
  if (min === max) { min -= 1; max += 1; }
  const padding = 22, range = max - min, buckets = Array.from({length: Math.max(1, Math.floor(width - padding))}, () => []);
  const first = samples[0].timestamp, start = Date.parse(first.slice(0, 10) + 'T00:00:00' + first.slice(-6));
  const end = Date.parse(samples.at(-1).timestamp), duration = Math.max(1, end - start);
  samples.forEach(sample => buckets[Math.min(buckets.length - 1, Math.floor((Date.parse(sample.timestamp) - start) * buckets.length / duration))].push(sample[field]));
  context.strokeStyle = '#77c9a5'; context.lineWidth = 1; context.beginPath();
  buckets.forEach((bucket, x) => {
    if (!bucket.length) return;
    const low = Math.min(...bucket), high = Math.max(...bucket);
    const y = value => height - padding - ((value - min) / range) * (height - padding * 2);
    context.moveTo(padding + x, y(low)); context.lineTo(padding + x, y(high));
  }); context.stroke();
  context.fillStyle = '#9eb7aa'; context.font = '11px system-ui';
  context.fillText(`${label}: ${(max / divisor).toFixed(2)} ${unit}`, 2, 11);
  context.fillText(`${(min / divisor).toFixed(2)} ${unit}`, 2, height - 3);
  context.fillText('00:00', padding, height - 3); context.fillText('now', width - 25, height - 3);
}
function render() { updateSummary(); fields.forEach(field => draw(...field)); }
async function refresh() {
  const latest = samples.at(-1), query = new URLSearchParams();
  if (day) query.set('day', day); if (latest) query.set('after', latest.sequence);
  try {
    const response = await fetch('/api/data?' + query), data = await response.json();
    if (data.snapshot) samples = data.samples; else samples.push(...data.samples);
    day = data.day; render();
  } catch (_) { document.querySelector('#status').textContent = 'Live data unavailable'; document.querySelector('#status').className = 'error'; }
}
addEventListener('resize', render); refresh(); setInterval(refresh, 5000); setInterval(updateSummary, 1000);
</script>
"""


def make_handler(project_root=PROJECT_ROOT, now_provider=local_now):
    class DashboardHandler(BaseHTTPRequestHandler):
        def send_json(self, payload):
            body = json.dumps(payload).encode("utf-8")
            self.send_response(HTTPStatus.OK)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

        def do_GET(self):
            request = urlparse(self.path)
            if request.path == "/":
                body = PAGE.encode("utf-8")
                self.send_response(HTTPStatus.OK)
                self.send_header("Content-Type", "text/html; charset=utf-8")
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)
                return
            if request.path == "/api/data":
                query = parse_qs(request.query)
                self.send_json(dashboard_payload(
                    project_root, now_provider, query.get("day", [None])[0],
                    query.get("after", [None])[0],
                ))
                return
            self.send_error(HTTPStatus.NOT_FOUND)

        def log_message(self, _format, *_args):
            return

    return DashboardHandler


def parse_arguments():
    parser = argparse.ArgumentParser(description="Serve the live VE.Direct dashboard.")
    parser.add_argument("--port", type=int, default=8000)
    return parser.parse_args()


def main():
    args = parse_arguments()
    server = ThreadingHTTPServer(("0.0.0.0", args.port), make_handler())
    print(f"Dashboard: http://0.0.0.0:{args.port}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()

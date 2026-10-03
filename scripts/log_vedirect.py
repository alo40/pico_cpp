#!/usr/bin/env python3
"""Log validated VE.Direct CSV rows from a Pico USB serial device."""

import argparse
import csv
from datetime import datetime
from pathlib import Path
import glob
import sys


PICO_HEADER = "sequence,battery_mv,panel_mv,battery_ma,panel_w"
PROCESSED_HEADER = (
    "timestamp",
    "sequence",
    "battery_mv",
    "panel_mv",
    "battery_ma",
    "panel_w",
)
PROJECT_ROOT = Path(__file__).resolve().parent.parent


def parse_data_line(line):
    """Return five integer values, or None for a header/malformed line."""
    if line == PICO_HEADER:
        return None

    fields = line.split(",")
    if len(fields) != 5:
        return None

    try:
        return tuple(int(field) for field in fields)
    except ValueError:
        return None


def expected_sequence(previous_sequence, current_sequence):
    """Return the expected value when a sequence gap exists, otherwise None."""
    if previous_sequence is None or current_sequence == previous_sequence + 1:
        return None
    return previous_sequence + 1


def write_raw_line(raw_file, raw_line):
    """Preserve one received serial line exactly as bytes."""
    raw_file.write(raw_line)
    raw_file.flush()


def select_port(explicit_port=None):
    if explicit_port:
        return explicit_port

    devices = sorted(glob.glob("/dev/cu.usbmodem*"))

    if not devices:
        raise RuntimeError(
            "No Pico USB serial device found matching /dev/cu.usbmodem*. "
            "Connect the Pico or specify --port."
        )

    if len(devices) > 1:
        device_list = "\n".join(f"  {device}" for device in devices)
        raise RuntimeError(
            "Multiple Pico USB serial devices found:\n"
            f"{device_list}\n"
            "Specify the device with --port."
        )

    return devices[0]


def daily_log_paths(log_date, project_root=PROJECT_ROOT):
    """Return the aligned raw and processed paths for one local calendar day."""
    date_text = log_date.isoformat()
    raw_directory = project_root / "data" / "raw"
    processed_directory = project_root / "data" / "processed"

    raw_directory.mkdir(parents=True, exist_ok=True)
    processed_directory.mkdir(parents=True, exist_ok=True)

    return (
        raw_directory / f"vedirect_{date_text}.log",
        processed_directory / f"vedirect_{date_text}.csv",
    )


class DailyLogFiles:
    """Own and rotate the raw/processed file pair for the current local date."""

    def __init__(self, project_root=PROJECT_ROOT):
        self.project_root = project_root
        self.log_date = None
        self.raw_path = None
        self.processed_path = None
        self.raw_file = None
        self.processed_file = None
        self.writer = None

    def ensure_open(self, log_date):
        if self.log_date == log_date:
            return

        self.close()
        self.raw_path, self.processed_path = daily_log_paths(
            log_date,
            self.project_root,
        )
        csv_needs_header = (
            not self.processed_path.exists()
            or self.processed_path.stat().st_size == 0
        )

        self.raw_file = self.raw_path.open("ab")
        self.processed_file = self.processed_path.open(
            "a",
            encoding="utf-8",
            newline="",
        )
        self.writer = csv.writer(self.processed_file, lineterminator="\n")
        self.log_date = log_date

        if csv_needs_header:
            self.writer.writerow(PROCESSED_HEADER)
            self.processed_file.flush()

        print(f"Raw log: {self.raw_path}")
        print(f"Processed CSV: {self.processed_path}")

    def close(self):
        if self.raw_file is not None:
            self.raw_file.close()
        if self.processed_file is not None:
            self.processed_file.close()

        self.raw_file = None
        self.processed_file = None
        self.writer = None
        self.log_date = None

    def __enter__(self):
        return self

    def __exit__(self, _exception_type, _exception, _traceback):
        self.close()


class LoggerStats:
    def __init__(self):
        self.valid_samples = 0


def local_now():
    return datetime.now().astimezone()


def log_stream(serial_port, log_files, now_provider=local_now, stats=None):
    if stats is None:
        stats = LoggerStats()
    previous_sequence = None

    try:
        while True:
            raw_line = serial_port.readline()
            if not raw_line:
                continue

            now = now_provider()
            log_files.ensure_open(now.date())
            write_raw_line(log_files.raw_file, raw_line)
            line = raw_line.decode("utf-8", errors="replace").rstrip("\r\n")

            if line == PICO_HEADER:
                continue

            values = parse_data_line(line)
            if values is None:
                print(f"WARNING: malformed line ignored: {line!r}", file=sys.stderr)
                continue

            sequence = values[0]
            expected = expected_sequence(previous_sequence, sequence)
            if expected is not None:
                print(
                    f"WARNING: sequence gap: expected {expected}, "
                    f"received {sequence}",
                    file=sys.stderr,
                )

            timestamp = now.isoformat(timespec="milliseconds")
            log_files.writer.writerow((timestamp,) + values)
            log_files.processed_file.flush()

            previous_sequence = sequence
            stats.valid_samples += 1

            if stats.valid_samples % 60 == 0:
                print(f"Received {stats.valid_samples} valid samples")
    except KeyboardInterrupt:
        return stats.valid_samples


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="Save Pico VE.Direct USB CSV data to raw and processed logs."
    )
    parser.add_argument(
        "--port",
        help="Pico serial device, for example /dev/cu.usbmodemXXXX",
    )
    return parser.parse_args()


def main():
    args = parse_arguments()

    try:
        port = select_port(args.port)
    except RuntimeError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 2

    try:
        import serial
    except ModuleNotFoundError:
        print(
            "ERROR: pyserial is required. Install it with: "
            "python3 -m pip install pyserial",
            file=sys.stderr,
        )
        return 2

    stats = LoggerStats()

    print(f"Serial device: {port}")

    log_files = DailyLogFiles()
    try:
        with serial.Serial(port, baudrate=115200, timeout=1) as serial_port:
            with log_files:
                log_stream(serial_port, log_files, stats=stats)
                print("\nStopping logger...")
    except serial.SerialException as error:
        print(f"ERROR: serial communication failed: {error}", file=sys.stderr)
        return_code = 1
    else:
        return_code = 0

    print(f"Valid samples written: {stats.valid_samples}")
    if log_files.raw_path is not None:
        print(f"Raw log: {log_files.raw_path}")
        print(f"Processed CSV: {log_files.processed_path}")
    return return_code


if __name__ == "__main__":
    raise SystemExit(main())

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


def create_log_paths():
    session = datetime.now().astimezone().strftime("%Y-%m-%d_%H%M%S")
    raw_directory = PROJECT_ROOT / "data" / "raw"
    processed_directory = PROJECT_ROOT / "data" / "processed"

    raw_directory.mkdir(parents=True, exist_ok=True)
    processed_directory.mkdir(parents=True, exist_ok=True)

    return (
        raw_directory / f"vedirect_{session}.log",
        processed_directory / f"vedirect_{session}.csv",
    )


def log_stream(serial_port, raw_file, processed_file):
    writer = csv.writer(processed_file, lineterminator="\n")
    writer.writerow(PROCESSED_HEADER)
    processed_file.flush()

    valid_samples = 0
    previous_sequence = None

    try:
        while True:
            raw_line = serial_port.readline()
            if not raw_line:
                continue

            write_raw_line(raw_file, raw_line)
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

            timestamp = datetime.now().astimezone().isoformat(timespec="milliseconds")
            writer.writerow((timestamp,) + values)
            processed_file.flush()

            previous_sequence = sequence
            valid_samples += 1

            if valid_samples % 60 == 0:
                print(f"Received {valid_samples} valid samples")
    except KeyboardInterrupt:
        return valid_samples


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

    raw_path, processed_path = create_log_paths()
    valid_samples = 0

    print(f"Serial device: {port}")
    print(f"Raw log: {raw_path}")
    print(f"Processed CSV: {processed_path}")

    try:
        with serial.Serial(port, baudrate=115200, timeout=1) as serial_port:
            with raw_path.open("wb") as raw_file:
                with processed_path.open("w", encoding="utf-8", newline="") as processed_file:
                    valid_samples = log_stream(
                        serial_port,
                        raw_file,
                        processed_file,
                    )
                    print("\nStopping logger...")
    except serial.SerialException as error:
        print(f"ERROR: serial communication failed: {error}", file=sys.stderr)
        return_code = 1
    else:
        return_code = 0

    print(f"Valid samples written: {valid_samples}")
    print(f"Raw log: {raw_path}")
    print(f"Processed CSV: {processed_path}")
    return return_code


if __name__ == "__main__":
    raise SystemExit(main())

import csv
from contextlib import redirect_stderr, redirect_stdout
from datetime import datetime, timedelta, timezone
import importlib.util
import io
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch


SCRIPT_PATH = Path(__file__).resolve().parents[1] / "scripts" / "log_vedirect.py"
SPEC = importlib.util.spec_from_file_location("log_vedirect", SCRIPT_PATH)
LOGGER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(LOGGER)


class LoggerTests(unittest.TestCase):
    def test_daily_paths_use_date_only_and_stay_aligned(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            raw_path, processed_path = LOGGER.daily_log_paths(
                datetime(2026, 9, 6).date(),
                root,
            )

            self.assertEqual(raw_path, root / "data/raw/vedirect_2026-09-06.log")
            self.assertEqual(
                processed_path,
                root / "data/processed/vedirect_2026-09-06.csv",
            )

    def test_new_csv_receives_one_header(self):
        with tempfile.TemporaryDirectory() as directory:
            log_files = LOGGER.DailyLogFiles(Path(directory))
            log_date = datetime(2026, 9, 6).date()

            log_files.ensure_open(log_date)
            log_files.close()

            rows = list(
                csv.reader(io.StringIO(log_files.processed_path.read_text()))
            )
            self.assertEqual(rows, [list(LOGGER.PROCESSED_HEADER)])

    def test_same_day_restart_appends_without_duplicate_header(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            log_date = datetime(2026, 9, 6).date()
            first = LOGGER.DailyLogFiles(root)
            first.ensure_open(log_date)
            first.writer.writerow(("first", 1, 2, 3, 4, 5))
            first.close()

            second = LOGGER.DailyLogFiles(root)
            second.ensure_open(log_date)
            second.writer.writerow(("second", 6, 7, 8, 9, 10))
            second.close()

            rows = list(csv.reader(io.StringIO(second.processed_path.read_text())))
            self.assertEqual(rows[0], list(LOGGER.PROCESSED_HEADER))
            self.assertEqual(rows.count(list(LOGGER.PROCESSED_HEADER)), 1)
            self.assertEqual(rows[1][0], "first")
            self.assertEqual(rows[2][0], "second")

    def test_same_day_raw_log_restart_appends_bytes(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            log_date = datetime(2026, 9, 6).date()
            first = LOGGER.DailyLogFiles(root)
            first.ensure_open(log_date)
            LOGGER.write_raw_line(first.raw_file, b"first\r\n")
            first.close()

            second = LOGGER.DailyLogFiles(root)
            second.ensure_open(log_date)
            LOGGER.write_raw_line(second.raw_file, b"second\r\n")
            second.close()

            self.assertEqual(second.raw_path, first.raw_path)
            self.assertEqual(second.raw_path.read_bytes(), b"first\r\nsecond\r\n")

    def test_existing_empty_csv_receives_header(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            log_date = datetime(2026, 9, 6).date()
            _, processed_path = LOGGER.daily_log_paths(log_date, root)
            processed_path.touch()

            log_files = LOGGER.DailyLogFiles(root)
            log_files.ensure_open(log_date)
            log_files.close()

            rows = list(csv.reader(io.StringIO(processed_path.read_text())))
            self.assertEqual(rows, [list(LOGGER.PROCESSED_HEADER)])

    def test_day_rollover_closes_old_files_and_writes_new_pair(self):
        received_lines = [
            b"42,13241,18470,840,15\n",
            b"43,13242,18471,841,16\n",
        ]
        times = iter(
            [
                datetime(2026, 9, 5, 23, 59, 59, tzinfo=timezone.utc),
                datetime(2026, 9, 6, 0, 0, 0, tzinfo=timezone.utc),
            ]
        )

        class FakeSerial:
            def readline(self):
                if received_lines:
                    return received_lines.pop(0)
                raise KeyboardInterrupt

        with tempfile.TemporaryDirectory() as directory:
            log_files = LOGGER.DailyLogFiles(Path(directory))
            log_files.ensure_open(datetime(2026, 9, 5).date())
            old_raw_file = log_files.raw_file
            old_processed_file = log_files.processed_file

            count = LOGGER.log_stream(FakeSerial(), log_files, lambda: next(times))
            log_files.close()

            self.assertEqual(count, 2)
            self.assertTrue(old_raw_file.closed)
            self.assertTrue(old_processed_file.closed)
            for day in ("2026-09-05", "2026-09-06"):
                raw_path = Path(directory) / f"data/raw/vedirect_{day}.log"
                csv_path = Path(directory) / f"data/processed/vedirect_{day}.csv"
                self.assertTrue(raw_path.exists())
                self.assertTrue(csv_path.exists())
                self.assertEqual(len(csv_path.read_text().splitlines()), 2)

    def test_historical_session_files_are_not_selected_or_modified(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            historical = root / "data/processed/vedirect_2026-09-05_230319.csv"
            historical.parent.mkdir(parents=True)
            historical.write_text("historical\n")

            log_files = LOGGER.DailyLogFiles(root)
            log_files.ensure_open(datetime(2026, 9, 5).date())
            log_files.close()

            self.assertEqual(historical.read_text(), "historical\n")
            self.assertEqual(
                log_files.processed_path.name,
                "vedirect_2026-09-05.csv",
            )

    def test_historical_raw_session_file_is_not_selected_or_modified(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            historical = root / "data/raw/vedirect_2026-09-05_230319.log"
            historical.parent.mkdir(parents=True)
            historical.write_bytes(b"historical\r\n")

            log_files = LOGGER.DailyLogFiles(root)
            log_files.ensure_open(datetime(2026, 9, 5).date())
            LOGGER.write_raw_line(log_files.raw_file, b"daily\r\n")
            log_files.close()

            self.assertEqual(historical.read_bytes(), b"historical\r\n")
            self.assertEqual(log_files.raw_path.name, "vedirect_2026-09-05.log")
            self.assertEqual(log_files.raw_path.read_bytes(), b"daily\r\n")

    def test_single_discovered_port_is_selected(self):
        with patch.object(
            LOGGER.glob,
            "glob",
            return_value=["/dev/cu.usbmodem1234"],
        ):
            self.assertEqual(
                LOGGER.select_port(),
                "/dev/cu.usbmodem1234",
            )

    def test_zero_discovered_ports_is_an_error(self):
        with patch.object(LOGGER.glob, "glob", return_value=[]):
            with self.assertRaisesRegex(RuntimeError, "No Pico"):
                LOGGER.select_port()

    def test_multiple_discovered_ports_is_an_error(self):
        devices = [
            "/dev/cu.usbmodem1111",
            "/dev/cu.usbmodem2222",
        ]

        with patch.object(LOGGER.glob, "glob", return_value=devices):
            with self.assertRaisesRegex(RuntimeError, "Multiple Pico"):
                LOGGER.select_port()

    def test_header_is_ignored(self):
        self.assertIsNone(LOGGER.parse_data_line(LOGGER.PICO_HEADER))

    def test_valid_row_is_accepted(self):
        self.assertEqual(
            LOGGER.parse_data_line("42,13241,18470,840,15"),
            (42, 13241, 18470, 840, 15),
        )

    def test_wrong_number_of_fields_is_rejected(self):
        self.assertIsNone(LOGGER.parse_data_line("42,13241,18470,840"))

    def test_non_integer_field_is_rejected(self):
        self.assertIsNone(LOGGER.parse_data_line("42,13241,bad,840,15"))

    def test_sequence_gap_is_detected(self):
        self.assertEqual(LOGGER.expected_sequence(102, 104), 103)
        self.assertIsNone(LOGGER.expected_sequence(102, 103))
        self.assertIsNone(LOGGER.expected_sequence(None, 42))

    def test_raw_line_is_preserved_when_parsing_fails(self):
        raw_file = io.BytesIO()
        malformed = b"not,a,valid,row\r\n"

        LOGGER.write_raw_line(raw_file, malformed)

        self.assertEqual(raw_file.getvalue(), malformed)
        self.assertIsNone(
            LOGGER.parse_data_line(malformed.decode("ascii").rstrip("\r\n"))
        )

    def test_stream_writes_raw_and_processed_logs(self):
        received_lines = [
            (LOGGER.PICO_HEADER + "\n").encode("ascii"),
            b"42,13241,18470,840,15\n",
            b"malformed\n",
            b"44,13255,18492,851,16\n",
        ]

        class FakeSerial:
            def readline(self):
                if received_lines:
                    return received_lines.pop(0)
                raise KeyboardInterrupt

        warnings = io.StringIO()
        now = datetime(2026, 9, 5, 12, 0, tzinfo=timezone(timedelta(hours=2)))

        with tempfile.TemporaryDirectory() as directory:
            log_files = LOGGER.DailyLogFiles(Path(directory))
            with redirect_stderr(warnings):
                sample_count = LOGGER.log_stream(
                    FakeSerial(),
                    log_files,
                    lambda: now,
                )
            log_files.close()

            self.assertEqual(sample_count, 2)
            self.assertIn(b"malformed\n", log_files.raw_path.read_bytes())
            rows = list(
                csv.reader(io.StringIO(log_files.processed_path.read_text()))
            )
            self.assertEqual(rows[0], list(LOGGER.PROCESSED_HEADER))
            self.assertEqual(rows[1][1:], ["42", "13241", "18470", "840", "15"])
            self.assertEqual(rows[2][1:], ["44", "13255", "18492", "851", "16"])
            self.assertRegex(rows[1][0], r"T.*[+-]\d\d:\d\d$")
            self.assertIn("expected 43, received 44", warnings.getvalue())

    def test_sample_count_is_preserved_when_serial_read_fails(self):
        class SimulatedSerialError(Exception):
            pass

        class FakeSerial:
            calls = 0

            def readline(self):
                self.calls += 1
                if self.calls == 1:
                    return b"42,13241,18470,840,15\n"
                raise SimulatedSerialError("serial failed")

        now = datetime(2026, 9, 5, 12, 0, tzinfo=timezone.utc)
        stats = LOGGER.LoggerStats()

        with tempfile.TemporaryDirectory() as directory:
            log_files = LOGGER.DailyLogFiles(Path(directory))
            with self.assertRaises(SimulatedSerialError):
                LOGGER.log_stream(
                    FakeSerial(),
                    log_files,
                    lambda: now,
                    stats,
                )
            log_files.close()

        self.assertEqual(stats.valid_samples, 1)

    def test_main_reports_samples_written_before_serial_error(self):
        class SimulatedSerialError(Exception):
            pass

        class FakeSerial:
            calls = 0

            def __init__(self, *_args, **_kwargs):
                pass

            def __enter__(self):
                return self

            def __exit__(self, _exception_type, _exception, _traceback):
                pass

            def readline(self):
                self.calls += 1
                if self.calls == 1:
                    return b"42,13241,18470,840,15\n"
                raise SimulatedSerialError("serial failed")

        fake_serial_module = SimpleNamespace(
            Serial=FakeSerial,
            SerialException=SimulatedSerialError,
        )
        output = io.StringIO()
        errors = io.StringIO()

        with tempfile.TemporaryDirectory() as directory:
            log_files = LOGGER.DailyLogFiles(Path(directory))
            with (
                patch.object(
                    LOGGER,
                    "parse_arguments",
                    return_value=SimpleNamespace(port="/dev/fake"),
                ),
                patch.object(LOGGER, "DailyLogFiles", return_value=log_files),
                patch.dict("sys.modules", {"serial": fake_serial_module}),
                redirect_stdout(output),
                redirect_stderr(errors),
            ):
                return_code = LOGGER.main()

        self.assertEqual(return_code, 1)
        self.assertIn("Valid samples written: 1", output.getvalue())
        self.assertIn("serial communication failed", errors.getvalue())


if __name__ == "__main__":
    unittest.main()

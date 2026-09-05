import csv
from contextlib import redirect_stderr
import importlib.util
import io
from pathlib import Path
import unittest
from unittest.mock import patch


SCRIPT_PATH = Path(__file__).resolve().parents[1] / "scripts" / "log_vedirect.py"
SPEC = importlib.util.spec_from_file_location("log_vedirect", SCRIPT_PATH)
LOGGER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(LOGGER)


class LoggerTests(unittest.TestCase):
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

        raw_file = io.BytesIO()
        processed_file = io.StringIO(newline="")
        warnings = io.StringIO()

        with redirect_stderr(warnings):
            sample_count = LOGGER.log_stream(
                FakeSerial(),
                raw_file,
                processed_file,
            )

        self.assertEqual(sample_count, 2)
        self.assertIn(b"malformed\n", raw_file.getvalue())
        rows = list(csv.reader(io.StringIO(processed_file.getvalue())))
        self.assertEqual(rows[0], list(LOGGER.PROCESSED_HEADER))
        self.assertEqual(rows[1][1:], ["42", "13241", "18470", "840", "15"])
        self.assertEqual(rows[2][1:], ["44", "13255", "18492", "851", "16"])
        self.assertRegex(rows[1][0], r"T.*[+-]\d\d:\d\d$")
        self.assertIn("expected 43, received 44", warnings.getvalue())


if __name__ == "__main__":
    unittest.main()

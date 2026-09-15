from datetime import datetime, timezone
import importlib.util
from pathlib import Path
import tempfile
import unittest


SCRIPT_PATH = Path(__file__).resolve().parents[1] / "scripts" / "dashboard.py"
SPEC = importlib.util.spec_from_file_location("dashboard", SCRIPT_PATH)
DASHBOARD = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(DASHBOARD)


NOW = datetime(2026, 9, 12, 12, 0, tzinfo=timezone.utc)


def now_provider():
    return NOW


def write_csv(root, text):
    path = DASHBOARD.current_csv_path(root, NOW.date())
    path.parent.mkdir(parents=True)
    path.write_text(text)


HEADER = "timestamp,sequence,battery_mv,panel_mv,battery_ma,panel_w\n"
ROW_1 = "2026-09-12T00:00:01+00:00,1,13210,18470,840,15\n"
ROW_2 = "2026-09-12T00:00:02+00:00,2,13220,18480,850,16\n"


class DashboardTests(unittest.TestCase):
    def test_current_day_snapshot_uses_valid_rows(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            write_csv(root, HEADER + ROW_1)
            payload = DASHBOARD.dashboard_payload(root, now_provider)

            self.assertTrue(payload["snapshot"])
            self.assertEqual(payload["day"], "2026-09-12")
            self.assertEqual(payload["samples"][0]["panel_w"], 15)

    def test_missing_or_empty_csv_has_no_samples(self):
        with tempfile.TemporaryDirectory() as directory:
            payload = DASHBOARD.dashboard_payload(Path(directory), now_provider)
            self.assertEqual(payload["samples"], [])

    def test_malformed_final_row_is_ignored(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            write_csv(root, HEADER + ROW_1 + "2026-09-12T00:00:03+00:00,3,broken")
            payload = DASHBOARD.dashboard_payload(root, now_provider)
            self.assertEqual([sample["sequence"] for sample in payload["samples"]], [1])

    def test_incremental_response_contains_only_new_rows(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            write_csv(root, HEADER + ROW_1 + ROW_2)
            payload = DASHBOARD.dashboard_payload(root, now_provider, "2026-09-12", "1")
            self.assertFalse(payload["snapshot"])
            self.assertEqual([sample["sequence"] for sample in payload["samples"]], [2])

    def test_old_client_day_receives_replacement_snapshot(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            write_csv(root, HEADER + ROW_1)
            payload = DASHBOARD.dashboard_payload(root, now_provider, "2026-09-11", "99")
            self.assertTrue(payload["snapshot"])
            self.assertEqual([sample["sequence"] for sample in payload["samples"]], [1])


if __name__ == "__main__":
    unittest.main()

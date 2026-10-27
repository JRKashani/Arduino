"""Run from tools/manual_drive: ..\\..\\.venv\\Scripts\\python.exe -m unittest -q"""
import csv
import json
import tempfile
import unittest
from pathlib import Path

import protocol
from link import Link, fields_warning
from recorder import Recorder, status_text
from session import Session
from summary import write_summaries
from teleop import DriveState, apply_key, drive_command

FIELDS = protocol.EXPECTED_FIELDS


def s_line(t=1000, speed=0, l_cm=31.2, l_ok=1, f_cm=85.0, f_ok=1, roll=0.5):
    values = [t, speed, 0, 0, l_cm, l_ok, 300, 29.8, 1, 286, f_cm, f_ok, 75.0,
              36.1, 1, 36.1, "F", 512, 498, 1, roll, -0.2, 0.01, 0.0, 1.0]
    return "S," + ",".join(str(v) for v in values)


class FakePort:
    def __init__(self, incoming=b""):
        self.incoming, self.sent = bytearray(incoming), []

    @property
    def in_waiting(self):
        return len(self.incoming)

    def read(self, n):
        data = bytes(self.incoming[:n])
        del self.incoming[:n]
        return data

    def write(self, data):
        self.sent.append(data.decode("ascii").strip())

    def feed(self, *lines):
        self.incoming.extend("".join(l + "\n" for l in lines).encode("ascii"))


class Clock:
    def __init__(self):
        self.now = 0.0

    def __call__(self):
        return self.now


class TempSession(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.session = Session(Path(self.tmp.name))

    def tearDown(self):
        self.session.close()
        self.tmp.cleanup()


class ProtocolTests(unittest.TestCase):
    def test_sample_round_trip(self):
        sample = protocol.parse_sample(s_line(), FIELDS)
        self.assertEqual(sample["l_cm"], 31.2)
        self.assertEqual(sample["us_new"], "F")
        self.assertEqual(sample["ldr_l"], 512)

    def test_rejects_wrong_field_count_and_nan(self):
        with self.assertRaises(ValueError):
            protocol.parse_sample("S,1,2", FIELDS)
        with self.assertRaises(ValueError):
            protocol.parse_sample(s_line(l_cm="nan"), FIELDS)

    def test_fields_and_config(self):
        self.assertEqual(protocol.parse_fields("FIELDS," + ",".join(FIELDS)), FIELDS)
        self.assertEqual(protocol.parse_config("CONFIG,US_SCALE,1.01535"), ("US_SCALE", 1.01535))
        self.assertEqual(protocol.parse_event("EVT,1234,SAFETY_BLOCK"), (1234, "SAFETY_BLOCK"))
        self.assertIsNone(fields_warning(FIELDS))
        self.assertIsNotNone(fields_warning(FIELDS[:-1]))


class TeleopTests(unittest.TestCase):
    def press(self, *keys):
        state = DriveState()
        for key in keys:
            state, _ = apply_key(state, key)
        return state

    def test_speed_skips_stall_band(self):
        self.assertEqual(self.press("w").speed, 40)
        self.assertEqual(self.press("w", "w").speed, 50)
        self.assertEqual(self.press("w", "s").speed, 0)
        self.assertEqual(self.press("s").speed, -40)
        self.assertEqual(self.press(*["w"] * 30).speed, 100)

    def test_steer_arrows_and_stop(self):
        self.assertEqual(self.press("left", "left").steer, 10)
        self.assertEqual(self.press("D").steer, -5)
        self.assertEqual(self.press("a", "c").steer, 0)
        self.assertEqual(self.press("w", "a", " "), DriveState())
        self.assertEqual(self.press(*["a"] * 30).steer, 60)

    def test_actions(self):
        self.assertEqual(apply_key(DriveState(), "3")[1], "segment:3")
        self.assertEqual(apply_key(DriveState(), "esc")[1], "quit")
        self.assertIsNone(apply_key(DriveState(), "z")[1])
        self.assertEqual(drive_command(DriveState(50, -10)), "D,50,-10")


class LinkTests(TempSession):
    def test_connect_reads_banner(self):
        port = FakePort()
        port.feed("BOOT,MANUAL,1", "IMU baseline roll=0.10 pitch=0.20", "MANUAL,1,1.0.0",
                  "CONFIG,LASER_SCALE,0.969241451", "FIELDS," + ",".join(FIELDS), "READY")
        version, config, fields, boot = Link(port, self.session).connect()
        self.assertEqual(version, "1.0.0")
        self.assertAlmostEqual(config["LASER_SCALE"], 0.969241451)
        self.assertEqual(fields, FIELDS)
        self.assertIn("IMU baseline roll=0.10 pitch=0.20", boot)

    def test_partial_lines_wait_for_newline(self):
        port = FakePort(b"S,1")
        link = Link(port, self.session)
        self.assertEqual(link.lines(), [])
        port.incoming.extend(b"23\nREADY\n")
        self.assertEqual(link.lines(), ["S,123", "READY"])


class RecorderTests(TempSession):
    def make(self, keys=()):
        self.port, self.clock, pending = FakePort(), Clock(), [list(keys)]
        self.session.open_samples(FIELDS)

        def read_keys():
            batch, pending[0] = pending[0], []
            return batch
        rec = Recorder(Link(self.port, self.session), self.session, FIELDS, read_keys,
                       out=lambda *a, **k: None, clock=self.clock)
        return rec

    def rows(self):
        self.session.samples_file.flush()
        with (self.session.path / "samples.csv").open(encoding="utf-8") as handle:
            return list(csv.DictReader(handle))

    def events(self):
        lines = (self.session.path / "events.jsonl").read_text(encoding="utf-8").splitlines()
        return [json.loads(l) for l in lines]

    def test_keys_send_drive_and_samples_are_tagged(self):
        rec = self.make(["w", "a", "2"])
        self.port.feed(s_line(t=10), "EVT,20,SAFETY_BLOCK")
        rec.tick()
        self.assertEqual(self.port.sent[-1], "D,40,5")
        row = self.rows()[0]
        self.assertEqual((row["run"], row["segment"], row["l_cm"]), ("1", "2", "31.2"))
        kinds = [e["kind"] for e in self.events()]
        self.assertIn("segment", kinds)
        self.assertIn("firmware", kinds)

    def test_heartbeat_repeats_command(self):
        rec = self.make(["w"])
        rec.tick()
        sent = len(self.port.sent)
        self.clock.now = 0.2
        rec.tick()
        self.assertEqual(self.port.sent[sent:], ["D,40,0"])

    def test_new_run_stops_and_resets_segment(self):
        rec = self.make(["w", "3", "n"])
        rec.tick()
        self.assertEqual((rec.run, rec.segment, rec.state), (2, 0, DriveState()))
        self.assertIn("STOP", self.port.sent)

    def test_quit_stops_robot(self):
        rec = self.make(["w", "q"])
        rec.loop(sleep=lambda _: None)
        self.assertTrue(rec.done)
        self.assertEqual(self.port.sent[-3:], ["STOP"] * 3)

    def test_bad_line_is_logged_not_fatal(self):
        rec = self.make()
        self.port.feed("S,broken", s_line())
        rec.tick()
        self.assertEqual((rec.bad_lines, rec.samples), (1, 1))

    def test_board_reset_stops(self):
        rec = self.make(["w"])
        rec.tick()
        self.port.feed("BOOT,MANUAL,1")
        rec.tick()
        self.assertEqual(rec.state, DriveState())
        self.assertIn("board_reset", [e["kind"] for e in self.events()])

    def test_status_text_marks_invalid(self):
        rec = self.make()
        self.port.feed(s_line(l_ok=0))
        rec.tick()
        self.assertIn("L   -- ", status_text(rec))


class SummaryTests(TempSession):
    def test_segment_stats_use_valid_readings_only(self):
        self.session.open_samples(FIELDS)
        for i, (seg, l_cm, ok) in enumerate([(1, 30.0, 1), (1, 32.0, 1), (1, 999.0, 0), (2, 50.0, 1)]):
            sample = protocol.parse_sample(s_line(t=i, l_cm=l_cm, l_ok=ok), FIELDS)
            self.session.sample(1, seg, sample)
        self.session.samples_file.flush()
        table = write_summaries([self.session.path], self.session.path / "out")
        left = {r["segment"]: r for r in table if r["channel"] == "l_cm"}
        self.assertEqual(left["1"]["n"], 2)
        self.assertEqual(left["1"]["valid_pct"], 66.7)
        self.assertEqual(left["1"]["median"], 31.0)
        self.assertEqual(left["2"]["max"], 50.0)
        self.assertTrue((self.session.path / "out" / "by-run.csv").exists())

    def test_plot_writes_png(self):
        from plot_runs import plot_sessions
        self.session.open_samples(FIELDS)
        for i in range(5):
            self.session.sample(1, 1 if i < 3 else 2, protocol.parse_sample(s_line(t=i), FIELDS))
        self.session.samples_file.flush()
        written = plot_sessions([self.session.path], self.session.path / "plots")
        self.assertEqual(len(written), 1)
        self.assertGreater(written[0].stat().st_size, 1000)


if __name__ == "__main__":
    unittest.main()

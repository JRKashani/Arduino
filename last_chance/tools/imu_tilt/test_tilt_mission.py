"""Run: python -m unittest discover -s tools/imu_tilt -p 'test_*.py' -v"""
import argparse
import contextlib
import csv
import copy
import io
import json
import math
import tempfile
import types
import unittest
from pathlib import Path
from unittest.mock import patch

from analysis import analyze, multiply, rotation, transpose
from core import (CAL_KEYS, angle_delta, board_to_heading, combine_reference, compare_stationary, convert_ratios, fit_axis,
                  heading_to_board, parse_mapping, parse_sample, phone_vector, stats,
                  summarize, to_body, to_sensor, vector_angles, vector_distance, within_tilt_tolerance)
from demo import generate
from recording import Device, EventLog, check_calibration_info, load_events, planned_surfaces, record, session_events


class GeometryTests(unittest.TestCase):
    def test_provisional_provenance_must_match_actual_firmware_constants(self):
        info = json.loads((Path(__file__).resolve().parents[2]/"docs"/"imu-calibration-replacement-2026-10-11.json").read_text())
        candidate = info["candidate_constants"]
        # Firmware reports rounded floats; preserve the full candidate in metadata.
        actual = {key: round(value, 7) for key, value in candidate.items()}
        check_calibration_info(info, actual)
        self.assertFalse(info["validation_performed"])
        actual["y_zero"] += 0.01
        with self.assertRaisesRegex(ValueError, "does not match"):
            check_calibration_info(info, actual)
        broken = copy.deepcopy(info)
        broken["candidate_constants"]["z_per_g"] = 0
        with self.assertRaisesRegex(ValueError, "invalid constants"):
            check_calibration_info(broken, candidate)

    def test_three_degree_target_includes_boundary_but_not_four_degree_bias(self):
        self.assertTrue(within_tilt_tolerance(3, -3))
        self.assertFalse(within_tilt_tolerance(3.01, 0))
        self.assertFalse(within_tilt_tolerance(4, 0))
        self.assertFalse(within_tilt_tolerance(float("nan"), 0))

    def assertVector(self, actual, expected):
        for a, b in zip(actual, expected):
            self.assertAlmostEqual(a, b, places=10)

    def test_cardinal_headings_and_opposite_signs(self):
        a = math.radians(20)
        board = (math.sin(a), 0, math.cos(a))
        angles = [vector_angles(board_to_heading(board, heading)) for heading in (0, 90, 180, 270)]
        for i, (roll, pitch) in enumerate(((0, 20), (-20, 0), (0, -20), (20, 0))):
            self.assertAlmostEqual(angles[i]["roll"], roll)
            self.assertAlmostEqual(angles[i]["pitch"], pitch)

    def test_cross_slope_phone_rotations_recover_one_board_vector(self):
        board = phone_vector(18, 7, "axis-elevation")
        readings = []
        for heading in (0, 90, 180, 270):
            phone = board_to_heading(board, heading)
            readings.append({"heading": heading, "up": math.degrees(math.asin(phone[0])),
                             "right": math.degrees(math.asin(phone[1]))})
        ref = combine_reference(readings, "axis-elevation")
        self.assertVector(ref["vector"], board)
        self.assertLess(ref["max_disagreement_deg"], 1e-5)

    def test_euler_pair_differs_from_independent_axis_elevations(self):
        euler = phone_vector(25, 15, "euler")
        angles = vector_angles(euler)
        self.assertAlmostEqual(angles["pitch"], 25)
        self.assertAlmostEqual(angles["roll"], 15)
        self.assertGreater(vector_distance(euler, phone_vector(25, 15, "axis-elevation")), 1)

    def test_gradient_is_normalized_and_angles_follow_ratio(self):
        v = phone_vector(20, 10, "gradient")
        self.assertAlmostEqual(sum(x*x for x in v), 1)
        self.assertAlmostEqual(math.degrees(math.atan2(v[0], v[2])), 20)
        self.assertAlmostEqual(math.degrees(math.atan2(v[1], v[2])), 10)

    def test_phone_convention_bounds_and_complete_orientations(self):
        for args in ((0, 0, "unverified"), (70, 70, "axis-elevation"), (float('nan'), 0, "euler")):
            with self.assertRaises(ValueError): phone_vector(*args)
        with self.assertRaises(ValueError): combine_reference([], "euler")

    def test_signed_permutation_and_mount_rotation_round_trip(self):
        mapping = parse_mapping("z,-x,y")
        v = (0.2, -0.4, 0.7)
        self.assertVector(to_sensor(to_body(v, mapping), mapping), v)
        matrix = rotation(12, -4, 22)
        self.assertVector(multiply(transpose(matrix), multiply(matrix, v)), v)
        self.assertVector(heading_to_board(board_to_heading(v, 90), 90), v)
        with self.assertRaises(ValueError): parse_mapping("x,x,z")

    def test_wrap_and_gravity_span_fit(self):
        self.assertEqual(angle_delta(-179, 179), 2)
        g = [-0.5, -0.2, 0.2, 0.5]
        fit = fit_axis(g, [0.51+0.102*x for x in g])
        self.assertTrue(fit["usable"])
        self.assertAlmostEqual(fit["zero"], 0.51)
        self.assertAlmostEqual(fit["per_g"], 0.102)
        weak = fit_axis([0.87, 0.92, 0.97, 1], [0.6, 0.61, 0.62, 0.63])
        self.assertFalse(weak["usable"])
        wrong = fit_axis(g, [0.51-0.102*x for x in g])
        self.assertFalse(wrong["usable"])


def wire_sample(sequence=0, elapsed=0):
    return f"DATA,1,{sequence},{elapsed},1,347,342,409,675,0,0,1,1,0,0,0,0"


class FakeSerial:
    def __init__(self, records):
        self.records = iter(records)
        self.sent = []

    def read_until(self, delimiter):
        value = next(self.records, b"")
        if isinstance(value, BaseException): raise value
        return value

    def write(self, value): self.sent.append(value)
    def flush(self): pass
    def reset_input_buffer(self): pass


class ProtocolTests(unittest.TestCase):
    def test_handshake_requires_protocol_and_calibration(self):
        with tempfile.TemporaryDirectory() as tmp:
            log = EventLog(tmp)
            port = FakeSerial([b"IDLE\n", b"IMUTILT,1,1.0.0\n",
                               b"CONFIG,0.5,0.5,0.5,0.1,0.1,0.1\n", b"BASELINE,NONE,0,0\n", b"READY\n"])
            try:
                with patch("recording.time.sleep"):
                    device = Device(port, log)
                    device.connect()
                self.assertEqual(device.calibration["x_per_g"], 0.1)
                self.assertEqual(port.sent, [b"ABORT\n", b"INFO\n"])
                bad = Device(FakeSerial([b"IMUTILT,2,2.0.0\n"]), log)
                with patch("recording.time.sleep"), self.assertRaises(RuntimeError):
                    bad.connect()
            finally:
                log.close()

    def test_parser_rejects_nan_bad_adc_and_wrong_field_count(self):
        self.assertEqual(parse_sample(wire_sample())["raw_ref"], 675)
        for line in (wire_sample().replace(",675,", ",1024,"), wire_sample().replace(",0,0,1,1,", ",nan,0,1,1,"), "DATA,1,2"):
            with self.assertRaises(ValueError): parse_sample(line)

    def capture(self, records):
        with tempfile.TemporaryDirectory() as tmp:
            log = EventLog(tmp)
            serial = FakeSerial(records)
            device = Device(serial, log)
            try:
                with contextlib.redirect_stdout(io.StringIO()):
                    result = device.capture(1, 0, 1000, 1000000)
            finally:
                log.close()
            return result, session_events(tmp), serial.sent

    def test_partial_lines_preserved_and_trial_linked(self):
        sample = (wire_sample()+"\n").encode()
        (samples, end), events, sent = self.capture([
            b"SETTLE,1\n", b"CAPTURE,1\n", sample[:20], sample[20:], b"END,1,DONE,1,0,1000000\n"])
        self.assertEqual(len(samples), 1)
        self.assertEqual(end["status"], "DONE")
        self.assertEqual([e["kind"] for e in events], ["sample", "end"])
        self.assertEqual(sent, [b"START,1,0,1000,1000000\n"])

    def test_ctrl_c_aborts_but_preserves_partial_capture(self):
        (samples, end), events, sent = self.capture([
            b"SETTLE,1\n", b"CAPTURE,1\n", (wire_sample()+"\n").encode(),
            KeyboardInterrupt(), b"END,1,ABORT,1,0,300000\n"])
        self.assertEqual(len(samples), 1)
        self.assertEqual(end["status"], "ABORT")
        self.assertIn(b"ABORT\n", sent)

    def test_reset_count_mismatch_and_wrong_trial_are_rejected(self):
        invalid = [
            [b"IMUTILT,1,1.0.0\n"],
            [b"CAPTURE,1\n", b"END,1,DONE,2,0,1000000\n"],
            [b"CAPTURE,1\n", (wire_sample().replace("DATA,1,", "DATA,2,")+"\n").encode()],
            [b"CAPTURE,1\n", (wire_sample()+"\n").encode(), b"END,1,DONE,1,0,500000\n"],
        ]
        for records in invalid:
            with self.assertRaises(RuntimeError): self.capture(records)

    def test_duplicate_sequence_rejected(self):
        with self.assertRaises(RuntimeError):
            self.capture([b"CAPTURE,1\n", (wire_sample()+"\n").encode(), (wire_sample()+"\n").encode()])

    def test_summary_keeps_noise_bias_and_invalid_distinct(self):
        samples = [parse_sample(wire_sample(0)), parse_sample(wire_sample(2, 40000))]
        samples[0]["magnitude"] = 1.3
        samples[1]["valid"] = 0
        summary = summarize(samples)
        self.assertEqual(summary["invalid"], 1)
        self.assertEqual(summary["sequence_missing"], 1)
        self.assertEqual(summary["magnitude"]["mean"], 1.3)
        self.assertEqual(stats([])["mean"], None)

    def test_torn_last_event_does_not_hide_middle_corruption(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp)/"events.jsonl"
            path.write_text('{"kind":"sample"}\n{"partial":', encoding="utf-8")
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(len(load_events(path)), 1)
            path.write_text('{bad}\n{"kind":"sample"}\n', encoding="utf-8")
            with self.assertRaises(ValueError): load_events(path)


def analysis_args(session, verified=True):
    return argparse.Namespace(session=str(session), phone_convention="axis-elevation" if verified else "unverified",
                              up_sign=1, right_sign=1, sensor_map="x,y,z", axes_verified=verified,
                              mount_roll=0, mount_pitch=0, mount_yaw=0, max_std=0.5, max_drift=0.5,
                              max_reference_disagreement=1, minimum_span=0.2, no_plots=True,
                              command="analyze")


class PipelineTests(unittest.TestCase):
    def test_inferred_mapping_compares_and_fits_but_never_recommends_application(self):
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)/"session"
            args = analysis_args(directory)
            args.axes_verified = False
            args.axes_inferred = True
            with contextlib.redirect_stdout(io.StringIO()):
                generate(directory)
                destination = analyze(args)
            report = json.loads((destination/"report.json").read_text())
            proposal = report["calibration_proposal"]
            self.assertIsNotNone(report["current_vector_rms_deg"])
            self.assertIsNotNone(proposal["candidate_constants"])
            self.assertTrue(proposal["held_out_improvement"])
            self.assertEqual(proposal["axes_status"], "inferred")
            self.assertFalse(proposal["recommended"])
            self.assertFalse(proposal["active_calibration_changed"])
            self.assertTrue(any("not physically verified" in w for w in report["warnings"]))

    def test_keep_repeat_combines_means_with_equal_capture_weight(self):
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)/"session"
            with contextlib.redirect_stdout(io.StringIO()): generate(directory)
            event_file = next(directory.glob("events-*.jsonl"))
            events = load_events(event_file)
            original_trial = next(e for e in events if e["kind"] == "trial" and e["trial_id"] == 3)
            original_trial["placement_id"] = "KEPT-PAIR"
            original_review = next(e for e in events if e["kind"] == "review" and e["trial_id"] == 3)
            original_review["decision"] = "retain"
            repeat = copy.deepcopy(original_trial)
            repeat["trial_id"] = 44
            repeat["settings"]["period_us"] = 40000
            original_samples = [e for e in events if e["kind"] == "sample" and e["trial_id"] == 3]
            new_samples = []
            for old in original_samples[:50]:
                new = copy.deepcopy(old)
                new.update(trial_id=44, raw_x=old["raw_x"]+1, elapsed_us=old["sequence"]*40000)
                g = convert_ratios([new["raw_"+a]/new["raw_ref"] for a in "xyz"], repeat["calibration"])
                angles = vector_angles(g)
                new.update(ax=g[0], ay=g[1], az=g[2], magnitude=math.sqrt(sum(v*v for v in g)),
                           raw_roll=angles["roll"], robot_roll=angles["roll"],
                           raw_pitch=-angles["pitch"], robot_pitch=angles["pitch"])
                new_samples.append(new)
            events.extend([repeat, *new_samples,
                           {**original_review, "kind": "end", "trial_id": 44, "status": "DONE", "firmware_samples": 50, "firmware_missed": 0, "duration_us": 2000000},
                           {**original_review, "trial_id": 44, "decision": "accept"}])
            event_file.write_text("".join(json.dumps(e)+"\n" for e in events), encoding="utf-8")
            with contextlib.redirect_stdout(io.StringIO()): destination = analyze(analysis_args(directory))
            report = json.loads((destination/"report.json").read_text())
            self.assertEqual(report["calibration_proposal"]["training_placements"], 20)
            self.assertEqual(len(report["calibration_proposal"]["training_trial_ids"]), 21)
            self.assertEqual(report["accepted_slope_placements"], 40)
            with (destination/"placements.csv").open(newline="") as file:
                combined = next(r for r in csv.DictReader(file) if r["placement_id"] == "KEPT-PAIR")
            self.assertEqual(int(combined["captures"]), 2)
            self.assertEqual(int(combined["valid_samples"]), 150)
            first_mean = summarize(original_samples)["ratio_x"]["mean"]
            next_mean = summarize(new_samples, 40000)["ratio_x"]["mean"]
            self.assertAlmostEqual(float(combined["ratio_x_mean"]), (first_mean+next_mean)/2)
            self.assertNotAlmostEqual(float(combined["ratio_x_mean"]), (first_mean*100+next_mean*50)/150, places=6)

    def test_flat_jump_flags_affected_interval_without_changing_raw_records(self):
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)/"session"
            with contextlib.redirect_stdout(io.StringIO()): generate(directory)
            event_file = next(directory.glob("events-*.jsonl"))
            events = load_events(event_file)
            final_id = next(e["trial_id"] for e in events if e["kind"] == "trial" and e["surface"] == "LEVEL_END")
            for e in events:
                if e["kind"] == "sample" and e["trial_id"] == final_id:
                    e["robot_roll"] += 11
                    e["raw_roll"] += 11
                    e["robot_pitch"] += 11
                    e["raw_pitch"] -= 11
                    e["magnitude"] += 0.24
                    e["ay"] += 0.2
                    e["ax"] += 0.2
                    e["raw_x"] += 15
                    e["raw_y"] += 15
                    e["raw_z"] += 15
            event_file.write_text("".join(json.dumps(e)+"\n" for e in events), encoding="utf-8")
            original = event_file.read_bytes()
            with contextlib.redirect_stdout(io.StringIO()): destination = analyze(analysis_args(directory))
            report = json.loads((destination/"report.json").read_text())
            self.assertTrue(report["flat_comparisons"][0]["changed"])
            self.assertEqual(report["calibration_proposal"]["training_placements"], 0)
            self.assertEqual(report["accepted_slope_placements"], 40) # Accepted history remains intact.
            self.assertIn("Bracketed flat checks changed", (destination/"trials.csv").read_text())
            self.assertEqual(event_file.read_bytes(), original)

    def test_repeat_keep_then_discard_excludes_the_entire_previous_group(self):
        class StubDevice:
            def __init__(self, port, log):
                self.log = log
                self.calibration = dict(zip(CAL_KEYS, (0.5, 0.5, 0.5, 0.1, 0.1, 0.1)))
            def connect(self): pass
            def send(self, command): pass
            def capture(self, trial_id, **settings):
                data = [parse_sample(wire_sample(i, i*100000)) for i in range(10)]
                for sample in data:
                    sample["trial_id"] = trial_id
                    self.log.write("sample", **sample)
                end = {"status": "DONE", "firmware_samples": 10, "firmware_missed": 0, "duration_us": 1000000}
                self.log.write("end", trial_id=trial_id, **end)
                return data, end
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            (directory/"session.json").write_text(json.dumps({"schema": 1, "session_id": "REPEAT-TEST", "simulated": False, "surfaces": ["S1"], "diagonal": False}))
            review_count = 0
            keep_count = 0
            def respond(prompt):
                nonlocal review_count, keep_count
                if "up right =" in prompt: return "0 0"
                if "Accept [a]" in prompt:
                    review_count += 1
                    return "r" if review_count in (2, 3) else "a"
                if "Keep previous capture(s)" in prompt:
                    keep_count += 1
                    return "k" if keep_count == 1 else "d"
                if "check board drift" in prompt: return "n"
                return ""
            fake_serial = types.SimpleNamespace(Serial=lambda *a, **k: types.SimpleNamespace(close=lambda: None))
            args = argparse.Namespace(port="FAKE", settle=0, duration=1, hz=10, resume=tmp, extend_surfaces=None)
            with patch.dict("sys.modules", {"serial": fake_serial}), patch("recording.Device", StubDevice), patch("builtins.input", respond), contextlib.redirect_stdout(io.StringIO()):
                record(args)
            events = session_events(directory)
            reviews = {e["trial_id"]: e for e in events if e["kind"] == "review"}
            self.assertEqual(reviews[2]["decision"], "reject")
            self.assertEqual(reviews[3]["decision"], "reject")
            self.assertEqual(reviews[4]["decision"], "accept")
            trials = {e["trial_id"]: e for e in events if e["kind"] == "trial"}
            self.assertEqual(trials[2]["placement_id"], trials[4]["placement_id"])
            self.assertEqual(len([e for e in events if e["kind"] == "sample" and e["trial_id"] in (2, 3)]), 20)
            with contextlib.redirect_stdout(io.StringIO()): destination = analyze(analysis_args(directory))
            with (destination/"placements.csv").open(newline="") as file:
                group = next(r for r in csv.DictReader(file) if r["placement_id"] == trials[4]["placement_id"])
            self.assertEqual(group["trial_ids"], "4")

    def test_resume_keeps_accepted_placement_and_uses_new_reference_for_pending_trials(self):
        class StubDevice:
            def __init__(self, port, log):
                self.log = log
                self.calibration = dict(zip(CAL_KEYS, (0.5, 0.5, 0.5, 0.1, 0.1, 0.1)))
            def connect(self): pass
            def send(self, command): pass
            def capture(self, trial_id, **settings):
                data = [parse_sample(wire_sample(i, i*100000)) for i in range(10)]
                for sample in data:
                    sample["trial_id"] = trial_id
                    self.log.write("sample", **sample)
                end = {"status": "DONE", "firmware_samples": 10, "firmware_missed": 0, "duration_us": 1000000}
                self.log.write("end", trial_id=trial_id, **end)
                return data, end

        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)
            metadata = {"schema": 1, "simulated": False, "surfaces": ["S1"], "diagonal": False}
            (directory/"session.json").write_text(json.dumps(metadata), encoding="utf-8")
            old_log = EventLog(directory)
            old_log.write("trial", trial_id=1, surface="S1", loop=1, heading=0, reference_id="OLD")
            old_log.write("review", trial_id=1, decision="accept")
            old_log.close()
            prompts = []
            def respond(prompt):
                prompts.append(prompt)
                if "up right =" in prompt: return "0 0"
                if "Accept [a]" in prompt: return "a"
                if "check board drift" in prompt: return "n"
                return ""
            args = argparse.Namespace(port="FAKE", settle=0, duration=1, hz=10, resume=tmp, extend_surfaces=2)
            fake_serial = types.SimpleNamespace(Serial=lambda *a, **k: types.SimpleNamespace(close=lambda: None))
            with patch.dict("sys.modules", {"serial": fake_serial}), patch("recording.Device", StubDevice), patch("builtins.input", respond), contextlib.redirect_stdout(io.StringIO()):
                record(args)
            events = session_events(directory)
            new_trials = [e for e in events if e["kind"] == "trial" and e["trial_id"] != 1]
            slopes = [e for e in new_trials if e["surface"] == "S1"]
            self.assertEqual(len(slopes), 7)
            self.assertEqual(len(new_trials), 17) # S1 remaining, all S2, two flat checks
            self.assertFalse(any(e["loop"] == 1 and e["heading"] == 0 for e in slopes))
            self.assertTrue(all(e["reference_id"] != "OLD" for e in slopes))
            self.assertEqual(len([e for e in events if e["kind"] == "phone_reading"]), 8)
            self.assertEqual(planned_surfaces(metadata, events), ["S1", "S2"])
            self.assertEqual(json.loads((directory/"session.json").read_text())["surfaces"], ["S1"])
            # A subsequent plain resume remembers the extension and skips both
            # fully accepted surfaces; it takes just the new flat checks.
            args.extend_surfaces = None
            with patch.dict("sys.modules", {"serial": fake_serial}), patch("recording.Device", StubDevice), patch("builtins.input", respond), contextlib.redirect_stdout(io.StringIO()):
                record(args)
            later = session_events(directory)
            self.assertEqual(len([e for e in later if e["kind"] == "trial"]), len(new_trials)+3)

    def test_analysis_counts_extended_plan_and_rejects_shrinking_plan(self):
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)/"session"
            with contextlib.redirect_stdout(io.StringIO()): generate(directory)
            original = (directory/"session.json").read_bytes()
            metadata = json.loads(original)
            log = EventLog(directory)
            log.write("plan_extended", surfaces=metadata["surfaces"]+["S6"])
            log.close()
            with contextlib.redirect_stdout(io.StringIO()): destination = analyze(analysis_args(directory))
            report = json.loads((destination/"report.json").read_text())
            self.assertEqual(report["expected_slope_placements"], 48)
            self.assertEqual(report["accepted_slope_placements"], 40)
            self.assertEqual((directory/"session.json").read_bytes(), original)
            with self.assertRaises(ValueError):
                planned_surfaces(metadata, [{"kind": "plan_extended", "surfaces": ["S1"]}])

    def test_shifted_after_reference_excluded_from_fitting(self):
        with tempfile.TemporaryDirectory() as tmp:
            directory = Path(tmp)/"session"
            with contextlib.redirect_stdout(io.StringIO()): generate(directory)
            event_file = next(directory.glob("events-*.jsonl"))
            events = load_events(event_file)
            new_board = phone_vector(12, 0.7, "axis-elevation")
            for event in events:
                if event["kind"] == "reference" and event["surface"] == "S1" and event["phase"] == "after":
                    for reading in event["readings"]:
                        v = board_to_heading(new_board, reading["heading"])
                        reading["up"] = math.degrees(math.asin(v[0]))
                        reading["right"] = math.degrees(math.asin(v[1]))
            event_file.write_text("".join(json.dumps(e)+"\n" for e in events), encoding="utf-8")
            with contextlib.redirect_stdout(io.StringIO()): destination = analyze(analysis_args(directory))
            report = json.loads((destination/"report.json").read_text())
            self.assertEqual(report["calibration_proposal"]["training_placements"], 16)
            self.assertEqual(report["calibration_proposal"]["validation_placements"], 16)
            self.assertIn("reference changed", (destination/"trials.csv").read_text())

    def test_full_session_repeat_rejection_holdout_and_unverified_mode(self):
        with tempfile.TemporaryDirectory() as tmp:
            session = Path(tmp)/"session"
            with contextlib.redirect_stdout(io.StringIO()):
                generate(session)
                destination = analyze(analysis_args(session))
            report = json.loads((destination/"report.json").read_text())
            proposal = report["calibration_proposal"]
            self.assertEqual(report["accepted_slope_placements"], 40)
            self.assertEqual(proposal["training_placements"], 20)
            self.assertEqual(proposal["validation_placements"], 20)
            self.assertTrue(proposal["recommended"])
            self.assertFalse(proposal["axes"]["z"]["usable"])
            self.assertLess(proposal["validation"]["candidate_vector_rms_deg"], proposal["validation"]["current_vector_rms_deg"])
            self.assertIn("reject", (destination/"trials.csv").read_text())
            with contextlib.redirect_stdout(io.StringIO()):
                raw_destination = analyze(analysis_args(session, verified=False))
            raw = json.loads((raw_destination/"report.json").read_text())
            self.assertIsNone(raw["current_vector_rms_deg"])
            self.assertIsNone(raw["placements_within_tolerance"])
            self.assertEqual(report["tilt_tolerance_deg"], 3)
            with (destination/"placements.csv").open(newline="") as file:
                compared = [r for r in csv.DictReader(file) if r.get("within_3deg_per_axis")]
            expected_passes = sum(abs(float(r["roll_error_deg"])) <= 3 and abs(float(r["pitch_error_deg"])) <= 3 for r in compared)
            self.assertEqual(report["placements_within_tolerance"], expected_passes)
            self.assertIsNone(raw["calibration_proposal"]["candidate_constants"])
            # Source records are never edited by analysis.
            self.assertEqual(len([e for e in session_events(session) if e["kind"] == "sample"]), 4300)

    def test_loop_two_is_not_used_for_fitting(self):
        with tempfile.TemporaryDirectory() as tmp:
            session = Path(tmp)/"session"
            with contextlib.redirect_stdout(io.StringIO()): generate(session)
            event_file = next(session.glob("events-*.jsonl"))
            events = load_events(event_file)
            loop_two = {e["trial_id"] for e in events if e["kind"] == "trial" and e["loop"] == 2}
            # Poison loop-two ratios in this synthetic fixture. Training must
            # recover the same coefficient values independently of validation.
            with contextlib.redirect_stdout(io.StringIO()): before = analyze(analysis_args(session))
            for event in events:
                if event["kind"] == "sample" and event["trial_id"] in loop_two:
                    event["raw_x"] += 10
            event_file.write_text("".join(json.dumps(e)+"\n" for e in events), encoding="utf-8")
            with contextlib.redirect_stdout(io.StringIO()): after = analyze(analysis_args(session))
            a = json.loads((before/"report.json").read_text())["calibration_proposal"]
            b = json.loads((after/"report.json").read_text())["calibration_proposal"]
            self.assertEqual(a["candidate_constants"], b["candidate_constants"])
            self.assertNotEqual(a["validation"], b["validation"])


if __name__ == "__main__":
    unittest.main()

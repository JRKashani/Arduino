import unittest
import contextlib
import io
import json
import os
import tempfile
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import MagicMock, patch

from calibrate import POSES, capture_ratios, failed_poses, fit_constants, load_training, run, validate


class CalibrationTests(unittest.TestCase):
    def setUp(self):
        self.zeros = [0.51, 0.49, 0.52]
        self.scales = [0.10, 0.11, 0.09]
        self.poses = {(axis, sign): [self.zeros[i]+(sign*self.scales[i] if a == axis else 0)
                       for i, a in enumerate("xyz")] for axis, sign in POSES}

    def test_recovers_constants_and_validates_independent_round(self):
        result = fit_constants(self.poses)
        for i, axis in enumerate("xyz"):
            self.assertAlmostEqual(result[axis+"_zero"], self.zeros[i])
            self.assertAlmostEqual(result[axis+"_per_g"], self.scales[i])
        self.assertTrue(validate(self.poses, result)["passes"])

    def test_common_offset_jump_fails_validation(self):
        changed = {pose: [r+0.015 for r in ratios] for pose, ratios in self.poses.items()}
        self.assertFalse(validate(changed, fit_constants(self.poses))["passes"])

    def test_reversed_pose_rejected(self):
        self.poses[("x", 1)], self.poses[("x", -1)] = self.poses[("x", -1)], self.poses[("x", 1)]
        with self.assertRaises(ValueError):
            fit_constants(self.poses)

    def test_raw_instability_detected_without_using_old_calibration(self):
        samples = [{"valid": 1, "raw_ref": 700, "raw_x": 350+(15 if i >= 50 else 0),
                    "raw_y": 350, "raw_z": 420} for i in range(100)]
        with self.assertRaises(ValueError):
            capture_ratios(samples)
        for sample in samples:
            sample["raw_x"] = 350
        self.assertEqual(capture_ratios(samples), [0.5, 0.5, 0.6])

    def test_retry_replaces_flagged_poses_then_requires_full_fresh_validation(self):
        imported = {pose: list(ratios) for pose, ratios in self.poses.items()}
        imported[("y", 1)][2] -= 0.02
        imported[("z", 1)][1] += 0.015
        self.assertEqual(failed_poses(validate(imported, fit_constants(imported))), [("y", 1), ("z", 1)])
        calibration = fit_constants(self.poses)
        device = MagicMock(calibration=calibration)
        def replacement(device, log, args, round_index, axis, sign, counter):
            return self.poses[(axis, sign)]
        original = Path.cwd()
        with tempfile.TemporaryDirectory() as temporary:
            try:
                os.chdir(temporary)
                args = SimpleNamespace(port="FAKE", duration=10, settle=5, retry_session="source")
                with patch("calibrate.load_training", return_value=(imported, calibration)), \
                     patch("calibrate.Device", return_value=device), patch("serial.Serial"), \
                     patch("calibrate.enter_only"), patch("builtins.input", return_value="r"), \
                     patch("calibrate.collect_pose", side_effect=replacement) as collect, \
                     contextlib.redirect_stdout(io.StringIO()):
                    run(args)
                self.assertEqual(collect.call_count, 8)  # Two replacements + six validation poses.
                directory = next(Path("imu_tilt_sessions").iterdir())
                result = json.loads((directory/"calibration-result.json").read_text())
                self.assertEqual(result["status"], "candidate_passed")
                self.assertFalse(result["active_calibration_changed"])
                # Imported means preserve traceability in the retry session.
                loaded, old_calibration = load_training(directory)
                self.assertEqual(len(loaded), 6)
                self.assertEqual(old_calibration, calibration)
            finally:
                os.chdir(original)

    def test_validation_retry_keeps_candidate_and_passing_poses_with_history(self):
        from recording import EventLog, session_events
        calibration = fit_constants(self.poses)
        failed = {pose: list(ratios) for pose, ratios in self.poses.items()}
        failed[("x", 1)][1] += 0.02
        failed[("y", 1)][2] += 0.02
        original = Path.cwd()
        with tempfile.TemporaryDirectory() as temporary:
            try:
                os.chdir(temporary)
                source = Path("source")
                source.mkdir()
                (source/"session.json").write_text(json.dumps({"type": "six_face_calibration"}))
                log = EventLog(source)
                for round_number, poses in ((1, self.poses), (2, failed)):
                    for i, ((axis, sign), ratios) in enumerate(poses.items()):
                        tid = (round_number-1)*6+i+1
                        log.write("calibration_trial", trial_id=tid, round=round_number, axis=axis, sign=sign,
                                  calibration=calibration)
                        log.write("calibration_review", trial_id=tid, decision="accept", ratios=ratios)
                log.close()
                (source/"calibration-result.json").write_text(json.dumps({"candidate_constants": calibration,
                    "previous_constants": calibration, "training": validate(self.poses, calibration),
                    "validation": validate(failed, calibration), "validation_retests": 1}))
                before = {p.name: p.read_bytes() for p in source.iterdir()}
                device = MagicMock(calibration=calibration)
                def replacement(device, log, args, round_index, axis, sign, counter):
                    self.assertEqual(round_index, 1)
                    counter[0] += 1
                    log.write("calibration_trial", trial_id=counter[0], round=2, axis=axis, sign=sign,
                              calibration=calibration)
                    log.write("calibration_review", trial_id=counter[0], decision="accept", ratios=self.poses[(axis, sign)])
                    return self.poses[(axis, sign)]
                args = SimpleNamespace(port="FAKE", duration=10, settle=5, retry_session=None, retry_validation=str(source))
                with patch("calibrate.Device", return_value=device), patch("serial.Serial"), \
                     patch("calibrate.enter_only"), patch("builtins.input", return_value="r"), \
                     patch("calibrate.collect_pose", side_effect=replacement) as collect, \
                     patch("calibrate.fit_constants", side_effect=AssertionError("Must never refit validation")), \
                     contextlib.redirect_stdout(io.StringIO()):
                    run(args)
                self.assertEqual(collect.call_count, 2)
                self.assertEqual([(c.args[4], c.args[5]) for c in collect.call_args_list], [("x", 1), ("y", 1)])
                directory = next(Path("imu_tilt_sessions").iterdir())
                result = json.loads((directory/"calibration-result.json").read_text())
                self.assertEqual(result["status"], "candidate_passed")
                self.assertEqual(result["candidate_constants"], calibration)
                self.assertEqual(result["validation_retests"], 2)
                self.assertEqual(load_training(directory, 2)[0], self.poses)
                checks = [e for e in session_events(directory) if e["kind"] == "validation_check"]
                self.assertEqual([e["check"]["passes"] for e in checks], [False, True])
                self.assertEqual({p.name: p.read_bytes() for p in source.iterdir()}, before)
            finally:
                os.chdir(original)


if __name__ == "__main__":
    unittest.main()

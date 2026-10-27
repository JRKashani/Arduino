"""Interactive six-face calibration using the existing stationary tilt firmware."""
import argparse
import json
import math
from datetime import datetime, timezone
from pathlib import Path

from core import convert_ratios, stats
from recording import Device, EventLog, session_events

POSES = [(axis, sign) for axis in "xyz" for sign in (1, -1)]


def capture_ratios(samples):
    # Calibration quality must not depend on the old acceleration constants.
    if len(samples) < 50:
        raise ValueError("Too few samples; repeat this position")
    if any(not s["valid"] or any(s[k] in (0, 1023) for k in
           ("raw_x", "raw_y", "raw_z", "raw_ref")) for s in samples):
        raise ValueError("Invalid or saturated ADC reading; inspect wiring")
    results = [stats(s["raw_"+axis]/s["raw_ref"] for s in samples) for axis in "xyz"]
    if any(r["std"] > 0.003 or abs(r["drift"]) > 0.003 for r in results):
        raise ValueError("Raw ratios moved during capture; settle/check wiring and repeat")
    return [r["mean"] for r in results]


def fit_constants(poses):
    constants = {}
    for index, axis in enumerate("xyz"):
        positive, negative = poses[(axis, 1)][index], poses[(axis, -1)][index]
        scale = (positive-negative)/2
        if not math.isfinite(scale) or not 0.03 <= scale <= 0.2:
            raise ValueError(f"{axis.upper()} has an implausible/reversed response; check axis labels and poses")
        constants[axis+"_zero"] = (positive+negative)/2
        constants[axis+"_per_g"] = scale
    return constants


def validate(poses, constants):
    details = []
    for axis, sign in POSES:
        actual = convert_ratios(poses[(axis, sign)], constants)
        target = [sign if a == axis else 0 for a in "xyz"]
        error = math.sqrt(sum((a-b)**2 for a, b in zip(actual, target)))
        details.append({"pose": f"{axis}{sign:+d}", "g": list(actual), "error_g": error})
    return {"max_vector_error_g": max(d["error_g"] for d in details),
            "passes": all(d["error_g"] <= 0.1 for d in details), "poses": details}


def enter_only(prompt):
    while input(prompt).strip():
        print("Press Enter only. No phone readings are needed for this calibration.")


def failed_poses(check):
    return [(d["pose"][0], int(d["pose"][1:])) for d in check["poses"] if d["error_g"] > 0.1]


def print_check(check):
    print("\nPose     corrected X/Y/Z (g)             vector error")
    for detail in check["poses"]:
        values = " / ".join(f"{v:+.3f}" for v in detail["g"])
        flag = "PASS" if detail["error_g"] <= 0.1 else "REPEAT / INVESTIGATE"
        print(f"{detail['pose']:5s}    {values}     {detail['error_g']:.3f} g  {flag}")
    print("Off-axis readings can mean imperfect alignment, changing signals, or model error.")


def load_training(directory, round_number=1):
    directory = Path(directory)
    metadata = json.loads((directory/"session.json").read_text(encoding="utf-8"))
    if metadata.get("type") != "six_face_calibration":
        raise ValueError("--retry-session must name a six-face calibration session")
    events = session_events(directory)
    trials = {e["trial_id"]: e for e in events if e["kind"] == "calibration_trial"}
    poses, calibration = {}, None
    for event in events:
        if event["kind"] == "calibration_review" and event["decision"] == "accept":
            trial = trials[event["trial_id"]]
            if trial["round"] == round_number:
                if calibration is not None and calibration != trial["calibration"]:
                    raise ValueError("Source session mixes different active calibration constants")
                calibration = trial["calibration"]
                poses[(trial["axis"], trial["sign"])] = event["ratios"]
    if set(poses) != set(POSES):
        raise ValueError(f"Source session must contain all six accepted round {round_number} poses")
    return poses, calibration


def collect_pose(device, log, args, round_index, axis, sign, counter):
    while True:
        direction = "UP toward the ceiling" if sign == 1 else "DOWN toward the floor"
        enter_only(f"\nPoint the sensor's +{axis.upper()} axis {direction}. Press Enter when still: ")
        counter[0] += 1
        trial_id = counter[0]
        log.write("calibration_trial", trial_id=trial_id, round=round_index+1,
                  axis=axis, sign=sign, calibration=device.calibration)
        samples, end = device.capture(trial_id, int(args.settle*1000), int(args.duration*1000), 20000)
        try:
            if end["status"] != "DONE":
                raise ValueError("Capture aborted")
            ratios = capture_ratios(samples)
        except ValueError as exc:
            print(f"\n{exc}")
            log.write("calibration_review", trial_id=trial_id, decision="reject", reason=str(exc))
            continue
        print("Mean X/Y/Z supply ratios: "+", ".join(f"{v:.6f}" for v in ratios))
        choice = ""
        while choice not in ("a", "r"):
            choice = input("Accept [a], or reposition and repeat [r]: ").strip().lower()
        log.write("calibration_review", trial_id=trial_id,
                  decision="accept" if choice == "a" else "reject", ratios=ratios)
        if choice == "a":
            return ratios


def run(args):
    import serial
    retry_validation = getattr(args, "retry_validation", None)
    source = retry_validation or args.retry_session
    imported, imported_calibration = load_training(source) if source else ({}, None)
    imported_validation, frozen_candidate = {}, None
    prior_retests = 0
    if retry_validation:
        saved = json.loads((Path(source)/"calibration-result.json").read_text(encoding="utf-8"))
        if not saved["training"]["passes"]:
            raise ValueError("Source training did not pass; use --retry-session instead")
        frozen_candidate = saved["candidate_constants"]
        imported_validation, validation_calibration = load_training(source, 2)
        if validation_calibration != imported_calibration or saved["previous_constants"] != imported_calibration:
            raise ValueError("Source training and validation used different active constants")
        prior_retests = saved.get("validation_retests", 0)
    directory = Path("imu_tilt_sessions") / (datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S_%fZ")+"_six_face")
    directory.mkdir(parents=True, exist_ok=False)
    (directory/"session.json").write_text(json.dumps({"type": "six_face_calibration", "port": args.port,
        "duration_seconds": args.duration, "settle_seconds": args.settle, "retry_source": source,
        "retry_validation": bool(retry_validation),
        "quality_limits": {"ratio_std": 0.003, "ratio_drift": 0.003, "vector_error_g": 0.1}}, indent=2), encoding="utf-8")
    log = EventLog(directory)
    try:
        print(f"\nRaw data and results: {directory.resolve()}")
        print("Use the existing imu_tilt firmware. Close other serial monitors first.")
        print("Motor outputs stay OFF. No phone readings or flat zero are required.")
        print("Orient the SENSOR axes, using the module axis markings, not the robot's heading.")
        print("Support the entire robot securely; do not hold it by hand during captures.")
        print("Each named axis must be vertical. Other two axes must be horizontal.")
        print("If the axis markings are unclear, stop rather than guess.")
        print("There are two rounds of six positions: calibration, then independent validation.")
        print("Active constants will remain unchanged. Ctrl+C stops and preserves the logs.\n")
        enter_only("Press Enter to connect and begin: ")
        with serial.Serial(args.port, 115200, timeout=0.25, write_timeout=2) as port:
            device = Device(port, log)
            device.connect()
            if imported and imported_calibration != device.calibration:
                raise ValueError("Active constants differ from the source session; start a fresh calibration")
            if imported:
                print("Reusing accepted training captures from:", source)
                print("Previous source logs remain unchanged. Failed positions will be repeated.")
                for round_number, imported_poses in ((1, imported), (2, imported_validation)):
                    for (axis, sign), ratios in imported_poses.items():
                        # Link each copied mean to its original source without copying raw samples.
                        counter_id = (round_number-1)*6+POSES.index((axis, sign))+1
                        log.write("calibration_trial", trial_id=counter_id, round=round_number, axis=axis, sign=sign,
                                  calibration=device.calibration, source_session=source)
                        log.write("calibration_review", trial_id=counter_id, decision="accept", ratios=ratios,
                                  source_session=source)
            if retry_validation:
                print("Validation retry: candidate constants are fixed; passing validation positions are retained.")
            print("Connected. Leave the robot powered for 60 seconds before the first capture.")
            rounds = []
            counter = [12 if imported_validation else 6 if imported else 0]
            validation_retests = prior_retests
            for round_index in range(2):
                print(f"\nROUND {round_index+1}: {'CALIBRATION' if round_index == 0 else 'VALIDATION - reposition for every capture'}")
                poses = dict(imported if round_index == 0 else imported_validation)
                pending = [] if poses else POSES
                while True:
                    for axis, sign in pending:
                        poses[(axis, sign)] = collect_pose(device, log, args, round_index, axis, sign, counter)
                    if round_index != 0:
                        validation = validate(poses, candidate)
                        log.write("validation_check", candidate_constants=candidate, check=validation,
                                  validation_retests=validation_retests)
                        result = {"candidate_constants": candidate, "training": training, "validation": validation,
                                  "previous_constants": device.calibration, "active_calibration_changed": False,
                                  "validation_retests": validation_retests,
                                  "status": "candidate_passed" if validation["passes"] else "validation_failed"}
                        (directory/"calibration-result.json").write_text(json.dumps(result, indent=2, allow_nan=False), encoding="utf-8")
                        print_check(validation)
                        if validation["passes"]:
                            break
                        print("Validation failed. Candidate constants remain fixed; all attempts are preserved.")
                        choice = ""
                        while choice not in ("r", "f", "q"):
                            choice = input("Repeat failed validation positions [r], fresh six validation positions [f], or quit [q]: ").strip().lower()
                        if choice == "q":
                            print(f"Failed validation saved in {directory.resolve()}")
                            return
                        pending = failed_poses(validation) if choice == "r" else POSES
                        validation_retests += 1
                        continue
                    candidate = frozen_candidate if frozen_candidate is not None else fit_constants(poses)
                    training = validate(poses, candidate)
                    log.write("training_check", candidate_constants=candidate, check=training)
                    (directory/"training-diagnostics.json").write_text(json.dumps(
                        {"candidate_constants": candidate, "check": training}, indent=2), encoding="utf-8")
                    print_check(training)
                    if training["passes"]:
                        break
                    if frozen_candidate is not None:
                        raise ValueError("Source training no longer matches its saved candidate; start a fresh calibration")
                    print("Training check failed; no constants were applied. Recheck support and sensor-axis alignment.")
                    choice = ""
                    while choice not in ("r", "f", "q"):
                        choice = input("Repeat flagged positions [r], fresh six positions [f], or quit [q]: ").strip().lower()
                    if choice == "q":
                        return
                    pending = failed_poses(training) if choice == "r" else POSES
                rounds.append(poses)
            validation = validate(rounds[1], candidate)
            print_check(validation)
            result = {"candidate_constants": candidate, "training": training, "validation": validation,
                      "previous_constants": device.calibration, "active_calibration_changed": False,
                      "validation_retests": validation_retests,
                      "status": "candidate_passed" if validation["passes"] else "validation_failed"}
            (directory/"calibration-result.json").write_text(json.dumps(result, indent=2, allow_nan=False), encoding="utf-8")
            print(f"\nValidation maximum vector error: {validation['max_vector_error_g']:.4f} g")
            print("Candidate PASSED the provisional 0.1 g check." if validation["passes"] else
                  "Validation FAILED. Do not apply these constants; inspect repeatability and positioning.")
            if validation["passes"]:
                text = "\n".join(f"const float {axis.upper()}_{name} = {candidate[axis+'_'+key]:.7f}f;"
                    for axis in "xyz" for name, key in (("ZERO_RATIO", "zero"), ("RATIO_PER_G", "per_g")))+"\n"
                (directory/"candidate-constants.txt").write_text(text, encoding="utf-8")
                print(text)
            print(f"Results saved in {directory.resolve()}\nActive robot constants were not changed.")
    finally:
        log.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True)
    parser.add_argument("--duration", type=int, choices=range(2, 61), default=10)
    parser.add_argument("--settle", type=int, choices=range(0, 61), default=5)
    retry = parser.add_mutually_exclusive_group()
    retry.add_argument("--retry-session", help="Reuse six accepted training poses; repeat failures and run fresh validation")
    retry.add_argument("--retry-validation", help="Keep the saved candidate and validation poses; repeat validation failures only")
    args = parser.parse_args()
    try:
        run(args)
    except KeyboardInterrupt:
        print("\nStopped. Raw records are saved; no calibration was applied.")
        return 130
    except (OSError, ValueError, RuntimeError) as exc:
        print(f"\nCalibration stopped: {exc}\nRaw records are saved; no calibration was applied.")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

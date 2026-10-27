"""Guided serial acquisition. All source records are appended and flushed."""
import json
import math
import time
import uuid
from datetime import datetime, timezone
from pathlib import Path

from core import CAL_KEYS, compare_stationary, mean_capture_summary, parse_sample, summarize


def utc_now():
    return datetime.now(timezone.utc).isoformat()


def load_events(path):
    lines = Path(path).read_text(encoding="utf-8").splitlines()
    events = []
    for i, line in enumerate(lines):
        try:
            events.append(json.loads(line))
        except json.JSONDecodeError:
            if i != len(lines)-1:
                raise ValueError(f"Corrupt event log at line {i+1}")
            print("Ignoring an incomplete final event from an interrupted write.")
    return events


class EventLog:
    def __init__(self, directory):
        self.path = Path(directory)
        self.boot_id = uuid.uuid4().hex
        # A partial last line must remain evidence; start the resumed stream in
        # a new file rather than appending onto a torn JSON object.
        self.event_path = self.path / f"events-{self.boot_id}.jsonl"
        self.events = self.event_path.open("x", encoding="utf-8")
        self.raw = (self.path / f"serial-{self.boot_id}.log").open("x", encoding="utf-8")

    def write(self, kind, **values):
        self.events.write(json.dumps({"kind": kind, "utc": utc_now(),
                                     "boot_id": self.boot_id, **values}, allow_nan=False)+"\n")
        self.events.flush()

    def wire(self, direction, text):
        self.raw.write(f"{utc_now()} {direction} {text}\n")
        self.raw.flush()

    def close(self):
        self.events.close()
        self.raw.close()


def session_events(directory):
    events = []
    for file in sorted(Path(directory).glob("events-*.jsonl")):
        events.extend(load_events(file))
    return sorted(events, key=lambda e: e["utc"])


def planned_surfaces(metadata, events):
    """Replay appended extensions without rewriting original session metadata."""
    surfaces = list(metadata["surfaces"])
    for event in events:
        if event["kind"] == "plan_extended":
            extended = event["surfaces"]
            if len(extended) < len(surfaces) or extended[:len(surfaces)] != surfaces:
                raise ValueError("Invalid surface-plan extension in session records")
            surfaces = list(extended)
    return surfaces


class Device:
    def __init__(self, port, log):
        self.port = port
        self.log = log
        self.buffer = bytearray()
        self.calibration = None

    def send(self, command):
        self.log.wire(">", command)
        self.port.write((command+"\n").encode("ascii"))
        self.port.flush()

    def read(self):
        # Serial.read_until may return a partial record on timeout; retain it.
        data = self.port.read_until(b"\n")
        if not data:
            return None
        self.buffer.extend(data)
        if len(self.buffer) > 512:
            raise RuntimeError("Oversized serial record; wrong firmware or corrupted connection")
        if not self.buffer.endswith(b"\n"):
            return None
        line = self.buffer.decode("ascii", errors="strict").strip()
        self.buffer.clear()
        self.log.wire("<", line)
        return line

    def connect(self):
        # Opening Mega USB usually resets it. Never zero the IMU on reconnect.
        time.sleep(2)
        self.port.reset_input_buffer()
        self.send("ABORT")
        self.send("INFO")
        deadline = time.monotonic()+8
        version = None
        while time.monotonic() < deadline:
            line = self.read()
            if not line:
                continue
            if line.startswith("IMUTILT,"):
                parts = line.split(",")
                if len(parts) != 3 or parts[1] != "1":
                    raise RuntimeError("Unsupported tilt firmware protocol")
                version = parts[2]
            elif line.startswith("CONFIG,"):
                values = [float(x) for x in line.split(",")[1:]]
                if len(values) != 6 or not all(math.isfinite(x) for x in values):
                    raise RuntimeError("Invalid firmware calibration record")
                self.calibration = dict(zip(CAL_KEYS, values))
            elif line == "READY" and version and self.calibration:
                self.log.write("boot", firmware=version, protocol=1,
                               calibration=self.calibration, baseline="none")
                return
            elif line.startswith("ERROR,"):
                raise RuntimeError(line)
        raise RuntimeError("No tilt firmware handshake. Upload environment imu_tilt first.")

    def capture(self, trial_id, settle_ms, duration_ms, period_us):
        self.send(f"START,{trial_id},{settle_ms},{duration_ms},{period_us}")
        deadline = time.monotonic()+(settle_ms+duration_ms)/1000+5
        samples = []
        captured = False
        aborting = False
        last_progress = 0
        while time.monotonic() < deadline:
            try:
                line = self.read()
                if not line:
                    continue
                if line.startswith("DATA,"):
                    sample = parse_sample(line)
                    if sample["trial_id"] != trial_id or not captured:
                        raise RuntimeError("Sample is not linked to the active trial")
                    if samples and (sample["sequence"] <= samples[-1]["sequence"] or
                                    sample["elapsed_us"] <= samples[-1]["elapsed_us"]):
                        raise RuntimeError("Nonmonotonic sample sequence/timestamp")
                    if sample["elapsed_us"] >= duration_ms*1000:
                        raise RuntimeError("Sample timestamp exceeds the requested capture window")
                    self.log.write("sample", **sample)
                    samples.append(sample)
                    if time.monotonic()-last_progress >= 1:
                        print(f"  Recording {sample['elapsed_us']/1e6:.1f}s, {len(samples)} samples", end="\r", flush=True)
                        last_progress = time.monotonic()
                elif line == f"CAPTURE,{trial_id}":
                    if captured:
                        raise RuntimeError("Duplicate capture start")
                    captured = True
                elif line.startswith("END,"):
                    parts = line.split(",")
                    if len(parts) != 6 or int(parts[1]) != trial_id or parts[2] not in ("DONE", "ABORT"):
                        raise RuntimeError("Malformed or mismatched trial completion")
                    end = {"status": parts[2], "firmware_samples": int(parts[3]),
                           "firmware_missed": int(parts[4]), "duration_us": int(parts[5])}
                    if end["firmware_samples"] != len(samples):
                        raise RuntimeError("Serial sample count differs from firmware count")
                    slots = (duration_ms*1000+period_us-1)//period_us
                    if end["status"] == "DONE" and (not captured or
                            end["firmware_samples"]+end["firmware_missed"] != slots or
                            end["duration_us"] < duration_ms*1000):
                        raise RuntimeError("Incomplete firmware capture window or schedule")
                    self.log.write("end", trial_id=trial_id, **end)
                    print(" "*70, end="\r")
                    return samples, end
                elif line.startswith("ERROR,"):
                    raise RuntimeError(line)
                elif line.startswith("IMUTILT,") or line == "READY":
                    raise RuntimeError("Board reset during trial; resume in a new boot segment")
                elif line != f"SETTLE,{trial_id}":
                    raise RuntimeError(f"Unexpected serial record: {line}")
            except KeyboardInterrupt:
                if aborting:
                    raise
                print("\nAborting capture; partial data will be retained.")
                self.send("ABORT")
                aborting = True
                deadline = time.monotonic()+3
        raise RuntimeError("Capture timeout/disconnection; partial samples retained")


def ask_pair(prompt):
    while True:
        try:
            values = input(prompt).replace(",", " ").split()
            up, right = map(float, values)
            if not all(math.isfinite(x) and abs(x) < 89 for x in (up, right)):
                raise ValueError()
            return up, right
        except ValueError:
            print("Enter TWO signed degree values, e.g. 12.4 -0.3 (each between -89 and 89).")


def phone_block(log, surface, block, phase):
    print(f"\n{surface}: phone readings {phase}. Top edge at 0 means toward board U.")
    print("Keep the same phone zero, display orientation, support and board marks.")
    print("Lock screen orientation; rotate the phone physically and use the same two labeled readouts.")
    print("After a 90-degree turn, the main slope should transfer approximately between the two components.")
    while True:
        readings = []
        attempt = uuid.uuid4().hex
        for heading in (0, 90, 180, 270):
            up, right = ask_pair(f"  Phone {heading:3} deg clockwise: up right = ")
            reading = {"heading": heading, "up": up, "right": right}
            readings.append(reading)
            # Preserve each reading immediately, even if interrupted before all four.
            log.write("phone_reading", surface=surface, reference_id=block, phase=phase,
                      phone_attempt_id=attempt, **reading)
        print("  Entered phone readings: "+str([(r["heading"], r["up"], r["right"]) for r in readings]))
        if input("Use these four readings? [Y/n]: ").strip().lower() != "n":
            log.write("reference", surface=surface, reference_id=block, phase=phase,
                      phone_attempt_id=attempt, readings=readings)
            return
        log.write("phone_reference_rejected", reference_id=block, phase=phase, phone_attempt_id=attempt)


def record(args):
    try:
        import serial
    except ImportError as exc:
        raise RuntimeError("Install tools/imu_tilt/requirements.txt before recording") from exc
    settings = {"settle_ms": round(args.settle*1000), "duration_ms": round(args.duration*1000),
                "period_us": round(1e6/args.hz)}
    if args.resume:
        directory = Path(args.resume)
        metadata = json.loads((directory/"session.json").read_text(encoding="utf-8"))
        if metadata.get("schema") != 1 or metadata.get("simulated"):
            raise ValueError("Cannot resume this session schema or a simulated session")
        diagonal = metadata["diagonal"]
        previous = session_events(directory)
        surfaces = planned_surfaces(metadata, previous)
        target = getattr(args, "extend_surfaces", None)
        if target is not None:
            if target < len(surfaces):
                raise ValueError("--extend-surfaces cannot reduce the saved surface count")
            surfaces = [f"S{i+1}" for i in range(target)]
    else:
        directory = Path(args.output) if args.output else Path("imu_tilt_sessions")/datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S_%fZ")
        directory.mkdir(parents=True, exist_ok=False)
        surfaces = [f"S{i+1}" for i in range(args.surfaces)]
        diagonal = args.diagonal
        metadata = {"schema": 1, "session_id": uuid.uuid4().hex, "created_utc": utc_now(),
                    "simulated": False, "surfaces": surfaces, "diagonal": diagonal,
                    "phone_model": "Samsung S21 FE",
                    "phone_app": input("Bubble Level publisher/version (or unknown): ").strip(),
                    "phone_display_resolution": input("Smallest displayed degree increment (e.g. 0.1, or unknown): ").strip(),
                    "phone_support": input("Phone case/support and app zero setting: ").strip(),
                    "mount_notes": input("IMU mount, chassis and tether notes: ").strip(),
                    "baseline": "none; level checks preserved as measurements"}
        calibration_info = getattr(args, "calibration_info", None)
        if calibration_info:
            metadata["calibration_info"] = json.loads(Path(calibration_info).read_text(encoding="utf-8"))
            check_calibration_info(metadata["calibration_info"], metadata["calibration_info"].get("candidate_constants", {}))
        with (directory/"session.json").open("x", encoding="utf-8") as file:
            json.dump(metadata, file, indent=2)
        previous = []
    prior_reviews = {e["trial_id"]: e for e in previous if e["kind"] == "review"}
    accepted = {tid for tid, review in prior_reviews.items() if review["decision"] == "accept"}
    completed = {(e["surface"], e["loop"], e["heading"]) for e in previous
                 if e["kind"] == "trial" and e["trial_id"] in accepted}
    counter = max((e.get("trial_id", 0) for e in previous), default=0)
    old_trials = {e["trial_id"]: e for e in previous if e["kind"] == "trial"}
    old_reviews = {e["trial_id"]: e for e in previous if e["kind"] == "review"}
    flat_reference = None
    for tid in sorted(old_trials):
        t, review = old_trials[tid], old_reviews.get(tid, {})
        if t.get("trial_kind") == "level" and review.get("decision") == "accept" and review.get("summary", {}).get("valid"):
            flat_reference = {"trial_id": tid, "calibration": t["calibration"],
                              "summary": review.get("placement_summary", review["summary"])}
    log = EventLog(directory)
    print(f"\nSaving raw records to: {directory.resolve()}")
    print("All captures are stationary. Ctrl+C during recording aborts that attempt.")
    port = None
    device = None
    try:
        if args.resume and surfaces != planned_surfaces(metadata, previous):
            log.write("plan_extended", surfaces=surfaces)
        print(f"Surface plan: {', '.join(surfaces)} ({len(surfaces)} total).")
        print(f"Each capture: {settings['settle_ms']/1000:g}s settling, {settings['duration_ms']/1000:g}s recording, {1e6/settings['period_us']:g} Hz.")
        print("Angles are absolute with the existing calibration; no automatic flat zero is applied.")
        port = serial.Serial(args.port, 115200, timeout=0.2, write_timeout=2)
        device = Device(port, log)
        device.connect()
        if metadata.get("calibration_info"):
            check_calibration_info(metadata["calibration_info"], device.calibration)
            print("Calibration status:", metadata["calibration_info"]["status"])
        log.write("run", settings=settings, resumed=bool(args.resume))

        def trial(surface, heading, loop, reference_id, kind="slope"):
            nonlocal counter, flat_reference
            placement_id = uuid.uuid4().hex
            retained = []
            retained_summaries = []
            while True:
                if kind == "level":
                    print(f"\n{surface}: place robot on the SAME flat reference, in the SAME direction.")
                else:
                    print(f"\nPlace robot on {surface}, nose {heading} deg clockwise from U, loop {loop}.")
                while input("Press Enter when still (no phone numbers here): ").strip():
                    print("This prompt starts the robot capture. Phone readings use separate 'up right' prompts.")
                counter += 1
                log.write("trial", trial_id=counter, surface=surface, heading=heading,
                          loop=loop, reference_id=reference_id, trial_kind=kind,
                          settings=settings, calibration=device.calibration, placement_id=placement_id)
                try:
                    samples, end = device.capture(counter, **settings)
                except BaseException as exc:
                    log.write("review", trial_id=counter, decision="reject", reason=f"interrupted: {exc}")
                    raise
                summary = summarize(samples, settings["period_us"])
                for previous_summary in retained_summaries:
                    comparison = compare_stationary(previous_summary, summary)
                    if comparison and comparison["changed"]:
                        summary["warnings"].append("KEPT CAPTURES DISAGREE: their means changed by >3 deg or >0.1 g; combined data will be flagged")
                        break
                if kind == "level" and flat_reference and flat_reference["calibration"] == device.calibration:
                    comparison = compare_stationary(flat_reference["summary"], summary)
                    if comparison:
                        log.write("flat_comparison", trial_id=counter, previous_trial_id=flat_reference["trial_id"], **comparison)
                        print(f"  Versus flat check {flat_reference['trial_id']}: roll {comparison['roll_delta_deg']:+.0f} deg, pitch {comparison['pitch_delta_deg']:+.0f} deg, magnitude {comparison['magnitude_delta_g']:+.3f} g")
                        if comparison["changed"]:
                            summary["warnings"].append("FLAT CHECK CHANGED: inspect IMU, connections and reference setup; repeating/zeroing does not resolve a changing signal")
                print(f"  {summary['valid']}/{summary['samples']} valid; rate={summary['actual_hz']} Hz")
                for axis in ("robot_roll", "robot_pitch"):
                    if summary[axis]["mean"] is not None:
                        deviation = summary[axis]['std']
                        deviation_text = "<1" if deviation < 1 else f"{deviation:.0f}"
                        print(f"  {axis}: mean {summary[axis]['mean']:.0f}, std {deviation_text} deg")
                for warning in summary["warnings"]:
                    print("  CHECK: "+warning)
                while True:
                    choice = input("Accept [a], repeat [r], or skip this placement [s]? ").strip().lower()
                    if choice in ("a", "r", "s"):
                        if choice == "a" and (end["status"] != "DONE" or not summary["valid"]):
                            print("An aborted/empty capture cannot be accepted. Repeat or skip.")
                            continue
                        break
                mode = None
                usable = end["status"] == "DONE" and bool(summary["valid"])
                if choice == "r" and (usable or retained):
                    while True:
                        mode = input("Keep previous capture(s) together with the new one [k], or discard previous capture(s) [d]? ").strip().lower()
                        if mode in ("k", "d"):
                            break
                    if mode == "k" and not usable:
                        print("The aborted/empty current capture is excluded; earlier kept captures remain.")
                if choice == "s" or (choice == "r" and mode == "d"):
                    for tid in retained:
                        log.write("review", trial_id=tid, decision="reject", reason="discarded repeat group", placement_id=placement_id)
                    retained.clear()
                    retained_summaries.clear()
                decision = "accept" if choice == "a" else "retain" if mode == "k" and usable else "reject"
                reason = input("Reason/note: ").strip() if choice != "a" or summary["warnings"] else ""
                log.write("review", trial_id=counter, decision=decision, repeat_action=mode,
                          reason=reason or ("repeat" if choice == "r" else "" if choice == "a" else "skip"),
                          placement_id=placement_id, summary=summary,
                          placement_summary=mean_capture_summary([*retained_summaries, summary]) if choice == "a" else None)
                if decision == "retain":
                    retained.append(counter)
                    retained_summaries.append(summary)
                    print(f"  Keeping {len(retained)} previous capture(s); the next capture stays in this placement group.")
                if choice == "a" and kind == "level":
                    flat_reference = {"trial_id": counter, "calibration": device.calibration,
                                      "summary": mean_capture_summary([*retained_summaries, summary])}
                if choice != "r":
                    return

        trial("LEVEL_START", 0, 0, None, "level")
        for surface in surfaces:
            placements = [(loop, heading) for loop in (1, 2) for heading in (0, 90, 180, 270)]
            if diagonal:
                placements.append((3, 45))
            pending = [(loop, heading) for loop, heading in placements
                       if (surface, loop, heading) not in completed]
            if not pending:
                continue
            input(f"\nSet up rigid surface {surface}; keep its support fixed. Enter when ready: ")
            block = uuid.uuid4().hex
            log.write("block", surface=surface, reference_id=block,
                      notes=input("Surface/support notes: ").strip())
            phone_block(log, surface, block, "before")
            for loop, heading in pending:
                trial(surface, heading, loop, block)
            if input("Repeat four phone readings to check board drift? [Y/n]: ").strip().lower() != "n":
                phone_block(log, surface, block, "after")
            if surface != surfaces[-1] and input("Take an extra flat check before the next slope? [y/N]: ").strip().lower() == "y":
                trial("LEVEL_BETWEEN_"+surface, 0, 0, None, "level")
        trial("LEVEL_END", 0, 0, None, "level")
        log.write("run_complete")
        print(f"\nSession saved. Analyze with: python tools/imu_tilt/tilt_mission.py analyze \"{directory}\"")
    finally:
        if device:
            try:
                device.send("ABORT")
            except Exception:
                pass
        if port:
            port.close()
        log.close()


def check_calibration_info(info, actual):
    """Do not label a session with provenance for different board constants."""
    candidate = info.get("candidate_constants", {})
    if not isinstance(info.get("status"), str) or not info["status"]:
        raise ValueError("Calibration provenance requires a status")
    for key in CAL_KEYS:
        value = candidate.get(key)
        if (not isinstance(value, (int, float)) or not math.isfinite(value)
                or (key.endswith("_per_g") and value <= 0)):
            raise ValueError("Calibration provenance has invalid constants")
        if key not in actual or not math.isclose(value, actual[key], rel_tol=0, abs_tol=1e-6):
            raise ValueError("Board calibration does not match the supplied provenance; upload the selected firmware")

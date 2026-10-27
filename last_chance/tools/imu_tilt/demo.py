"""Synthetic pipeline exercise, clearly isolated from real experiment sessions."""
import json
import math
import random
from pathlib import Path

from core import CAL_KEYS, board_to_heading, convert_ratios, phone_vector, vector_angles
from recording import EventLog, utc_now


def generate(directory):
    directory = Path(directory)
    directory.mkdir(parents=True, exist_ok=False)
    metadata = {"schema": 1, "session_id": "SIMULATED", "simulated": True,
                "created_utc": utc_now(), "surfaces": [f"S{i}" for i in range(1, 6)],
                "diagonal": False, "phone_convention": "axis-elevation",
                "note": "Generated pipeline check; no measured robot/phone data"}
    (directory/"session.json").write_text(json.dumps(metadata, indent=2), encoding="utf-8")
    current = dict(zip(CAL_KEYS, (0.5143, 0.5070, 0.5068, 0.1050, 0.1020, 0.0988)))
    true = dict(zip(CAL_KEYS, (0.5123, 0.5090, 0.5068, 0.1080, 0.1000, 0.0988)))
    settings = {"settle_ms": 0, "duration_ms": 2000, "period_us": 20000}
    rng = random.Random(431)
    log = EventLog(directory)
    trial_id = 0
    log.write("boot", firmware="SIMULATED", calibration=current, protocol=1, baseline="none")
    try:
        def trial(surface, reference, heading, loop, expected, kind="slope", decision="accept"):
            nonlocal trial_id
            trial_id += 1
            log.write("trial", trial_id=trial_id, surface=surface, reference_id=reference,
                      heading=heading, loop=loop, trial_kind=kind, settings=settings, calibration=current)
            for index in range(100):
                ref = 675
                raw = [round(ref*(true[CAL_KEYS[i]]+true[CAL_KEYS[i+3]]*expected[i])+rng.gauss(0, 0.12))
                       for i in range(3)]
                acceleration = convert_ratios([v/ref for v in raw], current)
                angles = vector_angles(acceleration)
                log.write("sample", trial_id=trial_id, sequence=index, elapsed_us=index*20000,
                          valid=1, raw_x=raw[0], raw_y=raw[1], raw_z=raw[2], raw_ref=ref,
                          ax=acceleration[0], ay=acceleration[1], az=acceleration[2],
                          magnitude=math.sqrt(sum(x*x for x in acceleration)),
                          raw_roll=angles["roll"], raw_pitch=-angles["pitch"],
                          robot_roll=angles["roll"], robot_pitch=angles["pitch"])
            log.write("end", trial_id=trial_id, status="DONE", firmware_samples=100, firmware_missed=0, duration_us=2000000)
            log.write("review", trial_id=trial_id, decision=decision, reason="SIMULATED")
        trial("LEVEL_START", None, 0, 0, (0, 0, 1), "level")
        for i, degrees in enumerate((4, 9, 16, 23, 30), 1):
            surface, reference = f"S{i}", f"SIMULATED-R{i}"
            board = phone_vector(degrees, 0.7, "axis-elevation")
            readings = []
            for heading in (0, 90, 180, 270):
                v = board_to_heading(board, heading)
                reading = {"heading": heading,
                           "up": math.degrees(math.asin(v[0])),
                           "right": math.degrees(math.asin(v[1]))}
                readings.append(reading)
                log.write("phone_reading", surface=surface, reference_id=reference, phase="before", **reading)
            log.write("reference", surface=surface, reference_id=reference, phase="before", readings=readings)
            if i == 1:
                trial(surface, reference, 0, 1, board, decision="reject")
            for loop in (1, 2):
                for heading in (0, 90, 180, 270):
                    trial(surface, reference, heading, loop, board_to_heading(board, heading))
            log.write("reference", surface=surface, reference_id=reference, phase="after", readings=readings)
        trial("LEVEL_END", None, 0, 0, (0, 0, 1), "level")
        log.write("run_complete")
    finally:
        log.close()
    print(f"SIMULATED session written to {directory.resolve()}")
    print(f'Analyze: python tools/imu_tilt/tilt_mission.py analyze "{directory}" --phone-convention axis-elevation --axes-verified')

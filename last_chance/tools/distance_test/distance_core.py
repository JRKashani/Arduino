"""Distance experiment data model. All distances refer to the sensor face."""
import json
import math
import statistics
from pathlib import Path

SENSORS = {
    "us": {"code": "U", "name": "Front HC-SR04", "raw_unit": "us", "range_cm": [2, 400]},
    "laser": {"code": "L", "name": "Right VL53L0X", "raw_unit": "mm", "range_cm": [0, 200]},
    "ir": {"code": "I", "name": "Sharp GP2Y0A21YK0F on A6", "raw_unit": "ADC counts", "range_cm": [10, 80]},
}
DEFAULT_POINTS = [200, 175, 150, 125, 100, 80, 60, 50, 40, 30, 25, 20, 15, 10]
IR_POINTS = [200, 175, 150, 125, 100, 80, 70, 60, 50, 40, 35, 30, 25, 20, 18, 16, 14, 12, 10]


def finite_number(text, low, high):
    number = float(text)
    if not math.isfinite(number) or not low <= number <= high:
        raise ValueError(f"Enter a number between {low:g} and {high:g}.")
    return number


def parse_sample(line, capture_id, expected_sequence, sensor):
    fields = line.split(",")
    if len(fields) != 8 or fields[0] != "DATA":
        raise ValueError("Malformed DATA record")
    cid, sequence, timestamp, raw, valid = map(int, fields[1:6])
    detail = int(fields[7])
    if cid != capture_id or sequence != expected_sequence:
        raise ValueError("Sample ID/sequence mismatch; capture stopped")
    if valid not in (0, 1) or not 0 <= timestamp <= 0xffffffff:
        raise ValueError("Invalid timestamp/validity")
    maximum = {"us": 30000, "laser": 65535, "ir": 1023}[sensor]
    if not 0 <= raw <= maximum or not -1 <= detail <= 15:
        raise ValueError("Invalid raw reading/status")
    if fields[6] not in {"OK", "NO_ECHO", "OUT_OF_RANGE", "ADC_RAIL", "TIMEOUT", "I2C_ERROR", "RANGE_STATUS"}:
        raise ValueError("Unknown sensor status")
    if sensor == "us" and valid and not 116 <= raw <= 23200:
        raise ValueError("Invalid ultrasonic echo marked valid")
    if sensor == "laser" and valid and (detail != 11 or not 0 < raw < 8190):
        raise ValueError("Invalid laser range marked valid")
    return {"capture_id": cid, "sequence": sequence, "timestamp_ms": timestamp,
            "raw": raw, "valid": bool(valid), "status": fields[6], "device_status": detail}


def convert(sample, meta):
    result = {"raw_cm": None, "calibrated_cm": None, "voltage": None}
    if not sample["valid"]:
        return result
    raw, sensor = sample["raw"], meta["sensor"]
    if sensor == "us":
        result["raw_cm"] = raw / 58.0
        cfg = meta["firmware_config"]
        result["calibrated_cm"] = raw / 58.0 * float(cfg["US_SCALE"]) + float(cfg["US_OFFSET_CM"])
    elif sensor == "laser":
        result["raw_cm"] = result["calibrated_cm"] = raw / 10.0
    else:
        result["voltage"] = raw * meta["adc_reference_v"] / 1023.0
    return result


def stats(values):
    values = [v for v in values if v is not None]
    if not values:
        return {"min": None, "mean": None, "max": None, "std": None}
    return {"min": min(values), "mean": statistics.mean(values), "max": max(values),
            "std": statistics.stdev(values) if len(values) > 1 else 0.0}


def load_events(directory):
    events = []
    for path in sorted(Path(directory).glob("events-*.jsonl")):
        lines = path.read_text(encoding="utf-8").splitlines()
        for index, line in enumerate(lines):
            try:
                events.append(json.loads(line))
            except json.JSONDecodeError:
                if index != len(lines) - 1:
                    raise ValueError(f"Damaged event log: {path}:{index+1}")
                # Each connection creates a new file; a torn final write is preserved.
    return events


def build_tables(events, meta):
    points, latest, captures, reviews, complete, samples = {}, {}, {}, {}, set(), []
    done, skipped = set(), set()
    for event in events:
        kind = event["kind"]
        if kind == "point":
            points[event["point_id"]] = event
            latest[event["point_index"]] = event["point_id"]
        elif kind == "point_done":
            done.add(event["point_id"])
        elif kind == "point_skipped":
            skipped.add(event["point_index"])
            latest.pop(event["point_index"], None)
        elif kind == "capture":
            captures[event["capture_id"]] = event
        elif kind == "review":
            reviews[event["capture_id"]] = event["decision"]
        elif kind == "capture_complete":
            complete.add(event["capture_id"])
        elif kind == "sample":
            samples.append(event)
    sample_rows, capture_rows = [], []
    for cid, capture in captures.items():
        point = points[capture["point_id"]]
        current = latest.get(point["point_index"]) == point["point_id"]
        decision = reviews.get(cid, "pending")
        included = current and decision == "accepted" and cid in complete
        rows = []
        for sample in samples:
            if sample["capture_id"] != cid:
                continue
            row = {"point_index": point["point_index"], "target_cm": point["target_cm"],
                   "actual_cm": point["actual_cm"], "repeat": capture["repeat"],
                   "capture_id": cid, "included": included, "decision": decision,
                   "superseded_point": not current,
                   **{k: sample[k] for k in ("sequence", "timestamp_ms", "raw", "valid", "status", "device_status")},
                   **convert(sample, meta)}
            rows.append(row)
            sample_rows.append(row)
        row = {"point_index": point["point_index"], "point_id": point["point_id"],
               "target_cm": point["target_cm"], "actual_cm": point["actual_cm"],
               "repeat": capture["repeat"], "capture_id": cid, "decision": decision,
               "included": included, "complete": cid in complete, "samples": len(rows),
               "valid_samples": sum(r["valid"] for r in rows),
               "requested_samples": capture.get("samples_per_capture", meta.get("samples_per_capture")),
               "period_ms": capture.get("period_ms", meta.get("period_ms")),
               "settle_ms": capture.get("settle_ms", meta.get("settle_ms"))}
        for quantity in ("raw", "raw_cm", "calibrated_cm", "voltage"):
            for name, value in stats([r[quantity] for r in rows if r["valid"]]).items():
                row[f"{quantity}_{name}"] = value
        capture_rows.append(row)
    summaries = []
    low, high = SENSORS[meta["sensor"]]["range_cm"]
    for index, pid in latest.items():
        point = points[pid]
        selected = [c for c in capture_rows if c["point_id"] == pid and c["included"]]
        row = {"point_index": index, "target_cm": point["target_cm"], "actual_cm": point["actual_cm"],
               "completed": pid in done, "accepted_captures": len(selected),
               "usable_captures": sum(c["valid_samples"] > 0 for c in selected),
               "valid_samples": sum(c["valid_samples"] for c in selected),
               "total_samples": sum(c["samples"] for c in selected),
               "within_nominal_range": low <= point["actual_cm"] <= high,
               "within_us_calibration_range": (15 <= point["actual_cm"] <= 150) if meta["sensor"] == "us" else None}
        row["three_usable_repeats"] = row["usable_captures"] >= 3
        for quantity in ("raw", "raw_cm", "calibrated_cm", "voltage"):
            for name, value in stats([c[f"{quantity}_mean"] for c in selected]).items():
                row[f"{quantity}_{name}"] = value
        row["error_cm"] = (row["calibrated_cm_mean"] - row["actual_cm"]
                           if row["calibrated_cm_mean"] is not None else None)
        summaries.append(row)
    return sample_rows, capture_rows, sorted(summaries, key=lambda r: r["actual_cm"])

"""Portable geometry, protocol validation and statistics for the tilt mission."""
import math
import statistics

SAMPLE_FIELDS = (
    "trial_id", "sequence", "elapsed_us", "valid", "raw_x", "raw_y", "raw_z",
    "raw_ref", "ax", "ay", "az", "magnitude", "raw_roll", "raw_pitch",
    "robot_roll", "robot_pitch",
)
INT_FIELDS = set(SAMPLE_FIELDS[:8])
CAL_KEYS = ("x_zero", "y_zero", "z_zero", "x_per_g", "y_per_g", "z_per_g")
TILT_TOLERANCE_DEG = 3.0


def within_tilt_tolerance(roll_error, pitch_error):
    """Per-axis comparison target; does not establish reference accuracy."""
    return all(math.isfinite(v) and abs(v) <= TILT_TOLERANCE_DEG for v in (roll_error, pitch_error))


def parse_sample(line):
    values = line.strip().split(",")
    if len(values) != len(SAMPLE_FIELDS) + 1 or values[0] != "DATA":
        raise ValueError("Malformed DATA record")
    sample = {}
    for key, value in zip(SAMPLE_FIELDS, values[1:]):
        sample[key] = int(value) if key in INT_FIELDS else float(value)
        if not math.isfinite(sample[key]):
            raise ValueError("Nonfinite DATA value")
    if (sample["valid"] not in (0, 1) or sample["trial_id"] <= 0 or
            sample["sequence"] < 0 or sample["elapsed_us"] < 0 or
            any(not 0 <= sample[k] <= 1023 for k in ("raw_x", "raw_y", "raw_z", "raw_ref"))):
        raise ValueError("DATA value outside protocol bounds")
    return sample


def unit(v):
    length = math.sqrt(sum(x * x for x in v))
    if length < 1e-12:
        raise ValueError("Zero reference vector")
    return tuple(x / length for x in v)


def phone_vector(up, right, convention):
    """Both signs mean the named edge is HIGH; angles are in degrees.

    axis-elevation: separate elevation of phone top/right axes above horizontal.
    euler: up=pitch (forward elevation), right=roll atan2(gy,gz).
    gradient: up=atan(gx/gz), right=atan(gy/gz); a pair of projected angles.
    """
    if not all(math.isfinite(x) and abs(x) < 89 for x in (up, right)):
        raise ValueError("Phone components must be finite and between -89 and 89 degrees")
    u, r = math.radians(up), math.radians(right)
    if convention == "axis-elevation":
        x, y = math.sin(u), math.sin(r)
        z2 = 1 - x*x - y*y
        if z2 <= 0:
            raise ValueError("Impossible pair of independent axis elevations")
        return x, y, math.sqrt(z2)
    if convention == "euler":
        return math.sin(u), math.cos(u)*math.sin(r), math.cos(u)*math.cos(r)
    if convention == "gradient":
        return unit((math.tan(u), math.tan(r), 1))
    raise ValueError("Phone convention is unverified; raw data remains usable")


def board_to_heading(v, heading):
    """Rotate board U/right/up components into nose/right/up at clockwise heading."""
    c, s = math.cos(math.radians(heading)), math.sin(math.radians(heading))
    return v[0]*c + v[1]*s, -v[0]*s + v[1]*c, v[2]


def heading_to_board(v, heading):
    return board_to_heading(v, -heading)


def vector_angles(v):
    # Existing robot convention: pitch is positive for positive sensor/body X.
    x, y, z = v
    return {"roll": math.degrees(math.atan2(y, z)),
            "pitch": math.degrees(math.atan2(x, math.hypot(y, z))),
            "tilt": math.degrees(math.atan2(math.hypot(x, y), z))}


def angle_delta(a, b):
    return (a - b + 180) % 360 - 180


def vector_distance(a, b):
    a, b = unit(a), unit(b)
    return math.degrees(math.acos(max(-1, min(1, sum(x*y for x, y in zip(a, b))))))


def parse_mapping(text):
    parts = text.lower().split(",")
    if len(parts) != 3 or {p.lstrip("+-") for p in parts} != {"x", "y", "z"}:
        raise ValueError("Sensor map must use x,y,z once each, e.g. x,-y,z")
    return tuple(("xyz".index(p.lstrip("+-")), -1 if p.startswith("-") else 1)
                 for p in parts)


def to_body(sensor, mapping):
    return tuple(sign*sensor[index] for index, sign in mapping)


def to_sensor(body, mapping):
    result = [0.0]*3
    for value, (index, sign) in zip(body, mapping):
        result[index] = sign*value
    return tuple(result)


def combine_reference(readings, convention, up_sign=1, right_sign=1):
    headings = [r["heading"] for r in readings]
    if sorted(headings) != [0, 90, 180, 270]:
        raise ValueError("A reference block needs exactly four cardinal phone orientations")
    vectors = [heading_to_board(phone_vector(r["up"]*up_sign,
               r["right"]*right_sign, convention), r["heading"]) for r in readings]
    average = unit(tuple(statistics.mean(v[i] for v in vectors) for i in range(3)))
    disagreements = [vector_distance(v, average) for v in vectors]
    return {"vector": average, "tilt_deg": vector_angles(average)["tilt"],
            "max_disagreement_deg": max(disagreements),
            "rms_disagreement_deg": math.sqrt(statistics.mean(d*d for d in disagreements))}


def stats(values):
    values = list(values)
    if not values:
        return {k: None for k in ("mean", "std", "min", "max", "drift")}
    half = max(1, len(values)//2)
    drift = statistics.mean(values[-half:]) - statistics.mean(values[:half])
    return {"mean": statistics.mean(values),
            "std": statistics.stdev(values) if len(values) > 1 else 0.0,
            "min": min(values), "max": max(values), "drift": drift}


def summarize(samples, period_us=20000):
    good = [s for s in samples if s["valid"] and s["raw_ref"] > 0]
    result = {"samples": len(samples), "valid": len(good),
              "invalid": len(samples)-len(good), "warnings": []}
    for key in ("raw_x", "raw_y", "raw_z", "raw_ref", "ax", "ay", "az",
                "magnitude", "raw_roll", "raw_pitch", "robot_roll", "robot_pitch"):
        result[key] = stats(s[key] for s in good)
    for key in ("x", "y", "z"):
        result["ratio_"+key] = stats(s["raw_"+key]/s["raw_ref"] for s in good)
    gaps = [b["elapsed_us"]-a["elapsed_us"] for a, b in zip(samples, samples[1:])]
    result["interval_us"] = stats(gaps)
    result["late_intervals"] = sum(g > period_us*1.5 for g in gaps)
    result["sequence_missing"] = (samples[-1]["sequence"]+1-len(samples)) if samples else 0
    span = samples[-1]["elapsed_us"]-samples[0]["elapsed_us"] if len(samples) > 1 else 0
    result["actual_hz"] = (len(samples)-1)*1e6/span if span > 0 else None
    saturated = sum(any(s[k] in (0, 1023) for k in
                       ("raw_x", "raw_y", "raw_z", "raw_ref")) for s in samples)
    result["saturated_samples"] = saturated
    if not good: result["warnings"].append("No valid samples")
    if result["invalid"]: result["warnings"].append("Invalid sensor readings")
    if saturated: result["warnings"].append("ADC reached a rail; inspect wiring/supply")
    if result["late_intervals"] or result["sequence_missing"]:
        result["warnings"].append("Missed or delayed samples")
    # Provisional pilot thresholds; warnings never silently remove an attempt.
    for key in ("raw_roll", "raw_pitch"):
        if good and (result[key]["std"] > TILT_TOLERANCE_DEG or abs(result[key]["drift"]) > TILT_TOLERANCE_DEG):
            result["warnings"].append(key+" varies by >3 deg; check settling")
    if good and abs(result["magnitude"]["mean"]-1) > 0.15:
        result["warnings"].append("Magnitude differs from 1 g; inspect existing calibration")
    if good and abs(result["magnitude"]["drift"]) > 0.1:
        result["warnings"].append("Acceleration magnitude changed by >0.1 g within this stationary capture")
    if good and abs(result["raw_ref"]["drift"]) > result["raw_ref"]["mean"]*0.02:
        result["warnings"].append("Supply measurement drift exceeds 2 percent")
    return result


def compare_stationary(previous, current, angle_limit=TILT_TOLERANCE_DEG, magnitude_limit=0.1):
    """Compare the SAME physical pose, using absolute readings, without zeroing.

    Limits are diagnostic experiment defaults, not ADXL335 accuracy specs.
    A repeat at a different position is not a valid use of this comparison.
    """
    if not previous.get("valid") or not current.get("valid"):
        return None
    result = {"roll_delta_deg": angle_delta(current["robot_roll"]["mean"], previous["robot_roll"]["mean"]),
              "pitch_delta_deg": angle_delta(current["robot_pitch"]["mean"], previous["robot_pitch"]["mean"]),
              "magnitude_delta_g": current["magnitude"]["mean"]-previous["magnitude"]["mean"],
              "reference_adc_delta": current["raw_ref"]["mean"]-previous["raw_ref"]["mean"],
              "axis_adc_deltas": [current["raw_"+a]["mean"]-previous["raw_"+a]["mean"] for a in "xyz"],
              "ratio_deltas": [current["ratio_"+a]["mean"]-previous["ratio_"+a]["mean"] for a in "xyz"]}
    result["changed"] = (max(abs(result[k]) for k in ("roll_delta_deg", "pitch_delta_deg")) > angle_limit
                         or abs(result["magnitude_delta_g"]) > magnitude_limit)
    return result


def mean_capture_summary(summaries):
    """Equal weight per capture; means only, for same-pose comparisons."""
    good = [s for s in summaries if s.get("valid")]
    if not good:
        return {"valid": 0}
    result = {"valid": sum(s["valid"] for s in good)}
    for key in ("raw_x", "raw_y", "raw_z", "raw_ref", "ax", "ay", "az", "magnitude",
                "raw_roll", "raw_pitch", "robot_roll", "robot_pitch", "ratio_x", "ratio_y", "ratio_z"):
        result[key] = {"mean": statistics.mean(s[key]["mean"] for s in good)}
    return result


def fit_axis(expected_g, measured_ratios, minimum_span=0.2):
    """Equal weight per independent placement, not per ADC sample.

    Fit ratio = zero + sensitivity * expected_g. Report the condition of
    [1,g]; refuse weak excitation, nonpositive scales or large condition.
    """
    if len(expected_g) < 4 or len(expected_g) != len(measured_ratios):
        return {"usable": False, "reason": "Need at least four independent placements"}
    span = max(expected_g)-min(expected_g)
    mean_g, mean_r = statistics.mean(expected_g), statistics.mean(measured_ratios)
    variance = statistics.mean((g-mean_g)**2 for g in expected_g)
    trace = 1 + statistics.mean(g*g for g in expected_g)
    largest = (trace + math.sqrt(max(0, trace*trace-4*variance)))/2
    smallest = variance/largest if largest else 0
    condition = math.sqrt(largest/smallest) if smallest > 0 else None
    if span < minimum_span or condition is None or condition > 100:
        return {"usable": False, "reason": "Insufficient axis gravity coverage",
                "span_g": span, "condition": condition}
    scale = statistics.mean((g-mean_g)*(r-mean_r)
                            for g, r in zip(expected_g, measured_ratios))/variance
    zero = mean_r-scale*mean_g
    if scale <= 0 or not 0 < zero < 1:
        return {"usable": False, "reason": "Implausible offset/scale; check axes and reference signs"}
    return {"usable": True, "zero": zero, "per_g": scale,
            "span_g": span, "condition": condition}


def convert_ratios(ratios, calibration):
    return tuple((ratios[i]-calibration[CAL_KEYS[i]])/calibration[CAL_KEYS[i+3]]
                 for i in range(3))

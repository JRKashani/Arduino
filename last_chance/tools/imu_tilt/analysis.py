"""Offline summaries, reference comparisons and conservative calibration fitting."""
import csv
import hashlib
import json
import math
import os
import statistics
from datetime import datetime, timezone
from pathlib import Path

from core import (CAL_KEYS, SAMPLE_FIELDS, TILT_TOLERANCE_DEG, within_tilt_tolerance, angle_delta, board_to_heading, combine_reference,
                  compare_stationary, convert_ratios, fit_axis, mean_capture_summary, parse_mapping, summarize, to_body, to_sensor,
                  vector_angles, vector_distance)
from recording import planned_surfaces, session_events


def write_csv(path, rows, fields=None):
    rows = list(rows)
    fields = fields or list(dict.fromkeys(k for row in rows for k in row))
    with Path(path).open("x", newline="", encoding="utf-8") as file:
        writer = csv.DictWriter(file, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def rotation(roll, pitch, yaw):
    """Explicit mounting correction Rz(yaw) Ry(pitch) Rx(roll), in degrees."""
    cr, sr = math.cos(math.radians(roll)), math.sin(math.radians(roll))
    cp, sp = math.cos(math.radians(pitch)), math.sin(math.radians(pitch))
    cy, sy = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    return ((cy*cp, cy*sp*sr-sy*cr, cy*sp*cr+sy*sr),
            (sy*cp, sy*sp*sr+cy*cr, sy*sp*cr-cy*sr),
            (-sp, cp*sr, cp*cr))


def multiply(matrix, vector):
    return tuple(sum(a*b for a, b in zip(row, vector)) for row in matrix)


def transpose(matrix):
    return tuple(zip(*matrix))


def rms(values):
    values = list(values)
    return math.sqrt(statistics.mean(x*x for x in values)) if values else None


def placement_key(trial):
    return trial.get("placement_id") or f"legacy-trial-{trial['trial_id']}"


def combine_placements(observations):
    """Keep each capture in trials.csv; combine completed repeat groups once.

    Equal weight per retained capture, then equal weight per placement in a fit.
    Different planned loops, phone blocks and interrupted groups stay distinct.
    """
    grouped = {}
    for item in observations:
        if item["included"]:
            grouped.setdefault(placement_key(item["trial"]), []).append(item)
    combined = []
    for key, members in grouped.items():
        first = members[0]
        identity = lambda m: (m["trial"]["surface"], m["trial"]["heading"], m["trial"]["loop"],
                              m["trial"]["reference_id"], m["trial"]["boot_id"], m["trial"]["calibration"])
        if any(identity(m) != identity(first) for m in members):
            raise ValueError("A repeat group mixes different poses, references, boots or calibrations")
        summary = mean_capture_summary([m["summary"] for m in members])
        measured = tuple(statistics.mean(m["measured"][i] for m in members) for i in range(3))
        row = {"placement_id": key, "trial_ids": ";".join(str(m["trial"]["trial_id"]) for m in members),
               "surface": first["trial"]["surface"], "reference_id": first["trial"]["reference_id"],
               "heading": first["trial"]["heading"], "loop": first["trial"]["loop"],
               "boot_id": first["trial"]["boot_id"], "trial_id": members[-1]["trial"]["trial_id"],
               "status": "accept", "captures": len(members), "valid_samples": summary["valid"]}
        row.update({key+"_mean": stat["mean"] for key, stat in summary.items() if isinstance(stat, dict)})
        row.update({"mapped_"+k+"_deg": v for k, v in vector_angles(measured).items()})
        row["magnitude_std"] = math.sqrt(statistics.mean(
            m["summary"]["magnitude"]["std"]**2 +
            (m["summary"]["magnitude"]["mean"]-summary["magnitude"]["mean"])**2 for m in members))
        comparisons = [compare_stationary(m["summary"], n["summary"])
                       for i, m in enumerate(members) for n in members[i+1:]]
        disagreement = any(c and c["changed"] for c in comparisons)
        warnings = [m["row"]["warnings"] for m in members if m["row"]["warnings"]]
        if disagreement:
            warnings.append("Kept captures disagree by >3 deg or >0.1 g; excluded from calibration fitting")
        row["warnings"] = "; ".join(dict.fromkeys(warnings))
        row["repeat_max_angle_difference_deg"] = max((max(abs(c["roll_delta_deg"]), abs(c["pitch_delta_deg"]))
                                                       for c in comparisons if c), default=0)
        row["repeat_max_magnitude_difference_g"] = max((abs(c["magnitude_delta_g"]) for c in comparisons if c), default=0)
        result = {"trial": first["trial"], "trial_ids": [m["trial"]["trial_id"] for m in members],
                  "row": row, "summary": summary, "members": members, "measured": measured,
                  "ratios": tuple(statistics.mean(m["ratios"][i] for m in members) for i in range(3)),
                  "eligible": all(m["eligible"] for m in members) and not disagreement}
        if all("expected" in m for m in members):
            expected = tuple(statistics.mean(m["expected"][i] for m in members) for i in range(3))
            result["expected"] = expected
            result["sensor_expected"] = tuple(statistics.mean(m["sensor_expected"][i] for m in members) for i in range(3))
            ea, ma = vector_angles(expected), vector_angles(measured)
            for angle in ("roll", "pitch"):
                row[f"expected_{angle}_deg"] = ea[angle]
                row[f"{angle}_error_deg"] = angle_delta(ma[angle], ea[angle])
            row["vector_error_deg"] = vector_distance(measured, expected)
            row["within_3deg_per_axis"] = within_tilt_tolerance(row["roll_error_deg"], row["pitch_error_deg"])
        if first["row"].get("reference_tilt_deg") is not None:
            row["reference_tilt_deg"] = first["row"]["reference_tilt_deg"]
        combined.append(result)
    return combined


def diagnose_flat_changes(placements):
    levels = sorted((p for p in placements if p["trial"]["trial_kind"] == "level"),
                    key=lambda p: min(p["trial_ids"]))
    checks, changes = [], []
    for current in levels:
        s = current["summary"]
        check = {"trial_ids": current["trial_ids"], "surface": current["trial"]["surface"],
                 "roll_deg": s["robot_roll"]["mean"], "pitch_deg": s["robot_pitch"]["mean"],
                 "magnitude_g": s["magnitude"]["mean"], "reference_adc": s["raw_ref"]["mean"],
                 "axis_adc": [s["raw_"+a]["mean"] for a in "xyz"],
                 "ratios": [s["ratio_"+a]["mean"] for a in "xyz"]}
        checks.append(check)
    for previous, current in zip(levels, levels[1:]):
        if previous["trial"]["calibration"] != current["trial"]["calibration"]:
            continue # A conversion change must not be reported as a physical jump.
        comparison = compare_stationary(previous["summary"], current["summary"])
        if comparison:
            inconsistent_flat_repeats = any(
                p["row"]["repeat_max_angle_difference_deg"] > TILT_TOLERANCE_DEG or
                p["row"]["repeat_max_magnitude_difference_g"] > 0.1 for p in (previous, current))
            comparison["flat_repeat_disagreement"] = inconsistent_flat_repeats
            comparison["changed"] |= inconsistent_flat_repeats
            comparison.update({"previous_trial_ids": previous["trial_ids"], "current_trial_ids": current["trial_ids"],
                               "previous_surface": previous["trial"]["surface"], "current_surface": current["trial"]["surface"]})
            changes.append(comparison)
            if comparison["changed"]:
                low, high = max(previous["trial_ids"]), min(current["trial_ids"])
                for p in placements:
                    if p["trial"]["trial_kind"] == "slope" and any(low < tid < high for tid in p["trial_ids"]):
                        p["eligible"] = False
                        warning = "Bracketed flat checks changed by >3 deg or >0.1 g; exclude from calibration until investigated"
                        p["row"]["warnings"] += ("; " if p["row"]["warnings"] else "")+warning
                        for member in p["members"]:
                            member["row"]["warnings"] += ("; " if member["row"]["warnings"] else "")+warning
    return checks, changes


def analyze(args):
    directory = Path(args.session)
    metadata = json.loads((directory/"session.json").read_text(encoding="utf-8"))
    if metadata.get("schema") != 1:
        raise ValueError("Unsupported session schema")
    events = session_events(directory)
    surfaces = planned_surfaces(metadata, events)
    destination = directory/"analysis"/datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S_%fZ")
    destination.mkdir(parents=True, exist_ok=False)
    mapping = parse_mapping(args.sensor_map)
    mount = rotation(args.mount_roll, args.mount_pitch, args.mount_yaw)
    axes_inferred = getattr(args, "axes_inferred", False)
    compare_axes = args.axes_verified or axes_inferred

    def body(sensor):
        return multiply(mount, to_body(sensor, mapping))

    trials, samples, reviews, endings, references = {}, {}, {}, {}, {}
    phone_rows = []
    for event in events:
        kind = event["kind"]
        if kind == "trial":
            key = event["trial_id"]
            if key in trials:
                raise ValueError("Duplicate attempt ID; session records are inconsistent")
            trials[key] = event
        elif kind == "sample":
            samples.setdefault(event["trial_id"], []).append(event)
        elif kind == "review":
            reviews[event["trial_id"]] = event
        elif kind == "end":
            endings[event["trial_id"]] = event
        elif kind == "reference":
            references[(event["reference_id"], event["phase"])] = event
        elif kind == "phone_reading":
            phone_rows.append({k: event.get(k) for k in ("utc", "surface", "reference_id", "phase", "phone_attempt_id", "heading", "up", "right")})
    if set(samples)-set(trials):
        raise ValueError("Samples exist without a linked trial record")

    converted_refs = {}
    for key, event in references.items():
        try:
            converted_refs[key] = combine_reference(event["readings"], args.phone_convention,
                                                    args.up_sign, args.right_sign)
        except ValueError as exc:
            converted_refs[key] = {"error": str(exc)}
    reference_rows = []
    for row in phone_rows:
        selected = references.get((row["reference_id"], row["phase"]))
        row["selected_reference"] = bool(selected and selected.get("phone_attempt_id") == row["phone_attempt_id"])
    for key, ref in converted_refs.items():
        row = {"reference_id": key[0], "phase": key[1], "surface": references[key]["surface"]}
        if "error" in ref:
            row["error"] = ref["error"]
        else:
            row.update({k: v for k, v in ref.items() if k != "vector"})
            row.update(dict(zip(("u_g", "right_g", "normal_g"), ref["vector"])))
        reference_rows.append(row)

    completed_groups = {placement_key(trial) for tid, trial in trials.items()
                        if reviews.get(tid, {}).get("decision") == "accept"}
    summary_rows, sample_rows, observations = [], [], []
    report_warnings = []
    if metadata.get("calibration_info"):
        report_warnings.append("Recorded calibration status: "+metadata["calibration_info"]["status"])
    if axes_inferred:
        report_warnings.append("Axis mapping and phone convention are analysis assumptions inferred from measurements, not physically verified. Comparisons and fitted constants are provisional; no candidate is recommended for application.")
    elif not args.axes_verified:
        report_warnings.append("Axis signs/mapping are unverified: reference errors and calibration fitting are disabled.")
    if args.phone_convention == "unverified":
        report_warnings.append("Phone angle convention is unverified: preserved original numbers; no reference conversion.")
    for trial_id, trial in sorted(trials.items()):
        data = samples.get(trial_id, [])
        review = reviews.get(trial_id, {"decision": "incomplete", "reason": "No review record"})
        end = endings.get(trial_id, {})
        summary = summarize(data, trial["settings"]["period_us"])
        status = review["decision"]
        included = status == "accept" or (status == "retain" and placement_key(trial) in completed_groups)
        row = {"trial_id": trial_id, "boot_id": trial["boot_id"], "surface": trial["surface"],
               "placement_id": placement_key(trial), "included_in_placement": included,
               "reference_id": trial["reference_id"], "loop": trial["loop"], "heading": trial["heading"],
               "status": status, "reason": review.get("reason", ""),
               "firmware_status": end.get("status", "incomplete"),
               "firmware_missed": end.get("firmware_missed"),
               "capture_duration_us": end.get("duration_us")}
        warnings = list(summary["warnings"])
        if status == "retain" and not included:
            warnings.append("Kept capture awaits an accepted final repeat; not yet included in combined results")
        integrity = (end.get("status") == "DONE" and end.get("firmware_samples") == len(data))
        if not integrity:
            warnings.append("Incomplete capture or inconsistent sample count")
        if data and any(b["sequence"] <= a["sequence"] or b["elapsed_us"] <= a["elapsed_us"]
                        for a, b in zip(data, data[1:])):
            integrity = False
            warnings.append("Nonmonotonic samples")
        for key, value in summary.items():
            if key == "warnings":
                continue
            if isinstance(value, dict):
                row.update({f"{key}_{stat}": number for stat, number in value.items()})
            else:
                row[key] = value
        for sample in data:
            sample_rows.append({"boot_id": trial["boot_id"], "surface": trial["surface"],
                                "placement_id": placement_key(trial), "included_in_placement": included,
                                "reference_id": trial["reference_id"], "heading": trial["heading"],
                                "loop": trial["loop"], "status": status,
                                **{key: sample[key] for key in SAMPLE_FIELDS}})
        ref = converted_refs.get((trial["reference_id"], "before"))
        after = converted_refs.get((trial["reference_id"], "after"))
        stable = summary["valid"] >= 10 and summary["saturated_samples"] == 0
        for key in ("raw_roll", "raw_pitch"):
            if summary[key]["std"] is not None:
                stable &= (summary[key]["std"] <= args.max_std and abs(summary[key]["drift"]) <= args.max_drift)
        if not stable:
            warnings.append("Capture fails provisional fitting quality limits")
        ref_ok = bool(ref and "error" not in ref)
        if ref_ok:
            row["reference_tilt_deg"] = ref["tilt_deg"]
            row["reference_max_disagreement_deg"] = ref["max_disagreement_deg"]
            ref_ok = ref["max_disagreement_deg"] <= args.max_reference_disagreement
            if not ref_ok:
                warnings.append("Phone orientations disagree beyond reference limit")
            if after and "error" not in after:
                drift = vector_distance(ref["vector"], after["vector"])
                row["reference_before_after_drift_deg"] = drift
                if drift > args.max_reference_disagreement or after["max_disagreement_deg"] > args.max_reference_disagreement:
                    ref_ok = False
                    warnings.append("Phone reference changed after robot trials")
        elif trial["trial_kind"] == "slope":
            warnings.append("No usable converted phone reference")

        if summary["valid"]:
            sensor = tuple(summary[k]["mean"] for k in ("ax", "ay", "az"))
            measured = body(sensor)
            angles = vector_angles(measured)
            row.update({"mapped_"+k+"_deg": v for k, v in angles.items()})
            ratios = tuple(summary["ratio_"+a]["mean"] for a in "xyz")
            item = {"trial": trial, "row": row, "summary": summary, "ratios": ratios,
                    "measured": measured, "included": included,
                    "eligible": included and integrity and stable and ref_ok and compare_axes}
            if ref and "error" not in ref and compare_axes:
                expected = board_to_heading(ref["vector"], trial["heading"])
                expected_angles = vector_angles(expected)
                for key in ("roll", "pitch"):
                    row[f"expected_{key}_deg"] = expected_angles[key]
                    row[f"{key}_error_deg"] = angle_delta(angles[key], expected_angles[key])
                row["vector_error_deg"] = vector_distance(measured, expected)
                sensor_expected = to_sensor(multiply(transpose(mount), expected), mapping)
                item.update(expected=expected, sensor_expected=sensor_expected)
            observations.append(item)
        row["warnings"] = "; ".join(warnings)
        summary_rows.append(row)

    placements = combine_placements(observations)
    flat_checks, flat_changes = diagnose_flat_changes(placements)
    if any(change["changed"] for change in flat_changes):
        report_warnings.append("Flat reference readings changed substantially with unchanged conversion constants. Affected bracketed slope placements are excluded from fitting; inspect setup and sensor signal before zeroing or recalibrating.")
    evaluated = [p for p in placements if "expected" in p]
    train = [e for e in evaluated if e["eligible"] and e["trial"]["loop"] == 1]
    validation = [e for e in evaluated if e["eligible"] and e["trial"]["loop"] == 2]
    proposal = {"model": "Independent normalized ADC offsets/scales with explicit signed axis map and fixed mount rotation",
                "assumption": "Phone surface and robot datum agree. Unmodeled chassis/mount offsets may be absorbed by fitted offsets; this is an installed-system proposal.",
                "training_placements": len(train), "validation_placements": len(validation),
                "training_trial_ids": [tid for e in train for tid in e["trial_ids"]],
                "validation_trial_ids": [tid for e in validation for tid in e["trial_ids"]],
                "axes": {}, "validation": {}, "recommended": False,
                "axes_status": "verified" if args.axes_verified else "inferred" if axes_inferred else "unverified",
                "active_calibration_changed": False}
    candidate = dict(train[0]["trial"]["calibration"]) if train else None
    fitted = 0
    if compare_axes and args.phone_convention != "unverified":
        for i, axis in enumerate("xyz"):
            fit = fit_axis([e["sensor_expected"][i] for e in train], [e["ratios"][i] for e in train], args.minimum_span)
            proposal["axes"][axis] = fit
            if fit["usable"] and candidate:
                candidate[CAL_KEYS[i]] = fit["zero"]
                candidate[CAL_KEYS[i+3]] = fit["per_g"]
                fitted += 1
    proposal["candidate_constants"] = candidate if fitted else None
    if candidate and fitted and validation:
        current_errors, candidate_errors = [], []
        current_roll, candidate_roll, current_pitch, candidate_pitch = [], [], [], []
        for item in validation:
            current = body(convert_ratios(item["ratios"], item["trial"]["calibration"]))
            proposed = body(convert_ratios(item["ratios"], candidate))
            expected = item["expected"]
            current_errors.append(vector_distance(current, expected))
            candidate_errors.append(vector_distance(proposed, expected))
            ea, ca, pa = vector_angles(expected), vector_angles(current), vector_angles(proposed)
            current_roll.append(angle_delta(ca["roll"], ea["roll"]))
            candidate_roll.append(angle_delta(pa["roll"], ea["roll"]))
            current_pitch.append(angle_delta(ca["pitch"], ea["pitch"]))
            candidate_pitch.append(angle_delta(pa["pitch"], ea["pitch"]))
            item["row"]["candidate_vector_error_deg"] = candidate_errors[-1]
            item["row"]["candidate_roll_error_deg"] = candidate_roll[-1]
            item["row"]["candidate_pitch_error_deg"] = candidate_pitch[-1]
            for member in item["members"]:
                member_proposed = body(convert_ratios(member["ratios"], candidate))
                member_angles, member_expected = vector_angles(member_proposed), vector_angles(member["expected"])
                member["row"]["candidate_vector_error_deg"] = vector_distance(member_proposed, member["expected"])
                for angle in ("roll", "pitch"):
                    member["row"][f"candidate_{angle}_error_deg"] = angle_delta(member_angles[angle], member_expected[angle])
        proposal["validation"] = {
            "current_vector_rms_deg": rms(current_errors), "candidate_vector_rms_deg": rms(candidate_errors),
            "current_max_vector_error_deg": max(current_errors), "candidate_max_vector_error_deg": max(candidate_errors),
            "current_roll_rms_deg": rms(current_roll), "candidate_roll_rms_deg": rms(candidate_roll),
            "current_pitch_rms_deg": rms(current_pitch), "candidate_pitch_rms_deg": rms(candidate_pitch)}
        proposal["held_out_improvement"] = (len(validation) >= 4 and rms(candidate_errors) < rms(current_errors)
                                   and max(candidate_errors) <= max(current_errors))
        proposal["recommended"] = proposal["held_out_improvement"] and args.axes_verified
        if not proposal["held_out_improvement"]:
            report_warnings.append("Candidate failed the held-out improvement check or has too few validation placements.")
    if not fitted:
        report_warnings.append("No calibration axis was identifiable with the verified references and quality limits.")
    if not validation:
        report_warnings.append("No eligible second-loop placements for validation.")
    if proposal["recommended"]:
        report_warnings.append("Candidate is provisional: verify on a new surface and inspect mounting assumptions before adopting it.")

    pairs = []
    by_pose = {(e["trial"]["surface"], e["trial"]["heading"], e["trial"]["loop"]): e for e in placements}
    for (surface, heading, loop), first in by_pose.items():
        second = by_pose.get((surface, heading, 2))
        if loop == 1 and second and first["trial"]["reference_id"] == second["trial"]["reference_id"]:
            a, b = vector_angles(first["measured"]), vector_angles(second["measured"])
            pairs.append({"surface": surface, "heading": heading,
                          "roll_difference_deg": angle_delta(b["roll"], a["roll"]),
                          "pitch_difference_deg": angle_delta(b["pitch"], a["pitch"]),
                          "vector_difference_deg": vector_distance(first["measured"], second["measured"])})

    levels = [p["row"] for p in placements if p["trial"]["trial_kind"] == "level"]
    level_drift = None
    if len(levels) >= 2:
        # Pair by time, not trial labels, including multiple resumed runs.
        level_drift = {key: angle_delta(levels[-1]["mapped_"+key+"_deg"], levels[0]["mapped_"+key+"_deg"])
                       for key in ("roll", "pitch")}

    accepted_slope = { (t["surface"], t["heading"], t["loop"]) for tid, t in trials.items()
                      if reviews.get(tid, {}).get("decision") == "accept" and t["trial_kind"] == "slope"}
    expected_count = len(surfaces)*(9 if metadata["diagonal"] else 8)
    write_csv(destination/"samples.csv", sample_rows,
              ["boot_id", "surface", "reference_id", "placement_id", "included_in_placement", "heading", "loop", "status", *SAMPLE_FIELDS])
    write_csv(destination/"trials.csv", summary_rows)
    write_csv(destination/"placements.csv", [p["row"] for p in placements])
    write_csv(destination/"phone-readings.csv", phone_rows,
              ["utc", "surface", "reference_id", "phase", "phone_attempt_id", "selected_reference", "heading", "up", "right"])
    write_csv(destination/"references.csv", reference_rows)
    write_csv(destination/"repeatability.csv", pairs)
    metrics = [e for e in evaluated if e["row"]["status"] == "accept"]
    report = {"schema": 1, "session_id": metadata["session_id"], "simulated": metadata["simulated"],
              "options": {k: v for k, v in vars(args).items() if k not in ("handler",)},
              "calibration_info": metadata.get("calibration_info"),
              "accepted_slope_placements": len(accepted_slope), "expected_slope_placements": expected_count,
              "level_drift_deg": level_drift, "warnings": report_warnings,
              "flat_checks": flat_checks, "flat_comparisons": flat_changes,
              "current_vector_rms_deg": rms(e["row"]["vector_error_deg"] for e in metrics),
              "tilt_tolerance_deg": TILT_TOLERANCE_DEG,
              "placements_compared": len(metrics),
              "placements_within_tolerance": sum(e["row"]["within_3deg_per_axis"] for e in metrics) if metrics else None,
              "calibration_proposal": proposal,
              "limits": ["Phone disagreement is a repeatability indicator, not absolute reference accuracy.",
                         "Loop 2 can share phone references with loop 1; this does not establish independent absolute accuracy.",
                         "Kept repeat captures are averaged with equal weight per capture, then each placement gets equal fit weight.",
                         "The +/-3 deg per-axis comparison target is not guaranteed absolute accuracy; phone reference uncertainty and mounting bias still apply.",
                         "Flat changes and repeat disagreement thresholds (3 deg or 0.1 g) assume the same physical pose; they are diagnostic defaults, not sensor specifications.",
                         "Unfitted axes retain existing constants from the first training placement.",
                         "No automatic mount estimate or full cross-axis fit is inferred from narrow slope coverage.",
                         "No requested-angle grid is fabricated from household slopes."]}
    source_files = [directory/"session.json", *sorted(directory.glob("events-*.jsonl")),
                    *sorted(directory.glob("serial-*.log"))]
    report["source_sha256"] = {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in source_files}
    with (destination/"report.json").open("x", encoding="utf-8") as file:
        json.dump(report, file, indent=2, allow_nan=False)
    with (destination/"calibration-proposal.json").open("x", encoding="utf-8") as file:
        json.dump(proposal, file, indent=2, allow_nan=False)
    with (destination/"flat-diagnostics.json").open("x", encoding="utf-8") as file:
        json.dump({"checks": flat_checks, "comparisons": flat_changes}, file, indent=2, allow_nan=False)
    text = ["IMU tilt mission report", f"Simulated data: {metadata['simulated']}",
            f"Accepted slope placements: {len(accepted_slope)}/{expected_count}",
            f"Phone convention: {args.phone_convention}; axes verified: {args.axes_verified}; axes inferred: {axes_inferred}",
            f"Level drift (last minus first): {level_drift}",
            f"Reference comparison vector RMS (deg): {report['current_vector_rms_deg']}",
            f"Within +/-3 deg on both axes versus phone reference under selected assumptions: {report['placements_within_tolerance']} / {len(metrics)} (None means unassessed)",
            f"Fitting: {len(train)} loop-1 placements; validation: {len(validation)} loop-2 placements.",
            f"Candidate passes held-out improvement check: {proposal.get('held_out_improvement', False)}",
            f"Candidate recommended with verified axes: {proposal['recommended']}",
            "Active robot calibration was not modified.", "", *report_warnings, "", *report["limits"]]
    for axis, fit in proposal["axes"].items():
        text.append(f"Axis {axis.upper()}: {fit}")
    if proposal["validation"]:
        text.append(f"Held-out comparison: {proposal['validation']}")
    for change in flat_changes:
        if change["changed"]:
            text.append(f"FLAT CHANGE {change['previous_trial_ids']} -> {change['current_trial_ids']}: roll {change['roll_delta_deg']:+.3f} deg, pitch {change['pitch_delta_deg']:+.3f} deg, magnitude {change['magnitude_delta_g']:+.3f} g; reference ADC delta {change['reference_adc_delta']:+.3f}; axis ADC deltas {change['axis_adc_deltas']}")
    (destination/"report.txt").write_text("\n".join(text)+"\n", encoding="utf-8")
    if not args.no_plots:
        make_plots(destination, [p["row"] for p in placements], pairs, reference_rows, metadata["simulated"], args.axes_verified, flat_checks)
    print("\n".join(text[:9]))
    print(f"\nAnalysis saved to: {destination.resolve()}")
    return destination


def make_plots(destination, rows, pairs, references, simulated=False, axes_verified=False, flat_checks=None):
    try:
        # Keep the plotting cache with generated artifacts, including in sandboxes.
        os.environ.setdefault("MPLCONFIGDIR", str((destination/"plot-cache").resolve()))
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError as exc:
        raise RuntimeError("Reports saved, but plotting requires requirements.txt; use --no-plots for CSV/JSON only") from exc
    good = [r for r in rows if r["status"] == "accept" and r.get("mapped_roll_deg") is not None]
    fig, axes = plt.subplots(2, 2, figsize=(11, 8), constrained_layout=True)
    for axis, angle in zip(axes[0], ("roll", "pitch")):
        comparable = [r for r in good if r.get(f"expected_{angle}_deg") is not None]
        if comparable:
            x = [r[f"expected_{angle}_deg"] for r in comparable]
            y = [r[f"mapped_{angle}_deg"] for r in comparable]
            for loop, marker in ((1, "o"), (2, "x"), (3, "+")):
                group = [r for r in comparable if r["loop"] == loop]
                if group:
                    axis.scatter([r[f"expected_{angle}_deg"] for r in group],
                                 [r[f"mapped_{angle}_deg"] for r in group],
                                 marker=marker, label=f"Loop {loop}", alpha=0.7, s=25)
            limits = (min(x+y)-1, max(x+y)+1)
            axis.plot(limits, limits, color="gray", linestyle="--")
            axis.set_xlabel("Phone reference (deg)")
            axis.legend()
        else:
            axis.plot([r["trial_id"] for r in good], [r[f"mapped_{angle}_deg"] for r in good], ".-")
            axis.set_xlabel("Attempt ID (reference unverified)")
        axis.set_ylabel(f"Measured {angle} (deg)")
        axis.grid(alpha=0.25)
    comparable = [r for r in good if r.get("vector_error_deg") is not None]
    for angle in ("roll", "pitch"):
        axes[1, 0].scatter([r.get("reference_tilt_deg") for r in comparable],
                           [r[f"{angle}_error_deg"] for r in comparable], label=angle, s=20)
    axes[1, 0].axhline(0, color="gray", linestyle="--")
    axes[1, 0].set(xlabel="Reference surface tilt (deg)", ylabel="Measured - reference (deg)")
    if comparable: axes[1, 0].legend()
    axes[1, 0].grid(alpha=0.25)
    magnitude_rows = [r for r in good if r.get("reference_tilt_deg") is not None]
    magnitude_x = [r["reference_tilt_deg"] for r in magnitude_rows]
    if not magnitude_rows:
        magnitude_rows = good
        magnitude_x = [r["trial_id"] for r in good]
    axes[1, 1].errorbar(magnitude_x, [r["magnitude_mean"] for r in magnitude_rows],
                       yerr=[r["magnitude_std"] for r in magnitude_rows], fmt=".", capsize=2)
    axes[1, 1].axhline(1, color="gray", linestyle="--")
    axes[1, 1].set(xlabel="Reference surface tilt (deg)" if comparable else "Attempt ID",
                   ylabel="Gravity magnitude (g), +/- sample std")
    axes[1, 1].grid(alpha=0.25)
    prefix = "SIMULATED — " if simulated else ""
    suffix = " (axis mapping unverified)" if not axes_verified else ""
    fig.suptitle(prefix+"Stationary IMU measurements"+suffix)
    fig.savefig(destination/"measurements.png", dpi=180)
    fig.savefig(destination/"measurements.svg")
    plt.close(fig)

    fig, axes = plt.subplots(1, 2, figsize=(11, 4), constrained_layout=True)
    for angle in ("roll", "pitch"):
        axes[0].plot(range(1, len(pairs)+1), [p[f"{angle}_difference_deg"] for p in pairs], ".", label=angle)
    axes[0].axhline(0, color="gray", linestyle="--")
    axes[0].set(xlabel="Paired placement", ylabel="Loop 2 - loop 1 (deg)")
    axes[0].legend()
    converted = [r for r in references if "max_disagreement_deg" in r]
    axes[1].bar(range(len(converted)), [r["max_disagreement_deg"] for r in converted])
    axes[1].set(xlabel="Phone reference block", ylabel="Max orientation disagreement (deg)")
    for axis in axes: axis.grid(alpha=0.25)
    fig.suptitle(prefix+"Repeatability and reference consistency"+suffix)
    fig.savefig(destination/"repeatability.png", dpi=180)
    fig.savefig(destination/"repeatability.svg")
    plt.close(fig)
    if flat_checks:
        fig, axes = plt.subplots(2, 2, figsize=(11, 7), constrained_layout=True)
        ids = [max(c["trial_ids"]) for c in flat_checks]
        for name in ("roll", "pitch"):
            axes[0, 0].plot(ids, [c[name+"_deg"] for c in flat_checks], ".-", label=name)
        axes[0, 0].set(ylabel="Absolute flat angle (deg)")
        axes[0, 0].legend()
        axes[0, 1].plot(ids, [c["magnitude_g"] for c in flat_checks], ".-")
        axes[0, 1].axhline(1, color="gray", linestyle="--")
        axes[0, 1].set(ylabel="Calculated gravity magnitude (g)")
        for i, axis in enumerate("xyz"):
            axes[1, 0].plot(ids, [c["ratios"][i] for c in flat_checks], ".-", label=axis.upper())
        axes[1, 0].set(ylabel="Axis ADC / reference ADC")
        axes[1, 0].legend()
        axes[1, 1].plot(ids, [c["reference_adc"] for c in flat_checks], ".-")
        axes[1, 1].set(ylabel="Supply-reference ADC counts")
        for axis in axes.flat:
            axis.set_xlabel("Flat-check final attempt ID")
            axis.grid(alpha=0.25)
        fig.suptitle(prefix+"Same-pose flat checks — no zero correction")
        fig.savefig(destination/"flat-checks.png", dpi=180)
        fig.savefig(destination/"flat-checks.svg")
        plt.close(fig)

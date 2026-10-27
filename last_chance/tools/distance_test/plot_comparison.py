"""Reproduce the three-sensor figure from archived accepted DATA samples."""
import csv
import argparse
import hashlib
import json
import os
import statistics
from pathlib import Path

from distance_core import build_tables, load_events


ROOT = Path(__file__).resolve().parents[2]
ARCHIVE = ROOT / "docs/distance-results-2026-10-11"
OUTPUT = ARCHIVE / "comparison"
RUNS = [
    ("ir", "20261011T152446_019469Z_ir", "IR", "#238443", "raw", "ADC counts"),
    ("us", "20261011T142003_780535Z_us", "Ultrasonic", "#2171b5", "calibrated_cm", "cm"),
    ("laser", "20261011T144802_531804Z_laser", "Laser", "#d95f02", "calibrated_cm", "cm"),
]


def laser_fit():
    """Fit original position means; reserve the separate follow-up for a check."""
    def read(session):
        d = ARCHIVE / session
        meta = json.loads((d / "session.json").read_text(encoding="utf-8"))
        return [p for p in build_tables(load_events(d), meta)[2]
                if p["completed"] and p["three_usable_repeats"]]

    training = read(RUNS[2][1])
    check_session = "20261011T152053_543501Z_laser"
    check = read(check_session)
    x = [p["raw_cm_mean"] for p in training]
    y = [p["actual_cm"] for p in training]
    mx, my = statistics.mean(x), statistics.mean(y)
    scale = sum((a-mx)*(b-my) for a, b in zip(x, y)) / sum((a-mx)**2 for a in x)
    offset = my - scale * mx
    assert scale > 0
    def errors(points):
        return [{"measured_cm": p["actual_cm"], "reported_cm": p["raw_cm_mean"],
                 "corrected_cm": scale*p["raw_cm_mean"] + offset,
                 "before_error_cm": p["raw_cm_mean"] - p["actual_cm"],
                 "after_error_cm": scale*p["raw_cm_mean"] + offset - p["actual_cm"]}
                for p in points]
    result = {"model": "corrected_cm = scale * reported_cm + offset_cm",
              "scale": scale, "offset_cm": offset, "training_session": RUNS[2][1],
              "training_measured_range_cm": [min(y), max(y)],
              "fit_method": "ordinary least squares; equal weight per position mean",
              "training": errors(training), "follow_up_session": check_session,
              "follow_up": errors(check), "applied_to_firmware": False,
              "limitations": "Experimental fit; mounting change near 60 cm; tape uncertainty +/-1 cm. "
              "Sample-range bars exclude fit uncertainty. Follow-up was not used to fit coefficients."}
    for label in ("training", "follow_up"):
        for stage in ("before", "after"):
            result[f"{label}_{stage}_mae_cm"] = statistics.mean(
                abs(p[f"{stage}_error_cm"]) for p in result[label])
    return result


def main(calibrated_laser=False):
    global OUTPUT
    calibration = laser_fit() if calibrated_laser else None
    if calibrated_laser:
        OUTPUT = ARCHIVE / "comparison-calibrated-laser"
    OUTPUT.mkdir(exist_ok=True)
    os.environ.setdefault("MPLCONFIGDIR", str(ROOT / "build/distance-plot-cache"))
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    plt.rcParams.update({"font.size": 11, "axes.titlesize": 16})
    fig, left = plt.subplots(figsize=(12, 8))
    right = left.twinx()
    fig.subplots_adjust(left=0.09, right=0.91, bottom=0.28, top=0.86)
    rows, handles, sources = [], [], {}
    for sensor, session, label, color, quantity, unit in RUNS:
        directory = ARCHIVE / session
        meta = json.loads((directory / "session.json").read_text(encoding="utf-8"))
        samples, _, summary = build_tables(load_events(directory), meta)
        for path in [directory / "session.json", *sorted(directory.glob("events-*.jsonl"))]:
            sources[path.relative_to(ARCHIVE).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
        points = []
        for point in summary:
            if not point["completed"] or not point["three_usable_repeats"]:
                continue
            values = [s[quantity] for s in samples
                      if s["included"] and s["valid"] and s["point_index"] == point["point_index"]
                      and s[quantity] is not None]
            if not values:
                continue
            mean = point[quantity + "_mean"]  # Equal weight for each accepted repeat.
            if sensor == "laser" and calibration:
                scale, offset = calibration["scale"], calibration["offset_cm"]
                values = [scale * v + offset for v in values]
                mean = scale * mean + offset
            assert min(values) <= mean <= max(values)
            row = {"sensor": sensor, "session": session, "measured_cm": point["actual_cm"],
                   "x_error_minus_cm": 1, "x_error_plus_cm": 1, "output_unit": unit,
                   "output_mean": mean, "sample_min": min(values), "sample_max": max(values),
                   "accepted_repeats": point["accepted_captures"], "valid_samples": len(values)}
            rows.append(row)
            points.append(row)
        ax = left if sensor == "ir" else right
        if sensor == "laser" and calibration:
            label = "Laser, corrected"
        handle = ax.errorbar(
            [p["measured_cm"] for p in points], [p["output_mean"] for p in points],
            xerr=1.0,
            yerr=[[p["output_mean"] - p["sample_min"] for p in points],
                  [p["sample_max"] - p["output_mean"] for p in points]],
            fmt="o-", color=color, markersize=4.5, linewidth=1.7,
            elinewidth=0.9, capsize=3, capthick=0.9,
            label=f"{label} ({'left axis: ADC' if sensor == 'ir' else 'right axis: cm'})",
        )
        handles.append(handle)

    left.set(xlabel="Distance measured with tape, from sensor face (cm)",
             ylabel="IR output (ADC counts)", xlim=(0, 190), ylim=(0, 550))
    left.tick_params(axis="y", colors="#238443")
    left.yaxis.label.set_color("#238443")
    right.set(ylabel="Ultrasonic / laser reported distance (cm)", ylim=(0, 200))
    left.set_xticks(range(0, 191, 10))
    left.grid(alpha=0.22)
    left.set_title("Three-sensor distance characteristic" +
                   (" — experimental laser calibration" if calibration else ""), pad=35)
    fig.legend(handles=handles, loc="upper center", bbox_to_anchor=(0.5, 0.925),
               ncol=3, frameon=False, fontsize=10)
    fig.text(0.09, 0.105,
             "Points: mean of 3 accepted repeats. Vertical bars: minimum–maximum of all accepted samples.\n"
             "Horizontal bars: ±1 cm tape-measure uncertainty (2 cm total width).\n"
             + ("US uses existing calibration; IR remains in ADC counts. " +
              (f"Laser: corrected = {calibration['scale']:.5f} × reported − {abs(calibration['offset_cm']):.3f} cm.\n"
               if calibration else "Laser uses reported cm.\n")) +
             "Laser mounting changed around 60 cm; this curve retains the original run.\n"
             "The two y-axes use independent scales; curve intersections do not imply equal outputs.",
             fontsize=9, va="center", linespacing=1.5)
    for extension in ("png", "svg", "pdf"):
        fig.savefig(OUTPUT / f"three-sensor-characteristic.{extension}", dpi=220)
    plt.close(fig)
    with (OUTPUT / "plotted-data.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    (OUTPUT / "sources-sha256.json").write_text(json.dumps(sources, indent=2) + "\n", encoding="utf-8")
    (OUTPUT / "README.md").write_text(
        "# Three-sensor comparison\n\n"
        "![Three-sensor characteristic](three-sensor-characteristic.png)\n\n"
        "[PDF](three-sensor-characteristic.pdf) | [SVG](three-sensor-characteristic.svg) | "
        "[Plotted values](plotted-data.csv)\n\n"
        "Shared x-axis: entered distance from the sensor face. IR uses the left ADC axis; "
        "ultrasonic and laser use the right cm axis. The three colors identify the sensors. "
        "Each point is the equally weighted mean of three accepted repeat means. Vertical "
        "bars span the minimum and maximum individual valid samples across those repeats; "
        "they are not standard deviations or confidence intervals. Horizontal bars are ±1 cm "
        "(2 cm total width), the assumed tape-measure uncertainty.\n\n"
        "One completed principal session per sensor is used: 19 ultrasonic positions, "
        "16 laser positions, 15 IR positions; 1,500 accepted samples total. Rejected and "
        "incomplete attempts and separate laser follow-up measurements are not mixed into "
        "these curves. The laser's reported mounting adjustment around 60 cm remains a "
        "limitation of its original run. Ultrasonic values include the existing calibration; "
        "laser mm are converted to cm without an empirical correction. No IR distance "
        "conversion is applied. Lines connect measured points only and do not establish "
        "response outside the tested distances. Independent y-axis scales mean visual "
        "intersections do not indicate equal sensor readings.\n\n"
        "Reproduce from the last_chance folder:\n\n```powershell\n"
        ".\\.venv\\Scripts\\python.exe tools\\distance_test\\plot_comparison.py\n```\n\n"
        "Input hashes are in sources-sha256.json; archived source files remain unchanged.\n",
        encoding="utf-8",
    )
    assert len(rows) == 50 and sum(r["valid_samples"] for r in rows) == 1500
    if calibration:
        d = ARCHIVE / calibration["follow_up_session"]
        for path in [d / "session.json", *sorted(d.glob("events-*.jsonl"))]:
            sources[path.relative_to(ARCHIVE).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
        (OUTPUT / "sources-sha256.json").write_text(json.dumps(sources, indent=2) + "\n", encoding="utf-8")
        (OUTPUT / "laser-calibration.json").write_text(json.dumps(calibration, indent=2) + "\n", encoding="utf-8")
        readme = OUTPUT / "README.md"
        text = readme.read_text(encoding="utf-8").replace(
            "laser mm are converted to cm without an empirical correction.",
            "laser values use the experimental empirical correction below.").replace(
            "plot_comparison.py\n", "plot_comparison.py --calibrated-laser\n")
        text += ("\n## Experimental laser correction\n\n"
                 f"`corrected_cm = {calibration['scale']:.9f} * reported_cm "
                 f"{calibration['offset_cm']:+.9f}`\n\n"
                 "Fit by ordinary least squares to the 16 position means of the original "
                 "20-170 cm laser run, with equal weight per position. This includes the "
                 "reported mounting change near 60 cm, so treat the result as an experimental "
                 "correction for this dataset. It has not been applied to firmware.\n\n"
                 f"Training mean absolute error: {calibration['training_before_mae_cm']:.3f} cm "
                 f"before, {calibration['training_after_mae_cm']:.3f} cm after. "
                 "These same-data residuals do not establish independent accuracy.\n\n"
                 "The separate follow-up positions (50, 60, 70, 91 cm) were not used in fitting. "
                 f"Their mean absolute error is {calibration['follow_up_before_mae_cm']:.3f} cm "
                 f"before and {calibration['follow_up_after_mae_cm']:.3f} cm after. "
                 "This is a limited consistency check with the same tape reference, not "
                 "validation over the full range.\n\n"
                 "Vertical bars transform every accepted sample using the same positive "
                 "scale and offset. They show sample range only; they exclude uncertainty "
                 "in the fitted coefficients. Horizontal bars remain ±1 cm. Coefficients, "
                 "source sessions and per-position residuals are in laser-calibration.json.\n")
        readme.write_text(text, encoding="utf-8")
    print(OUTPUT / "three-sensor-characteristic.png")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--calibrated-laser", action="store_true")
    main(parser.parse_args().calibrated_laser)

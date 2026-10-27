"""Create individual raw-output figures without changing the combined figures."""
import csv
import hashlib
import json
import os

from distance_core import build_tables, load_events
from plot_comparison import ARCHIVE, ROOT, RUNS


def main():
    output = ARCHIVE / "raw-sensor-graphs"
    output.mkdir(exist_ok=True)
    os.environ.setdefault("MPLCONFIGDIR", str(ROOT / "build/distance-plot-cache"))
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.backends.backend_pdf import PdfPages

    titles = {"us": "Ultrasonic sensor — raw echo time",
              "laser": "Laser sensor — raw reported range",
              "ir": "IR sensor — raw ADC output"}
    units = {"us": "Echo pulse duration (µs)", "laser": "Reported range (mm)",
             "ir": "Output (ADC counts)"}
    sources, rows = {}, []
    with PdfPages(output / "raw-sensor-graphs.pdf") as pdf:
        for sensor, session, label, color, _, _ in RUNS:
            directory = ARCHIVE / session
            meta = json.loads((directory / "session.json").read_text(encoding="utf-8"))
            samples, _, summary = build_tables(load_events(directory), meta)
            for path in [directory / "session.json", *sorted(directory.glob("events-*.jsonl"))]:
                sources[path.relative_to(ARCHIVE).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
            selected = [p for p in summary if p["completed"] and p["three_usable_repeats"]]
            points = []
            valid = [s for s in samples if s["included"] and s["valid"]
                     and s["point_index"] in {p["point_index"] for p in selected}]
            scale = float(meta["firmware_config"]["US_SCALE"])
            offset = float(meta["firmware_config"]["US_OFFSET_CM"])
            for point in selected:
                values = [s["raw"] for s in valid if s["point_index"] == point["point_index"]]
                row = {"sensor": sensor, "session": session, "actual_cm": point["actual_cm"],
                       "x_error_minus_cm": 1, "x_error_plus_cm": 1,
                       "raw_mean": point["raw_mean"], "raw_min": min(values), "raw_max": max(values),
                       "raw_unit": {"us": "us", "laser": "mm", "ir": "ADC counts"}[sensor],
                       "valid_samples": len(values), "calculated_cm_mean": "",
                       "calculated_cm_min": "", "calculated_cm_max": ""}
                assert row["raw_min"] <= row["raw_mean"] <= row["raw_max"]
                if sensor == "us":
                    for stat in ("mean", "min", "max"):
                        row[f"calculated_cm_{stat}"] = row[f"raw_{stat}"] / 58 * scale + offset
                    assert abs(row["calculated_cm_mean"] - point["calibrated_cm_mean"]) < 1e-9
                points.append(row)
                rows.append(row)

            fig, ax = plt.subplots(figsize=(11, 7))
            fig.subplots_adjust(left=0.11, right=0.87 if sensor == "us" else 0.96,
                                bottom=0.26, top=0.88)
            ax.scatter([s["actual_cm"] for s in valid], [s["raw"] for s in valid],
                       s=12, alpha=0.22, color=color, label="Individual accepted samples", zorder=2)
            ax.errorbar([p["actual_cm"] for p in points], [p["raw_mean"] for p in points],
                        xerr=1,
                        yerr=[[p["raw_mean"]-p["raw_min"] for p in points],
                              [p["raw_max"]-p["raw_mean"] for p in points]],
                        fmt="o-", color=color, linewidth=1.6, markersize=4.5,
                        elinewidth=1, capsize=3,
                        label="Mean; vertical sample range; horizontal ±1 cm", zorder=3)
            ax.set(title=titles[sensor], xlabel="Tape-measured distance from sensor face (cm)",
                   ylabel=units[sensor], ylim=(0, None))
            ax.grid(alpha=0.22)
            ax.legend(loc="best", fontsize=9)
            notes = ("Points: equally weighted mean of 3 accepted repeats; faint dots: individual raw samples.\n"
                     "Vertical bars: minimum–maximum of accepted samples. Horizontal bars: ±1 cm (2 cm total).\n")
            if sensor == "us":
                secondary = ax.secondary_yaxis(
                    "right", functions=(lambda raw: raw / 58 * scale + offset,
                                        lambda cm: (cm-offset) / scale * 58))
                secondary.set_ylabel("Calculated distance (cm; existing calibration)")
                notes += f"Right axis: cm = (echo µs / 58) × {scale:g} + {offset:g}. The same points and bars use both scales."
            elif sensor == "laser":
                notes += "Raw device-reported millimetres; no empirical laser correction applied.\nMounting was adjusted around 60 cm in this original run; separate follow-up data are excluded."
            else:
                notes += "Raw ADC counts; no distance conversion. The ambiguous far-distance response is retained."
            fig.text(0.11, 0.11, notes, fontsize=9, va="center", linespacing=1.6)
            for extension in ("png", "svg", "pdf"):
                fig.savefig(output / f"{sensor}-raw.{extension}", dpi=220)
            pdf.savefig(fig)
            plt.close(fig)

    assert len(rows) == 50 and sum(r["valid_samples"] for r in rows) == 1500
    with (output / "plotted-data.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    (output / "sources-sha256.json").write_text(json.dumps(sources, indent=2) + "\n", encoding="utf-8")
    (output / "README.md").write_text(
        "# Individual sensor raw-output graphs\n\n"
        "The [combined graph with experimental laser calibration](../comparison-calibrated-laser/three-sensor-characteristic.png) "
        "is preserved unchanged. These additional figures show raw outputs from the same principal sessions.\n\n"
        "[All three graphs as a PDF](raw-sensor-graphs.pdf) | [Plotted data](plotted-data.csv)\n\n"
        "- [Ultrasonic PNG](us-raw.png) / [PDF](us-raw.pdf): echo time in microseconds on the left; "
        "calculated cm using the existing ultrasonic calibration on the right. This is one curve with two equivalent scales.\n"
        "- [Laser PNG](laser-raw.png) / [PDF](laser-raw.pdf): uncorrected device-reported millimetres.\n"
        "- [IR PNG](ir-raw.png) / [PDF](ir-raw.pdf): ADC counts.\n\n"
        "Points are equally weighted means of three accepted repeats. Individual accepted samples are also shown "
        "as faint dots. Vertical bars span the extrema of individual accepted samples, not confidence intervals "
        "or the range of repeat means. Horizontal bars are ±1 cm (2 cm total width). "
        "Rejected, invalid and incomplete data are excluded. The laser alignment caveat remains noted on its figure. "
        "There are 19 ultrasonic, 16 laser and 15 IR positions, with 1,500 accepted samples in total.\n\n"
        "Reproduce from last_chance:\n\n```powershell\n"
        ".\\.venv\\Scripts\\python.exe tools\\distance_test\\plot_raw_sensors.py\n```\n",
        encoding="utf-8",
    )
    print(output)


if __name__ == "__main__":
    main()

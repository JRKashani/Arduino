"""Overlay an approximate trace of the user-supplied voltage plot on IR data."""
import csv
import hashlib
import json
import os

from distance_core import build_tables, load_events
from plot_comparison import ARCHIVE, ROOT

# Manually estimated coordinates from the user-provided 0-80 cm, 0-3 V image.
# These are NOT original numerical data, a datasheet curve, or calibration data.
REFERENCE = [
    (1, .80), (2, 1.12), (3, 1.55), (4, 1.92), (5, 1.80), (6, 2.14),
    (7, 2.48), (8, 2.62), (9, 2.59), (10, 2.45), (12, 2.20), (14, 2.01),
    (16, 1.63), (18, 1.50), (20, 1.40), (22, 1.29), (25, 1.16),
    (28, 1.05), (30, .99), (35, .90), (40, .76), (45, .69), (50, .63),
    (55, .60), (60, .58), (65, .54), (70, .51), (75, .50), (80, .49),
]


def main():
    output = ARCHIVE / "raw-sensor-graphs"
    output.mkdir(exist_ok=True)
    directory = ARCHIVE / "20261011T152446_019469Z_ir"
    meta = json.loads((directory / "session.json").read_text(encoding="utf-8"))
    samples, _, summary = build_tables(load_events(directory), meta)
    points = [p for p in summary if p["completed"] and p["three_usable_repeats"]]
    valid = [s for s in samples if s["included"] and s["valid"]]
    reference_v = meta["adc_reference_v"]
    os.environ.setdefault("MPLCONFIGDIR", str(ROOT / "build/distance-plot-cache"))
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    fig, ax = plt.subplots(figsize=(11, 7.5))
    fig.subplots_adjust(left=.11, right=.89, bottom=.25, top=.89)
    # Reference first, with low contrast, so the measured samples stay in front.
    ax.plot([x for x, _ in REFERENCE], [v * 1023 / reference_v for _, v in REFERENCE],
            color="#6996cf", alpha=.62, linewidth=2.5,
            label="Reference from supplied image (approximate trace)", zorder=1)
    ax.scatter([s["actual_cm"] for s in valid], [s["raw"] for s in valid],
               color="#238443", alpha=.23, s=12, label="Individual measured samples", zorder=2)
    low, high = [], []
    for p in points:
        values = [s["raw"] for s in valid if s["point_index"] == p["point_index"]]
        low.append(min(values)); high.append(max(values))
    ax.errorbar([p["actual_cm"] for p in points], [p["raw_mean"] for p in points],
                xerr=1,
                yerr=[[p["raw_mean"] - v for p, v in zip(points, low)],
                      [v - p["raw_mean"] for p, v in zip(points, high)]],
                fmt="o-", color="#238443", markersize=4.5, linewidth=1.7,
                elinewidth=1, capsize=3, label="Measured mean and sample range; x: ±1 cm", zorder=3)
    ax.set(xlabel="Distance from sensor face (cm)", ylabel="IR output (ADC counts)",
           xlim=(0, 83), ylim=(0, 3 * 1023 / reference_v),
           title="IR measurements compared with the supplied reference curve")
    ax.set_xticks(range(0, 81, 10))
    ax.grid(alpha=.22)
    secondary = ax.secondary_yaxis("right", functions=(lambda adc: adc * reference_v / 1023,
                                                      lambda volts: volts * 1023 / reference_v))
    secondary.set_ylabel(f"Output voltage (V; {reference_v:g} V ADC reference)")
    ax.legend(loc="upper right", fontsize=9)
    fig.text(.11, .11,
             "Green: experimental data. Pale blue: approximate curve traced from the image you supplied.\n"
             f"Reference converted to ADC counts using ADC = voltage × 1023 / {reference_v:g}; no rescaling to fit the data.\n"
             "Bars: sample minimum–maximum and ±1 cm tape uncertainty; points average 3 accepted repeats.\n"
             "Reference source conditions are unknown; this is a visual comparison, not an accuracy specification.",
             fontsize=9, va="center", linespacing=1.6)
    for extension in ("png", "svg", "pdf"):
        fig.savefig(output / f"ir-with-reference.{extension}", dpi=220)
    plt.close(fig)
    with (output / "ir-reference-trace.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.writer(stream)
        writer.writerow(["distance_cm", "approximate_voltage_V", "equivalent_ADC_at_4_71V"])
        writer.writerows((x, v, v*1023/reference_v) for x, v in REFERENCE)
    sources = {p.relative_to(ARCHIVE).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
               for p in [directory / "session.json", *sorted(directory.glob("events-*.jsonl"))]}
    (output / "ir-reference-sources.json").write_text(json.dumps({
        "measurement_sources_sha256": sources,
        "reference_source": "Image supplied by the user in chat; numerical points estimated manually",
        "reference_source_conditions": "unknown",
        "reference_adc_conversion_v": reference_v,
        "reference_used_for_calibration": False,
    }, indent=2) + "\n", encoding="utf-8")
    assert len(points) == 15 and len(valid) == 450
    print(output / "ir-with-reference.png")


if __name__ == "__main__":
    main()

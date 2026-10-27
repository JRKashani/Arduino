"""Expected sensor values per track segment, from one or more sessions.

Only valid readings count (l_ok, r_ok, f_ok, d_ok, imu_ok). Percentiles show
the typical band; min/max show outliers. Segment 0 means 'not labelled'.
"""
import csv
import statistics
from pathlib import Path

from protocol import DISTANCES

CHANNELS = {**DISTANCES, "roll_deg": "imu_ok", "pitch_deg": "imu_ok",
            "ldr_l": None, "ldr_r": None}
STAT_FIELDS = ("n", "valid_pct", "mean", "std", "min", "p10", "median", "p90", "max")


def load_rows(session_dirs):
    rows = []
    for directory in session_dirs:
        path = Path(directory) / "samples.csv"
        with path.open(encoding="utf-8", newline="") as handle:
            for row in csv.DictReader(handle):
                row["session"] = Path(directory).name
                rows.append(row)
    return rows


def percentile(sorted_values, fraction):
    index = (len(sorted_values) - 1) * fraction
    low = int(index)
    high = min(low + 1, len(sorted_values) - 1)
    return sorted_values[low] + (sorted_values[high] - sorted_values[low]) * (index - low)


def channel_stats(rows, channel, flag):
    values = sorted(float(r[channel]) for r in rows if flag is None or r[flag] == "1")
    result = {"n": len(values),
              "valid_pct": round(100.0 * len(values) / len(rows), 1) if rows else 0.0}
    if not values:
        return result
    result.update(mean=statistics.mean(values), min=values[0], max=values[-1],
                  std=statistics.stdev(values) if len(values) > 1 else 0.0,
                  p10=percentile(values, 0.1), median=percentile(values, 0.5),
                  p90=percentile(values, 0.9))
    return result


def group(rows, keys):
    groups = {}
    for row in rows:
        groups.setdefault(tuple(row[k] for k in keys), []).append(row)
    return dict(sorted(groups.items(), key=lambda item: tuple(map(str, item[0]))))


def summarize(rows, keys):
    """One output row per (group, channel)."""
    table = []
    for group_key, members in group(rows, keys).items():
        for channel, flag in CHANNELS.items():
            stats = channel_stats(members, channel, flag)
            row = dict(zip(keys, group_key), channel=channel, samples=len(members))
            row.update({k: round(stats[k], 2) if k in stats else "" for k in STAT_FIELDS})
            table.append(row)
    return table


def write_table(path, table, keys):
    fields = list(keys) + ["channel", "samples"] + list(STAT_FIELDS)
    with Path(path).open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(table)


def write_summaries(session_dirs, output):
    rows = load_rows(session_dirs)
    if not rows:
        raise ValueError("No samples in the given sessions")
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    by_segment = summarize(rows, ("segment",))
    write_table(output / "by-segment.csv", by_segment, ("segment",))
    write_table(output / "by-run.csv", summarize(rows, ("session", "run", "segment")),
                ("session", "run", "segment"))
    return by_segment


def print_table(table, out=print):
    out(f"{'seg':>3} {'channel':<10} {'valid%':>6} {'p10':>7} {'median':>7} {'p90':>7} {'min':>7} {'max':>7}")
    for row in table:
        if row["n"]:
            out(f"{row['segment']:>3} {row['channel']:<10} {row['valid_pct']:>6} {row['p10']:>7}"
                f" {row['median']:>7} {row['p90']:>7} {row['min']:>7} {row['max']:>7}")

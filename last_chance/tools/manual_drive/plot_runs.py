"""One figure per run: distances, IMU and command over time, segments shaded."""
from pathlib import Path

from protocol import DISTANCES
from summary import group, load_rows

LABELS = {"l_cm": "left laser", "r_cm": "right laser", "f_cm": "front US", "d_cm": "right-45 US"}
SEGMENT_COLORS = {"1": "#e8f0fe", "2": "#fde8e8", "3": "#e6f4ea", "4": "#fef7e0", "5": "#f3e8fd"}


def series(rows, channel, flag=None):
    points = [(float(r["pc_s"]), float(r[channel])) for r in rows if flag is None or r[flag] == "1"]
    return [p[0] for p in points], [p[1] for p in points]


def shade_segments(axes, rows):
    start = 0
    for i in range(1, len(rows) + 1):
        if i == len(rows) or rows[i]["segment"] != rows[start]["segment"]:
            color = SEGMENT_COLORS.get(rows[start]["segment"])
            if color:
                for ax in axes:
                    ax.axvspan(float(rows[start]["pc_s"]), float(rows[i - 1]["pc_s"]), color=color, lw=0)
            start = i


def plot_run(plt, rows, title, path):
    fig, axes = plt.subplots(3, 1, figsize=(12, 8), sharex=True, height_ratios=(3, 1.2, 1.2))
    shade_segments(axes, rows)
    for channel, flag in DISTANCES.items():
        axes[0].plot(*series(rows, channel, flag), ".", ms=3, label=LABELS[channel])
    axes[0].set_ylabel("cm from robot center\n(right-45: raw slant)")
    axes[0].legend(loc="upper right", fontsize=8)
    axes[1].plot(*series(rows, "roll_deg", "imu_ok"), label="roll")
    axes[1].plot(*series(rows, "pitch_deg", "imu_ok"), label="pitch")
    axes[1].set_ylabel("deg")
    axes[1].legend(loc="upper right", fontsize=8)
    axes[2].plot(*series(rows, "speed"), label="speed")
    axes[2].plot(*series(rows, "steer"), label="steer (+ left)")
    axes[2].set_ylabel("command")
    axes[2].set_xlabel("session time, s (shading = labelled segment 1..5)")
    axes[2].legend(loc="upper right", fontsize=8)
    fig.suptitle(title)
    fig.tight_layout()
    fig.savefig(path, dpi=120)
    plt.close(fig)


def plot_sessions(session_dirs, output):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    written = []
    for (session, run), rows in group(load_rows(session_dirs), ("session", "run")).items():
        path = output / f"{session}-run{run}.png"
        plot_run(plt, rows, f"session {session}, run {run}", path)
        written.append(path)
    return written

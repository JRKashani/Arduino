"""Generate the IMU wiring diagram as PNG and SVG using matplotlib."""
import os
from pathlib import Path

OUTPUT = Path(__file__).resolve().parent
os.environ.setdefault("MPLCONFIGDIR", str(OUTPUT.parents[1] / "build" / "diagram-plot-cache"))
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch, Circle

fig, ax = plt.subplots(figsize=(9, 5.4))
fig.patch.set_facecolor("white")
ax.set(xlim=(0, 10), ylim=(0, 6))
ax.axis("off")
left, right = 3.15, 6.85
red, black = "#c13434", "#303030"
for x, label in [(0.35, "Arduino Mega 2560"), (6.85, "ADXL335 module")]:
    ax.add_patch(FancyBboxPatch((x, 1.05), 2.8, 3.55,
        boxstyle="round,pad=0.03,rounding_size=0.14", facecolor="#f4f6f8",
        edgecolor="#34465a", linewidth=1.6))
    ax.text(x+1.4, 4.22, label, ha="center", va="center", fontsize=12,
            weight="bold", color="#203044")

def dot(x, y, color):
    ax.add_patch(Circle((x, y), .05, color=color, zorder=4))

def label(x, y, text, align):
    ax.text(x, y, text, ha=align, va="center", fontsize=12, weight="bold")

ax.plot([.65, 9.35], [5.03, 5.03], color=red, lw=2.2)
ax.text(5, 5.23, "VCC rail: 3.3V", ha="center", fontsize=11, color=red, weight="bold")
for x, bend, y, name, align in [(left, 3.7, 3.65, "3.3V", "right"),
                               (left, 4.55, 3.18, "A5", "right"),
                               (right, 6.3, 3.65, "VCC", "left")]:
    ax.plot([x, bend, bend], [y, y, 5.03], color=red, lw=2.2)
    label(x-.18 if align == "right" else x+.18, y, name, align)
    dot(x, y, red)
    dot(bend, 5.03, red)

for lp, rp, y, color in [("A2", "X", 2.65, "#136eae"),
                          ("A1", "Y", 2.13, "#198357"),
                          ("A0", "Z", 1.61, "#9c6a10")]:
    ax.plot([left, right], [y, y], color=color, lw=2.2)
    label(left-.18, y, lp, "right")
    label(right+.18, y, rp, "left")
    dot(left, y, color)
    dot(right, y, color)

ax.plot([.65, 9.35], [.68, .68], color=black, lw=2.2)
for x in [1.7, 8.2]:
    ax.plot([x, x], [1.05, .68], color=black, lw=2.2)
    label(x, 1.25, "GND", "center")
    dot(x, .68, black)
    dot(x, 1.05, black)
ax.text(5, .38, "GND rail: direct ground connection", ha="center", fontsize=11, color=black)
ax.text(5, 5.75, "IMU wiring for the stationary tilt experiment", ha="center",
        fontsize=15, weight="bold", color="#203044")
fig.text(.5, .025, "USB powers the Mega. A5 measures the 3.3V rail; it is not AREF.\n"
         "Battery switches and charging circuits are not shown.", ha="center",
         fontsize=9, color="#394656")
fig.subplots_adjust(left=.02, right=.98, top=.98, bottom=.13)
for extension in ("png", "svg"):
    fig.savefig(OUTPUT / f"imu-wiring.{extension}", dpi=180, facecolor="white")
plt.close(fig)

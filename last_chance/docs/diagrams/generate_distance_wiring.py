"""Generate a single-Arduino distance-sensor wiring diagram."""
import os
from pathlib import Path
OUTPUT = Path(__file__).resolve().parent
os.environ.setdefault('MPLCONFIGDIR', str(OUTPUT.parents[1] / 'build/diagram-plot-cache'))
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch, Circle, Arc


def main():
    fig, ax = plt.subplots(figsize=(11, 10))
    ax.set(xlim=(0, 16), ylim=(0, 15)); ax.axis('off')
    red, black, ink = '#c13434', '#303030', '#203044'
    def wire(xs, ys, color):
        ax.plot(xs, ys, color=color, lw=2.2, zorder=2)
    def dot(x, y, color):
        ax.add_patch(Circle((x, y), .05, color=color, zorder=5))
    def text(x, y, label, size=11, bold=False, align='center', color=ink):
        ax.text(x, y, label, fontsize=size, color=color, ha=align, va='center',
                weight='bold' if bold else 'normal', zorder=6)
    def box(x, y, w, h):
        ax.add_patch(FancyBboxPatch((x, y), w, h,
                     boxstyle='round,pad=0.03,rounding_size=0.14',
                     facecolor='#f4f6f8', edgecolor='#34465a', linewidth=1.6, zorder=1))
    text(8, 14.55, 'Three distance sensors - one Arduino Mega 2560', 16, True)
    wire([.6, 15.3], [13.5, 13.5], red)
    text(8, 13.92, 'VCC rail: 5V', 12, True, color=red)
    wire([.6, 15.3], [1.75, 1.75], black)
    text(8, 1.33, 'GND rail: shared ground for Arduino and all sensors', 11, color=black)
    box(.8, 2.6, 3.8, 10)
    text(2.7, 12.03, 'Arduino Mega', 14, True)
    text(2.7, 11.57, '2560', 12)
    wire([1.6, 1.6], [12.6, 13.5], red)
    dot(1.6, 12.6, red); dot(1.6, 13.5, red)
    text(1.6, 12.95, '5V', 11, True)
    wire([2.7, 2.7], [2.6, 1.75], black)
    dot(2.7, 2.6, black); dot(2.7, 1.75, black)
    text(2.7, 2.93, 'GND', 12, True)
    supply_x, ground_x = 15.1, 14.55
    wire([ground_x, ground_x], [1.75, 9.85], black)
    dot(ground_x, 1.75, black)
    wire([supply_x, supply_x], [13.5, 4.83], red)
    dot(supply_x, 13.5, red)
    modules = [
        (9.5, 2.9, 'Front ultrasonic', 'HC-SR04', 11.78, 9.85,
         [('D43', 'TRIG', 10.9, '#136eae'), ('D42', 'ECHO', 10.25, '#198357')]),
        (6.0, 3.0, 'Right laser', 'VL53L0X', 8.38, 6.35,
         [('D20', 'SDA', 7.85, '#136eae'), ('D21', 'SCL', 7.2, '#198357'),
          ('D22', 'XSHUT', 6.55, '#9c6a10')]),
        (2.6, 2.8, 'Analog IR', 'Sharp 2Y0A21', 4.83, 2.95,
         [('A6', 'Vo / OUT', 3.95, '#136eae')]),
    ]
    for y, height, title, model, vcc_y, gnd_y, pins in modules:
        box(9.4, y, 4.2, height)
        text(11.5, y+height-.35, title, 13, True)
        text(11.5, y+height-.78, model, 11)
        for arduino_pin, sensor_pin, signal_y, color in pins:
            wire([4.6, 9.4], [signal_y, signal_y], color)
            dot(4.6, signal_y, color); dot(9.4, signal_y, color)
            text(4.39, signal_y, arduino_pin, 12, True, 'right')
            text(9.61, signal_y, sensor_pin, 11, True, 'left')
        wire([13.6, ground_x], [gnd_y, gnd_y], black)
        dot(13.6, gnd_y, black); dot(ground_x, gnd_y, black)
        text(13.4, gnd_y, 'GND', 10, True, 'right')
        if vcc_y < 9.85:
            wire([13.6, ground_x-.13], [vcc_y, vcc_y], red)
            wire([ground_x+.13, supply_x], [vcc_y, vcc_y], red)
            ax.add_patch(Circle((ground_x, vcc_y), .14, facecolor='white', edgecolor='none', zorder=3))
            ax.add_patch(Arc((ground_x, vcc_y), .26, .26, theta1=0, theta2=180,
                             color=red, linewidth=2.2, zorder=4))
        else:
            wire([13.6, supply_x], [vcc_y, vcc_y], red)
        dot(13.6, vcc_y, red); dot(supply_x, vcc_y, red)
        text(13.4, vcc_y, 'VIN / VCC' if title == 'Right laser' else 'VCC', 10, True, 'right')
    fig.text(.5, .052,
             'Dots mark electrical junctions; wire bridges are crossings without a connection.\n'
             'Connections show pin labels, not physical connector order. Laser module supply: user-confirmed 5V.\n'
             'Right laser address: 0x30. XSHUT is pulled LOW or released, not driven HIGH to 5V.\n'
             'IR: recommended bypass capacitor >=10 µF across VCC-GND near the sensor (not drawn).',
             ha='center', fontsize=9, color='#394656', linespacing=1.5)
    fig.subplots_adjust(left=.02, right=.98, bottom=.12, top=.99)
    for extension in ('png', 'svg', 'pdf'):
        fig.savefig(OUTPUT / f'distance-sensors-wiring.{extension}', dpi=180, facecolor='white')
    plt.close(fig)


if __name__ == '__main__':
    main()

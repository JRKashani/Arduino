"""Run from the project root. See docs/imu-tilt-mission.md for the procedure."""
import argparse
import math
import sys
from core import TILT_TOLERANCE_DEG


def bounded(low, high):
    def value(text):
        number = float(text)
        if not math.isfinite(number) or not low <= number <= high:
            raise argparse.ArgumentTypeError(f"Must be between {low} and {high}")
        return number
    return value


def main(argv=None):
    parser = argparse.ArgumentParser(description="Stationary ADXL335 tilt mission")
    commands = parser.add_subparsers(dest="command", required=True)
    ports = commands.add_parser("ports", help="List available serial ports")
    record = commands.add_parser("record", help="Guide stationary trials and preserve all raw records")
    record.add_argument("--port", required=True, help="Mega serial port, e.g. COM5")
    output = record.add_mutually_exclusive_group()
    output.add_argument("--output", help="New session directory (must not exist)")
    output.add_argument("--resume", help="Existing real session; skip accepted placements")
    record.add_argument("--surfaces", type=int, choices=range(1, 21), default=5, metavar="1..20")
    record.add_argument("--extend-surfaces", type=int, choices=range(1, 21), metavar="1..20",
                        help="With --resume, extend the saved plan to this total surface count")
    record.add_argument("--settle", type=bounded(0, 60), default=3, help="Settling seconds (default 3)")
    record.add_argument("--duration", type=bounded(1, 60), default=10, help="Recording seconds (default 10)")
    record.add_argument("--hz", type=bounded(1, 50), default=50, help="Requested samples/second (default 50)")
    record.add_argument("--diagonal", action="store_true", help="Add a 45-degree placement on each surface")
    record.add_argument("--calibration-info", help="Calibration provenance JSON to preserve in a new session")
    analyze = commands.add_parser("analyze", help="Create new CSV reports, plots and calibration proposal")
    analyze.add_argument("session")
    analyze.add_argument("--phone-convention", choices=("unverified", "axis-elevation", "euler", "gradient"), default="unverified")
    analyze.add_argument("--up-sign", type=int, choices=(-1, 1), default=1, help="Multiplier so top edge HIGH is positive")
    analyze.add_argument("--right-sign", type=int, choices=(-1, 1), default=1, help="Multiplier so right edge HIGH is positive")
    analyze.add_argument("--sensor-map", default="x,y,z", help="Sensor axes for body nose/right/up, e.g. x,-y,z")
    axes = analyze.add_mutually_exclusive_group()
    axes.add_argument("--axes-verified", action="store_true", help="Confirm phone signs and sensor map were checked physically")
    axes.add_argument("--axes-inferred", action="store_true", help="Explore an inferred mapping; comparisons and fits remain provisional")
    for axis in ("roll", "pitch", "yaw"):
        analyze.add_argument("--mount-"+axis, type=bounded(-180, 180), default=0,
                             help="Known mounting correction in degrees (default 0)")
    analyze.add_argument("--max-std", type=bounded(0.001, 20), default=TILT_TOLERANCE_DEG, help="Capture standard deviation limit in degrees (default 3)")
    analyze.add_argument("--max-drift", type=bounded(0.001, 20), default=TILT_TOLERANCE_DEG)
    analyze.add_argument("--max-reference-disagreement", type=bounded(0.001, 20), default=TILT_TOLERANCE_DEG)
    analyze.add_argument("--minimum-span", type=bounded(0.01, 2), default=0.2, help="Minimum axis gravity span for a new scale")
    analyze.add_argument("--no-plots", action="store_true", help="Write CSV/JSON without matplotlib")
    demo = commands.add_parser("demo", help="Generate a marked SIMULATED session; no serial connection")
    demo.add_argument("--output", required=True, help="New directory for synthetic data")
    args = parser.parse_args(argv)
    if args.command == "record" and args.extend_surfaces is not None and not args.resume:
        parser.error("--extend-surfaces requires --resume")
    try:
        if args.command == "ports":
            from serial.tools import list_ports
            for port in list_ports.comports():
                print(f"{port.device}: {port.description}")
        elif args.command == "record":
            from recording import record
            record(args)
        elif args.command == "analyze":
            from analysis import analyze
            analyze(args)
        else:
            from demo import generate
            generate(args.output)
    except KeyboardInterrupt:
        print("\nStopped. Existing session records are retained; use --resume to continue.", file=sys.stderr)
        return 130
    except (OSError, ValueError, RuntimeError, ImportError) as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

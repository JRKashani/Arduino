"""Drive the robot from the keyboard while recording every sensor update.

Run from the last_chance folder (see docs/manual-drive.md):
  .venv\\Scripts\\python.exe -u tools\\manual_drive\\manual_drive.py record --port COM7
  .venv\\Scripts\\python.exe tools\\manual_drive\\manual_drive.py summarize manual_sessions\\<session> ...
"""
import argparse
import sys
from datetime import datetime, timezone
from pathlib import Path

DEFAULT_ROOT = "manual_sessions"


def record(args):
    import serial
    from keyboard import read_keys
    from link import Link, fields_warning
    from recorder import Recorder
    from session import Session, new_session_dir, utc_now
    from teleop import HELP

    session = Session(new_session_dir(args.root))
    print(f"Session: {session.path}")
    print("Place the robot FLAT and STILL: connecting resets the board and zeroes the IMU.")
    try:
        with serial.Serial(args.port, 115200, timeout=0) as port:
            link = Link(port, session)
            version, config, fields, boot = link.connect()
            session.write_metadata({"started_utc": utc_now(), "port": args.port, "note": args.note,
                                    "firmware": version, "config": config, "fields": fields,
                                    "boot_lines": boot})
            session.open_samples(fields)
            for line in boot:
                print("robot:", line)
            warning = fields_warning(fields)
            if warning:
                print("WARNING:", warning)
            print(HELP)
            print("\nRun 1. Press 1 when the robot is in segment 1.")
            Recorder(link, session, fields, read_keys).loop()
    finally:
        session.close()
    print(f"Saved: {session.path}")


def output_dir(args):
    if args.output:
        return Path(args.output)
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    return Path(args.sessions[0]) / "analysis" / stamp


def summarize(args):
    from summary import print_table, write_summaries
    out = output_dir(args)
    print_table(write_summaries(args.sessions, out))
    print(f"\nWritten: {out}")


def plot(args):
    from plot_runs import plot_sessions
    for path in plot_sessions(args.sessions, output_dir(args)):
        print("Written:", path)


def ports(_args):
    from serial.tools import list_ports
    for port in list_ports.comports():
        print(f"{port.device}: {port.description}")


def parser():
    p = argparse.ArgumentParser(description="Manual drive logger")
    sub = p.add_subparsers(dest="command", required=True)
    sub.add_parser("ports", help="List serial ports").set_defaults(func=ports)
    rec = sub.add_parser("record", help="Drive with the keyboard and record")
    rec.add_argument("--port", required=True, help="Mega serial port, e.g. COM7")
    rec.add_argument("--note", default="", help="Free text saved with the session (track layout etc.)")
    rec.add_argument("--root", default=DEFAULT_ROOT, help="Folder for new sessions")
    rec.set_defaults(func=record)
    for name, func, text in (("summarize", summarize, "Per-segment expected values (CSV)"),
                             ("plot", plot, "One PNG per run")):
        cmd = sub.add_parser(name, help=text)
        cmd.add_argument("sessions", nargs="+", help="Session folders (combined)")
        cmd.add_argument("--output", help="Output folder (default: <first session>/analysis/<UTC>)")
        cmd.set_defaults(func=func)
    return p


def main(argv=None):
    args = parser().parse_args(argv)
    try:
        args.func(args)
    except KeyboardInterrupt:
        print("\nStopped. Recorded data is kept.", file=sys.stderr)
        return 130
    except (OSError, ValueError, RuntimeError, ImportError) as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

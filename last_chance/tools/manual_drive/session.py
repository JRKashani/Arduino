"""Session files: everything is appended and flushed as it arrives.

  session.json   start time, port, firmware identity/config, note
  serial.log     every line sent (>) and received (<), with UTC time
  events.jsonl   keys, runs, segments, marks and firmware EVT lines
  samples.csv    one row per firmware S record, tagged with run and segment
"""
import csv
import json
import time
from datetime import datetime, timezone
from pathlib import Path

TAG_FIELDS = ("run", "segment", "pc_s")


def utc_now():
    return datetime.now(timezone.utc).isoformat()


def new_session_dir(root):
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    path = Path(root) / stamp
    path.mkdir(parents=True, exist_ok=False)
    return path


class Session:
    def __init__(self, directory):
        self.path = Path(directory)
        self.start = time.monotonic()
        self.serial = (self.path / "serial.log").open("x", encoding="utf-8")
        self.events = (self.path / "events.jsonl").open("x", encoding="utf-8")
        self.samples_file = None
        self.writer = None
        self.fields = None

    def elapsed(self):
        return round(time.monotonic() - self.start, 3)

    def wire(self, direction, line):
        self.serial.write(f"{utc_now()} {direction} {line}\n")
        self.serial.flush()

    def event(self, kind, **values):
        record = {"kind": kind, "utc": utc_now(), "pc_s": self.elapsed(), **values}
        self.events.write(json.dumps(record, allow_nan=False) + "\n")
        self.events.flush()

    def write_metadata(self, metadata):
        text = json.dumps(metadata, indent=2, allow_nan=False)
        (self.path / "session.json").write_text(text + "\n", encoding="utf-8")

    def open_samples(self, fields):
        self.fields = tuple(fields)
        self.samples_file = (self.path / "samples.csv").open("x", encoding="utf-8", newline="")
        self.writer = csv.writer(self.samples_file)
        self.writer.writerow(TAG_FIELDS + self.fields)

    def sample(self, run, segment, values):
        self.writer.writerow((run, segment, self.elapsed()) + tuple(values[f] for f in self.fields))
        self.samples_file.flush()

    def close(self):
        for handle in (self.samples_file, self.events, self.serial):
            if handle:
                handle.close()

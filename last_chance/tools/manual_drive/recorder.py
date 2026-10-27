"""Interactive loop: keys -> drive commands, serial lines -> session files."""
import time

from protocol import parse_event, parse_sample
from teleop import HELP, DriveState, apply_key, drive_command

HEARTBEAT_S = 0.15   # firmware watchdog stops the robot after 0.6 s of silence
STATUS_S = 0.25


def show(value, ok):
    return f"{value:5.1f}" if ok else "  -- "


def status_text(rec):
    s = rec.last
    text = (f"run {rec.run} seg {rec.segment} | spd {rec.state.speed:4d} str {rec.state.steer:4d}"
            f" | safety {'ON ' if rec.safety else 'OFF'}")
    if s:
        text += (f" | L {show(s['l_cm'], s['l_ok'])} R {show(s['r_cm'], s['r_ok'])}"
                 f" F {show(s['f_cm'], s['f_ok'])} D {show(s['d_cm'], s['d_ok'])}"
                 f" | roll {s['roll_deg']:5.1f} pitch {s['pitch_deg']:5.1f}")
        if s["blocked"]:
            text += " | FRONT BLOCKED"
    return text


class Recorder:
    def __init__(self, link, session, fields, read_keys, out=print, clock=time.monotonic):
        self.link, self.session, self.fields = link, session, fields
        self.read_keys, self.out, self.clock = read_keys, out, clock
        self.state, self.run, self.segment = DriveState(), 1, 0
        self.safety, self.marks, self.bad_lines, self.samples = True, 0, 0, 0
        self.last, self.done = None, False
        self.next_beat = self.next_status = 0.0

    # --- keys -------------------------------------------------------------
    def handle_key(self, key):
        self.state, action = apply_key(self.state, key)
        if action == "drive":
            self.session.event("drive", key=key, speed=self.state.speed, steer=self.state.steer)
            self.beat()
        elif action and action.startswith("segment:"):
            self.set_segment(int(action.split(":")[1]))
        elif action:
            getattr(self, "do_" + action)()

    def set_segment(self, segment):
        self.segment = segment
        self.session.event("segment", run=self.run, segment=segment)
        self.message(f"segment {segment}")

    def do_mark(self):
        self.marks += 1
        self.session.event("mark", run=self.run, segment=self.segment, mark=self.marks)
        self.message(f"mark {self.marks}")

    def do_new_run(self):
        self.stop()
        self.run += 1
        self.segment = 0
        self.session.event("new_run", run=self.run)
        self.message(f"run {self.run}: stopped; press 1 when you start segment 1")

    def do_toggle_safety(self):
        self.safety = not self.safety
        self.link.send("SAFETY,ON" if self.safety else "SAFETY,OFF")
        self.session.event("safety", on=self.safety)

    def do_help(self):
        self.message(HELP)

    def do_quit(self):
        self.stop()
        self.done = True

    def stop(self):
        self.state = DriveState()
        self.link.send("STOP")
        self.beat()

    # --- serial -----------------------------------------------------------
    def handle_line(self, line):
        try:
            if line.startswith("S,"):
                self.last = parse_sample(line, self.fields)
                self.session.sample(self.run, self.segment, self.last)
                self.samples += 1
            elif line.startswith("EVT,"):
                t_ms, name = parse_event(line)
                self.session.event("firmware", name=name, t_ms=t_ms, run=self.run)
                if name != "STOP":
                    self.message(f"robot: {name}")
            elif line.startswith("BOOT,"):
                self.on_reset()
            elif line.startswith("ERROR,"):
                self.session.event("firmware_error", line=line)
                self.message("robot: " + line)
        except ValueError as exc:
            self.bad_lines += 1
            self.session.event("bad_line", line=line, error=str(exc))

    def on_reset(self):
        self.state, self.safety = DriveState(), True
        self.session.event("board_reset", run=self.run)
        self.message("BOARD RESET (power/USB?). Stopped; IMU re-zeroed at the current pose.")

    # --- timing -----------------------------------------------------------
    def beat(self):
        self.link.send(drive_command(self.state))
        self.next_beat = self.clock() + HEARTBEAT_S

    def tick(self):
        for key in self.read_keys():
            self.handle_key(key)
        for line in self.link.lines():
            self.handle_line(line)
        now = self.clock()
        if now >= self.next_beat:
            self.beat()
        if now >= self.next_status:
            self.next_status = now + STATUS_S
            self.out("\r" + status_text(self).ljust(118), end="", flush=True)

    def message(self, text):
        self.out("\n" + text, flush=True)

    def loop(self, sleep=time.sleep):
        try:
            while not self.done:
                self.tick()
                sleep(0.01)
        finally:
            self.state = DriveState()
            try:
                for _ in range(3):
                    self.link.send("STOP")
            except OSError:
                pass  # link gone: the firmware watchdog stops the motors
            self.session.event("end", samples=self.samples, bad_lines=self.bad_lines, runs=self.run)
            self.out("", flush=True)

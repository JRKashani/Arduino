"""Keyboard -> drive state. Pure logic: no serial, no terminal."""
from dataclasses import dataclass, replace

SPEED_STEP = 10
START_SPEED = 40   # below ~30 the motors stall, so leave 0 straight to +-40
MAX_SPEED = 100
STEER_STEP = 5
MAX_STEER = 60     # positive = left, as in the firmware

HELP = """\
Keys (no Enter needed):
  W / Up      faster (0 -> 40, then +10)      S / Down   slower / reverse
  A / Left    steer more left (+5)            D / Right  steer more right (-5)
  C           straighten steering             Space      STOP (speed and steer 0)
  1..5        track segment you are in now    0          no segment
  M           mark this moment                N          new run (stops first)
  G           toggle front safety stop        H / ?      this help
  Q / Esc     stop and finish the session"""


@dataclass(frozen=True)
class DriveState:
    speed: int = 0
    steer: int = 0


def clamp(value, limit):
    return max(-limit, min(limit, value))


def faster(speed):
    if speed == 0:
        return START_SPEED
    if -START_SPEED <= speed < 0:
        return 0
    return clamp(speed + SPEED_STEP, MAX_SPEED)


def slower(speed):
    if speed == 0:
        return -START_SPEED
    if 0 < speed <= START_SPEED:
        return 0
    return clamp(speed - SPEED_STEP, MAX_SPEED)


DRIVE_KEYS = {
    "w": lambda s: replace(s, speed=faster(s.speed)),
    "s": lambda s: replace(s, speed=slower(s.speed)),
    "a": lambda s: replace(s, steer=clamp(s.steer + STEER_STEP, MAX_STEER)),
    "d": lambda s: replace(s, steer=clamp(s.steer - STEER_STEP, MAX_STEER)),
    "c": lambda s: replace(s, steer=0),
    " ": lambda s: DriveState(),
}
ARROWS = {"up": "w", "down": "s", "left": "a", "right": "d"}
ACTIONS = {"m": "mark", "n": "new_run", "g": "toggle_safety",
           "h": "help", "?": "help", "q": "quit", "esc": "quit"}


def apply_key(state, key):
    """Return (new_state, action). action: None, 'drive', 'segment:<n>', or ACTIONS."""
    key = ARROWS.get(key, key.lower() if len(key) == 1 else key)
    if key in DRIVE_KEYS:
        return DRIVE_KEYS[key](state), "drive"
    if len(key) == 1 and key in "012345":
        return state, "segment:" + key
    return state, ACTIONS.get(key)


def drive_command(state):
    return f"D,{state.speed},{state.steer}"

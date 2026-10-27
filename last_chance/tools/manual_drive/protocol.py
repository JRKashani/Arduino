"""manual_drive firmware line protocol (robot -> PC). Pure functions, no I/O."""
import math

PROTOCOL = "1"
# Order sent by firmware 1.0.0 in its FIELDS line. The firmware's own FIELDS
# line is authoritative; this copy is only used to warn about a mismatch.
EXPECTED_FIELDS = (
    "t_ms", "speed", "steer", "blocked",
    "l_cm", "l_ok", "l_raw_mm", "r_cm", "r_ok", "r_raw_mm",
    "f_cm", "f_ok", "f_sample_cm", "d_cm", "d_ok", "d_sample_cm", "us_new",
    "ldr_l", "ldr_r", "imu_ok", "roll_deg", "pitch_deg", "ax_g", "ay_g", "az_g",
)
INT_FIELDS = {"t_ms", "speed", "steer", "blocked", "l_ok", "l_raw_mm", "r_ok",
              "r_raw_mm", "f_ok", "d_ok", "ldr_l", "ldr_r", "imu_ok"}
TEXT_FIELDS = {"us_new"}
# Distance channels and the validity flag that gates each one.
DISTANCES = {"l_cm": "l_ok", "r_cm": "r_ok", "f_cm": "f_ok", "d_cm": "d_ok"}


def parse_fields(line):
    if not line.startswith("FIELDS,"):
        raise ValueError("Not a FIELDS line")
    fields = tuple(line[len("FIELDS,"):].split(","))
    if len(set(fields)) != len(fields) or "t_ms" not in fields:
        raise ValueError("Invalid FIELDS line")
    return fields


def parse_config(line):
    """'CONFIG,NAME,value' -> (name, float)."""
    parts = line.split(",")
    if len(parts) != 3 or parts[0] != "CONFIG":
        raise ValueError("Malformed CONFIG line")
    value = float(parts[2])
    if not math.isfinite(value):
        raise ValueError("Nonfinite CONFIG value")
    return parts[1], value


def parse_sample(line, fields):
    """'S,...' -> dict keyed by fields. Raises ValueError on any mismatch."""
    values = line.split(",")
    if values[0] != "S" or len(values) != len(fields) + 1:
        raise ValueError("Malformed S record")
    return {name: convert(name, text) for name, text in zip(fields, values[1:])}


def convert(name, text):
    if name in TEXT_FIELDS:
        return text
    if name in INT_FIELDS:
        return int(text)
    value = float(text)
    if not math.isfinite(value):
        raise ValueError(f"Nonfinite {name}")
    return value


def parse_event(line):
    """'EVT,<t_ms>,<NAME>' -> (t_ms, name)."""
    parts = line.split(",")
    if len(parts) != 3 or parts[0] != "EVT":
        raise ValueError("Malformed EVT line")
    return int(parts[1]), parts[2]

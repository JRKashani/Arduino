"""Serial connection to the manual_drive firmware."""
import time

from protocol import EXPECTED_FIELDS, PROTOCOL, parse_config, parse_fields

BOOT_TIMEOUT_S = 20   # laser bring-up + 1 s IMU baseline after the USB reset
MAX_LINE = 512


class Link:
    def __init__(self, port, session):
        self.port, self.session = port, session
        self.buffer = bytearray()

    def send(self, line):
        self.session.wire(">", line)
        self.port.write((line + "\n").encode("ascii"))

    def lines(self):
        """All complete lines received so far (non-blocking)."""
        waiting = self.port.in_waiting
        if waiting:
            self.buffer.extend(self.port.read(waiting))
        result = []
        while b"\n" in self.buffer:
            raw, _, rest = self.buffer.partition(b"\n")
            self.buffer = bytearray(rest)
            line = raw.decode("ascii", errors="replace").strip()
            if line:
                self.session.wire("<", line)
                result.append(line)
        if len(self.buffer) > MAX_LINE:
            raise RuntimeError("Oversized serial record; wrong firmware or baud rate")
        return result

    def connect(self):
        """Wait for the boot banner. Returns (version, config, fields, boot_lines)."""
        version, config, fields, boot = None, {}, None, []
        deadline = time.monotonic() + BOOT_TIMEOUT_S
        while time.monotonic() < deadline:
            for line in self.lines():
                if line.startswith("MANUAL,"):
                    parts = line.split(",")
                    if len(parts) != 3 or parts[1] != PROTOCOL:
                        raise RuntimeError("Unsupported manual_drive protocol: " + line)
                    version = parts[2]
                elif line.startswith("CONFIG,"):
                    name, value = parse_config(line)
                    config[name] = value
                elif line.startswith("FIELDS,"):
                    fields = parse_fields(line)
                elif line == "READY" and version and fields:
                    return version, config, fields, boot
                elif not line.startswith("S,"):
                    boot.append(line)   # SENS/IMU messages, baseline, errors
            time.sleep(0.02)
        raise RuntimeError("No manual_drive firmware response. Upload env manual_drive "
                           "and close other serial monitors.")


def fields_warning(fields):
    if tuple(fields) != EXPECTED_FIELDS:
        return "Firmware FIELDS differ from this tool's copy; recording with the firmware's list."
    return None

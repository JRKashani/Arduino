"""Guided stationary distance characterization. See docs/distance-test.md."""
import argparse
import json
import random
import os
import sys
import time
import uuid
from datetime import datetime, timezone
from pathlib import Path

from distance_core import (SENSORS, convert, finite_number, load_events,
                           parse_sample, stats)
from distance_export import export_session

CAPTURE_SETTINGS = {"samples_per_capture": 10, "period_ms": 500, "settle_ms": 500}
ACTIVE_CONSOLE = None


def utc_now():
    return datetime.now(timezone.utc).isoformat()


def stamp():
    return datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S_%fZ")


class StopRecording(Exception):
    pass


def ask(prompt):
    result = (ACTIVE_CONSOLE.input(prompt) if ACTIVE_CONSOLE else input(prompt)).strip()
    if result.lower() == "q":
        raise StopRecording()
    return result


def ask_number(prompt, low, high, allow_skip=False):
    while True:
        text = ask(prompt)
        if allow_skip and text.lower() == "s":
            return None
        try:
            return finite_number(text, low, high)
        except ValueError as error:
            print(error)


def another_distance():
    while True:
        answer = ask("Add another measured distance? [Y/n]: ").lower()
        if answer in ("", "y", "yes"):
            return True
        if answer in ("n", "no"):
            return False
        print("Enter y to add a distance, n to finish, or q to save and quit.")


class Log:
    def __init__(self, directory):
        self.boot = stamp() + "_" + uuid.uuid4().hex[:8]
        self.events = (directory / f"events-{self.boot}.jsonl").open("x", encoding="utf-8")
        self.serial = (directory / f"serial-{self.boot}.log").open("x", encoding="utf-8")

    def write(self, kind, **values):
        self.events.write(json.dumps({"kind": kind, "utc": utc_now(), **values}, allow_nan=False) + "\n")
        self.events.flush()

    def wire(self, direction, line):
        self.serial.write(f"{utc_now()} {direction} {line}\n")
        self.serial.flush()

    def close(self):
        self.events.close()
        self.serial.close()


class Device:
    def __init__(self, port, log):
        self.port, self.log = port, log
        self.buffer = bytearray()
        self.live_callback = None

    def send(self, line):
        self.log.wire(">", line)
        self.port.write((line + "\n").encode("ascii"))
        self.port.flush()

    def read(self):
        chunk = self.port.read_until(b"\n")
        if not chunk:
            return None
        self.buffer.extend(chunk)
        if len(self.buffer) > 512:
            raise RuntimeError("Serial record too long; check firmware and connection")
        if not self.buffer.endswith(b"\n"):
            return None
        line = self.buffer.decode("ascii").strip()
        self.buffer.clear()
        self.log.wire("<", line)
        if line.startswith("LIVE,"):
            if self.live_callback:
                self.live_callback(line)
            return None
        return line

    def enable_monitor(self, sensor):
        code = SENSORS[sensor]["code"]
        self.send(f"MONITOR,{code}")
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            line = self.read()
            if line == f"MONITOR,{code},500":
                return
            if line and (line.startswith("ERROR,") or line.startswith("BOOT,")):
                raise RuntimeError("Live monitor could not start. Upload the current distance_test firmware; " + line)
        raise RuntimeError("Live monitor did not respond. Upload the current distance_test firmware.")

    def connect(self):
        time.sleep(2)
        self.port.reset_input_buffer()
        self.send("ABORT")
        self.send("INFO")
        config, version = {}, None
        deadline = time.monotonic() + 8
        while time.monotonic() < deadline:
            line = self.read()
            if not line:
                continue
            if line.startswith("DISTANCE,"):
                fields = line.split(",")
                if len(fields) != 3 or fields[1] != "1":
                    raise RuntimeError("Unsupported distance firmware version")
                version = fields[2]
            elif line.startswith("CONFIG,"):
                _, key, value = line.split(",", 2)
                config[key] = value
            elif line == "READY" and version:
                required = {"US_SCALE", "US_OFFSET_CM", "US_TIMEOUT_US", "IR_PIN",
                            "IR_MODEL", "ADC_REFERENCE", "LASER_PROFILE", "DISTANCE_ORIGIN"}
                if not required <= config.keys() or config["DISTANCE_ORIGIN"] != "sensor_face":
                    raise RuntimeError("Incomplete/incompatible sensor configuration")
                finite_number(config["US_SCALE"], 0.1, 10)
                finite_number(config["US_OFFSET_CM"], -100, 100)
                return version, config
        raise RuntimeError("No distance-test firmware response. Upload environment distance_test and close other serial monitors.")

    def capture(self, cid, sensor, count, period, settle):
        code = SENSORS[sensor]["code"]
        self.send(f"START,{cid},{code},{count},{period},{settle}")
        samples, began = [], False
        deadline = time.monotonic() + settle / 1000 + count * (period / 1000 + 0.6) + 5
        while time.monotonic() < deadline:
            line = self.read()
            if not line:
                continue
            if line.startswith("BOOT,"):
                raise RuntimeError("Board reset during capture. Partial data saved; resume after checking power/connection.")
            if line.startswith("ERROR,"):
                raise RuntimeError("Board reported: " + line[6:])
            if line.startswith("BEGIN,"):
                if began or line != f"BEGIN,{cid},{code},{count}":
                    raise RuntimeError("Unexpected capture acknowledgement")
                began = True
            elif line.startswith("DATA,"):
                if not began or len(samples) >= count:
                    raise RuntimeError("Unexpected sample outside capture")
                sample = parse_sample(line, cid, len(samples), sensor)
                self.log.write("sample", **sample)
                samples.append(sample)
            elif line.startswith("END,"):
                if not began or line != f"END,{cid},{count},complete" or len(samples) != count:
                    raise RuntimeError("Capture incomplete; source data saved")
                self.log.write("capture_complete", capture_id=cid)
                return samples
        raise RuntimeError("Capture timed out; partial data saved")


def live_text(line, meta):
    fields = line.split(",")
    if len(fields) < 7 or fields[1] != SENSORS[meta["sensor"]]["code"]:
        return None
    sample = parse_sample("DATA,1,0," + ",".join(fields[2:7]), 1, 0, meta["sensor"])
    prefix = f"LIVE {meta['sensor'].upper()} t={sample['timestamp_ms']} ms | raw={sample['raw']} {SENSORS[meta['sensor']]['raw_unit']}"
    if not sample["valid"]:
        return prefix + f" | {sample['status']}"
    converted = convert(sample, meta)
    if meta["sensor"] == "us":
        value = f"raw={converted['raw_cm']:.2f} cm | corrected={converted['calibrated_cm']:.2f} cm"
    elif meta["sensor"] == "laser":
        value = f"distance={converted['calibrated_cm']:.2f} cm"
    else:
        value = f"output={converted['voltage']:.3f} V ({meta['adc_reference_source']} reference)"
    return prefix + " | " + value + " | " + sample["status"]


class LiveConsole:
    """Drain serial while editing a prompt, retaining the user's typed text."""
    def __init__(self, device, meta):
        self.device, self.meta = device, meta
        self.prompt, self.typed = None, ""

    def display(self, line):
        text = live_text(line, self.meta)
        if text is None:
            return
        if self.prompt is not None:
            sys.stdout.write("\r" + " " * (len(self.prompt) + len(self.typed)) + "\r")
        print(text, flush=True)
        if self.prompt is not None:
            sys.stdout.write(self.prompt + self.typed)
            sys.stdout.flush()

    def input(self, prompt):
        if not sys.stdin.isatty():
            return input(prompt)
        self.prompt, self.typed = prompt, ""
        sys.stdout.write(prompt)
        sys.stdout.flush()
        try:
            if os.name == "nt":
                import msvcrt
                while True:
                    if msvcrt.kbhit():
                        char = msvcrt.getwch()
                        if char in ("\x00", "\xe0"):
                            msvcrt.getwch()  # extended arrow/function key
                        elif char in ("\r", "\n"):
                            print()
                            return self.typed
                        elif char == "\x03":
                            raise KeyboardInterrupt()
                        elif char in ("\x04", "\x1a"):
                            raise EOFError()
                        elif char == "\b":
                            if self.typed:
                                self.typed = self.typed[:-1]
                                sys.stdout.write("\b \b")
                                sys.stdout.flush()
                        elif char >= " ":
                            self.typed += char
                            sys.stdout.write(char)
                            sys.stdout.flush()
                    self.poll()
                    time.sleep(0.01)
            else:
                import select
                while True:
                    if select.select([sys.stdin], [], [], 0.01)[0]:
                        result = sys.stdin.readline()
                        if not result:
                            raise EOFError()
                        return result.rstrip("\n")
                    self.poll()
        finally:
            self.prompt = None

    def poll(self):
        if self.device.buffer or self.device.port.in_waiting:
            line = self.device.read()
            if line and (line.startswith("BOOT,") or line.startswith("ERROR,")):
                raise RuntimeError("Board stopped while monitoring: " + line)


def record(args):
    global ACTIVE_CONSOLE
    import serial
    if args.resume:
        directory = Path(args.resume)
        meta = json.loads((directory / "session.json").read_text(encoding="utf-8"))
        if meta.get("simulated"):
            raise ValueError("Cannot resume a simulated session on hardware")
        if args.sensor and args.sensor != meta["sensor"]:
            raise ValueError("Resume must use the session's original sensor")
    else:
        sensor = args.sensor or "us"
        directory = Path(args.output or f"distance_sessions/{stamp()}_{sensor}")
        directory.mkdir(parents=True, exist_ok=False)
        meta = {"schema": 2, "created_utc": utc_now(), "sensor": sensor,
                "sensor_description": SENSORS[sensor]["name"], "simulated": False,
                "collection_mode": "open_ended", "repeats": args.repeats, **CAPTURE_SETTINGS,
                "distance_origin": "sensor_face",
                "adc_reference_v": 5.0, "adc_reference_source": "nominal",
                "firmware_config": None}
        (directory / "session.json").write_text(json.dumps(meta, indent=2), encoding="utf-8")
    print(f"\nSession: {directory.resolve()}")
    print(f"Sensor: {SENSORS[meta['sensor']]['name']}. Motors remain off.")
    print("Enter actual tape distance in cm from the SENSOR FACE to a flat target.")
    print("Choose any number of distances. After each position, add another or finish.")
    print("Live output every 0.5 s; each repeat records ten readings over 5 s.")
    print("Type q at any prompt to save and quit. Ctrl+C also saves the source records.")
    log = Log(directory)
    port, device = None, None
    try:
        port = serial.Serial(args.port, 115200, timeout=0.2, write_timeout=2)
        device = Device(port, log)
        version, config = device.connect()
        log.write("connection", firmware_version=version, firmware_config=config)
        if meta["firmware_config"] is not None and meta["firmware_config"] != config:
            raise RuntimeError("Firmware configuration changed; start a new session to preserve calibration provenance")
        new_setup = meta["firmware_config"] is None
        meta["firmware_config"] = config
        meta.update(CAPTURE_SETTINGS)
        console = LiveConsole(device, meta)
        device.live_callback = console.display
        device.enable_monitor(meta["sensor"])
        ACTIVE_CONSOLE = console
        if new_setup:
            meta["firmware_config"], meta["firmware_version"] = config, version
            if meta["sensor"] == "ir":
                print("IR: output to A6, supply to 5 V, common GND. Specified distance: 10-80 cm.")
                while True:
                    entered = ask("Measured Arduino 5V rail (V), or Enter for nominal 5.0: ")
                    try:
                        if entered:
                            meta["adc_reference_v"] = finite_number(entered, 4.0, 5.5)
                            meta["adc_reference_source"] = "user_measured_5V_rail"
                        break
                    except ValueError as error:
                        print(error)
            meta["setup_notes"] = ask("Target, lighting and power/setup notes (optional): ")
            (directory / "session.json").write_text(json.dumps(meta, indent=2), encoding="utf-8")
        else:
            print("Resuming with the saved capture settings. Keep the same target and power setup.")
        log.write("collection_mode", mode="open_ended")
        log.write("capture_settings", **CAPTURE_SETTINGS)
        events = load_events(directory)
        done_ids = {e["point_id"] for e in events if e["kind"] == "point_done"}
        skipped_indices = {e["point_index"] for e in events if e["kind"] == "point_skipped"}
        latest_points = {e["point_index"]: e for e in events if e["kind"] == "point"}
        unfinished = [p for i, p in sorted(latest_points.items())
                      if p["point_id"] not in done_ids and i not in skipped_indices]
        highest_index = max([*latest_points, *skipped_indices], default=0)
        index = unfinished[0]["point_index"] if unfinished else highest_index + 1
        next_index = max(highest_index + 1, index + 1)
        offer_another = highest_index > 0 and not unfinished
        capture_id = max((e["capture_id"] for e in events if e["kind"] == "capture"), default=0)
        while True:
            if offer_another and not another_distance():
                break
            print(f"\nMeasurement position {index}.")
            if index in latest_points:
                print("This point was interrupted. Remeasure and repeat all captures; earlier attempts remain saved and excluded.")
            actual = ask_number("ACTUAL measured distance (cm): ", 1, 400)
            point_id = uuid.uuid4().hex
            log.write("point", point_id=point_id, point_index=index, target_cm=actual, actual_cm=actual)
            if meta["sensor"] == "ir" and not 10 <= actual <= 80:
                print("Outside the IR's 10-80 cm specification: recording its raw response for comparison.")
            if meta["sensor"] == "us" and not 15 <= actual <= 150:
                print("Existing US correction was measured over 15-150 cm; corrected values here are extrapolated.")
            for repeat in range(1, meta["repeats"] + 1):
                while True:
                    print(f"  Repeat {repeat}/{meta['repeats']} at measured {actual:g} cm.")
                    while ask("  Press Enter when still to capture (no distance here): "):
                        print("  Press Enter only, or q to save and quit.")
                    capture_id += 1
                    log.write("capture", capture_id=capture_id, point_id=point_id, repeat=repeat, **CAPTURE_SETTINGS)
                    samples = device.capture(capture_id, meta["sensor"], meta["samples_per_capture"], meta["period_ms"], meta["settle_ms"])
                    valid = [s for s in samples if s["valid"]]
                    raw = stats([s["raw"] for s in valid])
                    print(f"  {len(valid)}/{len(samples)} usable readings.")
                    if valid:
                        print(f"  Raw mean {raw['mean']:.2f} {SENSORS[meta['sensor']]['raw_unit']}; std {raw['std']:.2f}.")
                        quantity = "voltage" if meta["sensor"] == "ir" else "calibrated_cm"
                        result = stats([convert(s, meta)[quantity] for s in valid])
                        unit = "V" if quantity == "voltage" else "cm from sensor face"
                        print(f"  {'Output' if quantity == 'voltage' else 'Reported distance'}: {result['mean']:.3f} {unit}.")
                    flags = sorted({s["status"] for s in samples if s["status"] != "OK"})
                    if flags:
                        print("  Status: " + ", ".join(flags))
                    while True:
                        decision = ask("  Accept [a/Enter], redo this repeat [r], or skip repeat [s]: ").lower()
                        if decision in ("", "a", "r", "s"):
                            break
                    log.write("review", capture_id=capture_id,
                              decision={"": "accepted", "a": "accepted", "r": "rejected", "s": "skipped"}[decision])
                    if decision != "r":
                        break
                    print("  Previous attempt remains in raw logs and is excluded from the curves.")
            log.write("point_done", point_id=point_id)
            index, next_index = next_index, next_index + 1
            offer_another = True
        log.write("run_complete")
        print("\nSensor run complete. Start a new run for the next sensor.")
    except (StopRecording, KeyboardInterrupt, EOFError):
        log.write("interrupted")
        print("\nStopped. Records saved; --resume can continue this session.")
    except Exception as error:
        log.write("error", message=str(error))
        raise
    finally:
        ACTIVE_CONSOLE = None
        if device:
            try:
                device.send("ABORT")
            except Exception:
                pass
        if port:
            port.close()
        log.close()
        if meta["firmware_config"] is not None:
            export_session(directory, plots=not args.no_plots)


def demo(args):
    directory = Path(args.output)
    directory.mkdir(parents=True, exist_ok=False)
    sensor = args.sensor or "us"
    meta = {"schema": 1, "created_utc": utc_now(), "sensor": sensor, "simulated": True,
            "points_cm": [200, 150, 80, 40, 20, 10], "repeats": 3, "samples_per_capture": 10,
            "period_ms": 100, "settle_ms": 1000, "distance_origin": "sensor_face",
            "adc_reference_v": 5.0, "adc_reference_source": "SIMULATED nominal",
            "firmware_config": {"US_SCALE": "1.015350", "US_OFFSET_CM": "0.709100"}}
    (directory / "session.json").write_text(json.dumps(meta, indent=2), encoding="utf-8")
    log, rng, cid = Log(directory), random.Random(42), 0
    for index, actual in enumerate(meta["points_cm"], 1):
        pid = str(index)
        log.write("point", point_id=pid, point_index=index, target_cm=actual, actual_cm=actual)
        for repeat in range(1, 4):
            cid += 1
            log.write("capture", capture_id=cid, point_id=pid, repeat=repeat)
            for sequence in range(10):
                if sensor == "us":
                    raw = round((actual - 0.7091) / 1.01535 * 58 + rng.gauss(0, 12))
                elif sensor == "laser":
                    raw = round(actual * 10 + rng.gauss(0, 5))
                else:
                    raw = round(5500 / (actual + 2) + rng.gauss(0, 2))
                log.write("sample", capture_id=cid, sequence=sequence, timestamp_ms=cid*3000+sequence*100,
                          raw=raw, valid=True, status="OK", device_status=11 if sensor == "laser" else -1)
            log.write("capture_complete", capture_id=cid)
            log.write("review", capture_id=cid, decision="accepted")
        log.write("point_done", point_id=pid)
    log.write("run_complete")
    log.close()
    export_session(directory, plots=not args.no_plots)


def main(argv=None):
    parser = argparse.ArgumentParser(description="One sensor at a time: actual distance, then three stationary captures")
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("ports", help="List serial ports")
    record_parser = commands.add_parser("record", help="Capture from the Arduino")
    record_parser.add_argument("--sensor", choices=SENSORS, help="Default: us; resume uses saved sensor")
    record_parser.add_argument("--port", default="COM7")
    location = record_parser.add_mutually_exclusive_group()
    location.add_argument("--output")
    location.add_argument("--resume")
    record_parser.add_argument("--repeats", type=int, choices=range(3, 11), default=3, metavar="3..10")
    record_parser.add_argument("--no-plots", action="store_true")
    analyze = commands.add_parser("analyze", help="Rebuild Excel, CSV and plots from saved records")
    analyze.add_argument("session")
    analyze.add_argument("--no-plots", action="store_true")
    simulation = commands.add_parser("demo", help="Create clearly labeled simulated data; no hardware")
    simulation.add_argument("--output", required=True)
    simulation.add_argument("--sensor", choices=SENSORS, default="us")
    simulation.add_argument("--no-plots", action="store_true")
    args = parser.parse_args(argv)
    if args.command == "ports":
        from serial.tools import list_ports
        for port in list_ports.comports():
            print(f"{port.device}: {port.description}")
    elif args.command == "record":
        record(args)
    elif args.command == "analyze":
        export_session(args.session, plots=not args.no_plots)
    else:
        demo(args)


if __name__ == "__main__":
    try:
        main()
    except (ValueError, RuntimeError, OSError) as error:
        raise SystemExit(f"Distance test stopped: {error}")

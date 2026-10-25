#!/usr/bin/env python3
"""Log cli_telemetry JSON samples from the NUCLEO-F446ZE to CSV.

Usage:
    pip install pyserial
    python telemetry_logger.py COM3 --period 500 --out log.csv
"""
import argparse
import csv
import json
import sys
import time

import serial

FIELDS = ["t_ms", "temp_c", "vdda_mv", "btn"]


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("port", help="serial port, e.g. COM3 or /dev/ttyACM0")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--period", type=int, default=500, help="sample period in ms (50..60000)")
    ap.add_argument("--out", default="telemetry.csv", help="CSV output file")
    ap.add_argument("--count", type=int, default=0, help="stop after N samples (0 = run until Ctrl+C)")
    args = ap.parse_args()

    with serial.Serial(args.port, args.baud, timeout=1) as port, \
            open(args.out, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=["host_time"] + FIELDS)
        writer.writeheader()

        port.write(b"\r")
        time.sleep(0.1)
        port.reset_input_buffer()
        port.write(f"stream {args.period} json\r".encode())

        n = 0
        try:
            while args.count == 0 or n < args.count:
                raw = port.readline().decode(errors="replace").strip()
                if not raw.startswith("{"):
                    if raw:
                        print(f"# {raw}")
                    continue
                try:
                    sample = json.loads(raw)
                except json.JSONDecodeError:
                    print(f"# bad line: {raw}", file=sys.stderr)
                    continue
                row = {"host_time": time.strftime("%Y-%m-%d %H:%M:%S"), **sample}
                writer.writerow(row)
                f.flush()
                n += 1
                print(f"{n:5d}  t={sample['t_ms']/1000:9.1f}s  "
                      f"temp={sample['temp_c']:6.2f} C  vdda={sample['vdda_mv']} mV  btn={sample['btn']}")
        except KeyboardInterrupt:
            pass
        finally:
            port.write(b"stream off\r")

    print(f"saved {n} samples to {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

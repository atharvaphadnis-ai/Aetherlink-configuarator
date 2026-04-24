#!/usr/bin/env python3
"""Fetch blackbox records over serial JSON API and export CSV."""

from __future__ import annotations

import argparse
import csv
import json
import time

import serial


def send_cmd(ser: serial.Serial, payload: dict) -> None:
    ser.write((json.dumps(payload) + "\n").encode("utf-8"))


def recv_json(ser: serial.Serial, timeout_s: float = 2.0) -> dict:
    end = time.time() + timeout_s
    while time.time() < end:
        line = ser.readline().decode("utf-8", errors="ignore").strip()
        if not line:
            continue
        try:
            return json.loads(line)
        except json.JSONDecodeError:
            continue
    raise TimeoutError("Timeout waiting for JSON response")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True)
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--out", default="blackbox.csv")
    args = parser.parse_args()

    with serial.Serial(args.port, args.baud, timeout=0.5) as ser:
        time.sleep(1.5)
        send_cmd(ser, {"cmd": "blackbox_count"})
        count_resp = recv_json(ser)
        if count_resp.get("type") != "blackbox_count":
            raise RuntimeError(f"Unexpected response: {count_resp}")
        count = int(count_resp.get("count", 0))
        print(f"records: {count}")

        with open(args.out, "w", newline="", encoding="utf-8") as f:
            writer = csv.writer(f)
            writer.writerow(["index", "t", "roll", "pitch", "yaw", "gx", "gy", "gz", "m1", "m2", "m3", "m4", "bv"])
            for i in range(count):
                send_cmd(ser, {"cmd": "blackbox_get", "index": i})
                rec = recv_json(ser)
                if rec.get("type") != "blackbox_record":
                    continue
                writer.writerow(
                    [
                        i,
                        rec.get("t"),
                        rec.get("roll"),
                        rec.get("pitch"),
                        rec.get("yaw"),
                        rec.get("gx"),
                        rec.get("gy"),
                        rec.get("gz"),
                        rec.get("m1"),
                        rec.get("m2"),
                        rec.get("m3"),
                        rec.get("m4"),
                        rec.get("bv"),
                    ]
                )

    print(f"Export complete: {args.out}")


if __name__ == "__main__":
    main()

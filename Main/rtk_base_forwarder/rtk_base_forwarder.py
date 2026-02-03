#!/usr/bin/env python3
import socket
import subprocess
import sys
import time

import serial
from serial.tools import list_ports

WIFI_SSID = "UAVDocking"
WIFI_PASS = "UAVDocking"
WIFI_INTERFACE = "en0"
AUTO_CONNECT_WIFI = True

BAUD_CANDIDATES = [115200, 230400, 460800]
SCAN_SECONDS_PER_BAUD = 2.0

TARGETS = [
    ("192.168.1.2", 14550),
    ("192.168.1.3", 14550),
]
USE_BROADCAST = True
BROADCAST_TARGET = ("192.168.1.255", 14550)

# Base position lock (u-blox F9P)
ENABLE_SURVEY_IN = True
SVIN_MIN_DURATION_S = 120
SVIN_ACC_LIMIT_MM = 5000


def connect_wifi():
    if not AUTO_CONNECT_WIFI:
        return
    try:
        subprocess.run(
            [
                "networksetup",
                "-setairportnetwork",
                WIFI_INTERFACE,
                WIFI_SSID,
                WIFI_PASS,
            ],
            check=False,
            capture_output=True,
            text=True,
        )
    except Exception:
        pass


def looks_like_rtcm(data: bytes) -> bool:
    # RTCM3 preamble is 0xD3
    if b"\xD3" in data:
        return True
    return False


def ubx_checksum(payload: bytes) -> bytes:
    ck_a = 0
    ck_b = 0
    for b in payload:
        ck_a = (ck_a + b) & 0xFF
        ck_b = (ck_b + ck_a) & 0xFF
    return bytes([ck_a, ck_b])


def ubx_message(cls_id: int, msg_id: int, payload: bytes) -> bytes:
    length = len(payload)
    header = bytes([0xB5, 0x62, cls_id, msg_id, length & 0xFF, (length >> 8) & 0xFF])
    ck = ubx_checksum(bytes([cls_id, msg_id, length & 0xFF, (length >> 8) & 0xFF]) + payload)
    return header + payload + ck


def cfg_tmode3_survey_in(min_dur_s: int, acc_limit_mm: int) -> bytes:
    version = 0
    reserved1 = 0
    flags = 0x00000001  # survey-in enable
    ecef_x = 0
    ecef_y = 0
    ecef_z = 0
    ecef_hp_x = 0
    ecef_hp_y = 0
    ecef_hp_z = 0
    reserved2 = 0
    fixed_pos_acc = 0
    svin_min_dur = int(min_dur_s)
    svin_acc_limit = int(acc_limit_mm)
    payload = (
        bytes([version, reserved1]) +
        flags.to_bytes(4, "little") +
        int(ecef_x).to_bytes(4, "little", signed=True) +
        int(ecef_y).to_bytes(4, "little", signed=True) +
        int(ecef_z).to_bytes(4, "little", signed=True) +
        int(ecef_hp_x).to_bytes(1, "little", signed=True) +
        int(ecef_hp_y).to_bytes(1, "little", signed=True) +
        int(ecef_hp_z).to_bytes(1, "little", signed=True) +
        bytes([reserved2]) +
        int(fixed_pos_acc).to_bytes(4, "little") +
        int(svin_min_dur).to_bytes(4, "little") +
        int(svin_acc_limit).to_bytes(4, "little")
    )
    return ubx_message(0x06, 0x71, payload)


def try_start_survey_in(ser: serial.Serial):
    if not ENABLE_SURVEY_IN:
        return
    msg = cfg_tmode3_survey_in(SVIN_MIN_DURATION_S, SVIN_ACC_LIMIT_MM)
    ser.write(msg)
    ser.flush()


def find_base_port():
    ports = list(list_ports.comports())
    if not ports:
        return None, None

    for p in ports:
        for baud in BAUD_CANDIDATES:
            try:
                ser = serial.Serial(p.device, baudrate=baud, timeout=0.2)
            except Exception:
                continue
            deadline = time.time() + SCAN_SECONDS_PER_BAUD
            buf = bytearray()
            while time.time() < deadline:
                chunk = ser.read(512)
                if chunk:
                    buf.extend(chunk)
                    if len(buf) > 2048:
                        buf = buf[-2048:]
                    if looks_like_rtcm(buf):
                        return ser, baud
            ser.close()
    return None, None


def main():
    connect_wifi()
    ser, baud = find_base_port()
    if ser is None:
        print("RTK base not found.")
        sys.exit(1)

    print(f"RTK base: {ser.port} @ {baud}")
    try_start_survey_in(ser)

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

    total = 0
    last_log = time.time()
    while True:
        data = ser.read(1024)
        if not data:
            continue
        total += len(data)
        for host, port in TARGETS:
            sock.sendto(data, (host, port))
        if USE_BROADCAST:
            sock.sendto(data, BROADCAST_TARGET)
        now = time.time()
        if now - last_log > 2.0:
            print(f"RTCM bytes sent: {total}")
            last_log = now


if __name__ == "__main__":
    main()


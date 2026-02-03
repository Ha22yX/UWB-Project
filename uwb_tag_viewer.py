import re
import threading
import time
from queue import Queue, Empty

import matplotlib.pyplot as plt
import serial

PORT = "COM14"
BAUD = 115200

INCH_TO_M = 0.0254
SIDE_M = 35.5 * INCH_TO_M
HALF = SIDE_M * 0.5
FIXED_H = 0.30  # meters, set your tag height above anchor plane

# Anchor positions (meters), midpoints of square edges
# Physical order clockwise: 1 -> 2 -> 3 -> 4
# Mapping to UWB IDs (an0..an3):
#   an0 = Anchor 1 (right midpoint)
#   an1 = Anchor 2 (bottom midpoint)
#   an2 = Anchor 3 (left midpoint)
#   an3 = Anchor 4 (top midpoint)
ANCHORS = [
    ("A1", +HALF, 0.0),
    ("A2", 0.0, -HALF),
    ("A3", -HALF, 0.0),
    ("A4", 0.0, +HALF),
]

POS_RE = re.compile(r"\[POS\]\s*x=([-0-9.]+)\s*m,\s*y=([-0-9.]+)\s*m,\s*z=([-0-9.]+)\s*m")
DIST_RE = re.compile(r"\[D\]\s*an(\d)=([-0-9.]+)\s*m")


def reader_thread(port: str, baud: int, out_q: Queue):
    while True:
        try:
            with serial.Serial(port, baud, timeout=1) as ser:
                buf = ""
                while True:
                    data = ser.read(ser.in_waiting or 1)
                    if not data:
                        continue
                    try:
                        buf += data.decode("utf-8", errors="ignore")
                    except Exception:
                        continue
                    while "\n" in buf:
                        line, buf = buf.split("\n", 1)
                        line = line.strip()
                        if line:
                            out_q.put(line)
        except Exception:
            time.sleep(1)


def main():
    q = Queue()
    t = threading.Thread(target=reader_thread, args=(PORT, BAUD, q), daemon=True)
    t.start()

    plt.ion()
    fig, ax = plt.subplots()
    ax.set_aspect("equal", "box")
    ax.set_title("UWB Relative Positions (meters)")
    ax.set_xlabel("X (m)")
    ax.set_ylabel("Y (m)")
    margin = 0.2
    ax.set_xlim(-HALF - margin, HALF + margin)
    ax.set_ylim(-HALF - margin, HALF + margin)

    # Plot anchors
    for name, x, y in ANCHORS:
        ax.scatter([x], [y], c="blue", s=60)
        ax.text(x, y, f" {name}", fontsize=9, color="blue")

    # Tag point
    tag_scatter = ax.scatter([0], [0], c="red", s=80)
    tag_text = ax.text(0, 0, " Tag", fontsize=10, color="red")

    dist = {0: None, 1: None, 2: None, 3: None}

    def solve_2d(d):
        idxs = [i for i, v in d.items() if v is not None]
        if len(idxs) < 3:
            return None
        dxy = {}
        for i in idxs:
            d2 = d[i] * d[i] - FIXED_H * FIXED_H
            if d2 <= 0:
                return None
            dxy[i] = d2 ** 0.5
        ref = idxs[0]
        x0, y0, d0 = ANCHORS[ref][1], ANCHORS[ref][2], dxy[ref]
        a11 = a12 = a22 = 0.0
        b1 = b2 = 0.0
        for i in idxs[1:]:
            xi, yi, di = ANCHORS[i][1], ANCHORS[i][2], dxy[i]
            ai = 2.0 * (xi - x0)
            bi = 2.0 * (yi - y0)
            ci = (xi * xi + yi * yi - di * di) - (x0 * x0 + y0 * y0 - d0 * d0)
            a11 += ai * ai
            a12 += ai * bi
            a22 += bi * bi
            b1 += ai * ci
            b2 += bi * ci
        det = a11 * a22 - a12 * a12
        if abs(det) < 1e-9:
            return None
        x = (b1 * a22 - b2 * a12) / det
        y = (a11 * b2 - a12 * b1) / det
        return x, y

    while True:
        try:
            line = q.get_nowait()
        except Empty:
            plt.pause(0.02)
            continue

        print(line)

        m = POS_RE.search(line)
        if m:
            x = float(m.group(1))
            y = float(m.group(2))
            z = float(m.group(3))
            tag_scatter.set_offsets([[x, y]])
            tag_text.set_position((x, y))
            ax.set_title(f"UWB Relative Positions (m), z={z:.2f}")
            fig.canvas.draw_idle()
        else:
            dm = DIST_RE.search(line)
            if dm:
                idx = int(dm.group(1))
                val = float(dm.group(2))
                if idx in dist:
                    dist[idx] = val
                solved = solve_2d(dist)
                if solved:
                    x, y = solved
                    tag_scatter.set_offsets([[x, y]])
                    tag_text.set_position((x, y))
                    fig.canvas.draw_idle()

        plt.pause(0.001)


if __name__ == "__main__":
    main()


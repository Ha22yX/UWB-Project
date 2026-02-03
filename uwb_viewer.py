import threading
import time
import re
from collections import deque

import serial
from serial import SerialException

import tkinter as tk
from tkinter import ttk

import matplotlib
matplotlib.use("TkAgg")
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure


PORT = "COM13"
BAUD = 115200
MAX_POINTS = 4
AUTO_START = True
MAX_HISTORY = 50


pos_re = re.compile(r"^P(\d+):\s*\(([-\d.]+),\s*([-\d.]+),\s*([-\d.]+)\)")
matrix_row_re = re.compile(r"^\s*(\d+\.\d+|----)")


class UwbViewer:
    def __init__(self, root):
        self.root = root
        self.root.title("UWB Relative Position Viewer")

        self.status_var = tk.StringVar(value="Disconnected")
        self.last_update_var = tk.StringVar(value="Last update: --")
        self.auto_start_var = tk.BooleanVar(value=AUTO_START)

        self.positions = [(0.0, 0.0, 0.0)] * MAX_POINTS
        self.position_valid = [False] * MAX_POINTS
        self.matrix_rows = deque(maxlen=MAX_POINTS)
        self.history = []
        self.run_counter = 0
        self.active_run_index = None
        self.current_run_lines = []
        self.current_matrix = []
        self.current_run_solved = False
        self.pending_save = False
        self.pending_timer = None
        self.current_run_saved = False

        self._build_ui()

        self.serial_thread = None
        self.serial_stop = threading.Event()
        self.ser = None

    def _build_ui(self):
        header = ttk.Frame(self.root)
        header.pack(fill="x", padx=10, pady=8)

        ttk.Label(header, text="Port:").pack(side="left")
        self.port_entry = ttk.Entry(header, width=12)
        self.port_entry.insert(0, PORT)
        self.port_entry.pack(side="left", padx=6)

        ttk.Label(header, text="Baud:").pack(side="left")
        self.baud_entry = ttk.Entry(header, width=8)
        self.baud_entry.insert(0, str(BAUD))
        self.baud_entry.pack(side="left", padx=6)

        self.connect_btn = ttk.Button(header, text="Connect", command=self.toggle_connection)
        self.connect_btn.pack(side="left", padx=8)

        self.run_btn = ttk.Button(header, text="Run", command=self.manual_run)
        self.run_btn.pack(side="left", padx=8)
        self.auto_check = ttk.Checkbutton(header, text="Auto", variable=self.auto_start_var)
        self.auto_check.pack(side="left", padx=4)

        ttk.Label(header, textvariable=self.status_var).pack(side="left", padx=8)
        ttk.Label(header, textvariable=self.last_update_var).pack(side="left", padx=8)

        body = ttk.Frame(self.root)
        body.pack(fill="both", expand=True, padx=10, pady=10)

        fig = Figure(figsize=(6, 5))
        self.ax = fig.add_subplot(111, projection="3d")
        self.ax.set_xlabel("X (m)")
        self.ax.set_ylabel("Y (m)")
        self.ax.set_zlabel("Z (m)")
        self.ax.set_title("UWB Relative Positions")

        self.canvas = FigureCanvasTkAgg(fig, master=body)
        self.canvas.get_tk_widget().pack(side="left", fill="both", expand=True)

        side = ttk.Frame(body)
        side.pack(side="left", fill="y", padx=(10, 0))
        ttk.Label(side, text="History").pack(anchor="w")
        self.history_list = tk.Listbox(side, height=18, width=30)
        self.history_list.pack(fill="y", expand=True)
        self.history_list.bind("<<ListboxSelect>>", self._on_history_select)
        self.clear_history_btn = ttk.Button(side, text="Clear History", command=self._clear_history)
        self.clear_history_btn.pack(fill="x", pady=(6, 0))

        self.log_text = tk.Text(self.root, height=10)
        self.log_text.pack(fill="both", expand=False, padx=10, pady=(0, 10))

    def toggle_connection(self):
        if self.ser and self.ser.is_open:
            self._disconnect()
        else:
            self._connect()

    def _connect(self):
        port = self.port_entry.get().strip()
        try:
            baud = int(self.baud_entry.get().strip())
        except ValueError:
            self.status_var.set("Invalid baud")
            return
        try:
            self.ser = serial.Serial(port, baud, timeout=0.2)
        except SerialException as e:
            self.status_var.set(f"Connect failed: {e}")
            return

        self.serial_stop.clear()
        self.serial_thread = threading.Thread(target=self._serial_loop, daemon=True)
        self.serial_thread.start()
        self.status_var.set(f"Connected to {port}")
        self.connect_btn.configure(text="Disconnect")
        if self.auto_start_var.get():
            threading.Thread(target=self._auto_start_sequence, daemon=True).start()

    def _send_line(self, line: str):
        if not self.ser or not self.ser.is_open:
            return
        try:
            self.ser.write((line + "\r\n").encode("utf-8"))
        except SerialException:
            pass

    def _auto_start_sequence(self):
        time.sleep(0.2)
        self.manual_run()

    def manual_run(self):
        self._send_line("tagid off")
        time.sleep(0.1)
        self._send_line("run")

    def _disconnect(self):
        self.serial_stop.set()
        if self.serial_thread:
            self.serial_thread.join(timeout=1.0)
        if self.ser:
            try:
                self.ser.close()
            except SerialException:
                pass
        self.status_var.set("Disconnected")
        self.connect_btn.configure(text="Connect")

    def _serial_loop(self):
        buffer = ""
        while not self.serial_stop.is_set():
            try:
                data = self.ser.read(256)
            except SerialException:
                break
            if not data:
                continue
            try:
                chunk = data.decode("utf-8", errors="ignore")
            except Exception:
                continue
            buffer += chunk
            while "\n" in buffer:
                line, buffer = buffer.split("\n", 1)
                line = line.strip("\r")
                if line:
                    self._handle_line(line)
        self.status_var.set("Disconnected")
        self.connect_btn.configure(text="Connect")

    def _handle_line(self, line: str):
        self.log_text.insert("end", line + "\n")
        self.log_text.see("end")
        self.current_run_lines.append(line)

        m = pos_re.match(line)
        if m:
            idx = int(m.group(1))
            if 0 <= idx < MAX_POINTS:
                x, y, z = float(m.group(2)), float(m.group(3)), float(m.group(4))
                self.positions[idx] = (x, y, z)
                self.position_valid[idx] = True
                return

        if matrix_row_re.match(line):
            self.matrix_rows.append(line)
            self.current_matrix.append(line)

        if line.startswith("Distance matrix"):
            self.matrix_rows.clear()
            self.current_matrix = []

        if line.startswith("Solved 3D positions"):
            self.last_update_var.set(f"Last update: {time.strftime('%H:%M:%S')}")
            self.current_run_solved = True
            if not self.current_run_saved:
                self._save_run()
        if line.startswith("Residuals"):
            if not self.current_run_saved:
                self._save_run()
        if line.startswith("[AUTO] done"):
            self._send_line("solve")
            self.pending_save = True
            if self.pending_timer:
                self.root.after_cancel(self.pending_timer)
            self.pending_timer = self.root.after(1500, self._save_if_pending)
        if line.startswith("[AUTO] start"):
            self._reset_current_run()
            self.pending_save = False
            if self.pending_timer:
                self.root.after_cancel(self.pending_timer)
                self.pending_timer = None

    def _reset_current_run(self):
        self.current_run_lines = []
        self.current_matrix = []
        self.current_run_solved = False
        self.current_run_saved = False

    def _save_run(self, force: bool = False):
        if not force and not any(self.position_valid):
            return
        stamp = time.strftime("%H:%M:%S")
        suffix = "" if self.current_run_solved else " (no solve)"
        run = {
            "name": f"Run {self.run_counter + 1} @ {stamp}{suffix}",
            "positions": list(self.positions),
            "valid": list(self.position_valid),
            "matrix": list(self.current_matrix),
            "lines": list(self.current_run_lines),
        }
        self.run_counter += 1
        self.history.append(run)
        if len(self.history) > MAX_HISTORY:
            self.history.pop(0)
        self.history_list.insert("end", run["name"])
        self.history_list.selection_clear(0, "end")
        self.history_list.selection_set("end")
        self.history_list.see("end")
        self._show_history_run(len(self.history) - 1)
        self.pending_save = False
        if self.pending_timer:
            self.root.after_cancel(self.pending_timer)
            self.pending_timer = None
        self.current_run_saved = True

    def _save_if_pending(self):
        if self.pending_save:
            self._save_run(force=True)
            self._reset_current_run()

    def _show_history_run(self, index: int):
        if index < 0 or index >= len(self.history):
            return
        run = self.history[index]
        self.positions = list(run["positions"])
        self.position_valid = list(run["valid"])
        self.active_run_index = index
        self._update_plot()

    def _on_history_select(self, _event):
        sel = self.history_list.curselection()
        if not sel:
            return
        self._show_history_run(sel[0])

    def _clear_history(self):
        self.history.clear()
        self.history_list.delete(0, "end")
        self.active_run_index = None

    def _update_plot(self):
        self.ax.cla()
        self.ax.set_xlabel("X (m)")
        self.ax.set_ylabel("Y (m)")
        self.ax.set_zlabel("Z (m)")
        self.ax.set_title("UWB Relative Positions")

        xs, ys, zs = [], [], []
        labels = []
        for i, ok in enumerate(self.position_valid):
            if ok:
                x, y, z = self.positions[i]
                xs.append(x)
                ys.append(y)
                zs.append(z)
                labels.append(f"P{i}")

        if xs:
            self.ax.scatter(xs, ys, zs, s=60)
            for i, label in enumerate(labels):
                self.ax.text(xs[i], ys[i], zs[i], label)

        self.canvas.draw_idle()


def main():
    root = tk.Tk()
    app = UwbViewer(root)
    root.protocol("WM_DELETE_WINDOW", app._disconnect)
    root.mainloop()


if __name__ == "__main__":
    main()


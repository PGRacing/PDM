import can
from can import Message
import struct
import threading
import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import csv
from collections import defaultdict, deque
import time
import os
import datetime

from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure

# ================= CONFIG =================
CHANNEL_COUNT = 16
BASE_ID = 0x400

CAN_CHANNEL = "COM5"
CAN_BITRATE = 1000000
SERIAL_BAUD = 115200

HISTORY_LEN = 300
GUI_UPDATE_MS = 50
PLOT_UPDATE_MS = 10
AVG_WINDOW_MS = 200

# ================= CAN IDS =================
IDS = {
    "SYS_STATUS": BASE_ID + 0x001,
    "STATUS_1_8": BASE_ID + 0x002,
    "STATUS_9_16": BASE_ID + 0x003,
    "STATE_1_16": BASE_ID + 0x004,
    "VOLT_1_4": BASE_ID + 0x005,
    "VOLT_5_8": BASE_ID + 0x006,
    "VOLT_9_12": BASE_ID + 0x007,
    "VOLT_13_16": BASE_ID + 0x008,
    "CURR_1_4": BASE_ID + 0x009,
    "CURR_5_8": BASE_ID + 0x00A,
    "CURR_9_12": BASE_ID + 0x00B,
    "CURR_13_16": BASE_ID + 0x00C,
    "NAMES": BASE_ID + 0x00D,
}

# ================= ENUMS =================
OUT_STATE_MAP = {0: "OFF", 1: "ON", 2: "ERR"}
OUT_STATUS_MAP = {
    0: "OK", 1: "OPEN_LOAD", 8: "SAFETY_OPEN",
    9: "PRIO_DIV", 10: "SOC_FAULT", 11: "I2T_FAULT",
    20: "SHORT_VSS", 21: "CTRL_FAIL", 22: "HARD_FAULT"
}

def get_tag(state, status):
    if status == 0 and state == 0:
        return "off"
    if status == 0:
        return "ok"
    elif status < 20:
        return "warn"
    return "fault"

# ================= GLOBAL DATA =================
channels = [
    {"name": "", "status": 0, "state": 0, "voltage": 0, "current": 0, "current_avg": 0}
    for _ in range(CHANNEL_COUNT)
]

sys_status = {"status": 0, "batt": 0, "core_temp": 0.0, "safety": 0, "total_current": 0}

name_parts = defaultdict(dict)

voltage_history = [deque(maxlen=HISTORY_LEN) for _ in range(CHANNEL_COUNT)]
current_history = [deque(maxlen=HISTORY_LEN) for _ in range(CHANNEL_COUNT)]
current_avg_history = [deque(maxlen=HISTORY_LEN) for _ in range(CHANNEL_COUNT)]
current_avg_window = [deque() for _ in range(CHANNEL_COUNT)]

lock = threading.Lock()
bus_lock = threading.Lock()
can_bus = None

# continuous CSV logging globals (created at program start)
LOG_FILE_NAME = f"pdmdisp_{datetime.datetime.now():%Y%m%d-%H%M%S}.csv"
log_f = None
log_writer = None

# acquisition control
acquire_event = threading.Event()
acquire_event.set()

# ================= PARSING =================
def parse_u16x4(data):
    return struct.unpack("<4H", data)


def parse_system_status(data):
    status = data[0]
    batt_voltage = data[1] | (data[2] << 8)
    core_temp_raw = struct.unpack_from("<h", data, 3)[0]
    safety_line_state = data[5] if len(data) > 5 else 0
    return status, batt_voltage, core_temp_raw / 10.0, safety_line_state


def update_names(data):
    meta = data[0]
    part = (meta >> 4) & 0x0F
    ch = meta & 0x0F

    txt = data[1:].decode(errors="ignore").rstrip("\x00")
    name_parts[ch][part] = txt

    channels[ch]["name"] = "".join(name_parts[ch][i] for i in sorted(name_parts[ch]))


def trim_history(hist, now):
    cutoff = now - (AVG_WINDOW_MS / 1000.0)
    while hist and hist[0][0] < cutoff:
        hist.popleft()


def average_history(hist):
    if not hist:
        return 0
    return sum(v for _, v in hist) / len(hist)


def parse_hex_bytes(text):
    cleaned = text.replace(",", " ").split()
    if not cleaned:
        return b""
    values = []
    for token in cleaned:
        token = token.strip()
        if not token:
            continue
        values.append(int(token, 16))
    if len(values) > 8:
        raise ValueError("CAN payload must be 8 bytes or less")
    for value in values:
        if value < 0 or value > 0xFF:
            raise ValueError(f"Invalid byte value: {value}")
    return bytes(values)


def parse_can_id(text):
    cleaned = text.strip()
    if not cleaned:
        raise ValueError("CAN ID is empty")

    if "(" in cleaned:
        cleaned = cleaned.split("(", 1)[-1].rstrip(")")

    if cleaned.lower().startswith("0x"):
        return int(cleaned, 16)

    try:
        return int(cleaned, 0)
    except ValueError:
        if cleaned in IDS:
            return IDS[cleaned]
        raise


def send_can_frame(arbitration_id, payload):
    global can_bus
    if can_bus is None:
        raise RuntimeError("CAN bus is not ready")
    if arbitration_id < 0 or arbitration_id > 0x1FFFFFFF:
        raise ValueError("CAN ID must be between 0 and 0x1FFFFFFF")

    is_extended_id = arbitration_id > 0x7FF
    msg = Message(arbitration_id=arbitration_id, data=payload, is_extended_id=is_extended_id)
    can_bus.send(msg)


# ================= CAN THREAD =================
def can_worker():
    global can_bus
    global log_f, log_writer
    try:
        bus = can.interface.Bus(
            channel=CAN_CHANNEL,
            interface="slcan",
            bitrate=CAN_BITRATE,
            ttyBaudrate=SERIAL_BAUD,
        )
        can_bus = bus
        print("CAN connected")
    except Exception as e:
        print("CAN init failed:", e)
        return

    # open CSV log file (new file per program start) and start table logger
    try:
        log_f = open(LOG_FILE_NAME, "a", newline="")
        log_writer = csv.writer(log_f)
        try:
            if os.path.getsize(LOG_FILE_NAME) == 0:
                header = ["timestamp", "total_i_mA", "batt_mV", "core_temp_C", "safety"]
                for i in range(CHANNEL_COUNT):
                    n = i + 1
                    header += [
                        f"ch{n}_name",
                        f"ch{n}_status",
                        f"ch{n}_state",
                        f"ch{n}_voltage_mV",
                        f"ch{n}_i_inst_mA",
                        f"ch{n}_i_avg_mA",
                    ]
                log_writer.writerow(header)
                log_f.flush()
        except OSError:
            pass
    except Exception as e:
        print("Could not open log file:", e)
        log_f = None
        log_writer = None

    # start background logger thread to write table snapshots
    def table_logger(interval=0.05):
        global log_writer, log_f
        while True:
            time.sleep(interval)
            if log_writer is None:
                continue
            with lock:
                ts = time.time()
                try:
                    row = [
                        ts,
                        f"{sys_status.get('total_current',0):.1f}",
                        sys_status.get("batt", ""),
                        f"{sys_status.get('core_temp',0):.1f}",
                        sys_status.get("safety", ""),
                    ]
                    for ch in channels:
                        row.extend([
                            ch.get("name", ""),
                            ch.get("status", ""),
                            ch.get("state", ""),
                            ch.get("voltage", ""),
                            ch.get("current", ""),
                            f"{ch.get('current_avg', 0):.1f}",
                        ])
                    log_writer.writerow(row)
                    log_f.flush()
                except Exception:
                    pass

    t = threading.Thread(target=table_logger, args=(0.05,), daemon=True)
    t.start()

    while True:
        # wait until acquisition enabled
        acquire_event.wait()
        msg = bus.recv(timeout=1)
        if msg is None:
            continue

        cid = msg.arbitration_id
        d = msg.data

        with lock:

            # -------- SYS STATUS --------
            if cid == IDS["SYS_STATUS"]:
                        status, batt_voltage, core_temp, safety_line_state = parse_system_status(d)
                        sys_status["status"] = status
                        sys_status["batt"] = batt_voltage
                        sys_status["core_temp"] = core_temp
                        sys_status["safety"] = safety_line_state

            # -------- STATUS --------
            elif cid == IDS["STATUS_1_8"]:
                for i in range(8):
                    channels[i]["status"] = d[i]

            elif cid == IDS["STATUS_9_16"]:
                for i in range(8):
                    channels[i+8]["status"] = d[i]

            # -------- STATE --------
            elif cid == IDS["STATE_1_16"]:
                for i in range(16):
                    b = d[i//2]
                    channels[i]["state"] = (b >> 4) & 0x0F if i % 2 == 0 else b & 0x0F

            # -------- VOLTAGE --------
            elif cid in (IDS["VOLT_1_4"], IDS["VOLT_5_8"], IDS["VOLT_9_12"], IDS["VOLT_13_16"]):
                base = {
                    IDS["VOLT_1_4"]: 0,
                    IDS["VOLT_5_8"]: 4,
                    IDS["VOLT_9_12"]: 8,
                    IDS["VOLT_13_16"]: 12
                }[cid]

                vals = parse_u16x4(d)
                t = time.time()

                for i in range(4):
                    ch = base + i
                    channels[ch]["voltage"] = vals[i]
                    voltage_history[ch].append((t, vals[i]))

            # -------- CURRENT --------
            elif cid in (IDS["CURR_1_4"], IDS["CURR_5_8"], IDS["CURR_9_12"], IDS["CURR_13_16"]):
                base = {
                    IDS["CURR_1_4"]: 0,
                    IDS["CURR_5_8"]: 4,
                    IDS["CURR_9_12"]: 8,
                    IDS["CURR_13_16"]: 12
                }[cid]

                vals = parse_u16x4(d)
                t = time.time()

                for i in range(4):
                    ch = base + i
                    inst = vals[i] * 10
                    channels[ch]["current"] = inst
                    current_history[ch].append((t, inst))

                    win = current_avg_window[ch]
                    win.append((t, inst))
                    trim_history(win, t)
                    avg = average_history(win)
                    channels[ch]["current_avg"] = avg
                    current_avg_history[ch].append((t, avg))

                sys_status["total_current"] = sum(ch["current_avg"] for ch in channels)

            elif cid == IDS["NAMES"]:
                update_names(d)

    # close log on exit
    try:
        if log_f is not None:
            log_f.close()
    except Exception:
        pass


# ================= GUI =================
class App:
    def __init__(self, root):
        self.root = root
        self.root.title("PDM CAN Dashboard")
        self.root.state("zoomed")

        # ===== TOP BAR =====
        top = tk.Frame(root)
        top.pack(fill=tk.X)

        # --- checkboxes ---
        cb_frame = tk.LabelFrame(top, text="Channels")
        cb_frame.pack(side=tk.LEFT, fill=tk.X, expand=True)

        self.vars = []
        for i in range(CHANNEL_COUNT):
            v = tk.BooleanVar(value=(i == 0))
            tk.Checkbutton(cb_frame, text=str(i+1), variable=v).grid(row=i//8, column=i%8)
            self.vars.append(v)

        # --- system status ---
        sys_frame = tk.LabelFrame(top, text="System")
        sys_frame.pack(side=tk.RIGHT, padx=10)

        self.sys_lbl = tk.StringVar()
        self.batt_lbl = tk.StringVar()
        self.temp_lbl = tk.StringVar()
        self.safe_lbl = tk.StringVar()
        self.total_i_lbl = tk.StringVar()

        tk.Label(sys_frame, text="State").grid(row=0, column=0)
        tk.Label(sys_frame, textvariable=self.sys_lbl).grid(row=0, column=1)

        tk.Label(sys_frame, text="Battery").grid(row=1, column=0)
        tk.Label(sys_frame, textvariable=self.batt_lbl).grid(row=1, column=1)

        tk.Label(sys_frame, text="Core Temp").grid(row=2, column=0)
        tk.Label(sys_frame, textvariable=self.temp_lbl).grid(row=2, column=1)

        tk.Label(sys_frame, text="Safety").grid(row=3, column=0)
        tk.Label(sys_frame, textvariable=self.safe_lbl).grid(row=3, column=1)

        tk.Label(sys_frame, text="Total I avg").grid(row=4, column=0)
        tk.Label(sys_frame, textvariable=self.total_i_lbl).grid(row=4, column=1)

        # --- transmit frame ---
        tx_frame = tk.LabelFrame(top, text="Send CAN frame")
        tx_frame.pack(side=tk.RIGHT, padx=10)

        self.send_id_var = tk.StringVar(value="SYS_STATUS")
        self.send_data_var = tk.StringVar(value="00 00 00 00 00 00 00 00")

        self.send_id_map = {name: cid for name, cid in IDS.items()}
        self.send_id_combo = ttk.Combobox(
            tx_frame,
            textvariable=self.send_id_var,
            values=[f"{name} (0x{cid:03X})" for name, cid in IDS.items()],
            state="normal",
            width=24,
        )
        self.send_id_combo.set("0x401")
        self.send_id_combo.grid(row=0, column=0, columnspan=2, sticky="ew", padx=2, pady=2)

        tk.Label(tx_frame, text="Data (hex)").grid(row=1, column=0, sticky="w")
        tk.Entry(tx_frame, textvariable=self.send_data_var, width=28).grid(row=1, column=1, sticky="ew", padx=2, pady=2)

        tk.Button(tx_frame, text="Send", command=self.send_selected_frame).grid(row=2, column=0, columnspan=2, sticky="ew", padx=2, pady=2)

        tx_frame.grid_columnconfigure(1, weight=1)

        # Save CSV snapshot (button removed; logging is continuous)

        # ===== TABLE =====
        self.tree = ttk.Treeview(root)
        self.tree["columns"] = ("Name", "Status", "State", "V [mV]", "I inst [mA]", "I avg [mA]")
        for c in self.tree["columns"]:
            self.tree.heading(c, text=c)
        self.tree.pack(fill=tk.BOTH, expand=True)

        self.tree.tag_configure("ok", background="#d4ffd4")
        self.tree.tag_configure("off", background="#d8d8d8")
        self.tree.tag_configure("warn", background="#fff3b0")
        self.tree.tag_configure("fault", background="#ffb3b3")

        self.rows = [self.tree.insert("", "end", text=str(i+1), values=("", "", "", "", "", ""))
                 for i in range(CHANNEL_COUNT)]

        # ===== PLOT =====
        fig = Figure(figsize=(6, 4))
        self.ax_v = fig.add_subplot(211)
        self.ax_i = fig.add_subplot(212)

        self.canvas = FigureCanvasTkAgg(fig, root)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

        self.update_gui()
        self.update_plot()

    def selected(self):
        return [i for i, v in enumerate(self.vars) if v.get()]

    def send_selected_frame(self):
        selected_text = self.send_id_combo.get()
        try:
            arbitration_id = parse_can_id(selected_text)
        except Exception:
            name = selected_text.split(" (")[0]
            arbitration_id = self.send_id_map.get(name)

        if arbitration_id is None:
            messagebox.showerror("Send CAN frame", "Please enter a valid CAN ID")
            return

        try:
            payload = parse_hex_bytes(self.send_data_var.get())
        except Exception as e:
            messagebox.showerror("Send CAN frame", f"Invalid hex payload: {e}")
            return

        try:
            send_can_frame(arbitration_id, payload)
        except Exception as e:
            messagebox.showerror("Send CAN frame", f"Failed to send CAN frame: {e}")
            return

        print(f"Sent CAN frame id=0x{arbitration_id:X} data={payload.hex(' ')}")

    def save_csv(self):
        fn = filedialog.asksaveasfilename(
            defaultextension=".csv",
            filetypes=[("CSV files", "*.csv"), ("All files", "*")],
            title="Save snapshot as CSV",
        )
        if not fn:
            return

        try:
            with open(fn, "w", newline="") as f:
                writer = csv.writer(f)
                writer.writerow(["timestamp", time.time()])
                # system status
                writer.writerow(["system_status", "status", sys_status["status"]])
                writer.writerow(["system_status", "batt_mV", sys_status["batt"]])
                writer.writerow(["system_status", "core_temp_C", f"{sys_status['core_temp']:.1f}"])
                writer.writerow(["system_status", "safety", sys_status["safety"]])
                writer.writerow(["system_status", "total_current_mA", f"{sys_status['total_current']:.1f}"])
                writer.writerow([])
                # channels
                writer.writerow(["ch", "name", "status", "state", "voltage_mV", "current_mA", "current_avg_mA"])
                for i, ch in enumerate(channels):
                    writer.writerow([
                        i+1,
                        ch.get("name", ""),
                        ch.get("status", ""),
                        ch.get("state", ""),
                        ch.get("voltage", ""),
                        ch.get("current", ""),
                        f"{ch.get('current_avg', 0):.1f}",
                    ])
        except Exception as e:
            messagebox.showerror("Save CSV", f"Failed to save CSV: {e}")
            return

        messagebox.showinfo("Save CSV", f"Saved snapshot to {fn}")

    # ===== GUI UPDATE =====
    def update_gui(self):
        with lock:
            self.sys_lbl.set(str(sys_status["status"]))
            self.batt_lbl.set(f"{sys_status['batt']} mV")
            self.temp_lbl.set(f"{sys_status['core_temp']:.1f} °C")
            self.safe_lbl.set(str(sys_status["safety"]))
            self.total_i_lbl.set(f"{sys_status['total_current']:.1f} mA")

            for i, ch in enumerate(channels):
                tag = get_tag(ch["state"], ch["status"])
                self.tree.item(
                    self.rows[i],
                    values=(
                        ch["name"],
                        OUT_STATUS_MAP.get(ch["status"], ch["status"]),
                        OUT_STATE_MAP.get(ch["state"], ch["state"]),
                        ch["voltage"],
                        ch["current"],
                        f"{ch['current_avg']:.1f}",
                    ),
                    tags=(tag,)
                )

        self.root.after(GUI_UPDATE_MS, self.update_gui)

    # ===== PLOT UPDATE =====
    def update_plot(self):
        sel = self.selected()

        self.ax_v.clear()
        self.ax_i.clear()

        with lock:
            for ch in sel:
                if len(voltage_history[ch]) > 1:
                    t, v = zip(*voltage_history[ch])
                    self.ax_v.plot(t, v, label=f"CH{ch+1}")

                if len(current_history[ch]) > 1:
                    t, c = zip(*current_history[ch])
                    self.ax_i.plot(t, c, label=f"CH{ch+1} inst", linewidth=1, alpha=0.6)

                if len(current_avg_history[ch]) > 1:
                    t, c = zip(*current_avg_history[ch])
                    self.ax_i.plot(t, c, label=f"CH{ch+1} avg", linewidth=2)

        self.ax_v.legend()
        self.ax_i.legend()

        self.ax_v.set_title("Voltage")
        self.ax_i.set_title("Current")

        self.canvas.draw_idle()

        self.root.after(PLOT_UPDATE_MS, self.update_plot)


# ================= MAIN =================
if __name__ == "__main__":
    threading.Thread(target=can_worker, daemon=True).start()

    root = tk.Tk()
    app = App(root)
    # key bindings: space toggles acquisition, Enter restarts (enabled)
    def _toggle_acq(event=None):
        if acquire_event.is_set():
            acquire_event.clear()
        else:
            acquire_event.set()

    def _start_acq(event=None):
        acquire_event.set()

    root.bind("<space>", _toggle_acq)
    root.bind("<Return>", _start_acq)
    root.mainloop()
import can
import struct
import threading
import tkinter as tk
from tkinter import ttk
from collections import defaultdict, deque
import time

from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure

# ================= CONFIG =================
CHANNEL_COUNT = 16
BASE_ID = 0x400

CAN_CHANNEL = "COM14"
CAN_BITRATE = 1000000
# For 1 Mbps CAN traffic, 115200 serial can be a bottleneck on many adapters.
# Try higher serial speeds first and fall back to 115200.
SERIAL_BAUD_CANDIDATES = (1000000, 460800, 230400, 115200)

HISTORY_LEN = 100
GUI_UPDATE_MS = 200
PLOT_UPDATE_MS = 20
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
    {"name": "", "status": 0, "state": 0, "voltage": 0, "current": 0, "frequency": 0}
    for _ in range(CHANNEL_COUNT)
]

sys_status = {"status": 0, "batt": 0, "safety": 0}

name_parts = defaultdict(dict)

voltage_history = [deque(maxlen=HISTORY_LEN) for _ in range(CHANNEL_COUNT)]
current_history = [deque(maxlen=HISTORY_LEN) for _ in range(CHANNEL_COUNT)]
voltage_avg_history = [deque(maxlen=HISTORY_LEN) for _ in range(CHANNEL_COUNT)]
current_avg_history = [deque(maxlen=HISTORY_LEN) for _ in range(CHANNEL_COUNT)]

# frequency history for averaging (timestamps)
frequency_history = [deque() for _ in range(CHANNEL_COUNT)]

# per-channel current history for 1s averaging
current_1s_history = [deque() for _ in range(CHANNEL_COUNT)]

# frequency tracking
last_update_time = [0.0 for _ in range(CHANNEL_COUNT)]
update_count = [0 for _ in range(CHANNEL_COUNT)]

lock = threading.Lock()

# acquisition control
acquire_event = threading.Event()
acquire_event.set()

# ================= PARSING =================
def parse_u16x4(data):
    return struct.unpack("<4H", data)


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


# ================= CAN THREAD =================
def can_worker():
    backoff = 1.0
    while True:
        # try to open the bus and reconnect on errors
        bus = None
        last_error = None
        for tty_baud in SERIAL_BAUD_CANDIDATES:
            try:
                bus = can.interface.Bus(
                    channel=CAN_CHANNEL,
                    interface="slcan",
                    bitrate=CAN_BITRATE,
                    ttyBaudrate=tty_baud,
                )
                print(f"CAN connected (ttyBaudrate={tty_baud})")
                backoff = 1.0
                break
            except Exception as e:
                last_error = e

        if bus is None:
            print("CAN init failed:", repr(last_error))
            time.sleep(backoff)
            backoff = min(backoff * 2, 30.0)
            continue

        try:
            while True:
                # wait until acquisition enabled
                acquire_event.wait()
                try:
                    msg = bus.recv(timeout=0.2)
                except can.exceptions.CanOperationError as e:
                    # serial/read/write specific errors -> backoff and reconnect
                    print("CAN receive error, reconnecting:", repr(e))
                    time.sleep(backoff)
                    backoff = min(backoff * 2, 30.0)
                    break
                except Exception as e:
                    # other unexpected errors -> reconnect
                    print("CAN receive unexpected error, reconnecting:", repr(e))
                    time.sleep(backoff)
                    backoff = min(backoff * 2, 30.0)
                    break

                if msg is None:
                    continue

                cid = msg.arbitration_id
                d = msg.data

                with lock:
                    # -------- SYS STATUS --------
                    if cid == IDS["SYS_STATUS"]:
                        sys_status["status"] = d[0]
                        sys_status["batt"] = d[1] | (d[2] << 8)
                        sys_status["safety"] = d[3]

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
                            # update frequency: append timestamp and compute count in last 1s
                            last_update_time[ch] = t
                            fh = frequency_history[ch]
                            fh.append(t)
                            cutoff = t - 1.0
                            while fh and fh[0] < cutoff:
                                fh.popleft()
                            channels[ch]["frequency"] = len(fh) / 1.0
                            voltage_history[ch].append((t, vals[i]))
                            trim_history(voltage_history[ch], t)
                            voltage_avg_history[ch].append((t, average_history(voltage_history[ch])))

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
                            channels[ch]["current"] = vals[i] * 10
                            # update frequency: append timestamp and compute count in last 1s
                            last_update_time[ch] = t
                            fh = frequency_history[ch]
                            fh.append(t)
                            cutoff = t - 1.0
                            while fh and fh[0] < cutoff:
                                fh.popleft()
                            channels[ch]["frequency"] = len(fh) / 1.0
                            # append to 1s current history and compute per-channel 1s average
                            v = vals[i] * 10
                            ch_hist = current_1s_history[ch]
                            ch_hist.append((t, v))
                            cutoff_c = t - 1.0
                            while ch_hist and ch_hist[0][0] < cutoff_c:
                                ch_hist.popleft()
                            if ch_hist:
                                avg_1s = sum(x for _, x in ch_hist) / len(ch_hist)
                            else:
                                avg_1s = 0
                            channels[ch]["current_avg_1s"] = avg_1s
                            # also store the AVG_WINDOW_MS average per-channel for display/plot
                            current_history[ch].append((t, v))
                            channels[ch]["current_avg"] = average_history(current_history[ch])
                            current_avg_history[ch].append((t, average_history(current_history[ch])))

                        # update system cumulative current averaged per 1s
                        total = sum(channels[c].get("current_avg_1s", 0) for c in range(CHANNEL_COUNT))
                        sys_status["total_current"] = total

                    elif cid == IDS["NAMES"]:
                        update_names(d)
        except Exception as e:
            # generic safeguard: log and attempt reconnect
            print("CAN worker error, reconnecting:", repr(e))
            time.sleep(backoff)
            backoff = min(backoff * 2, 30.0)
        finally:
            try:
                bus.shutdown()
            except Exception:
                try:
                    bus.close()
                except Exception:
                    pass
            # ensure a pause before next reconnect attempt
            time.sleep(backoff)


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
        self.safe_lbl = tk.StringVar()
        self.total_i_lbl = tk.StringVar()

        tk.Label(sys_frame, text="State").grid(row=0, column=0)
        tk.Label(sys_frame, textvariable=self.sys_lbl).grid(row=0, column=1)

        tk.Label(sys_frame, text="Battery").grid(row=1, column=0)
        tk.Label(sys_frame, textvariable=self.batt_lbl).grid(row=1, column=1)

        tk.Label(sys_frame, text="Safety").grid(row=2, column=0)
        tk.Label(sys_frame, textvariable=self.safe_lbl).grid(row=2, column=1)

        tk.Label(sys_frame, text="Total I").grid(row=3, column=0)
        tk.Label(sys_frame, textvariable=self.total_i_lbl).grid(row=3, column=1)

        # ===== TABLE =====
        self.tree = ttk.Treeview(root)
        self.tree["columns"] = ("Name", "Status", "State", "V [mV]", "I [mA]", "I avg [mA]", "Freq [Hz]")
        for c in self.tree["columns"]:
            self.tree.heading(c, text=c)
        self.tree.pack(fill=tk.BOTH, expand=True)

        self.tree.tag_configure("ok", background="#d4ffd4")
        self.tree.tag_configure("off", background="#d8d8d8")
        self.tree.tag_configure("warn", background="#fff3b0")
        self.tree.tag_configure("fault", background="#ffb3b3")

        self.rows = [self.tree.insert("", "end", text=str(i+1), values=("", "", "", "", "", "", ""))
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

    # ===== GUI UPDATE =====
    def update_gui(self):
        with lock:
            self.sys_lbl.set(str(sys_status["status"]))
            self.batt_lbl.set(f"{sys_status['batt']} mV")
            self.safe_lbl.set(str(sys_status["safety"]))

            for i, ch in enumerate(channels):
                tag = get_tag(ch["state"], ch["status"])
                freq = ch.get("frequency", 0) or 0
                freq_str = f"{freq:.2f}"
                i_avg = ch.get("current_avg", 0) or 0
                i_avg_str = f"{i_avg:.1f}"
                self.tree.item(
                    self.rows[i],
                    values=(
                        ch["name"],
                        OUT_STATUS_MAP.get(ch["status"], ch["status"]),
                        OUT_STATE_MAP.get(ch["state"], ch["state"]),
                        ch["voltage"],
                        ch["current"],
                        i_avg_str,
                        freq_str,
                    ),
                    tags=(tag,)
                )

            # system cumulative current (1s average)
            total = sys_status.get("total_current", 0)
            self.total_i_lbl.set(f"{total:.1f} mA")

        self.root.after(GUI_UPDATE_MS, self.update_gui)

    # ===== PLOT UPDATE =====
    def update_plot(self):
        sel = self.selected()

        self.ax_v.clear()
        self.ax_i.clear()

        with lock:
            for ch in sel:
                # gather available time arrays
                t_v = v = None
                t_i = i_vals = None
                t_ca = c = None

                if len(voltage_avg_history[ch]) > 1:
                    t_v, v = zip(*voltage_avg_history[ch])

                if len(current_history[ch]) > 1:
                    t_i, i_vals = zip(*current_history[ch])

                if len(current_avg_history[ch]) > 1:
                    t_ca, c = zip(*current_avg_history[ch])

                # determine a common t0 (earliest timestamp among available series)
                candidates = []
                if t_v:
                    candidates.append(t_v[0])
                if t_i:
                    candidates.append(t_i[0])
                if t_ca:
                    candidates.append(t_ca[0])

                if candidates:
                    t0 = min(candidates)
                else:
                    t0 = None

                # plot voltage averaged history (relative to t0)
                if t_v is not None:
                    if t0 is None:
                        t_rel = [x for x in t_v]
                    else:
                        t_rel = [x - t0 for x in t_v]
                    self.ax_v.plot(t_rel, v, label=f"CH{ch+1}")

                # plot instantaneous current (raw) aligned to same t0
                if t_i is not None:
                    if t0 is None:
                        t_i_rel = [x for x in t_i]
                    else:
                        t_i_rel = [x - t0 for x in t_i]
                    self.ax_i.plot(t_i_rel, i_vals, label=f"CH{ch+1} inst", linewidth=1, alpha=0.6)

                # plot averaged current (AVG_WINDOW_MS) aligned to same t0
                if t_ca is not None:
                    if t0 is None:
                        t_ca_rel = [x for x in t_ca]
                    else:
                        t_ca_rel = [x - t0 for x in t_ca]
                    self.ax_i.plot(t_ca_rel, c, label=f"CH{ch+1} avg", linewidth=2)

        # move legends outside plot area to avoid overlap with data
        try:
            self.ax_v.legend(loc='upper left', bbox_to_anchor=(1.02, 1))
            self.ax_i.legend(loc='upper left', bbox_to_anchor=(1.02, 1))
        except Exception:
            pass

        self.ax_v.set_title("Voltage")
        self.ax_i.set_title("Current")

        # adjust layout so legends don't overlap
        try:
            self.canvas.figure.tight_layout()
            self.canvas.figure.subplots_adjust(right=0.8)
        except Exception:
            pass

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
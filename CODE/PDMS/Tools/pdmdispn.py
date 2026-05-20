import sys
import os
import time
import struct
import datetime
import csv
from collections import defaultdict, deque
import multiprocessing

import can
from can import Message

from PyQt5.QtCore import QTimer, Qt
from PyQt5.QtGui import QColor
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, 
    QGridLayout, QLabel, QCheckBox, QGroupBox, QComboBox, 
    QLineEdit, QPushButton, QTableWidget, QTableWidgetItem, QMessageBox, QHeaderView
)

# Replace Matplotlib with ultra-fast PyQtGraph
import pyqtgraph as pg

# ================= CONFIG =================
CHANNEL_COUNT = 16
PHY_INPUT_COUNT = 8
BASE_ID = 0x400

CAN_CHANNEL = "COM16"
CAN_BITRATE = 1000000
SERIAL_BAUD = 115200

HISTORY_LEN = 300
GUI_UPDATE_MS = 30    # Snappy response for text matrices
PLOT_UPDATE_MS = 10   # 25 FPS update rate - absolutely smooth with PyQtGraph

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
    "PHY_INPUTS_1_4": BASE_ID + 0x00E,
    "PHY_INPUTS_5_8": BASE_ID + 0x00F,
}

OUT_STATE_MAP = {0: "OFF", 1: "ON", 2: "ERR"}
OUT_STATUS_MAP = {
    0: "OK", 1: "OPEN_LOAD", 8: "SAFETY_OPEN",
    9: "PRIO_DIV", 10: "SOC_FAULT", 11: "I2T_FAULT",
    20: "SHORT_VSS", 21: "CTRL_FAIL", 22: "HARD_FAULT"
}

# High-Visibility Motorsport Dark Theme Configurations
DARK_STYLESHEET = """
    QMainWindow { background-color: #121212; }
    QWidget { color: #E0E0E0; font-family: 'Segoe UI', Arial, sans-serif; font-size: 12px; }
    QGroupBox { background-color: #1E1E1E; border: 1px solid #333333; border-radius: 6px; margin-top: 12px; font-weight: bold; color: #BB86FC; }
    QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }
    QTableWidget { background-color: #1E1E1E; gridline-color: #2D2D2D; border: 1px solid #333333; border-radius: 4px; }
    QHeaderView::section { background-color: #2D2D2D; color: #E0E0E0; padding: 4px; border: 1px solid #121212; font-weight: bold; }
    QComboBox, QLineEdit { background-color: #2D2D2D; border: 1px solid #444444; border-radius: 4px; padding: 4px; color: #FFFFFF; }
    QPushButton { background-color: #BB86FC; color: #121212; border: none; border-radius: 4px; padding: 6px; font-weight: bold; }
    QPushButton:hover { background-color: #D7B7FD; }
    QCheckBox::indicator { width: 14px; height: 14px; background-color: #2D2D2D; border: 1px solid #444444; border-radius: 3px; }
    QCheckBox::indicator:checked { background-color: #BB86FC; border: 1px solid #BB86FC; }
"""

# Palette of highly distinguishable colors for line tracks
TRACK_COLORS = [
    '#FF5722', '#3F51B5', '#4CAF50', '#FFEB3B', '#00BCD4', '#9C27B0', '#E91E63', '#009688',
    '#FF9800', '#795548', '#9E9E9E', '#607D8B', '#FF8A80', '#EA80FC', '#82B1FF', '#B2FF59'
]

def get_row_colors(state, status):
    if status == 0 and state == 0: return "#242424", "#A0A0A0"
    if status == 0: return "#1B3B2B", "#81C784"
    elif status < 20: return "#3E2723", "#FFB74D"
    return "#3D1C1C", "#E57373"

def parse_u16x4(data):
    if not isinstance(data, (bytes, bytearray)): data = bytes(data)
    if len(data) < 8: data = data + b"\x00" * (8 - len(data))
    return struct.unpack("<4H", data[:8])

def parse_system_status(data):
    status = data[0]
    batt_voltage = data[1] | (data[2] << 8)
    core_temp_raw = struct.unpack_from("<h", data, 3)[0]
    safety_line_state = data[5] if len(data) > 5 else 0
    return status, batt_voltage, core_temp_raw / 10.0, safety_line_state


# ================= MULTIPROCESSING BACKEND WORKER =================
def can_isolated_process(pipe_conn, tx_queue):
    try:
        can_bus = can.interface.Bus(
            channel=CAN_CHANNEL, interface="slcan", bitrate=CAN_BITRATE, ttyBaudrate=SERIAL_BAUD
        )
    except Exception as e:
        pipe_conn.send({"error": f"Link crash: {e}"})
        return

    channels = [{"name": "", "status": 0, "state": 0, "voltage": 0, "current": 0, "current_avg": 0} for _ in range(CHANNEL_COUNT)]
    phy_inputs = [0] * PHY_INPUT_COUNT
    sys_status = {"status": 0, "batt": 0, "core_temp": 0.0, "safety": 0, "total_current": 0}
    
    name_parts = defaultdict(dict)
    current_avg_window = [deque() for _ in range(CHANNEL_COUNT)]

    log_file_name = f"pdmdisp_{datetime.datetime.now():%Y%m%d-%H%M%S}.csv"
    try:
        log_f = open(log_file_name, "a", newline="")
        log_writer = csv.writer(log_f)
        header = ["timestamp", "total_i_mA", "batt_mV", "core_temp_C", "safety"]
        for i in range(CHANNEL_COUNT):
            n = i + 1
            header += [f"ch{n}_name", f"ch{n}_status", f"ch{n}_state", f"ch{n}_voltage_mV", f"ch{n}_i_inst_mA", f"ch{n}_i_avg_mA"]
        for i in range(PHY_INPUT_COUNT): header += [f"phy_in{i+1}"]
        log_writer.writerow(header)
    except Exception:
        log_writer = None

    last_gui_update = time.time()
    last_log_time = time.time()
    acquiring = True
    start_time = time.time()
    frame_counts = defaultdict(int)
    frame_first_seen = {}
    frame_last_seen = {}

    while True:
        while not tx_queue.empty():
            task = tx_queue.get_nowait()
            if task.get("cmd") == "EXIT":
                if log_writer: log_f.close()
                return
            elif task.get("cmd") == "TOGGLE": acquiring = not acquiring
            elif task.get("cmd") == "START": acquiring = True
            elif task.get("cmd") == "TX":
                try:
                    cid = task["id"]
                    pld = task["payload"]
                    can_bus.send(Message(arbitration_id=cid, data=pld, is_extended_id=(cid > 0x7FF)))
                except Exception as e:
                    pipe_conn.send({"error": f"TX err: {e}"})

        if not acquiring:
            time.sleep(0.02)
            continue

        try:
            msg = can_bus.recv(timeout=0.001)
        except Exception as e:
            pipe_conn.send({"error": f"Read err: {e}"})
            continue

        if msg is not None:
            cid = msg.arbitration_id
            d = msg.data
            t_now = time.time()
            frame_counts[cid] += 1
            frame_first_seen.setdefault(cid, t_now)
            frame_last_seen[cid] = t_now

            if cid == IDS["SYS_STATUS"]:
                status, batt_voltage, core_temp, safety_line_state = parse_system_status(d)
                sys_status.update({"status": status, "batt": batt_voltage, "core_temp": core_temp, "safety": safety_line_state})
            elif cid == IDS["STATUS_1_8"]:
                for i in range(min(8, len(d))): channels[i]["status"] = d[i]
            elif cid == IDS["STATUS_9_16"]:
                for i in range(min(8, len(d))): channels[i+8]["status"] = d[i]
            elif cid == IDS["STATE_1_16"]:
                for i in range(min(16, len(d)*2)):
                    b = d[i // 2]
                    channels[i]["state"] = (b >> 4) & 0x0F if i % 2 == 0 else b & 0x0F
            elif cid in (IDS["VOLT_1_4"], IDS["VOLT_5_8"], IDS["VOLT_9_12"], IDS["VOLT_13_16"]):
                base = {IDS["VOLT_1_4"]: 0, IDS["VOLT_5_8"]: 4, IDS["VOLT_9_12"]: 8, IDS["VOLT_13_16"]: 12}[cid]
                vals = parse_u16x4(d)
                for i in range(4):
                    ch = base + i
                    if ch < CHANNEL_COUNT: channels[ch]["voltage"] = vals[i]
            elif cid in (IDS["CURR_1_4"], IDS["CURR_5_8"], IDS["CURR_9_12"], IDS["CURR_13_16"]):
                base = {IDS["CURR_1_4"]: 0, IDS["CURR_5_8"]: 4, IDS["CURR_9_12"]: 8, IDS["CURR_13_16"]: 12}[cid]
                vals = parse_u16x4(d)
                for i in range(4):
                    ch = base + i
                    if ch < CHANNEL_COUNT:
                        inst = vals[i] * 10
                        channels[ch]["current"] = inst
                        win = current_avg_window[ch]
                        win.append((t_now, inst))
                        cutoff = t_now - 0.2
                        while win and win[0][0] < cutoff: win.popleft()
                        channels[ch]["current_avg"] = sum(v for _, v in win) / len(win) if win else 0
                sys_status["total_current"] = sum(ch["current_avg"] for ch in channels)
            elif cid == IDS["NAMES"]:
                meta = d[0]
                part, ch = (meta >> 4) & 0x0F, meta & 0x0F
                if ch < CHANNEL_COUNT:
                    name_parts[ch][part] = d[1:].decode(errors="ignore").rstrip("\x00")
                    channels[ch]["name"] = "".join(name_parts[ch][i] for i in sorted(name_parts[ch]))
            elif cid == IDS["PHY_INPUTS_1_4"]:
                vals = parse_u16x4(d)
                for i in range(min(4, PHY_INPUT_COUNT)): phy_inputs[i] = vals[i]
            elif cid == IDS["PHY_INPUTS_5_8"]:
                vals = parse_u16x4(d)
                for i in range(min(4, PHY_INPUT_COUNT - 4)): phy_inputs[i + 4] = vals[i]

            if t_now - last_log_time >= 0.05:
                last_log_time = t_now
                if log_writer:
                    try:
                        row = [t_now, f"{sys_status['total_current']:.1f}", sys_status["batt"], f"{sys_status['core_temp']:.1f}", sys_status["safety"]]
                        for ch in channels: row.extend([ch["name"], ch["status"], ch["state"], ch["voltage"], ch["current"], f"{ch['current_avg']:.1f}"])
                        row.extend(phy_inputs)
                        log_writer.writerow(row)
                    except Exception: pass

            if t_now - last_gui_update >= 0.03: 
                last_gui_update = t_now
                packet = {
                    "time": t_now - start_time,
                    "sys": sys_status.copy(),
                    "ch": [c.copy() for c in channels],
                    "phy": list(phy_inputs),
                    "frames": [
                        {
                            "id": fid,
                            "count": frame_counts[fid],
                            "freq": frame_counts[fid] / max(1e-6, frame_last_seen[fid] - frame_first_seen[fid]),
                        }
                        for fid in sorted(frame_counts)
                    ],
                }
                pipe_conn.send(packet)


# ================= FRONTEND PROCESS =================
class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("PDM CAN Dashboard (PyQtGraph Hardware-Accelerated Mode)")
        self.resize(1920, 1400)

        # High-performance plotting setup defaults globally
        pg.setConfigOption('background', '#121212')
        pg.setConfigOption('foreground', '#E0E0E0')
        pg.setConfigOption('antialias', False) 

        self.pipe_ui, self.pipe_worker = multiprocessing.Pipe(duplex=False)
        self.tx_queue = multiprocessing.Queue()
        
        self.worker_process = multiprocessing.Process(
            target=can_isolated_process, args=(self.pipe_worker, self.tx_queue), daemon=True
        )
        self.worker_process.start()

        self.latest_sys = {"status": 0, "batt": 0, "core_temp": 0.0, "safety": 0, "total_current": 0}
        self.latest_ch = [{"name": "", "status": 0, "state": 0, "voltage": 0, "current": 0, "current_avg": 0} for _ in range(CHANNEL_COUNT)]
        self.latest_phy = [0] * PHY_INPUT_COUNT
        self.latest_frames = []

        self.hist_v = [deque(maxlen=HISTORY_LEN) for _ in range(CHANNEL_COUNT)]
        self.hist_i = [deque(maxlen=HISTORY_LEN) for _ in range(CHANNEL_COUNT)]
        self.hist_iavg = [deque(maxlen=HISTORY_LEN) for _ in range(CHANNEL_COUNT)]

        # PyQtGraph curve handle mappings
        self.curves_v = {}
        self.curves_i_inst = {}
        self.curves_i_avg = {}

        self.init_ui()

        self.gui_timer = QTimer(self)
        self.gui_timer.timeout.connect(self.process_gui_refresh)
        self.gui_timer.start(GUI_UPDATE_MS)

        self.plot_timer = QTimer(self)
        self.plot_timer.timeout.connect(self.update_plots)
        self.plot_timer.start(PLOT_UPDATE_MS)

    def init_ui(self):
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout(central_widget)

        top_panel = QHBoxLayout()
        main_layout.addLayout(top_panel, stretch=0) 

        ch_box = QGroupBox("Channels Plot Display Selection")
        ch_grid = QGridLayout(ch_box)
        self.ch_checkboxes = []
        for i in range(CHANNEL_COUNT):
            cb = QCheckBox(f"CH{i+1}")
            if i == 0: cb.setChecked(True)
            ch_grid.addWidget(cb, i // 8, i % 8)
            self.ch_checkboxes.append(cb)
        top_panel.addWidget(ch_box, stretch=3)

        sys_box = QGroupBox("System Overview")
        sys_grid = QGridLayout(sys_box)
        sys_grid.addWidget(QLabel("State:"), 0, 0)
        self.lbl_sys_state = QLabel("-")
        sys_grid.addWidget(self.lbl_sys_state, 0, 1)
        sys_grid.addWidget(QLabel("Battery:"), 1, 0)
        self.lbl_batt = QLabel("- mV")
        sys_grid.addWidget(self.lbl_batt, 1, 1)
        sys_grid.addWidget(QLabel("Core Temp:"), 2, 0)
        self.lbl_temp = QLabel("- °C")
        sys_grid.addWidget(self.lbl_temp, 2, 1)
        sys_grid.addWidget(QLabel("Safety Line:"), 3, 0)
        self.lbl_safety = QLabel("-")
        sys_grid.addWidget(self.lbl_safety, 3, 1)
        sys_grid.addWidget(QLabel("Total I Avg:"), 4, 0)
        self.lbl_total_i = QLabel("- mA")
        sys_grid.addWidget(self.lbl_total_i, 4, 1)
        top_panel.addWidget(sys_box, stretch=2)

        tx_box = QGroupBox("Send CAN Frame")
        tx_layout = QGridLayout(tx_box)
        self.combo_tx_id = QComboBox()
        self.combo_tx_id.setEditable(True)
        for name, cid in IDS.items(): self.combo_tx_id.addItem(f"{name} (0x{cid:03X})")
        self.combo_tx_id.setCurrentText("SYS_STATUS (0x401)")
        tx_layout.addWidget(self.combo_tx_id, 0, 0, 1, 2)
        tx_layout.addWidget(QLabel("Data (hex):"), 1, 0)
        self.entry_tx_data = QLineEdit("00 00 00 00 00 00 00 00")
        tx_layout.addWidget(self.entry_tx_data, 1, 1)
        btn_send = QPushButton("Transmit Frame")
        btn_send.clicked.connect(self.send_selected_frame)
        tx_layout.addWidget(btn_send, 2, 0, 1, 2)
        top_panel.addWidget(tx_box, stretch=2)

        tables_layout = QHBoxLayout()
        main_layout.addLayout(tables_layout, stretch=2)

        out_table_box = QGroupBox("PDM Output Channel Bus Matrix")
        out_table_vbox = QVBoxLayout(out_table_box)
        self.table_channels = QTableWidget(CHANNEL_COUNT, 6)
        self.table_channels.setHorizontalHeaderLabels(["Name", "Status", "State", "V [mV]", "I inst [mA]", "I avg [mA]"])
        self.table_channels.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)
        for row in range(CHANNEL_COUNT):
            for col in range(6):
                item = QTableWidgetItem("")
                if col > 2: item.setTextAlignment(Qt.AlignRight | Qt.AlignVCenter)
                self.table_channels.setItem(row, col, item)
        out_table_vbox.addWidget(self.table_channels)
        tables_layout.addWidget(out_table_box, stretch=5)

        phy_table_box = QGroupBox("Device Physical Inputs")
        phy_table_vbox = QVBoxLayout(phy_table_box)
        self.table_phy = QTableWidget(PHY_INPUT_COUNT, 2)
        self.table_phy.setHorizontalHeaderLabels(["Input Line", "Input voltage [mV]"])
        self.table_phy.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeToContents)
        self.table_phy.horizontalHeader().setStretchLastSection(True)
        for row in range(PHY_INPUT_COUNT):
            self.table_phy.setItem(row, 0, QTableWidgetItem(f"Physical Input {row + 1}"))
            num_item = QTableWidgetItem("0")
            num_item.setTextAlignment(Qt.AlignRight | Qt.AlignVCenter)
            self.table_phy.setItem(row, 1, num_item)
        phy_table_vbox.addWidget(self.table_phy)
        tables_layout.addWidget(phy_table_box, stretch=2)

        frames_box = QGroupBox("Received CAN Frames")
        frames_vbox = QVBoxLayout(frames_box)
        self.table_frames = QTableWidget(0, 2)
        self.table_frames.setHorizontalHeaderLabels(["Frame ID", "Frequency [Hz]"])
        self.table_frames.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)
        frames_vbox.addWidget(self.table_frames)
        tables_layout.addWidget(frames_box, stretch=2)

        # Create PyQtGraph Layout Context Container
        graph_container = pg.GraphicsLayoutWidget()
        graph_container.setBackground('#1E1E1E')
        
        # Instantiate separate plot layouts
        self.plot_v = graph_container.addPlot(row=0, col=0)
        self.plot_i = graph_container.addPlot(row=1, col=0)

        # Add legends to plots
        self.plot_v.addLegend(offset=(10, 10))
        self.plot_i.addLegend(offset=(10, 10))

        # Style Plot Objects
        self.plot_v.setTitle("Voltage Tracks History", color='#BB86FC', size='11pt')
        self.plot_v.setLabel('left', 'Voltage', units='V')
        self.plot_v.showGrid(x=True, y=True, alpha=0.15)

        self.plot_i.setTitle("Current Consumption Profiler", color='#BB86FC', size='11pt')
        self.plot_i.setLabel('left', 'Current', units='mA')
        self.plot_i.showGrid(x=True, y=True, alpha=0.15)

        # Build hardware accelerated PlotDataItem curves
        for ch in range(CHANNEL_COUNT):
            clr = TRACK_COLORS[ch % len(TRACK_COLORS)]
            
            # Voltage Curve
            self.curves_v[ch] = pg.PlotDataItem(pen=pg.mkPen(color=clr, width=1.5), name=f"CH{ch+1}")
            self.plot_v.addItem(self.curves_v[ch])
            
            # Current Instantaneous Curve (Thick / Semi-Transparent)
            self.curves_i_inst[ch] = pg.PlotDataItem(pen=pg.mkPen(color=clr + "40", width=2.5), name=f"CH{ch+1}")
            self.plot_i.addItem(self.curves_i_inst[ch])
            
            # Current Average Curve (Thin / Solid)
            self.curves_i_avg[ch] = pg.PlotDataItem(pen=pg.mkPen(color=clr, width=1), name=f"CH{ch+1}")
            self.plot_i.addItem(self.curves_i_avg[ch])

        main_layout.addWidget(graph_container, stretch=3)

    def send_selected_frame(self):
        tx_id_text = self.combo_tx_id.currentText()
        try:
            cleaned = tx_id_text.split(" (")[0]
            arbitration_id = IDS[cleaned] if cleaned in IDS else int(cleaned, 0)
        except Exception:
            QMessageBox.critical(self, "CAN Field Error", "Invalid Target Arbitration Identifier")
            return
        try:
            payload = bytes(int(x, 16) for x in self.entry_tx_data.text().replace(",", " ").split())
        except Exception as e:
            QMessageBox.critical(self, "Payload Hex Parse Error", f"Failed parsing byte context: {e}")
            return

        self.tx_queue.put({"cmd": "TX", "id": arbitration_id, "payload": payload})

    def process_gui_refresh(self):
        updated = False
        while self.pipe_ui.poll():
            try:
                packet = self.pipe_ui.recv()
                if "error" in packet: continue
                
                self.latest_sys = packet["sys"]
                self.latest_ch = packet["ch"]
                self.latest_phy = packet["phy"]
                self.latest_frames = packet.get("frames", [])
                t_now = packet["time"]

                for i, ch in enumerate(self.latest_ch):
                    self.hist_v[i].append((t_now, ch["voltage"]))
                    self.hist_i[i].append((t_now, ch["current"]))
                    self.hist_iavg[i].append((t_now, ch["current_avg"]))
                
                updated = True
            except Exception:
                break

        if not updated: return

        self.lbl_sys_state.setText(str(self.latest_sys["status"]))
        self.lbl_batt.setText(f"{self.latest_sys['batt']} mV")
        self.lbl_temp.setText(f"{self.latest_sys['core_temp']:.1f} °C")
        self.lbl_safety.setText(str(self.latest_sys["safety"]))
        self.lbl_total_i.setText(f"{self.latest_sys['total_current']:.1f} mA")

        for i, ch in enumerate(self.latest_ch):
            bg_hex, text_hex = get_row_colors(ch["state"], ch["status"])
            status_str = OUT_STATUS_MAP.get(ch["status"], str(ch["status"]))
            state_str = OUT_STATE_MAP.get(ch["state"], str(ch["state"]))
            
            current_display_name = ch["name"] if ch["name"].strip() else f"CH{i+1}"
            if self.ch_checkboxes[i].text() != current_display_name:
                self.ch_checkboxes[i].setText(current_display_name)
            
            vals = (ch["name"], status_str, state_str, str(ch["voltage"]), str(ch["current"]), f"{ch['current_avg']:.1f}")
            
            for col_idx, text in enumerate(vals):
                item = self.table_channels.item(i, col_idx)
                if item.text() != text: item.setText(text)
                item.setBackground(QColor(bg_hex))
                item.setForeground(QColor(text_hex))

        for i, val in enumerate(self.latest_phy):
            item = self.table_phy.item(i, 1)
            if item.text() != str(val): item.setText(str(val))

        self.table_frames.setRowCount(len(self.latest_frames))
        for row, frame in enumerate(self.latest_frames):
            fid_text = f"0x{frame['id']:03X}"
            freq_text = f"{frame['freq']:.1f}"
            id_item = self.table_frames.item(row, 0)
            if id_item is None:
                id_item = QTableWidgetItem()
                self.table_frames.setItem(row, 0, id_item)
            if id_item.text() != fid_text:
                id_item.setText(fid_text)
            freq_item = self.table_frames.item(row, 1)
            if freq_item is None:
                freq_item = QTableWidgetItem()
                freq_item.setTextAlignment(Qt.AlignRight | Qt.AlignVCenter)
                self.table_frames.setItem(row, 1, freq_item)
            if freq_item.text() != freq_text:
                freq_item.setText(freq_text)

    def update_plots(self):
        # High performance pointer loop map
        selected_channels = [i for i, cb in enumerate(self.ch_checkboxes) if cb.isChecked()]

        for ch in range(CHANNEL_COUNT):
            if ch in selected_channels:
                # Direct pointer updating without rebuilding the layout geometry
                if len(self.hist_v[ch]) > 1:
                    t, v = zip(*self.hist_v[ch])
                    self.curves_v[ch].setData(t, v)
                else:
                    self.curves_v[ch].setData([], [])

                if len(self.hist_i[ch]) > 1:
                    t, c = zip(*self.hist_i[ch])
                    self.curves_i_inst[ch].setData(t, c)
                else:
                    self.curves_i_inst[ch].setData([], [])

                if len(self.hist_iavg[ch]) > 1:
                    t, c = zip(*self.hist_iavg[ch])
                    self.curves_i_avg[ch].setData(t, c)
                else:
                    self.curves_i_avg[ch].setData([], [])
            else:
                # Clear unselected line arrays instantly from GPU scene graph memory
                self.curves_v[ch].setData([], [])
                self.curves_i_inst[ch].setData([], [])
                self.curves_i_avg[ch].setData([], [])

    def closeEvent(self, event):
        self.tx_queue.put({"cmd": "EXIT"})
        self.worker_process.join(timeout=0.5)
        super().closeEvent(event)


if __name__ == "__main__":
    multiprocessing.freeze_support()
    app = QApplication(sys.argv)
    app.setStyleSheet(DARK_STYLESHEET)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())
import multiprocessing
import time
import sys
from pathlib import Path

from PyQt5.QtCore import QTimer, Qt
from PyQt5.QtGui import QColor, QIcon, QKeySequence, QPixmap
from PyQt5.QtWidgets import QSplashScreen
from PyQt5.QtWidgets import (
    QApplication,
    QComboBox,
    QGridLayout,
    QGroupBox,
    QHeaderView,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMainWindow,
    QMessageBox,
    QPushButton,
    QTabWidget,
    QTableWidget,
    QTableWidgetItem,
    QShortcut,
    QVBoxLayout,
    QWidget,
)

from pdm_can_worker import can_isolated_process
from pdm_config_tab import ConfigTab
from pdm_plot_tab import PlotPanel
from pdm_shared import (
    CHANNEL_COUNT,
    build_dark_stylesheet,
    GUI_UPDATE_MS,
    IDS,
    OUT_STATE_MAP,
    OUT_STATUS_MAP,
    PHY_INPUT_COUNT,
    PLOT_UPDATE_MS,
    get_row_colors,
)

import ctypes
# Tells Windows to treat the script as a distinct application rather than a Python script
myappid = 'mycompany.myproduct.subproduct.version' 
ctypes.windll.shell32.SetCurrentProcessExplicitAppUserModelID(myappid)

CHECKBOX_TICK_PATH = (Path(__file__).resolve().parent / "assets" / "checkbox-tick.svg").as_posix()

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("PDMS Control App")
        self.resize(1920, 1280)

        self.pipe_ui, self.pipe_worker = multiprocessing.Pipe(duplex=False)
        self.tx_queue = multiprocessing.Queue()

        self.worker_process = None
        self.worker_started = False
        self.last_frame_rx_time = 0.0

        self.latest_sys = {"status": 0, "batt": 0, "core_temp": 0.0, "safety": 0, "total_current": 0}
        self.latest_ch = [
            {"name": "", "status": 0, "state": 0, "voltage": 0, "current": 0, "current_avg": 0}
            for _ in range(CHANNEL_COUNT)
        ]
        self.latest_phy = [0] * PHY_INPUT_COUNT
        self.latest_frames = []
        self.plotting_enabled = True
        self.config_tab = None

        self.init_ui()

        self.plot_toggle_shortcut = QShortcut(QKeySequence("Ctrl+Space"), self)
        self.plot_toggle_shortcut.activated.connect(self.toggle_plotting)

        self.gui_timer = QTimer(self)
        self.gui_timer.timeout.connect(self.process_gui_refresh)
        self.gui_timer.start(GUI_UPDATE_MS)

        self.plot_timer = QTimer(self)
        self.plot_timer.timeout.connect(self.update_plots)
        self.plot_timer.start(PLOT_UPDATE_MS)

        self.hb_timer = QTimer(self)
        self.hb_timer.timeout.connect(self.update_heartbeat_status)
        self.hb_timer.start(200)

        QTimer.singleShot(0, self.start_can_worker)

    def start_can_worker(self):
        if self.worker_started:
            return
        self.worker_process = multiprocessing.Process(
            target=can_isolated_process, args=(self.pipe_worker, self.tx_queue), daemon=True
        )
        self.worker_process.start()
        self.worker_started = True

    def init_ui(self):
        tabs = QTabWidget()
        self.setCentralWidget(tabs)

        dashboard = QWidget()
        dashboard_layout = QVBoxLayout(dashboard)

        top_panel = QHBoxLayout()
        dashboard_layout.addLayout(top_panel)

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
        sys_grid.addWidget(QLabel("Invalid logic:"), 5, 0)
        self.lbl_logic_valid_mask = QLabel("-")
        self.lbl_logic_valid_mask.setWordWrap(True)
        sys_grid.addWidget(self.lbl_logic_valid_mask, 5, 1)
        top_panel.addWidget(sys_box, stretch=2)

        tx_box = QGroupBox("Send CAN Frame")
        tx_layout = QGridLayout(tx_box)
        self.combo_tx_id = QComboBox()
        self.combo_tx_id.setEditable(True)
        for name, cid in IDS.items():
            self.combo_tx_id.addItem(f"{name} (0x{cid:03X})", cid)
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
        dashboard_layout.addLayout(tables_layout, stretch=2)

        out_table_box = QGroupBox("PDM Output Channel Bus Matrix")
        out_table_vbox = QVBoxLayout(out_table_box)
        self.table_channels = QTableWidget(CHANNEL_COUNT, 6)
        self.table_channels.setHorizontalHeaderLabels(["Name", "Status", "State", "V [mV]", "I inst [mA]", "I avg [mA]"])
        self.table_channels.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)
        for row in range(CHANNEL_COUNT):
            for col in range(6):
                item = QTableWidgetItem("")
                if col > 2:
                    item.setTextAlignment(Qt.AlignRight | Qt.AlignVCenter)
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

        self.plot_panel = PlotPanel()
        dashboard_layout.addWidget(self.plot_panel, stretch=3)

        tabs.addTab(dashboard, "Dashboard")
        self.config_tab = ConfigTab()
        self.config_tab.send_binary_requested.connect(self.send_isotp_config)
        self.config_tab.send_reset_requested.connect(self.send_reset_device)
        self.config_tab.request_config_requested.connect(self.request_config_from_device)
        self.config_tab.can_frames_changed.connect(self._refresh_tx_frame_options)
        tabs.addTab(self.config_tab, "Configuration")
        self._refresh_tx_frame_options(self.config_tab.get_defined_can_frame_options())

        hb_corner = QWidget()
        hb_corner_layout = QHBoxLayout(hb_corner)
        hb_corner_layout.setContentsMargins(8, 0, 8, 0)
        hb_corner_layout.setSpacing(6)
        hb_corner_layout.addStretch(1)
        self.lbl_hb_dot = QLabel("●")
        self.lbl_hb_dot.setStyleSheet("color: #FF8A80; font-size: 14px; font-weight: bold;")
        self.lbl_hb_status = QLabel("Disconnected")
        self.lbl_hb_status.setStyleSheet("color: #FF8A80; font-weight: bold;")
        hb_corner_layout.addWidget(self.lbl_hb_dot)
        hb_corner_layout.addWidget(self.lbl_hb_status)
        tabs.setCornerWidget(hb_corner, Qt.TopRightCorner)

    def _refresh_tx_frame_options(self, options):
        current_data = self.combo_tx_id.currentData()
        current_text = self.combo_tx_id.currentText()

        self.combo_tx_id.blockSignals(True)
        self.combo_tx_id.clear()

        if options:
            for label, can_id in options:
                self.combo_tx_id.addItem(label, can_id)
        else:
            for name, cid in IDS.items():
                self.combo_tx_id.addItem(f"{name} (0x{cid:03X})", cid)

        restored = False
        if current_data is not None:
            idx = self.combo_tx_id.findData(current_data)
            if idx >= 0:
                self.combo_tx_id.setCurrentIndex(idx)
                restored = True

        if not restored and current_text:
            idx = self.combo_tx_id.findText(current_text)
            if idx >= 0:
                self.combo_tx_id.setCurrentIndex(idx)
                restored = True

        if not restored and self.combo_tx_id.count() > 0:
            self.combo_tx_id.setCurrentIndex(0)

        self.combo_tx_id.blockSignals(False)

    def send_isotp_config(self, payload):
        self.tx_queue.put({"cmd": "ISOTP_SEND", "id": 0x450, "payload": payload})

    def send_reset_device(self):
        self.tx_queue.put({"cmd": "RESET_DEVICE"})

    def request_config_from_device(self):
        self.tx_queue.put({"cmd": "REQUEST_CONFIG"})

    def send_selected_frame(self):
        tx_id_text = self.combo_tx_id.currentText()
        try:
            current_data = self.combo_tx_id.currentData()
            if current_data is not None:
                arbitration_id = int(current_data)
            else:
                cleaned = tx_id_text.split(" (")[0]
                arbitration_id = IDS[cleaned] if cleaned in IDS else int(cleaned, 0)
        except Exception:
            QMessageBox.critical(self, "CAN Field Error", "Invalid Target Arbitration Identifier")
            return

        try:
            payload = bytes(int(x, 16) for x in self.entry_tx_data.text().replace(",", " ").split())
        except Exception as exc:
            QMessageBox.critical(self, "Payload Hex Parse Error", f"Failed parsing byte context: {exc}")
            return

        self.tx_queue.put({"cmd": "TX", "id": arbitration_id, "payload": payload})

    def process_gui_refresh(self):
        updated = False
        while self.pipe_ui.poll():
            try:
                packet = self.pipe_ui.recv()
                if "isotp_progress" in packet:
                    if self.config_tab is not None:
                        self.config_tab.set_isotp_state(
                            packet.get("isotp_progress", 0),
                            packet.get("isotp_status", ""),
                            busy=not packet.get("isotp_done", False),
                        )
                        if packet.get("isotp_error"):
                            self.config_tab.set_isotp_state(0, f"Error: {packet['isotp_error']}", busy=False)
                    continue
                if "isotp_config_payload" in packet:
                    if self.config_tab is not None:
                        self.config_tab.load_binary_payload(packet["isotp_config_payload"])
                        self.config_tab.set_isotp_state(100, "Config received and loaded", busy=False)
                    continue
                if "error" in packet:
                    if self.config_tab is not None:
                        self.config_tab.set_isotp_state(0, f"Error: {packet['error']}", busy=False)
                    continue

                self.latest_sys = packet["sys"]
                self.latest_ch = packet["ch"]
                self.latest_phy = packet["phy"]
                self.latest_frames = packet.get("frames", [])
                self.plot_panel.update_from_packet(packet)
                self.last_frame_rx_time = time.time()
                updated = True
            except Exception:
                break

        if not updated:
            return

        self.lbl_sys_state.setText(str(self.latest_sys["status"]))
        self.lbl_batt.setText(f"{self.latest_sys['batt']} mV")
        self.lbl_temp.setText(f"{self.latest_sys['core_temp']:.1f} °C")
        self.lbl_safety.setText(str(self.latest_sys["safety"]))
        self.lbl_total_i.setText(f"{self.latest_sys['total_current']:.1f} mA")
        logic_valid_mask = int(self.latest_sys.get("logicValidMask", 0))
        invalid_outputs = [str(index + 1) for index in range(CHANNEL_COUNT) if not (logic_valid_mask & (1 << index))]
        if invalid_outputs:
            self.lbl_logic_valid_mask.setText(", ".join(invalid_outputs))
            self.lbl_logic_valid_mask.setStyleSheet("color: #FF8A80; font-weight: bold;")
        else:
            self.lbl_logic_valid_mask.setText("None")
            self.lbl_logic_valid_mask.setStyleSheet("")

        for i, ch in enumerate(self.latest_ch):
            bg_hex, text_hex = get_row_colors(ch["state"], ch["status"])
            status_str = OUT_STATUS_MAP.get(ch["status"], str(ch["status"]))
            state_str = OUT_STATE_MAP.get(ch["state"], str(ch["state"]))
            vals = (ch["name"], status_str, state_str, str(ch["voltage"]), str(ch["current"]), f"{ch['current_avg']:.1f}")

            for col_idx, text in enumerate(vals):
                item = self.table_channels.item(i, col_idx)
                if item.text() != text:
                    item.setText(text)
                item.setBackground(QColor(bg_hex))
                item.setForeground(QColor(text_hex))

        self.plot_panel.apply_channel_names([ch["name"] for ch in self.latest_ch])

        for i, val in enumerate(self.latest_phy):
            item = self.table_phy.item(i, 1)
            if item.text() != str(val):
                item.setText(str(val))

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

        self.update_heartbeat_status()

    def update_heartbeat_status(self):
        if self.last_frame_rx_time <= 0:
            self.lbl_hb_status.setText("Disconnected")
            self.lbl_hb_status.setStyleSheet("color: #FF8A80; font-weight: bold;")
            self.lbl_hb_dot.setStyleSheet("color: #FF8A80; font-size: 14px; font-weight: bold;")
            return

        age_s = time.time() - self.last_frame_rx_time
        if age_s > 1.0:
            self.lbl_hb_status.setText(f"Disconnected ({age_s:.1f}s)")
            self.lbl_hb_status.setStyleSheet("color: #FF8A80; font-weight: bold;")
            self.lbl_hb_dot.setStyleSheet("color: #FF8A80; font-size: 14px; font-weight: bold;")
        else:
            self.lbl_hb_status.setText(f"Connected ({age_s:.1f}s)")
            self.lbl_hb_status.setStyleSheet("color: #81C784; font-weight: bold;")
            self.lbl_hb_dot.setStyleSheet("color: #81C784; font-size: 14px; font-weight: bold;")

    def update_plots(self):
        if not self.plotting_enabled:
            return
        self.plot_panel.refresh_plots()

    def toggle_plotting(self):
        self.plotting_enabled = not self.plotting_enabled
        if self.plotting_enabled:
            self.plot_timer.start(PLOT_UPDATE_MS)
        else:
            self.plot_timer.stop()

    def closeEvent(self, event):
        if self.worker_started:
            self.tx_queue.put({"cmd": "EXIT"})
            self.worker_process.join(timeout=0.5)
        super().closeEvent(event)


if __name__ == "__main__":
    multiprocessing.freeze_support()
    app = QApplication(sys.argv)
    app.setStyleSheet(build_dark_stylesheet(CHECKBOX_TICK_PATH))
    app.setWindowIcon(QIcon("assets/pdms.ico"))
    # Show a splash/loading screen during application bring-up
    splash_pix_path = Path(__file__).resolve().parent / "assets" / "pdms_splash.png"
    if splash_pix_path.exists():
        pix = QPixmap(str(splash_pix_path))
    else:
        # Fallback: small blank pixmap with icon if no splash image available
        pix = QPixmap(480, 300)
        pix.fill(QColor('#2D2D2D'))

    splash = QSplashScreen(pix)
    splash.showMessage("Loading PDMS...", Qt.AlignBottom | Qt.AlignHCenter, QColor("#FFFFFF"))
    splash.show()
    app.processEvents()

    window = MainWindow()
    window.show()

    # Finish the splash after the main window is visible (short delay to let init finish)
    QTimer.singleShot(600, lambda: splash.finish(window))

    sys.exit(app.exec_())

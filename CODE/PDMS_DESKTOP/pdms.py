import multiprocessing
import json
import time
import sys
from pathlib import Path

from PyQt5.QtCore import QTimer, Qt
from PyQt5.QtGui import QColor, QIcon, QKeySequence, QPixmap, QPainter, QFont
from PyQt5.QtWidgets import QSplashScreen
from PyQt5.QtWidgets import (
    QAction,
    QApplication,
    QComboBox,
    QDialog,
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
from pdm_control_tab import ControlConfigPage
from pdm_config_tab import ConfigTab
from pdm_plot_tab import PlotPanel
import pdm_shared
from pdm_shared import (
    APP_VERSION,
    CHANNEL_COUNT,
    build_dark_stylesheet,
    GUI_UPDATE_MS,
    IDS,
    OUT_STATE_MAP,
    OUT_STATUS_MAP,
    PHY_INPUT_COUNT,
    PLOT_UPDATE_MS,
    get_row_colors,
    get_asset_path,
    load_app_config,
    save_usb_device_config,
    set_can_channel_runtime,
)

import ctypes
# Tells Windows to treat the script as a distinct application rather than a Python script
myappid = 'mycompany.myproduct.subproduct.version' 
ctypes.windll.shell32.SetCurrentProcessExplicitAppUserModelID(myappid)

CHECKBOX_TICK_PATH = get_asset_path("assets/checkbox-tick.svg")


def _discover_serial_devices():
    try:
        from serial.tools import list_ports
    except Exception:
        return []

    devices = []
    try:
        for port in list_ports.comports():
            device = str(getattr(port, "device", "") or "").strip()
            if not device:
                continue
            description = str(getattr(port, "description", "") or "Unknown device")
            devices.append((device, description))
    except Exception:
        return []

    devices.sort(key=lambda item: item[0])
    return devices


def _load_saved_usb_device():
    config = load_app_config()
    candidate = config.get("usb_device") if isinstance(config, dict) else None
    if isinstance(candidate, str) and candidate.strip():
        return candidate.strip()
    return None


class StartupDeviceDialog(QDialog):
    def __init__(self, devices, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Select USB Device")
        self.setModal(True)
        self.setMinimumWidth(460)
        self.selected_device = None
        self.offline_mode = False

        layout = QVBoxLayout(self)
        intro = QLabel("No previous selection of USB to CAN interface was found. Choose a SLCAN interface device or continue in offline mode.")
        intro.setWordWrap(True)
        layout.addWidget(intro)
        layout.addSpacing(12)

        self.combo_device = QComboBox()
        for device, description in devices:
            self.combo_device.addItem(f"{device} - {description}", device)
        layout.addWidget(self.combo_device)
        layout.addSpacing(12)

        button_row = QHBoxLayout()
        btn_use = QPushButton("Use Device")
        btn_offline = QPushButton("Offline")
        btn_cancel = QPushButton("Cancel")
        button_row.addWidget(btn_use)
        button_row.addWidget(btn_offline)
        button_row.addWidget(btn_cancel)
        layout.addLayout(button_row)

        btn_use.clicked.connect(self._accept_device)
        btn_offline.clicked.connect(self._accept_offline)
        btn_cancel.clicked.connect(self.reject)

    def _accept_device(self):
        device = self.combo_device.currentData()
        if not device and self.combo_device.currentText():
            device = self.combo_device.currentText().split(" - ", 1)[0].strip()
        if not device:
            QMessageBox.warning(self, "No device selected", "Please select a USB device or choose Offline mode.")
            return
        self.selected_device = str(device)
        self.offline_mode = False
        self.accept()

    def _accept_offline(self):
        self.selected_device = None
        self.offline_mode = True
        self.accept()


def _resolve_startup_transport():
    saved_device = _load_saved_usb_device()
    if saved_device:
        set_can_channel_runtime(saved_device)
        return {"offline": False, "channel": saved_device}

    devices = _discover_serial_devices()
    if not devices:
        answer = QMessageBox.question(
            None,
            "No serial devices",
            "No serial devices were detected and no valid app_config.json exists. Start in offline mode?",
            QMessageBox.Yes | QMessageBox.No,
            QMessageBox.Yes,
        )
        if answer == QMessageBox.Yes:
            return {"offline": True, "channel": None}
        return None

    dialog = StartupDeviceDialog(devices)
    if dialog.exec_() != QDialog.Accepted:
        return None

    if dialog.offline_mode:
        return {"offline": True, "channel": None}

    selected_device = dialog.selected_device
    if not selected_device:
        return None

    set_can_channel_runtime(selected_device)
    try:
        save_usb_device_config(selected_device)
    except Exception as exc:
        QMessageBox.warning(
            None,
            "Configuration warning",
            f"Selected device will be used for this session, but app_config.json could not be saved:\n{exc}",
        )
    return {"offline": False, "channel": selected_device}


def _kpi_label_style(color="#E0E0E0"):
    return (
        "QLabel {"
        f" color: {color};"
        " font-size: 22px;"
        " font-weight: bold;"
        " padding: 8px 12px;"
        " border: 1px solid #333333;"
        " border-radius: 8px;"
        " background-color: #151515;"
        " }"
    )


def _blend_color(start_hex, end_hex, ratio):
    ratio = max(0.0, min(1.0, float(ratio)))
    start = QColor(start_hex)
    end = QColor(end_hex)
    red = int(start.red() + (end.red() - start.red()) * ratio)
    green = int(start.green() + (end.green() - start.green()) * ratio)
    blue = int(start.blue() + (end.blue() - start.blue()) * ratio)
    return f"#{red:02X}{green:02X}{blue:02X}"


SYS_STATUS_MAP = {
    0: "OK",
    1: "ERROR",
    2: "UNDER VOLTAGE LOCK OUT",
}


def _format_voltage_mv(value):
    try:
        return f"{float(value) / 1000.0:.1f}V"
    except Exception:
        return "-"


def _format_channel_error_summary(channels):
    error_channels = []
    for index, channel in enumerate(channels or []):
        try:
            if int(channel.get("status", 0)) != 0:
                error_channels.append(str(index + 1))
        except Exception:
            continue
    if not error_channels:
        return "In error: None", "#09BC8A"
    return f"In error: {', '.join(error_channels)}", "#E30026"


def _battery_kpi_color(voltage_mv):
    try:
        voltage_v = float(voltage_mv) / 1000.0
    except Exception:
        return "#E30026"

    if voltage_v <= 8.0 or voltage_v >= 18.0:
        return "#E30026"

    if voltage_v <= 14.5:
        if voltage_v <= 10.0:
            ratio = (voltage_v - 8.0) / (10.0 - 8.0)
            return _blend_color("#E30026", "#F2994A", ratio)
        if voltage_v <= 12.0:
            ratio = (voltage_v - 10.0) / (12.0 - 10.0)
            return _blend_color("#F2994A", "#F2C94C", ratio)
        ratio = (voltage_v - 12.0) / (14.5 - 12.0)
        return _blend_color("#F2C94C", "#09BC8A", ratio)

    if voltage_v <= 16.0:
        ratio = (voltage_v - 14.5) / (16.0 - 14.5)
        return _blend_color("#09BC8A", "#F2C94C", ratio)
    if voltage_v <= 17.0:
        ratio = (voltage_v - 16.0) / (17.0 - 16.0)
        return _blend_color("#F2C94C", "#F2994A", ratio)
    ratio = (voltage_v - 17.0) / (18.0 - 17.0)
    return _blend_color("#F2994A", "#E30026", ratio)

class MainWindow(QMainWindow):
    def __init__(self, offline_mode=False):
        super().__init__()
        self.setWindowTitle("PDMS Control App")
        self.resize(1920, 1280)
        self.offline_mode = bool(offline_mode)

        self.pipe_ui, self.pipe_worker = multiprocessing.Pipe(duplex=False)
        self.tx_queue = multiprocessing.Queue()

        self.worker_process = None
        self.worker_started = False
        self.serial_device_ready = False
        self._device_config_request_pending = False
        self._device_config_prompt = None
        self._pending_device_config = None
        self._connection_state = "disconnected"
        self._config_check_suppressed_until = 0.0
        self.last_frame_rx_time = 0.0

        self.latest_sys = {"status": 0, "batt": 0, "core_temp": 0.0, "safety": 0, "total_current": 0}
        self.latest_ch = [
            {
                "name": "",
                "status": 0,
                "state": 0,
                "voltage": 0,
                "current": 0,
                "current_avg": 0,
                "current_rms": 0,
                "i2t_heat": 0,
                "soc_threshold": 0,
            }
            for _ in range(CHANNEL_COUNT)
        ]
        self.latest_phy = [0] * PHY_INPUT_COUNT

        self.lates_imu = {"accX": 0.0, "accY": 0.0, "accZ": 0.0, "pitch": 0.0, "roll": 0.0, "yaw": 0.0}
        self.latest_frames = []
        self.plotting_enabled = True
        self.config_tab = None
        self.control_tab = None

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

        if self.offline_mode:
            self._connection_state = "offline"
            self._set_heartbeat_status("Offline mode", "#90CAF9")
        else:
            QTimer.singleShot(0, self.start_can_worker)

    def start_can_worker(self):
        if self.worker_started:
            return
        self.offline_mode = False
        self.serial_device_ready = False
        self.last_frame_rx_time = 0.0
        self.worker_process = multiprocessing.Process(
            target=can_isolated_process, args=(self.pipe_worker, self.tx_queue), daemon=True
        )
        self.worker_process.start()
        self.worker_started = True

    def stop_can_worker(self):
        if not self.worker_started:
            return
        try:
            self.tx_queue.put({"cmd": "EXIT"})
        except Exception:
            pass
        if self.worker_process is not None:
            try:
                self.worker_process.join(timeout=0.8)
            except Exception:
                pass
            if self.worker_process.is_alive():
                try:
                    self.worker_process.terminate()
                    self.worker_process.join(timeout=0.5)
                except Exception:
                    pass
        self.worker_process = None
        self.worker_started = False
        self.serial_device_ready = False
        self.last_frame_rx_time = 0.0
        self._connection_state = "disconnected"

    def _show_transport_dialog(self, force_prompt=False):
        if not force_prompt:
            saved_device = _load_saved_usb_device()
            if saved_device:
                set_can_channel_runtime(saved_device)
                return {"offline": False, "channel": saved_device}

        devices = _discover_serial_devices()
        if not devices:
            answer = QMessageBox.question(
                self,
                "No serial devices",
                "No serial devices were detected. Switch to offline mode?",
                QMessageBox.Yes | QMessageBox.No,
                QMessageBox.Yes,
            )
            if answer == QMessageBox.Yes:
                return {"offline": True, "channel": None}
            return None

        dialog = StartupDeviceDialog(devices, self)
        if dialog.exec_() != QDialog.Accepted:
            return None

        if dialog.offline_mode:
            return {"offline": True, "channel": None}

        selected_device = dialog.selected_device
        if not selected_device:
            return None

        set_can_channel_runtime(selected_device)
        try:
            save_usb_device_config(selected_device)
        except Exception as exc:
            QMessageBox.warning(
                self,
                "Configuration warning",
                f"Selected device will be used for this session, but app_config.json could not be saved:\n{exc}",
            )
        return {"offline": False, "channel": selected_device}

    def change_usb_device(self):
        selection = self._show_transport_dialog(force_prompt=True)
        if selection is None:
            return

        target_offline = bool(selection.get("offline", False))
        previous_offline = self.offline_mode

        if target_offline:
            self.stop_can_worker()
            self.offline_mode = True
            self._set_heartbeat_status("Offline mode", "#90CAF9")
            QMessageBox.information(self, "Connection updated", "Switched to offline mode.")
            return

        self.stop_can_worker()
        self.offline_mode = False
        self.start_can_worker()
        if previous_offline:
            QMessageBox.information(
                self,
                "Connection updated",
                f"Switched from offline mode to {pdm_shared.CAN_CHANNEL}.",
            )
        else:
            QMessageBox.information(
                self,
                "Connection updated",
                f"USB device changed to {pdm_shared.CAN_CHANNEL}.",
            )

    def init_ui(self):
        file_menu = self.menuBar().addMenu("File")
        self.action_load_binary = QAction("Load binary", self)
        self.action_export_binary = QAction("Export binary", self)
        self.action_load_json = QAction("Load JSON", self)
        self.action_export_json = QAction("Export JSON", self)
        file_menu.addAction(self.action_load_binary)
        file_menu.addAction(self.action_export_binary)
        file_menu.addAction(self.action_load_json)
        file_menu.addAction(self.action_export_json)

        device_menu = self.menuBar().addMenu("Device")
        self.action_request_config = QAction("Request config", self)
        self.action_send_to_device = QAction("Send to device", self)
        self.action_reset_device = QAction("Reset device", self)
        device_menu.addAction(self.action_request_config)
        device_menu.addAction(self.action_send_to_device)
        device_menu.addAction(self.action_reset_device)
        device_menu.addSeparator()

        self.action_change_usb = QAction("Change USB device...", self)
        self.action_change_usb.triggered.connect(self.change_usb_device)
        device_menu.addAction(self.action_change_usb)

        tabs = QTabWidget()
        tabs.setContentsMargins(0, 6, 0, 0)
        tabs.setStyleSheet("QTabWidget { margin-top: 5px; }")
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

        sys_grid.addWidget(QLabel("Device temp:"), 2, 0)
        self.lbl_temp = QLabel("- °C")
        sys_grid.addWidget(self.lbl_temp, 2, 1)

        sys_grid.addWidget(QLabel("Safety Line:"), 3, 0)
        self.lbl_safety = QLabel("-")
        sys_grid.addWidget(self.lbl_safety, 3, 1)

        sys_grid.addWidget(QLabel("Total I Avg:"), 4, 0)
        self.lbl_total_i = QLabel("- mA")
        sys_grid.addWidget(self.lbl_total_i, 4, 1)

        big_kpi_row = QHBoxLayout()
        big_kpi_row.setSpacing(8)

        self.lbl_batt_big = QLabel("Battery: -")
        self.lbl_batt_big.setAlignment(Qt.AlignCenter)
        self.lbl_batt_big.setStyleSheet(_kpi_label_style("#4FC3F7"))
        big_kpi_row.addWidget(self.lbl_batt_big, stretch=1)

        self.lbl_itot = QLabel("I<sub>tot</sub>: - A")
        self.lbl_itot.setTextFormat(Qt.RichText)
        self.lbl_itot.setAlignment(Qt.AlignCenter)
        self.lbl_itot.setStyleSheet(_kpi_label_style("#F2C94C"))
        big_kpi_row.addWidget(self.lbl_itot, stretch=1)

        self.lbl_safety_big = QLabel("Safety Line: -")
        self.lbl_safety_big.setAlignment(Qt.AlignCenter)
        self.lbl_safety_big.setStyleSheet(_kpi_label_style("#E0E0E0"))
        big_kpi_row.addWidget(self.lbl_safety_big, stretch=1)

        self.lbl_error_big = QLabel("Channels in error: None")
        self.lbl_error_big.setAlignment(Qt.AlignCenter)
        self.lbl_error_big.setStyleSheet(_kpi_label_style("#09BC8A"))
        big_kpi_row.addWidget(self.lbl_error_big, stretch=1)

        sys_grid.addLayout(big_kpi_row, 6, 0, 1, 4)

        sys_grid.addWidget(QLabel("Invalid logic:"), 5, 0)
        self.lbl_logic_valid_mask = QLabel("-")
        self.lbl_logic_valid_mask.setWordWrap(True)
        
        sys_grid.addWidget(QLabel("Acc X:"), 0, 2)
        self.lbl_acc_x = QLabel("- g")
        sys_grid.addWidget(self.lbl_acc_x, 0, 3)

        sys_grid.addWidget(QLabel("Acc Y:"), 1, 2)
        self.lbl_acc_y = QLabel("- g")
        sys_grid.addWidget(self.lbl_acc_y, 1, 3)

        sys_grid.addWidget(QLabel("Acc Z:"), 2, 2)
        self.lbl_acc_z = QLabel("- g")
        sys_grid.addWidget(self.lbl_acc_z, 2, 3)

        sys_grid.addWidget(QLabel("Pitch:"), 3, 2)
        self.lbl_gyro_x = QLabel("- dps")
        sys_grid.addWidget(self.lbl_gyro_x, 3, 3)

        sys_grid.addWidget(QLabel("Roll:"), 4, 2)
        self.lbl_gyro_y = QLabel("- dps")
        sys_grid.addWidget(self.lbl_gyro_y, 4, 3)

        sys_grid.addWidget(QLabel("Yaw:"), 5, 2)
        self.lbl_gyro_z = QLabel("- dps")
        sys_grid.addWidget(self.lbl_gyro_z, 5, 3)
        
        
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
        self.table_channels = QTableWidget(CHANNEL_COUNT, 9)
        self.table_channels.setHorizontalHeaderLabels([
            "Name",
            "Status",
            "State",
            "V [mV]",
            "I inst [mA]",
            "I avg [mA]",
            "Irms [mA]",
            "I2T heat [%]",
            "SOC threshold",
        ])
        self.table_channels.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)
        self.table_channels.verticalHeader().setDefaultSectionSize(22)
        self.table_channels.verticalHeader().setMinimumSectionSize(18)
        self.table_channels.setStyleSheet("QTableWidget::item { padding: 1px 3px; }")
        for row in range(CHANNEL_COUNT):
            for col in range(9):
                item = QTableWidgetItem("")
                if col > 2:
                    item.setTextAlignment(Qt.AlignRight | Qt.AlignVCenter)
                self.table_channels.setItem(row, col, item)
            self.table_channels.setRowHeight(row, 22)
        out_table_vbox.addWidget(self.table_channels)
        tables_layout.addWidget(out_table_box, stretch=5)

        phy_table_box = QGroupBox("Device Physical Inputs")
        phy_table_vbox = QVBoxLayout(phy_table_box)
        self.table_phy = QTableWidget(PHY_INPUT_COUNT, 2)
        self.table_phy.setHorizontalHeaderLabels(["Input Line", "Input voltage [mV]"])
        self.table_phy.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeToContents)
        self.table_phy.horizontalHeader().setStretchLastSection(True)
        self.table_phy.verticalHeader().setDefaultSectionSize(22)
        self.table_phy.verticalHeader().setMinimumSectionSize(18)
        self.table_phy.setStyleSheet("QTableWidget::item { padding: 1px 3px; }")
        for row in range(PHY_INPUT_COUNT):
            self.table_phy.setItem(row, 0, QTableWidgetItem(f"Physical Input {row + 1}"))
            num_item = QTableWidgetItem("0")
            num_item.setTextAlignment(Qt.AlignRight | Qt.AlignVCenter)
            self.table_phy.setItem(row, 1, num_item)
            self.table_phy.setRowHeight(row, 22)
        phy_table_vbox.addWidget(self.table_phy)
        tables_layout.addWidget(phy_table_box, stretch=2)

        frames_box = QGroupBox("Received CAN Frames")
        frames_vbox = QVBoxLayout(frames_box)
        self.table_frames = QTableWidget(0, 2)
        self.table_frames.setHorizontalHeaderLabels(["Frame ID", "Frequency [Hz]"])
        self.table_frames.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)
        self.table_frames.verticalHeader().setDefaultSectionSize(20)
        self.table_frames.verticalHeader().setMinimumSectionSize(18)
        self.table_frames.setStyleSheet("QTableWidget::item { padding: 1px 3px; }")
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
        self.config_tab.config_applied.connect(self._refresh_control_tab)
        self.config_tab.inputs_page.inputsChanged.connect(self._refresh_control_tab)
        for channel_widget in self.config_tab.channel_widgets:
            channel_widget.logic_page.logicChanged.connect(self._refresh_control_tab)
            channel_widget.edit_name.textChanged.connect(self._refresh_control_tab)
        tabs.addTab(self.config_tab, "Configuration")
        self.control_tab = ControlConfigPage()
        self.control_tab.can_frame_requested.connect(self.send_can_frame)
        tabs.addTab(self.control_tab, "Control")
        self._refresh_tx_frame_options(self.config_tab.get_defined_can_frame_options())
        self._default_config_signature = self._config_signature(
            self._canonicalize_config(self.config_tab.collect_config())
        )
        self._refresh_control_tab()

        self.action_load_binary.triggered.connect(self.config_tab.load_binary)
        self.action_export_binary.triggered.connect(self.config_tab.export_binary)
        self.action_load_json.triggered.connect(self.config_tab.load_json)
        self.action_export_json.triggered.connect(self.config_tab.export_json)
        self.action_request_config.triggered.connect(self.config_tab.request_device_config)
        self.action_send_to_device.triggered.connect(self.config_tab.send_binary_to_device)
        self.action_reset_device.triggered.connect(self.config_tab.reset_device)

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

    def _set_heartbeat_status(self, text, color_hex):
        self.lbl_hb_status.setText(text)
        self.lbl_hb_status.setStyleSheet(f"color: {color_hex}; font-weight: bold;")
        self.lbl_hb_dot.setStyleSheet(f"color: {color_hex}; font-size: 14px; font-weight: bold;")

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
        self._config_check_suppressed_until = time.time() + 5.0
        self._refresh_control_tab()
        self.tx_queue.put({"cmd": "ISOTP_SEND", "id": 0x450, "payload": payload})

    def send_reset_device(self):
        self.tx_queue.put({"cmd": "RESET_DEVICE"})

    def request_config_from_device(self):
        if self._device_config_request_pending:
            return
        self._device_config_request_pending = True
        self.tx_queue.put({"cmd": "REQUEST_CONFIG"})

    def send_can_frame(self, arbitration_id, payload):
        self.tx_queue.put({"cmd": "TX", "id": int(arbitration_id), "payload": payload})

    def _refresh_control_tab(self):
        if self.control_tab is None or self.config_tab is None:
            return

        self.control_tab.set_physical_inputs(self.config_tab.inputs_page.to_dict().get("physical", []))
        can_inputs = self.config_tab.inputs_page.to_dict().get("can", [])
        usage_by_input = self.config_tab.build_usage_by_input()
        self.control_tab.refresh_controls(can_inputs, usage_by_input)

    def _request_config_if_needed(self):
        if self.config_tab is None or self._device_config_request_pending:
            return
        if time.time() < self._config_check_suppressed_until:
            return
        self.request_config_from_device()

    def _config_signature(self, config):
        try:
            return json.dumps(config, sort_keys=True, separators=(",", ":"))
        except Exception:
            return None

    def _canonicalize_config(self, config):
        if not isinstance(config, dict):
            return config

        result = dict(config)

        def _normalize_i2t_cfg(i2t_cfg):
            if not isinstance(i2t_cfg, dict):
                return {
                    "useI2t": False,
                    "nominalCurrent": 0,
                    "nominalCurrentSq": 0,
                    "timeThreshold": 0,
                    "i2tThreshold": 0,
                }

            nominal_current = int(i2t_cfg.get("nominalCurrent", 0))
            nominal_current = max(0, (nominal_current // 10) * 10)
            time_threshold = int(i2t_cfg.get("timeThreshold", 0))
            nominal_current_sq = nominal_current * nominal_current
            i2t_threshold = nominal_current_sq * time_threshold
            return {
                "useI2t": bool(i2t_cfg.get("useI2t", False)),
                "nominalCurrent": nominal_current,
                "nominalCurrentSq": nominal_current_sq,
                "timeThreshold": time_threshold,
                "i2tThreshold": i2t_threshold,
            }

        def _normalize_soc_cfg(soc_cfg):
            if not isinstance(soc_cfg, dict):
                return {
                    "useSoc": False,
                    "nominalThreshold": 0,
                    "allowInrush": False,
                    "inrushInput": None,
                    "inrushWindowFromStart": 0,
                    "inrushThreshold": 0,
                    "inrushTimeThreshold": 0,
                }

            allow_inrush = bool(soc_cfg.get("allowInrush", False))
            raw_inrush_input = soc_cfg.get("inrushInput")
            inrush_input = None
            if allow_inrush and raw_inrush_input is not None:
                try:
                    candidate = int(raw_inrush_input)
                except Exception:
                    candidate = 0xFFFF
                if candidate not in (0xFF, 0xFFFF):
                    inrush_input = candidate

            return {
                "useSoc": bool(soc_cfg.get("useSoc", False)),
                "nominalThreshold": int(soc_cfg.get("nominalThreshold", 0)),
                "allowInrush": allow_inrush,
                "inrushInput": inrush_input,
                "inrushWindowFromStart": int(soc_cfg.get("inrushWindowFromStart", 0)),
                "inrushThreshold": int(soc_cfg.get("inrushThreshold", 0)),
                "inrushTimeThreshold": int(soc_cfg.get("inrushTimeThreshold", 0)),
            }

        inputs = result.get("inputs")
        if isinstance(inputs, dict):
            canonical_inputs = {}

            physical_inputs = inputs.get("physical") or []
            canonical_inputs["physical"] = [
                {
                    "location": int(item.get("location", index)) if isinstance(item, dict) else index,
                    "type": int(item.get("type", 0)) if isinstance(item, dict) else 0,
                    "mode": int(item.get("mode", 0)) if isinstance(item, dict) else 0,
                }
                for index, item in enumerate(physical_inputs)
            ]

            can_inputs = inputs.get("can") or []
            canonical_inputs["can"] = [
                {
                    "isUsed": bool(item.get("isUsed", False)) if isinstance(item, dict) else False,
                    "canInstance": int(item.get("canInstance", 0)) if isinstance(item, dict) else 0,
                    "canId": int(item.get("canId", 0)) if isinstance(item, dict) else 0,
                    "offset": int(item.get("offset", 0)) if isinstance(item, dict) else 0,
                    "dataType": int(item.get("dataType", 0)) if isinstance(item, dict) else 0,
                    "location": int(item.get("location", index)) if isinstance(item, dict) else index,
                    "type": int(item.get("type", 0)) if isinstance(item, dict) else 0,
                    "mode": int(item.get("mode", 0)) if isinstance(item, dict) else 0,
                }
                for index, item in enumerate(can_inputs)
            ]

            result["inputs"] = canonical_inputs

        channels = result.get("channels")
        if isinstance(channels, list):
            canonical_channels = []
            for item in channels:
                if not isinstance(item, dict):
                    canonical_channels.append(item)
                    continue

                channel_item = dict(item)
                safety = dict(channel_item.get("safety") or {})
                safety["socCfg"] = _normalize_soc_cfg(safety.get("socCfg"))
                safety["i2tCfg"] = _normalize_i2t_cfg(safety.get("i2tCfg"))
                channel_item["safety"] = safety
                canonical_channels.append(channel_item)

            result["channels"] = canonical_channels

        logic = result.get("logic")
        if isinstance(logic, list):
            def _canonical_logic_item(item):
                exp = item.get("exp", {}) if isinstance(item, dict) else {}
                input1_type = int(exp.get("input1Type", 0))
                input2_type = int(exp.get("input2Type", 0))

                input1_id = int(exp.get("input1ID", 0)) if input1_type == 0x00 else 0
                input1_const = int(exp.get("input1Const", 0)) if input1_type != 0x00 else 0
                input2_id = int(exp.get("input2ID", 0)) if input2_type == 0x00 else 0
                input2_const = int(exp.get("input2Const", 0)) if input2_type != 0x00 else 0

                return {
                    "isUsed": bool(item.get("isUsed", False)) if isinstance(item, dict) else False,
                    "exp": {
                        "input1Type": input1_type,
                        "input1ID": input1_id,
                        "input1Const": input1_const,
                        "input2Type": input2_type,
                        "input2ID": input2_id,
                        "input2Const": input2_const,
                        "opr": int(exp.get("opr", 0)),
                    },
                }

            result["logic"] = [
                _canonical_logic_item(item)
                for item in logic
            ]

        return result

    def _config_diff_summary(self, current_config, device_config):
        differences = []

        current_channels = current_config.get("channels") or []
        device_channels = device_config.get("channels") or []
        channel_diff_indices = [i for i, (current_item, device_item) in enumerate(zip(current_channels, device_channels)) if current_item != device_item]
        if len(current_channels) != len(device_channels):
            channel_diff_indices.extend(range(min(len(current_channels), len(device_channels)), max(len(current_channels), len(device_channels))))
        if channel_diff_indices:
            channel_text = ", ".join(f"CH{index + 1}" for index in sorted(set(channel_diff_indices)))
            differences.append(f"CHANNELS ({channel_text})")

        current_inputs = current_config.get("inputs")
        device_inputs = device_config.get("inputs")
        if current_inputs != device_inputs:
            input_parts = []
            if isinstance(current_inputs, dict) and isinstance(device_inputs, dict):
                physical_current = current_inputs.get("physical") or []
                physical_device = device_inputs.get("physical") or []
                physical_diff_indices = [i for i, (current_item, device_item) in enumerate(zip(physical_current, physical_device)) if current_item != device_item]
                if len(physical_current) != len(physical_device):
                    physical_diff_indices.extend(range(min(len(physical_current), len(physical_device)), max(len(physical_current), len(physical_device))))
                if physical_diff_indices:
                    input_parts.append("physical " + ", ".join(f"IN{i + 1}" for i in sorted(set(physical_diff_indices))))

                can_current = current_inputs.get("can") or []
                can_device = device_inputs.get("can") or []
                can_diff_indices = [i for i, (current_item, device_item) in enumerate(zip(can_current, can_device)) if current_item != device_item]
                if len(can_current) != len(can_device):
                    can_diff_indices.extend(range(min(len(can_current), len(can_device)), max(len(can_current), len(can_device))))
                if can_diff_indices:
                    input_parts.append("CAN " + ", ".join(f"CAN{i + 1}" for i in sorted(set(can_diff_indices))))

            differences.append("INPUTS" + (f" ({'; '.join(input_parts)})" if input_parts else ""))

        current_logic = current_config.get("logic") or []
        device_logic = device_config.get("logic") or []
        logic_diff_rows = []
        for index, (current_item, device_item) in enumerate(zip(current_logic, device_logic)):
            if current_item == device_item:
                continue

            current_exp = current_item.get("exp", {}) if isinstance(current_item, dict) else {}
            device_exp = device_item.get("exp", {}) if isinstance(device_item, dict) else {}
            field_names = []
            for field_name in ("isUsed",):
                if (current_item.get(field_name) if isinstance(current_item, dict) else None) != (device_item.get(field_name) if isinstance(device_item, dict) else None):
                    field_names.append(field_name)
            for field_name in ("input1Type", "input1ID", "input1Const", "input2Type", "input2ID", "input2Const", "opr"):
                if current_exp.get(field_name) != device_exp.get(field_name):
                    field_names.append(field_name)
            logic_diff_rows.append(f"L{index + 1}")

        if len(current_logic) != len(device_logic):
            logic_diff_rows.extend(
                f"L{index + 1}: row missing"
                for index in range(min(len(current_logic), len(device_logic)), max(len(current_logic), len(device_logic)))
            )

        if logic_diff_rows:
            differences.append("LOGIC (" + "; ".join(logic_diff_rows) + ")")

        return differences

    def _handle_device_config_payload(self, payload):
        if self.config_tab is None:
            return

        try:
            device_config = self.config_tab.parse_binary_payload(payload)
        except Exception as exc:
            self._device_config_request_pending = False
            QMessageBox.critical(self, "Load failed", f"Could not parse device configuration: {exc}")
            return

        current_config = self._canonicalize_config(self.config_tab.collect_config())
        device_config = self._canonicalize_config(device_config)
        if self._config_signature(current_config) == self._default_config_signature:
            self.config_tab.apply_config(device_config)
            self.config_tab.set_isotp_state(100, "Device configuration loaded", busy=False)
            self._device_config_request_pending = False
            return

        if self._config_signature(current_config) == self._config_signature(device_config):
            self._device_config_request_pending = False
            return

        differences = self._config_diff_summary(current_config, device_config)
        diff_text = "\n".join(f"- {item}" for item in differences) if differences else "- configuration content"

        prompt = QMessageBox(self)
        prompt.setWindowTitle("Device configuration differs")
        prompt.setIcon(QMessageBox.Question)
        prompt.setText(
            "The configuration stored on the device is different from the one currently shown in the GUI."
        )
        prompt.setInformativeText(
            "Differences detected in:\n"
            f"{diff_text}\n\n"
            "Do you want to load the device configuration and replace the current GUI settings?"
        )
        prompt.setWindowModality(Qt.NonModal)
        no_button = prompt.addButton("No, keep current", QMessageBox.RejectRole)
        yes_button = prompt.addButton("Yes, load device", QMessageBox.AcceptRole)
        prompt.setDefaultButton(no_button)

        self._device_config_prompt = prompt
        self._pending_device_config = device_config

        def _handle_prompt_click(clicked_button):
            try:
                if clicked_button == yes_button and self._pending_device_config is not None and self.config_tab is not None:
                    self.config_tab.apply_config(self._pending_device_config)
                    self._refresh_control_tab()
                    self.config_tab.set_isotp_state(100, "Device configuration loaded", busy=False)
            finally:
                self._pending_device_config = None
                self._device_config_prompt = None
                self._device_config_request_pending = False

        prompt.buttonClicked.connect(_handle_prompt_click)
        prompt.show()

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

        self.send_can_frame(arbitration_id, payload)

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
                        self._handle_device_config_payload(packet["isotp_config_payload"])
                    continue
                if packet.get("serial_ready"):
                    self.serial_device_ready = True
                    self._connection_state = "serial_ready"
                    continue
                if "error" in packet:
                    self._device_config_request_pending = False
                    if self.config_tab is not None:
                        self.config_tab.set_isotp_state(0, f"Error: {packet['error']}", busy=False)
                    continue

                self.latest_sys = packet["sys"]
                self.latest_ch = packet["ch"]
                self.latest_phy = packet["phy"]
                self.lates_imu = packet["imu"]
                self.latest_frames = packet.get("frames", [])
                if self.control_tab is not None:
                    self.control_tab.set_live_channels(self.latest_ch)
                    self.control_tab.set_live_physical_values(self.latest_phy)
                self.plot_panel.update_from_packet(packet)
                self.last_frame_rx_time = time.time()
                updated = True
            except Exception:
                break

        if not updated:
            return

        self.lbl_sys_state.setText(SYS_STATUS_MAP.get(int(self.latest_sys["status"]), str(self.latest_sys["status"])))
        self.lbl_batt.setText(f"{self.latest_sys['batt']} mV")
        self.lbl_temp.setText(f"{self.latest_sys['core_temp']:.1f} °C")
        self.lbl_safety.setText(str(self.latest_sys["safety"]))
        self.lbl_total_i.setText(f"{self.latest_sys['total_current']:.1f} mA")
        total_current_a = float(self.latest_sys["total_current"]) / 1000.0
        self.lbl_itot.setText(f"I<sub>tot</sub>: {total_current_a:.1f} A")
        self.lbl_itot.setStyleSheet(_kpi_label_style(_blend_color("#09BC8A", "#E30026", min(1.0, max(0.0, total_current_a / 100.0)))))

        self.lbl_batt_big.setText(f"Battery: {_format_voltage_mv(self.latest_sys.get('batt', 0))}")
        self.lbl_batt_big.setStyleSheet(_kpi_label_style(_battery_kpi_color(self.latest_sys.get('batt', 0))))

        safety_text = str(self.latest_sys.get("safety", "-"))
        safety_color = "#09BC8A" if safety_text.upper() in ("OK", "ON", "1") else "#E30026"
        self.lbl_safety_big.setText(f"Safety Line: {safety_text}")
        self.lbl_safety_big.setStyleSheet(_kpi_label_style(safety_color))

        error_text, error_color = _format_channel_error_summary(self.latest_ch)
        self.lbl_error_big.setText(error_text)
        self.lbl_error_big.setStyleSheet(_kpi_label_style(error_color))

        # IMU
        self.lbl_acc_x.setText(f"{self.lates_imu['accX']:.2f} g")
        self.lbl_acc_y.setText(f"{self.lates_imu['accY']:.2f} g")
        self.lbl_acc_z.setText(f"{self.lates_imu['accZ']:.2f} g")

        self.lbl_gyro_x.setText(f"{self.lates_imu['pitch']:.2f} dps")
        self.lbl_gyro_y.setText(f"{self.lates_imu['roll']:.2f} dps")
        self.lbl_gyro_z.setText(f"{self.lates_imu['yaw']:.2f} dps")

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
            irms_text = str(ch.get("current_rms", 0)) if i < 8 else "-"
            heat_text = f"{int(ch.get('i2t_heat', 0))}%" if i < 8 else "-"
            soc_text = str(int(ch.get('soc_threshold', 0))) if i < 8 else "-"
            vals = (
                ch["name"],
                status_str,
                state_str,
                str(ch["voltage"]),
                str(ch["current"]),
                f"{ch['current_avg']:.1f}",
                irms_text,
                heat_text,
                soc_text,
            )

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
            self.table_frames.setRowHeight(row, 20)

        self.update_heartbeat_status()

    def update_heartbeat_status(self):
        if self.offline_mode:
            self._connection_state = "offline"
            self._set_heartbeat_status("Offline mode", "#90CAF9")
            return

        worker_alive = self.worker_process is not None and self.worker_process.is_alive()

        previous_state = self._connection_state

        if not worker_alive:
            self._connection_state = "disconnected"
            self._set_heartbeat_status("Disconnected", "#FF8A80")
            return

        if not self.serial_device_ready:
            self._connection_state = "disconnected"
            self._set_heartbeat_status("Disconnected", "#FF8A80")
            return

        if self.last_frame_rx_time <= 0:
            self._connection_state = "serial_ready"
            self._set_heartbeat_status(f"Serial ready: {pdm_shared.CAN_CHANNEL}", "#FFD54F")
            return

        age_s = time.time() - self.last_frame_rx_time
        if age_s > 1.0:
            self._connection_state = "serial_ready"
            self._set_heartbeat_status(f"Serial ready: {pdm_shared.CAN_CHANNEL}", "#FFD54F")
        else:
            self._connection_state = "connected"
            self._set_heartbeat_status(f"Connected ({age_s:.1f}s)", "#81C784")

        if self._connection_state == "connected" and previous_state != self._connection_state:
            self._request_config_if_needed()

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
            self.stop_can_worker()
        super().closeEvent(event)


def _build_splash_pixmap(base_pixmap):
    canvas = QPixmap(base_pixmap)
    painter = QPainter(canvas)
    painter.setRenderHint(QPainter.TextAntialiasing, True)

    loading_font = QFont("Segoe UI", 14)
    painter.setFont(loading_font)
    painter.setPen(QColor("#FFFFFF"))
    painter.drawText(canvas.rect().adjusted(0, 0, 0, -12), Qt.AlignHCenter | Qt.AlignBottom, "Loading PDMS...")

    version_font = QFont("Segoe UI", 10)
    painter.setFont(version_font)
    painter.setPen(QColor("#E0E0E0"))
    painter.drawText(canvas.rect().adjusted(0, 0, -12, -8), Qt.AlignRight | Qt.AlignBottom, f"Version {APP_VERSION}")

    painter.end()
    return canvas


if __name__ == "__main__":
    multiprocessing.freeze_support()
    app = QApplication(sys.argv)
    app.setStyleSheet(build_dark_stylesheet(CHECKBOX_TICK_PATH))
    app.setWindowIcon(QIcon(get_asset_path("assets/pdms.ico")))
    # Show a splash/loading screen during application bring-up
    splash_pix_path = get_asset_path("assets/pdms_splash.png")
    if Path(splash_pix_path).exists():
        pix = QPixmap(splash_pix_path)
    else:
        # Fallback: small blank pixmap with icon if no splash image available
        pix = QPixmap(480, 300)
        pix.fill(QColor('#2D2D2D'))

    splash = QSplashScreen(_build_splash_pixmap(pix))
    splash.show()
    app.processEvents()

    startup_transport = _resolve_startup_transport()
    if startup_transport is None:
        splash.close()
        sys.exit(0)

    window = MainWindow(offline_mode=startup_transport.get("offline", False))
    window.show()

    # Finish the splash after the main window is visible (short delay to let init finish)
    QTimer.singleShot(600, lambda: splash.finish(window))

    sys.exit(app.exec_())

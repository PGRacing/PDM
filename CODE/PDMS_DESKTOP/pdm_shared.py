import json
import os
import struct
import sys
from pathlib import Path

CHANNEL_COUNT = 16
PHY_INPUT_COUNT = 8
BASE_ID = 0x400
BASE_DIR = Path(__file__).resolve().parent
APP_VERSION = "1.2.1"


def get_runtime_base_dir():
    if getattr(sys, "frozen", False):
        return Path(sys.executable).resolve().parent
    return BASE_DIR


def get_bundle_base_dir():
    return Path(getattr(sys, "_MEIPASS", str(BASE_DIR)))


CONFIG_DIR = get_runtime_base_dir() / "config"
APP_CONFIG_PATH = CONFIG_DIR / "app_config.json"


def get_asset_path(relative_path):
    """Get absolute path to a bundled resource, works for dev and PyInstaller."""
    base_path = Path(getattr(sys, "_MEIPASS", os.path.abspath(".")))
    return (base_path / relative_path).as_posix()


def _load_app_config():
    candidate_paths = [
        get_runtime_base_dir() / "config" / "app_config.json",
        get_runtime_base_dir() / "app_config.json",
        get_bundle_base_dir() / "config" / "app_config.json",
        get_bundle_base_dir() / "app_config.json",
    ]

    for candidate_path in candidate_paths:
        try:
            if candidate_path.exists():
                with candidate_path.open("r", encoding="utf-8") as config_file:
                    data = json.load(config_file)
                    if isinstance(data, dict):
                        return data
        except Exception:
            pass
    return {}


_APP_CONFIG = _load_app_config()


def get_app_config_candidate_paths():
    return [
        get_runtime_base_dir() / "app_config.json",
        get_runtime_base_dir() / "config" / "app_config.json",
        get_bundle_base_dir() / "app_config.json",
        get_bundle_base_dir() / "config" / "app_config.json",
    ]


def load_app_config():
    return dict(_load_app_config())


def get_can_channel(default="COM16"):
    return str(_APP_CONFIG.get("usb_device", default))


def set_can_channel_runtime(channel_name):
    _APP_CONFIG["usb_device"] = str(channel_name)
    global CAN_CHANNEL
    CAN_CHANNEL = str(channel_name)


def save_usb_device_config(channel_name):
    data = {"usb_device": str(channel_name)}
    for candidate_path in get_app_config_candidate_paths():
        try:
            candidate_path.parent.mkdir(parents=True, exist_ok=True)
            with candidate_path.open("w", encoding="utf-8") as config_file:
                json.dump(data, config_file, indent=2)
            set_can_channel_runtime(channel_name)
            return candidate_path
        except Exception:
            continue
    raise OSError("Could not write app_config.json")


CAN_CHANNEL = get_can_channel("COM16")
CAN_BITRATE = 1000000
SERIAL_BAUD = 115200

HISTORY_LEN = 300
GUI_UPDATE_MS = 30
PLOT_UPDATE_MS = 10

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
    "I2T_HEAT_1_8": BASE_ID + 0x014,
    "SOC_TRESH_1_4": BASE_ID + 0x015,
    "SOC_TRESH_5_8": BASE_ID + 0x016,
    "PWM_DUTY_1_8": BASE_ID + 0x017,
    "DEV_DIAG": BASE_ID + 0x018,
    "IMU_ACC": BASE_ID + 0x10,
    "IMU_RATES": BASE_ID + 0x11,
    "IRMS_1_4": BASE_ID + 0x12,
    "IRMS_5_8": BASE_ID + 0x13,
}

OUT_STATE_MAP = {0: "OFF", 1: "ON", 2: "ERR"}
OUT_STATUS_MAP = {
    0: "OK",
    1: "OPEN_LOAD",
    8: "SAFETY_OPEN",
    9: "PRIO_DIV",
    10: "SOC_FAULT",
    11: "I2T_FAULT",
    20: "SHORT_VSS",
    21: "CTRL_FAIL",
    22: "HARD_FAULT",
}

def build_dark_stylesheet(checkbox_tick_path=None):
    checkbox_tick_rule = ""
    if checkbox_tick_path:
        checkbox_tick_rule = (
            f'    QCheckBox::indicator:checked {{ background-color: #BB86FC; border: 1px solid #BB86FC; image: url("{checkbox_tick_path}"); }}\n'
        )
    return (
        "    QMainWindow { background-color: #121212; }\n"
        "    QWidget { background-color: #121212; color: #E0E0E0; font-family: 'Segoe UI', Arial, sans-serif; font-size: 12px; }\n"
        "    QMenuBar { background-color: #BB86FC; color: #1A1A1A; border-bottom: 1px solid #6C4AA8; spacing: 0px; padding: 0px 8px 4px 8px; }\n"
        "    QMenuBar::item { background: transparent; padding: 6px 14px; margin: 0px; color: #1A1A1A; border-right: 1px solid #8F5FD6; }\n"
        "    QMenuBar::item:selected { background-color: #D7B7FD; color: #121212; }\n"
        "    QMenuBar::item:pressed { background-color: #E5CEFF; color: #121212; }\n"
        "    QMenu { background-color: #1E1E1E; color: #E0E0E0; border: 1px solid #333333; padding: 4px; }\n"
        "    QMenu::item { padding: 6px 24px; border-radius: 4px; }\n"
        "    QMenu::item:selected { background-color: #3C3C3C; color: #FFFFFF; }\n"
        "    QMenu::separator { height: 1px; background: #333333; margin: 4px 8px; }\n"
        "    QTabWidget::pane { background-color: #121212; border: 1px solid #333333; }\n"
        "    QTabBar::tab { background-color: #1E1E1E; color: #E0E0E0; padding: 6px 10px; border: 1px solid #333333; }\n"
        "    QTabBar::tab:selected { background-color: #2D2D2D; color: #BB86FC; }\n"
        "    QAbstractScrollArea { background-color: #1E1E1E; }\n"
        "    QGroupBox { background-color: #1E1E1E; border: 1px solid #333333; border-radius: 6px; margin-top: 12px; font-weight: bold; color: #BB86FC; }\n"
        "    QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }\n"
        "    QTableWidget, QTableView, QAbstractItemView { background-color: #121212; alternate-background-color: #1E1E1E; gridline-color: #2D2D2D; color: #E0E0E0; border: 1px solid #333333; border-radius: 4px; selection-background-color: #2D2D2D; selection-color: #FFFFFF; }\n"
        "    QTableWidget::item, QTableView::item { padding: 4px; }\n"
        "    QHeaderView::section { background-color: #2D2D2D; color: #E0E0E0; padding: 4px; border: 1px solid #121212; font-weight: bold; }\n"
        "    QComboBox, QLineEdit { background-color: #2D2D2D; border: 1px solid #444444; border-radius: 4px; padding: 4px; color: #FFFFFF; }\n"
        "    QPushButton { background-color: #BB86FC; color: #121212; border: none; border-radius: 4px; padding: 6px; font-weight: bold; }\n"
        "    QPushButton:hover { background-color: #D7B7FD; }\n"
        "    QCheckBox::indicator { width: 14px; height: 14px; background-color: #2D2D2D; border: 1px solid #444444; border-radius: 3px; }\n"
        f"{checkbox_tick_rule}"
        "    QCheckBox::indicator:unchecked { background-color: #2D2D2D; border: 1px solid #444444; }\n"
    )


DARK_STYLESHEET = build_dark_stylesheet()

TRACK_COLORS = [
    '#FF5722', '#3F51B5', '#4CAF50', '#FFEB3B', '#00BCD4', '#9C27B0', '#E91E63', '#009688',
    '#FF9800', '#795548', '#9E9E9E', '#607D8B', '#FF8A80', '#EA80FC', '#82B1FF', '#B2FF59'
]


def get_row_colors(state, status):
    if status == 0 and state == 0:
        return "#242424", "#A0A0A0"
    if status == 0:
        return "#1B3B2B", "#81C784"
    # OPEN_LOAD (1) and SAFETY_OPEN (8) are warnings, not hard errors.
    if 0 < status < 9:
        return "#3E2723", "#FFB74D"
    if status >= 9:
        return "#3D1C1C", "#E57373"
    if state == 2:
        return "#4A1F1F", "#FF8A80"
    return "#3D1C1C", "#E57373"


def parse_u16x4(data):
    if not isinstance(data, (bytes, bytearray)):
        data = bytes(data)
    if len(data) < 8:
        data = data + b"\x00" * (8 - len(data))
    return struct.unpack("<4H", data[:8])


def parse_system_status(data):
    status = data[0]
    batt_voltage = data[1] | (data[2] << 8)
    core_temp_raw = struct.unpack_from("<h", data, 3)[0]
    safety_line_state = "OK" if len(data) > 5 and data[5] == 1 else "FAULT"
    logic_valid_mask = 0
    if len(data) >= 8:
        logic_valid_mask = data[6] | (data[7] << 8)
    return status, batt_voltage, core_temp_raw / 10.0, safety_line_state, logic_valid_mask

def parse_i16x3(data):
    if not isinstance(data, (bytes, bytearray)):
        data = bytes(data)
    if len(data) < 6:
        data = data + b"\x00" * (6 - len(data))
    return struct.unpack("<3h", data[:6])
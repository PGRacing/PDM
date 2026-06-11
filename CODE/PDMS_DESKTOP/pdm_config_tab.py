import json
import struct
from pathlib import Path

from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtWidgets import (
    QCheckBox,
    QComboBox,
    QFrame,
    QFileDialog,
    QFormLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMessageBox,
    QPushButton,
    QProgressBar,
    QGridLayout,
    QScrollArea,
    QSpinBox,
    QTabWidget,
    QSizePolicy,
    QVBoxLayout,
    QWidget,
)

from pdm_shared import CHANNEL_COUNT, get_asset_path, get_runtime_base_dir

BASE_DIR = Path(__file__).resolve().parent
CONFIG_DIR = get_runtime_base_dir() / "config"
CONFIG_DIR.mkdir(parents=True, exist_ok=True)

OUT_TYPE_BTS500 = 0x00
OUT_TYPE_SPOC2 = 0x01

OUT_MODE_UNUSED = 0x00
OUT_MODE_STD = 0x01
OUT_MODE_PWM = 0x02
OUT_MODE_BATCH = 0x03
OUT_PWM_MAP_RESOLUTION = 8

OUT_ERR_BEH_NO = 0x00
OUT_ERR_BEH_LATCH = 0x01
OUT_ERR_BEH_TIME_LATCH = 0x03
OUT_ERR_BEH_RETRY = 0x04

UINT32_MAX = 0xFFFFFFFF

SPOC2_ID_1 = 0x00
SPOC2_ID_2 = 0x01

SPOC2_CH_ID_1 = 0x00
SPOC2_CH_ID_2 = 0x01
SPOC2_CH_ID_3 = 0x02
SPOC2_CH_ID_4 = 0x03

FIELD_LABELS = {
    "channel": {
        "channel_id": "Channel ID:",
        "type": "Type:",
        "spoc_mapping": "SPOC mapping:",
        "mode": "Mode:",
        "name": "Name:",
        "batch": "Batch:",
    },
    "safety": {
        "after_error_behavior": "After error behavior:",
        "after_error_latch_time": "After error latch time:",
        "act_on_safety": "Act on safety line open:",
        "err_retry_threshold": "Allowed retries:",
        "retry_timer_interval": "Retry interval:",
    },
    "soc": {
        "use_soc": "Use overcurrent (SOC) protection",
        "nominal_threshold": "Nominal threshold:",
        "allow_inrush": "Allow inrush:",
        "inrush_window_from_start": "Inrush window from start:",
        "inrush_threshold": "Inrush threshold:",
        "inrush_time_threshold": "Inrush time threshold:",
    },
    "i2t": {
        "use_i2t": "(UNUSED) Use I2T protection",
        "nominal_current": "Nominal current:",
        "nominal_current_sq": "Nominal current squared:",
        "time_threshold": "Time threshold:",
        "i2t_threshold": "I2T threshold:",
    },
    "pwm": {
        "base_duty": "Base duty:",
        "duty_input": "Duty input:",
        "mapping": "Mapping:",
    },
    "softstart": {
        "use_softstart": "Use soft-start",
        "start_duty": "Start duty:",
        "end_duty": "End duty:",
        "time_threshold": "Time threshold:",
    },
}

LOGIC_OPERATOR_LABELS = [
    ("AND", 0x00),
    ("OR", 0x01),
    ("NE", 0x02),
    ("E", 0x03),
    ("G", 0x04),
    ("GE", 0x05),
    ("L", 0x06),
    ("LE", 0x07),
    ("IT", 0x08),
    ("IF", 0x09),
]

LOGIC_INPUT_TYPE_LABELS = [
    ("SENSOR", 0x00),
    ("CONST_SCHMITT", 0x01),
    ("CONST_ANALOG", 0x02),
    ("UNSET", 0x03),
]

LOGIC_VAR_TYPE_LABELS = [
    ("SCHMITT", 0x00),
    ("ANALOG", 0x01),
    ("ANY", 0x02),
    ("NONE", 0x03),
]

LOGIC_OPERATOR_RELATIONS = {
    0x00: (0x00, 0x00),
    0x01: (0x00, 0x00),
    0x02: (0x02, 0x02),
    0x03: (0x02, 0x02),
    0x04: (0x01, 0x01),
    0x05: (0x01, 0x01),
    0x06: (0x01, 0x01),
    0x07: (0x01, 0x01),
    0x08: (0x00, 0x03),
    0x09: (0x00, 0x03),
}

LOGIC_VAR_TO_INPUT_TYPES = {
    0x00: [0x00, 0x01],
    0x01: [0x00, 0x02],
    0x02: [0x00, 0x01, 0x02],
    0x03: [0x03],
}

IN_MODE_UNUSED = 0x00
IN_MODE_SCHMITT = 0x01
IN_MODE_ANALOG = 0x02
IN_MODE_OUTPUT = 0x03

IN_TYPE_PHY = 0x00
IN_TYPE_CAN = 0x01
IN_TYPE_INVALID = 0xFF

LOGIC_INPUT_TYPE_SENSOR = 0x00
LOGIC_INPUT_TYPE_CONST_SCHMITT = 0x01
LOGIC_INPUT_TYPE_CONST_ANALOG = 0x02
LOGIC_INPUT_TYPE_UNSET = 0x03

OUTPUT_CONFIG_COUNT = CHANNEL_COUNT
CAN_INPUT_COUNT = 16
INPUT_CONFIG_COUNT = CHANNEL_COUNT + 8
LOGIC_CONFIG_COUNT = CHANNEL_COUNT

CAN_INPUT_RECORD_FORMAT = "<BBIHB"
CAN_INPUT_RECORD_SIZE = struct.calcsize(CAN_INPUT_RECORD_FORMAT)

INPUT_RECORD_FORMAT = "<HBB"
INPUT_RECORD_SIZE = struct.calcsize(INPUT_RECORD_FORMAT)

LOGIC_RECORD_FORMAT = "<BBIBIB"
LOGIC_RECORD_SIZE = struct.calcsize(LOGIC_RECORD_FORMAT)

CAN_INPUT_TOTAL_SIZE = CAN_INPUT_COUNT * CAN_INPUT_RECORD_SIZE
INPUT_TOTAL_SIZE = INPUT_CONFIG_COUNT * INPUT_RECORD_SIZE
LOGIC_TOTAL_SIZE = LOGIC_CONFIG_COUNT * LOGIC_RECORD_SIZE

INPUT_INTERPRETATION_LABELS = [
    ("Digital (Schmitt)", 0x00),
    ("Analog", 0x01),
]

CAN_INSTANCE_LABELS = [
    ("CANH_INSTANCE_1", 0x00),
    ("CANH_INSTANCE_2", 0x01),
]

CAN_INPUT_DATA_TYPE_LABELS = [
    ("CAN_INPUT_TYPE_BOOL", 0x00),
    ("CAN_INPUT_TYPE_UINT16", 0x01),
    ("CAN_INPUT_TYPE_UINT32", 0x02),
    ("CAN_INPUT_TYPE_INT16", 0x03),
    ("CAN_INPUT_TYPE_INT32", 0x04),
    ("CAN_INPUT_TYPE_FLOAT", 0x05),
]

SPOC_MAPPING_LABELS = {
    8: "SPOC2_ID_1 / SPOC2_CH_ID_1",
    9: "SPOC2_ID_1 / SPOC2_CH_ID_2",
    10: "SPOC2_ID_1 / SPOC2_CH_ID_3",
    11: "SPOC2_ID_1 / SPOC2_CH_ID_4",
    12: "SPOC2_ID_2 / SPOC2_CH_ID_1",
    13: "SPOC2_ID_2 / SPOC2_CH_ID_2",
    14: "SPOC2_ID_2 / SPOC2_CH_ID_3",
    15: "SPOC2_ID_2 / SPOC2_CH_ID_4",
}

SPINBOX_STYLE = (
    "QSpinBox:enabled, QDoubleSpinBox:enabled {"
    "    background-color: #2D2D2D;"
    "    border: 1px solid #444444;"
    "    border-radius: 6px;"
    "    padding: 4px 0px 4px 0px;"
    "    color: #FFFFFF;"
    "}"
    "QSpinBox:enabled QLineEdit, QDoubleSpinBox:enabled QLineEdit {"
    "    background-color: #2D2D2D;"
    "    color: #FFFFFF;"
    "    selection-background-color: #BB86FC;"
    "    selection-color: #FFFFFF;"
    "    border: none;"
    "}"
    
    "QSpinBox:disabled, QDoubleSpinBox:disabled {"
    "    background-color: #2D2D2D;"
    "    border: 1px solid #333333;"
    "    border-radius: 6px;"
    "    padding: 4px 0px 4px 0px;"
    "    color: #6A6A6A;"
    "}"
    "QSpinBox:disabled QLineEdit, QDoubleSpinBox:disabled QLineEdit {"
    "    background-color: #2D2D2D;"
    "    color: #6A6A6A;"
    "    border: none;"
    "}"
)

CHECKBOX_TICK_PATH = get_asset_path("assets/checkbox-tick.svg")
CHECKBOX_STYLE = (
    "QCheckBox { color: #E0E0E0; background-color: none; padding: 4px; } "
    "QCheckBox::indicator { width: 14px; height: 14px; background-color: #2D2D2D; border: 1px solid #444444; border-radius: 3px; } "
    f'QCheckBox::indicator:checked {{ background-color: #BB86FC; border: 1px solid #BB86FC; image: url("{CHECKBOX_TICK_PATH}"); }}'
)

BINARY_MAGIC = b"PDMB"
BINARY_VERSION = 3
# Packed record layout for one T_OUT_CFG instance:
# id:u8, type:u8, mode:u8, spocId:u8, spocChId:u8, name[32], batch:u8,
# afterErrorCfg.behavior:u8, afterErrorCfg.latchTime:u32, actOnSafety:u8,
# errRetryThreshold:u16, retryTimerInterval:u32,
# socCfg.useSoc:u8, socCfg.nominalThreshold:u32, socCfg.allowInrush:u8,
# socCfg.inrushWindowFromStart:u32, socCfg.inrushThreshold:u32,
# socCfg.inrushTimeThreshold:u32,
# i2tCfg.useI2t:u8, i2tCfg.nominalCurrent:u32, i2tCfg.nominalCurrentSq:u32,
# i2tCfg.timeThreshold:u32, i2tCfg.i2tThreshold:u32,
# pwmCfg.baseDuty:u8, pwmCfg.dutyInput:u16,
# pwmCfg.inputAxis[OUT_PWM_MAP_RESOLUTION]:u16,
# pwmCfg.dutyAxis[OUT_PWM_MAP_RESOLUTION]:u8,
# softStart.useSoftStart:u8,
# softStart.startDuty:u8,
# softStart.endDuty:u8,
# softStart.timeThreshold:u32.
LEGACY_BINARY_RECORD_FORMAT = "<" + "".join(
    [
        "B",
        "B",
        "B",
        "B",
        "B",
        "32s",
        "B",
        "B",
        "I",
        "B",
        "H",
        "I",
        "B",
        "I",
        "B",
        "I",
        "I",
        "I",
        "B",
        "I",
        "I",
        "I",
        "I",
    ]
)
LEGACY_BINARY_RECORD_SIZE = struct.calcsize(LEGACY_BINARY_RECORD_FORMAT)
BINARY_RECORD_FORMAT = (
    LEGACY_BINARY_RECORD_FORMAT
    + "B"
    + "H"
    + ("H" * OUT_PWM_MAP_RESOLUTION)
    + ("B" * OUT_PWM_MAP_RESOLUTION)
    + "B"
    + "B"
    + "B"
    + "I"
)
BINARY_RECORD_SIZE = struct.calcsize(BINARY_RECORD_FORMAT)

OUTPUT_RECORD_FORMAT = BINARY_RECORD_FORMAT
OUTPUT_RECORD_SIZE = BINARY_RECORD_SIZE
LEGACY_OUTPUT_RECORD_FORMAT = LEGACY_BINARY_RECORD_FORMAT
LEGACY_OUTPUT_RECORD_SIZE = LEGACY_BINARY_RECORD_SIZE

OUTPUT_CONFIG_TOTAL_SIZE = OUTPUT_CONFIG_COUNT * OUTPUT_RECORD_SIZE
LEGACY_OUTPUT_CONFIG_TOTAL_SIZE = OUTPUT_CONFIG_COUNT * LEGACY_OUTPUT_RECORD_SIZE

def _make_spinbox(minimum, maximum, value, suffix="", step = 1):
    spin = QSpinBox()
    spin.setStyleSheet(SPINBOX_STYLE)
    spin.setRange(minimum, maximum)
    spin.setSingleStep(step)
    spin.setValue(value)
    if suffix:
        spin.setSuffix(suffix)
    return spin


PAGE_LABEL_STYLE = (
    "QLabel { background-color: none; color: #E0E0E0; "
    "padding: 4px 6px;}"
)

TRANSFER_GROUP_STYLE = (
    "QGroupBox { color: #E0E0E0; font-weight: bold; border: 1px solid #444444; border-radius: 6px; margin-top: 8px; padding-top: 12px; }"
    "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; }"
    "QProgressBar { background-color: #2D2D2D; border: 1px solid #444444; border-radius: 4px; color: #E0E0E0; text-align: center; height: 18px; }"
    "QProgressBar::chunk { background-color: #BB86FC; border-radius: 3px; }"
)

ACTION_BUTTON_STYLE = (
    "QPushButton { padding: 6px 12px; }"
)


class CtrlClickButton(QPushButton):
    ctrlClicked = pyqtSignal()

    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton and event.modifiers() & Qt.ControlModifier:
            super().mousePressEvent(event)
            return
        event.ignore()

    def mouseReleaseEvent(self, event):
        if event.button() == Qt.LeftButton and event.modifiers() & Qt.ControlModifier:
            super().mouseReleaseEvent(event)
            self.ctrlClicked.emit()
            return
        event.ignore()

SEND_DEVICE_BUTTON_STYLE = (
    "QPushButton { padding: 6px 12px; background-color: #09BC8A; color: #2D2D2D; border: none; border-radius: 4px; font-weight: bold; }"
)

RESET_BUTTON_STYLE = (
    "QPushButton { padding: 6px 12px; background-color: #e30026; color: #f5f5f5; border: none; border-radius: 4px; font-weight: bold; }"
)



class ChannelConfigPage(QWidget):
    batchChanged = pyqtSignal()
    modeChanged = pyqtSignal()

    def __init__(self, channel_index, parent=None):
        super().__init__(parent)
        self.channel_index = channel_index
        self.setStyleSheet(PAGE_LABEL_STYLE)

        outer = QVBoxLayout(self)
        outer.setContentsMargins(20, 20, 20, 20)
        outer.setSpacing(8)
        outer.setAlignment(Qt.AlignTop)

        channel_group = QGroupBox("Output channel")
        channel_form = QFormLayout(channel_group)

        self.label_channel_id = QLabel(f"OUT_ID_{channel_index + 1}")
        self.label_channel_id.setTextInteractionFlags(Qt.TextSelectableByMouse)
        self.label_type = QLabel("BTS500" if channel_index < 8 else "SPOC2")
        self.label_type.setTextInteractionFlags(Qt.TextSelectableByMouse)
        self.label_spoc = QLabel("n/a")
        self.label_spoc.setTextInteractionFlags(Qt.TextSelectableByMouse)
        self.combo_mode = QComboBox()
        self.combo_mode.addItem("UNUSED", OUT_MODE_UNUSED)
        self.combo_mode.addItem("STD", OUT_MODE_STD)
        self.combo_mode.addItem("PWM (BETA)", OUT_MODE_PWM)
        self.combo_mode.addItem("BATCH", OUT_MODE_BATCH)
        self.edit_name = QLineEdit(f"OUT_{channel_index + 1}")
        
        # self.combo_batch = _make_spinbox(0, 16, 0)
        self.combo_batch = QComboBox()
        self.fill_batch_combo(channel_index)

        channel_form.addRow(FIELD_LABELS["channel"]["channel_id"], self.label_channel_id)
        channel_form.addRow(FIELD_LABELS["channel"]["type"], self.label_type)

        if channel_index >= 8:
            channel_form.addRow(FIELD_LABELS["channel"]["spoc_mapping"], self.label_spoc)
        channel_form.addRow(FIELD_LABELS["channel"]["mode"], self.combo_mode)
        channel_form.addRow(FIELD_LABELS["channel"]["name"], self.edit_name)
        channel_form.addRow(FIELD_LABELS["channel"]["batch"], self.combo_batch)
        outer.addWidget(channel_group)

        self.pwm_box = QGroupBox("PWM")
        pwm_form = QFormLayout(self.pwm_box)
        self.pwm_source_provider = lambda allowed_var=None: []
        self.edit_base_duty = _make_spinbox(0, 100, 0, "%")
        self.combo_duty_input = QComboBox()
        self.combo_duty_input.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        self.pwm_input_axis = []
        self.pwm_duty_axis = []

        pwm_form.addRow(FIELD_LABELS["pwm"]["base_duty"], self.edit_base_duty)
        pwm_form.addRow(FIELD_LABELS["pwm"]["duty_input"], self.combo_duty_input)

        pwm_map_widget = QWidget()
        pwm_map_layout = QGridLayout(pwm_map_widget)
        pwm_map_layout.setContentsMargins(0, 0, 0, 0)
        pwm_map_layout.setHorizontalSpacing(2)
        pwm_map_layout.setVerticalSpacing(6)
        # pwm_map_layout.addWidget(QLabel(""), 0, 0)
        # for column in range(OUT_PWM_MAP_RESOLUTION):
        #     header = QLabel(str(column + 1))
        #     header.setAlignment(Qt.AlignCenter)
        #     pwm_map_layout.addWidget(header, 0, column + 1)

        input_axis_label = QLabel("Input axis [0-5000 mV]")
        input_axis_label.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        pwm_map_layout.addWidget(input_axis_label, 1, 0)
        for column in range(OUT_PWM_MAP_RESOLUTION):
            spin_box = _make_spinbox(0, 5000, 0, " mV")
            self.pwm_input_axis.append(spin_box)
            pwm_map_layout.addWidget(spin_box, 1, column + 1)

        duty_axis_label = QLabel("Duty axis [0-100 %]")
        duty_axis_label.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        pwm_map_layout.addWidget(duty_axis_label, 2, 0)
        for column in range(OUT_PWM_MAP_RESOLUTION):
            spin_box = _make_spinbox(0, 100, 0, " %")
            self.pwm_duty_axis.append(spin_box)
            pwm_map_layout.addWidget(spin_box, 2, column + 1)

        pwm_form.addRow(FIELD_LABELS["pwm"]["mapping"], pwm_map_widget)
        outer.addWidget(self.pwm_box)

        self.logic_page = LogicChannelPage(channel_index)
        outer.addWidget(self.logic_page)

        safety_group = QGroupBox("Safety")
        safety_form = QFormLayout(safety_group)

        self.combo_after_error_behavior = QComboBox()
        self.combo_after_error_behavior.addItem("NO", OUT_ERR_BEH_NO)
        self.combo_after_error_behavior.addItem("LATCH", OUT_ERR_BEH_LATCH)
        self.combo_after_error_behavior.addItem("(UNUSED) TIME_LATCH", OUT_ERR_BEH_TIME_LATCH)
        self.combo_after_error_behavior.addItem("RETRY", OUT_ERR_BEH_RETRY)
        self.combo_after_error_behavior.setCurrentIndex(self.combo_after_error_behavior.findData(OUT_ERR_BEH_LATCH))
        self.edit_after_error_latch_time = _make_spinbox(0, 2147483647, 0, " ms")
        # initial state will be synced by _sync_after_error_state
        safety_form.addRow(FIELD_LABELS["safety"]["after_error_behavior"], self.combo_after_error_behavior)
        safety_form.addRow(FIELD_LABELS["safety"]["after_error_latch_time"], self.edit_after_error_latch_time)

        self.edit_err_retry_threshold = _make_spinbox(0, 65535, 0)
        self.edit_retry_timer_interval = _make_spinbox(0, 2147483647, 0, " ms")
        safety_form.addRow(FIELD_LABELS["safety"]["err_retry_threshold"], self.edit_err_retry_threshold)
        safety_form.addRow(FIELD_LABELS["safety"]["retry_timer_interval"], self.edit_retry_timer_interval)

        self.check_act_on_safety = QCheckBox()
        self.check_act_on_safety.setStyleSheet(CHECKBOX_STYLE)
        self.check_act_on_safety.setChecked(False)
        safety_form.addRow(FIELD_LABELS["safety"]["act_on_safety"], self.check_act_on_safety)
        outer.addWidget(safety_group)


        self.softstart_enable = QCheckBox(FIELD_LABELS["softstart"]["use_softstart"])
        self.softstart_enable.setChecked(False)
        outer.addWidget(self.softstart_enable)

        self.softstart_box = QGroupBox("Soft-start")
        softstart_form = QFormLayout(self.softstart_box)
        self.edit_softstart_start_duty = _make_spinbox(0, 100, 30, "%")
        self.edit_softstart_end_duty = _make_spinbox(0, 100, 100, "%")
        self.edit_softstart_time_threshold = _make_spinbox(0, 2147483647, 5000, " ms")
        softstart_form.addRow(FIELD_LABELS["softstart"]["start_duty"], self.edit_softstart_start_duty)
        softstart_form.addRow(FIELD_LABELS["softstart"]["end_duty"], self.edit_softstart_end_duty)
        softstart_form.addRow(FIELD_LABELS["softstart"]["time_threshold"], self.edit_softstart_time_threshold)
        outer.addWidget(self.softstart_box)

        self.soc_enable = QCheckBox(FIELD_LABELS["soc"]["use_soc"])
        self.soc_enable.setChecked(True)
        outer.addWidget(self.soc_enable)

        self.soc_box = QGroupBox("SOC")
        soc_form = QFormLayout(self.soc_box)
        self.edit_nominal_threshold = _make_spinbox(1000, 65535, 2000, " mA", 100)
        self.check_allow_inrush = QCheckBox()
        self.check_allow_inrush.setStyleSheet(CHECKBOX_STYLE)
        self.check_allow_inrush.setChecked(False)
        self.edit_inrush_window_from_start = _make_spinbox(0, 2147483647, 0, " ms")
        self.check_inrush_window_infinite = QCheckBox("Infinite")
        self.check_inrush_window_infinite.setStyleSheet(CHECKBOX_STYLE)
        self.label_inrush_window_infinite = QLabel("∞")
        self.label_inrush_window_infinite.setStyleSheet("QLabel { color: #E0E0E0; font-size: 16px; font-weight: bold; }")
        self.label_inrush_window_infinite.hide()
        inrush_window_row = QWidget()
        inrush_window_layout = QHBoxLayout(inrush_window_row)
        inrush_window_layout.setContentsMargins(0, 0, 0, 0)
        inrush_window_layout.setSpacing(8)
        inrush_window_layout.addWidget(self.edit_inrush_window_from_start)
        inrush_window_layout.addWidget(self.check_inrush_window_infinite)
        inrush_window_layout.addWidget(self.label_inrush_window_infinite)
        inrush_window_layout.addStretch(1)
        self.check_inrush_window_infinite.setChecked(True)
        self.edit_inrush_threshold = _make_spinbox(1000, 65535, 4000, " mA", 100)
        self.edit_inrush_time_threshold = _make_spinbox(100, 2147483647, 1000, " ms")
        soc_form.addRow(FIELD_LABELS["soc"]["nominal_threshold"], self.edit_nominal_threshold)
        soc_form.addRow(FIELD_LABELS["soc"]["allow_inrush"], self.check_allow_inrush)
        soc_form.addRow(FIELD_LABELS["soc"]["inrush_window_from_start"], inrush_window_row)
        soc_form.addRow(FIELD_LABELS["soc"]["inrush_threshold"], self.edit_inrush_threshold)
        soc_form.addRow(FIELD_LABELS["soc"]["inrush_time_threshold"], self.edit_inrush_time_threshold)
        outer.addWidget(self.soc_box)

        self.i2t_enable = QCheckBox(FIELD_LABELS["i2t"]["use_i2t"])
        self.i2t_enable.setChecked(False)
        self.i2t_enable.setEnabled(False)
        outer.addWidget(self.i2t_enable)

        self.i2t_box = QGroupBox("I2T")
        i2t_form = QFormLayout(self.i2t_box)
        self.edit_nominal_current = _make_spinbox(0, 65535, 0, " mA", 100)
        self.edit_nominal_current_sq = QLineEdit("0")
        self.edit_nominal_current_sq.setReadOnly(True)
        self.edit_nominal_current_sq.setVisible(False)
        self.edit_time_threshold = _make_spinbox(0, 2147483647, 0, " ms")
        self.edit_i2t_threshold = QLineEdit("0")
        self.edit_i2t_threshold.setReadOnly(True)
        i2t_form.addRow(FIELD_LABELS["i2t"]["nominal_current"], self.edit_nominal_current)
        #i2t_form.addRow(FIELD_LABELS["i2t"]["nominal_current_sq"], self.edit_nominal_current_sq)
        i2t_form.addRow(FIELD_LABELS["i2t"]["time_threshold"], self.edit_time_threshold)
        i2t_form.addRow(FIELD_LABELS["i2t"]["i2t_threshold"], self.edit_i2t_threshold)
        outer.addWidget(self.i2t_box)

        self.soc_enable.toggled.connect(self._sync_soc_state)
        self.i2t_enable.toggled.connect(self._sync_i2t_state)
        self.check_allow_inrush.toggled.connect(self._sync_soc_inrush_state)
        self.edit_nominal_current.valueChanged.connect(self._update_i2t_fields)
        self.edit_time_threshold.valueChanged.connect(self._update_i2t_fields)
        self.check_inrush_window_infinite.toggled.connect(self._sync_inrush_window_state)
        self.combo_after_error_behavior.currentIndexChanged.connect(self._sync_after_error_state)
        self.combo_mode.currentIndexChanged.connect(self._sync_mode_state)
        self.softstart_enable.toggled.connect(self._sync_softstart_state)
        self.combo_batch.currentIndexChanged.connect(self._emit_batch_changed)

        self._apply_spoc_mapping()
        self.refresh_pwm_sources()
        self._sync_soc_state()
        self._sync_i2t_state()
        self._sync_inrush_window_state()
        self._sync_after_error_state()
        self._sync_mode_state()
        self._sync_softstart_state()
        self._update_i2t_fields()

    def _apply_spoc_mapping(self):
        if self.channel_index < 8:
            self.label_spoc.hide()
            self.spoc_id = 0
            self.spoc_ch_id = 0
            return

        self.label_spoc.show()
        self.label_spoc.setText(SPOC_MAPPING_LABELS[self.channel_index])
        self.spoc_id, self.spoc_ch_id = {
            8: (SPOC2_ID_1, SPOC2_CH_ID_1),
            9: (SPOC2_ID_1, SPOC2_CH_ID_2),
            10: (SPOC2_ID_1, SPOC2_CH_ID_3),
            11: (SPOC2_ID_1, SPOC2_CH_ID_4),
            12: (SPOC2_ID_2, SPOC2_CH_ID_1),
            13: (SPOC2_ID_2, SPOC2_CH_ID_2),
            14: (SPOC2_ID_2, SPOC2_CH_ID_3),
            15: (SPOC2_ID_2, SPOC2_CH_ID_4),
        }[self.channel_index]

    def _sync_soc_state(self):
        enabled = self.soc_enable.isChecked()
        for widget in (
            self.edit_nominal_threshold,
            self.check_allow_inrush,
            # self.edit_inrush_window_from_start,
            # self.check_inrush_window_infinite,
            # self.label_inrush_window_infinite,
            # self.edit_inrush_threshold,
            # self.edit_inrush_time_threshold,
        ):
            widget.setEnabled(enabled)
        #self.check_allow_inrush.setChecked(enabled)
        self._sync_soc_inrush_state()
        self._sync_inrush_window_state()

    def _sync_soc_inrush_state(self):
        enabled = self.check_allow_inrush.isChecked()
        for widget in (
            self.edit_inrush_window_from_start,
            self.check_inrush_window_infinite,
            self.label_inrush_window_infinite,
            self.edit_inrush_threshold,
            self.edit_inrush_time_threshold,
        ):
            widget.setEnabled(enabled)
        ##self.check_allow_inrush.setChecked(enabled)
        self.soc_box.setStyleSheet("" if enabled else "QGroupBox { color: #888888; opacity: 0.2;}")

    def _sync_i2t_state(self):
        enabled = self.i2t_enable.isChecked()
        for widget in (
            self.edit_nominal_current,
            self.edit_nominal_current_sq,
            self.edit_time_threshold,
            self.edit_i2t_threshold,
        ):
            widget.setEnabled(enabled)
        self.i2t_box.setStyleSheet("" if enabled else "QGroupBox { color: #888888; opacity: 0.2;}")

    def _sync_inrush_window_state(self):
        infinite = self.check_inrush_window_infinite.isChecked()
        self.edit_inrush_window_from_start.setVisible(not infinite)
        self.label_inrush_window_infinite.setVisible(infinite)
        self.edit_inrush_window_from_start.setEnabled(self.soc_enable.isChecked() and self.check_allow_inrush.isChecked() and not infinite)
        self.check_inrush_window_infinite.setEnabled(self.soc_enable.isChecked() and self.check_allow_inrush.isChecked())

    def _set_inrush_window_from_start(self, value):
        value = int(value)
        if value >= UINT32_MAX:
            self.check_inrush_window_infinite.setChecked(True)
        else:
            self.check_inrush_window_infinite.setChecked(False)
            self.edit_inrush_window_from_start.setValue(max(0, min(2147483647, value)))

    def _get_inrush_window_from_start(self):
        if self.check_inrush_window_infinite.isChecked():
            return UINT32_MAX
        return self.edit_inrush_window_from_start.value()

    def _update_i2t_fields(self, *args):
        nominal_current = self.edit_nominal_current.value()
        time_threshold = self.edit_time_threshold.value()
        nominal_current_sq = nominal_current * nominal_current
        i2t_threshold = nominal_current_sq * time_threshold
        self.edit_nominal_current_sq.setText(str(nominal_current_sq))
        self.edit_nominal_current_sq.setVisible(False)
        self.edit_i2t_threshold.setText(str(i2t_threshold))
    def _sync_after_error_state(self):
        behavior = self.combo_after_error_behavior.currentData()
        # Enable latch time only for TIME_LATCH behavior
        latch_enabled = behavior == OUT_ERR_BEH_TIME_LATCH
        self.edit_after_error_latch_time.setEnabled(latch_enabled)
        if not latch_enabled:
            self.edit_after_error_latch_time.setToolTip("Unused")
        else:
            self.edit_after_error_latch_time.setToolTip("")

        # Enable retry fields only when behavior == RETRY
        retry_enabled = behavior == OUT_ERR_BEH_RETRY
        for widget in (self.edit_err_retry_threshold, self.edit_retry_timer_interval):
            widget.setEnabled(retry_enabled)

    def set_pwm_source_provider(self, provider):
        self.pwm_source_provider = provider or (lambda allowed_var=None: [])
        self.refresh_pwm_sources()

    def refresh_pwm_sources(self):
        current_value = self.combo_duty_input.currentData()
        if current_value is None:
            current_value = 0xFFFF

        sources = [("Not selected", 0xFFFF)]
        try:
            sources.extend(self.pwm_source_provider(0x01))
        except Exception:
            pass

        self.combo_duty_input.blockSignals(True)
        self.combo_duty_input.clear()
        for label, value in sources:
            self.combo_duty_input.addItem(label, value)

        selected_index = self.combo_duty_input.findData(current_value)
        if selected_index < 0:
            selected_index = self.combo_duty_input.findData(0xFFFF)
        if selected_index >= 0:
            self.combo_duty_input.setCurrentIndex(selected_index)
        self.combo_duty_input.blockSignals(False)

    def _sync_mode_state(self):
        mode = self.combo_mode.currentData() 
        self.pwm_box.setVisible(mode == OUT_MODE_PWM)
        self.combo_batch.setEnabled(mode == OUT_MODE_BATCH)
        if mode != OUT_MODE_BATCH:
            self.combo_batch.setCurrentIndex(0)
        softstart_allowed = mode == OUT_MODE_STD
        self.softstart_enable.setVisible(softstart_allowed)
        if not softstart_allowed and self.softstart_enable.isChecked():
            self.softstart_enable.blockSignals(True)
            self.softstart_enable.setChecked(False)
            self.softstart_enable.blockSignals(False)
        self._sync_softstart_state()
        self._emit_mode_changed()

    def _sync_softstart_state(self):
        enabled = self.combo_mode.currentData() == OUT_MODE_STD and self.softstart_enable.isChecked()
        self.softstart_box.setVisible(enabled)
        for widget in (
            self.edit_softstart_start_duty,
            self.edit_softstart_end_duty,
            self.edit_softstart_time_threshold,
        ):
            widget.setEnabled(enabled)

    def _emit_batch_changed(self, *_args):
        self.batchChanged.emit()

    def _emit_mode_changed(self, *_args):
        self.modeChanged.emit()

    def fill_batch_combo(self, ch_idx):
        self.combo_batch.clear()
        if ch_idx < 8:
            start_offset = 4 if ch_idx >= 4 else 0
            self.combo_batch.addItem("NONE", ch_idx)
            for i in range(start_offset, start_offset + 4):
                if i != ch_idx:
                    self.combo_batch.addItem(f"OUT_{i+1}", i)
        else:
            # Handles indices 8 and above
            self.combo_batch.addItem("NOT ALLOWED", 0)

    def apply_dict(self, data):
        mode_value = data.get("mode", OUT_MODE_UNUSED)
        mode_index = self.combo_mode.findData(mode_value)
        if mode_index >= 0:
            self.combo_mode.setCurrentIndex(mode_index)

        self.edit_name.setText(str(data.get("name", self.edit_name.text())))
        # self.combo_batch.setValue(int(data.get("batch", self.combo_batch.value())))

        batch_value = data.get("batch", 0)
        batch_index = self.combo_batch.findData(batch_value)
        if batch_index >= 0:
            self.combo_batch.setCurrentIndex(batch_index)

        pwm = data.get("pwmCfg", {}) or {}
        self.edit_base_duty.setValue(int(pwm.get("baseDuty", self.edit_base_duty.value())))
        self.refresh_pwm_sources()
        duty_input_value = pwm.get("dutyInput", 0xFFFF)
        duty_input_index = self.combo_duty_input.findData(duty_input_value)
        if duty_input_index < 0:
            duty_input_index = self.combo_duty_input.findData(0xFFFF)
        if duty_input_index >= 0:
            self.combo_duty_input.setCurrentIndex(duty_input_index)
        for axis_index, spin_box in enumerate(self.pwm_input_axis):
            axis_values = pwm.get("inputAxis", []) or []
            if axis_index < len(axis_values):
                spin_box.setValue(int(axis_values[axis_index]))
        for axis_index, spin_box in enumerate(self.pwm_duty_axis):
            axis_values = pwm.get("dutyAxis", []) or []
            if axis_index < len(axis_values):
                spin_box.setValue(int(axis_values[axis_index]))

        safety = data.get("safety", {}) or {}
        after_error = safety.get("afterErrorCfg", {}) or {}
        behavior_index = self.combo_after_error_behavior.findData(after_error.get("behavior", OUT_ERR_BEH_NO))
        if behavior_index >= 0:
            self.combo_after_error_behavior.setCurrentIndex(behavior_index)
        self.edit_after_error_latch_time.setValue(int(after_error.get("latchTime", self.edit_after_error_latch_time.value())))
        self.check_act_on_safety.setChecked(bool(safety.get("actOnSafety", self.check_act_on_safety.isChecked())))
        self.edit_err_retry_threshold.setValue(int(safety.get("errRetryThreshold", self.edit_err_retry_threshold.value())))
        self.edit_retry_timer_interval.setValue(int(safety.get("retryTimerInterval", self.edit_retry_timer_interval.value())))

        soc = safety.get("socCfg", {}) or {}
        self.soc_enable.setChecked(bool(soc.get("useSoc", self.soc_enable.isChecked())))
        self.edit_nominal_threshold.setValue(int(soc.get("nominalThreshold", self.edit_nominal_threshold.value())))
        self.check_allow_inrush.setChecked(bool(soc.get("allowInrush", self.check_allow_inrush.isChecked())))
        self._set_inrush_window_from_start(soc.get("inrushWindowFromStart", self._get_inrush_window_from_start()))
        self.edit_inrush_threshold.setValue(int(soc.get("inrushThreshold", self.edit_inrush_threshold.value())))
        self.edit_inrush_time_threshold.setValue(int(soc.get("inrushTimeThreshold", self.edit_inrush_time_threshold.value())))

        i2t = safety.get("i2tCfg", {}) or {}
        self.i2t_enable.setChecked(bool(i2t.get("useI2t", self.i2t_enable.isChecked())))
        self.edit_nominal_current.setValue(int(i2t.get("nominalCurrent", self.edit_nominal_current.value())))
        self.edit_time_threshold.setValue(int(i2t.get("timeThreshold", self.edit_time_threshold.value())))

        self._sync_soc_state()
        self._sync_soc_inrush_state()
        self._sync_i2t_state()
        self.softstart_enable.setChecked(bool(data.get("softStart", {}).get("useSoftStart", self.softstart_enable.isChecked())))
        softstart = data.get("softStart", {}) or {}
        self.edit_softstart_start_duty.setValue(int(softstart.get("startDuty", self.edit_softstart_start_duty.value())))
        self.edit_softstart_end_duty.setValue(int(softstart.get("endDuty", self.edit_softstart_end_duty.value())))
        self.edit_softstart_time_threshold.setValue(int(softstart.get("timeThreshold", self.edit_softstart_time_threshold.value())))
        self._sync_softstart_state()
        self._update_i2t_fields()
        self._sync_mode_state()

    def pack_binary_record(self):
        name_bytes = self.edit_name.text().encode("utf-8")[:32]
        name_bytes = name_bytes.ljust(32, b"\0")
        type_value = OUT_TYPE_BTS500 if self.channel_index < 8 else OUT_TYPE_SPOC2
        record_values = (
            self.channel_index,  # channel_id
            type_value,  # type
            self.combo_mode.currentData(),  # mode
            0 if self.channel_index < 8 else self.spoc_id,  # spoc_id
            0 if self.channel_index < 8 else self.spoc_ch_id,  # spoc_ch_id
            name_bytes,  # name[16]
            self.combo_batch.currentData(),  # batch
            self.combo_after_error_behavior.currentData(),  # after_error_behavior
            self.edit_after_error_latch_time.value(),  # after_error_latch_time
            1 if self.check_act_on_safety.isChecked() else 0,  # act_on_safety
            self.edit_err_retry_threshold.value(),  # err_retry_threshold
            self.edit_retry_timer_interval.value(),  # retry_timer_interval
            1 if self.soc_enable.isChecked() else 0,  # use_soc
            self.edit_nominal_threshold.value(),  # nominal_threshold
            1 if self.check_allow_inrush.isChecked() else 0,  # allow_inrush
            self._get_inrush_window_from_start(),  # inrush_window_from_start
            self.edit_inrush_threshold.value(),  # inrush_threshold
            self.edit_inrush_time_threshold.value(),  # inrush_time_threshold
            1 if self.i2t_enable.isChecked() else 0,  # use_i2t
            self.edit_nominal_current.value(),  # nominal_current
            self.edit_nominal_current.value() ** 2,  # nominal_current_sq
            self.edit_time_threshold.value(),  # time_threshold
            self.edit_nominal_current.value() ** 2 * self.edit_time_threshold.value(),  # i2t_threshold
            self.edit_base_duty.value(),  # base_duty
            self.combo_duty_input.currentData(),  # duty_input
            *[spin_box.value() for spin_box in self.pwm_input_axis],  # inputAxis[]
            *[spin_box.value() for spin_box in self.pwm_duty_axis],  # dutyAxis[]
            1 if self.softstart_enable.isChecked() else 0,  # use_softstart
            self.edit_softstart_start_duty.value(),  # start_duty
            self.edit_softstart_end_duty.value(),  # end_duty
            self.edit_softstart_time_threshold.value(),  # time_threshold
        )
        return struct.pack(
            BINARY_RECORD_FORMAT,
            *record_values,
        )

    def to_dict(self):
        type_value = OUT_TYPE_BTS500 if self.channel_index < 8 else OUT_TYPE_SPOC2
        spoc_id = 0 if self.channel_index < 8 else self.spoc_id
        spoc_ch_id = 0 if self.channel_index < 8 else self.spoc_ch_id

        return {
            "id": self.channel_index,
            "type": type_value,
            "mode": self.combo_mode.currentData(),
            "spocId": spoc_id,
            "spocChId": spoc_ch_id,
            "name": self.edit_name.text(),
            "batch": self.combo_batch.currentData(),
                "pwmCfg": {
                    "baseDuty": self.edit_base_duty.value(),
                    "dutyInput": self.combo_duty_input.currentData(),
                    "inputAxis": [spin_box.value() for spin_box in self.pwm_input_axis],
                    "dutyAxis": [spin_box.value() for spin_box in self.pwm_duty_axis],
                },
                "softStart": {
                    "useSoftStart": self.softstart_enable.isChecked(),
                    "startDuty": self.edit_softstart_start_duty.value(),
                    "endDuty": self.edit_softstart_end_duty.value(),
                    "timeThreshold": self.edit_softstart_time_threshold.value(),
                },
            "safety": {
                "afterErrorCfg": {
                    "behavior": self.combo_after_error_behavior.currentData(),
                    "latchTime": self.edit_after_error_latch_time.value(),
                },
                "actOnSafety": self.check_act_on_safety.isChecked(),
                "errRetryThreshold": self.edit_err_retry_threshold.value(),
                "retryTimerInterval": self.edit_retry_timer_interval.value(),
                "socCfg": {
                    "useSoc": self.soc_enable.isChecked(),
                    "nominalThreshold": self.edit_nominal_threshold.value(),
                    "allowInrush": self.check_allow_inrush.isChecked(),
                    "inrushWindowFromStart": self._get_inrush_window_from_start(),
                    "inrushThreshold": self.edit_inrush_threshold.value(),
                    "inrushTimeThreshold": self.edit_inrush_time_threshold.value(),
                },
                "i2tCfg": {
                    "useI2t": self.i2t_enable.isChecked(),
                    "nominalCurrent": self.edit_nominal_current.value(),
                    "nominalCurrentSq": self.edit_nominal_current.value() ** 2,
                    "timeThreshold": self.edit_time_threshold.value(),
                    "i2tThreshold": self.edit_nominal_current.value() ** 2 * self.edit_time_threshold.value(),
                },
            },
        }


class LogicChannelPage(QWidget):
    logicChanged = pyqtSignal()

    def __init__(self, channel_index, parent=None):
        super().__init__(parent)
        self.channel_index = channel_index
        self.setStyleSheet(PAGE_LABEL_STYLE)
        self._sensor_source_provider = lambda: []

        outer = QVBoxLayout(self)
        outer.setContentsMargins(0, 0, 0, 0)
        outer.setSpacing(8)
        outer.setAlignment(Qt.AlignTop)

        logic_group = QGroupBox(f"Control logic")
        logic_form = QFormLayout(logic_group)
        self.logic_group = logic_group
        logic_form.setContentsMargins(10, 12, 10, 10)
        logic_form.setVerticalSpacing(6)
        logic_form.setHorizontalSpacing(12)

        isused_group = QHBoxLayout()
        isused_group.setSpacing(0)
        self.check_used = QCheckBox("")
        self.check_used.setStyleSheet(CHECKBOX_STYLE)
        self.check_used.setChecked(False)
        self.outside_controlled = QLabel("CHANNEL CONTROLLED VIA BATCH")
        self.outside_controlled.setStyleSheet("color: red; font-weight: bold; font-size: 1.2em;")
        self.outside_controlled.setVisible(False)
        
        isused_group.addWidget(self.check_used)
        isused_group.addWidget(self.outside_controlled)

        logic_form.addRow("Channel used:", isused_group)

        self.check_always_on = QCheckBox("")
        self.check_always_on.setStyleSheet(CHECKBOX_STYLE)
        self.check_always_on.setChecked(False)
        logic_form.addRow("Always on", self.check_always_on)

        self.combo_operator = QComboBox()
        for label, value in LOGIC_OPERATOR_LABELS:
            self.combo_operator.addItem(label, value)
        self.combo_operator.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        logic_form.addRow("Operator:", self.combo_operator)

        self.label_relation = QLabel("First: - | Second: -")
        self.label_relation.setStyleSheet("QLabel { color: #B0B0B0; font-style: italic; }")
        logic_form.addRow("Allowed:", self.label_relation)

        self.combo_input1_type = QComboBox()
        self.combo_input2_type = QComboBox()
        self.combo_input1_type.setFixedWidth(150)
        self.combo_input2_type.setFixedWidth(150)

        self.combo_input1_source = QComboBox()
        self.combo_input2_source = QComboBox()
        self.combo_input1_source.setFixedWidth(240)
        self.combo_input2_source.setFixedWidth(240)
        self.combo_input1_source.setVisible(False)
        self.combo_input2_source.setVisible(False)

        self.edit_input1_value = QLineEdit("0")
        self.edit_input1_value.setPlaceholderText("Input ID or constant")
        self.edit_input2_value = QLineEdit("0")
        self.edit_input2_value.setPlaceholderText("Input ID or constant")
        self.edit_input1_value.setFixedWidth(180)
        self.edit_input2_value.setFixedWidth(180)

        self.combo_input1_bool = QComboBox()
        self.combo_input1_bool.addItem("OFF", 0)
        self.combo_input1_bool.addItem("ON", 1)
        self.combo_input1_bool.setFixedWidth(120)
        self.combo_input1_bool.setVisible(False)

        self.combo_input2_bool = QComboBox()
        self.combo_input2_bool.addItem("OFF", 0)
        self.combo_input2_bool.addItem("ON", 1)
        self.combo_input2_bool.setFixedWidth(120)
        self.combo_input2_bool.setVisible(False)

        input1_row = QWidget()
        input1_layout = QHBoxLayout(input1_row)
        input1_layout.setContentsMargins(0, 0, 0, 0)
        input1_layout.setSpacing(10)
        input1_layout.addWidget(self.combo_input1_type)
        input1_layout.addWidget(self.combo_input1_source)
        input1_layout.addWidget(self.combo_input1_bool)
        input1_layout.addWidget(self.edit_input1_value)
        input1_layout.addStretch(1)

        input2_row = QWidget()
        input2_layout = QHBoxLayout(input2_row)
        input2_layout.setContentsMargins(0, 0, 0, 0)
        input2_layout.setSpacing(10)
        input2_layout.addWidget(self.combo_input2_type)
        input2_layout.addWidget(self.combo_input2_source)
        input2_layout.addWidget(self.combo_input2_bool)
        input2_layout.addWidget(self.edit_input2_value)
        input2_layout.addStretch(1)

        logic_form.addRow("Input 1:", input1_row)
        logic_form.addRow("Input 2:", input2_row)
        outer.addWidget(logic_group)

        self.combo_operator.currentIndexChanged.connect(self._sync_logic_operator_state)
        self.combo_input1_type.currentIndexChanged.connect(lambda *_: self._sync_input_row(self.combo_input1_type, self.edit_input1_value))
        self.combo_input2_type.currentIndexChanged.connect(lambda *_: self._sync_input_row(self.combo_input2_type, self.edit_input2_value))
        self.combo_input1_source.currentIndexChanged.connect(self._emit_logic_changed)
        self.combo_input2_source.currentIndexChanged.connect(self._emit_logic_changed)
        self.check_always_on.toggled.connect(self._on_always_on_toggled)

        self.check_used.toggled.connect(self._emit_logic_changed)
        self.combo_operator.currentIndexChanged.connect(self._emit_logic_changed)
        self.combo_input1_type.currentIndexChanged.connect(self._emit_logic_changed)
        self.combo_input2_type.currentIndexChanged.connect(self._emit_logic_changed)
        self.combo_input1_bool.currentIndexChanged.connect(self._emit_logic_changed)
        self.combo_input2_bool.currentIndexChanged.connect(self._emit_logic_changed)
        self.edit_input1_value.textChanged.connect(self._emit_logic_changed)
        self.edit_input2_value.textChanged.connect(self._emit_logic_changed)
        self.check_always_on.toggled.connect(self._emit_logic_changed)

        self._sync_logic_operator_state()
        self._sync_input_row(self.combo_input1_type, self.edit_input1_value)
        self._sync_input_row(self.combo_input2_type, self.edit_input2_value)
        self.refresh_sensor_sources()

    def set_sensor_source_provider(self, provider):
        self._sensor_source_provider = provider or (lambda: [])
        self.refresh_sensor_sources()

    def refresh_sensor_sources(self):
        # Determine allowed var types set by the operator logic (SCHMITT/ANALOG/ANY)
        first_allowed_var = getattr(self, "_allowed_first_var", None)
        second_allowed_var = getattr(self, "_allowed_second_var", None)

        try:
            sources_first = list(self._sensor_source_provider(first_allowed_var))
        except TypeError:
            # provider may not accept an argument; call without and fall back
            sources_first = list(self._sensor_source_provider())

        try:
            sources_second = list(self._sensor_source_provider(second_allowed_var))
        except TypeError:
            sources_second = list(self._sensor_source_provider())

        if not sources_first:
            sources_first = [("No used inputs available", None)]
        if not sources_second:
            sources_second = [("No used inputs available", None)]

        # Populate input1 source combo
        current_value = self.combo_input1_source.currentData()
        combo = self.combo_input1_source
        combo.blockSignals(True)
        combo.clear()
        for label, value in sources_first:
            combo.addItem(label, value)
        if current_value is not None:
            current_index = combo.findData(current_value)
            if current_index >= 0:
                combo.setCurrentIndex(current_index)
            else:
                combo.setCurrentIndex(0)
        else:
            combo.setCurrentIndex(0)
        combo.blockSignals(False)

        # Populate input2 source combo
        current_value = self.combo_input2_source.currentData()
        combo = self.combo_input2_source
        combo.blockSignals(True)
        combo.clear()
        for label, value in sources_second:
            combo.addItem(label, value)
        if current_value is not None:
            current_index = combo.findData(current_value)
            if current_index >= 0:
                combo.setCurrentIndex(current_index)
            else:
                combo.setCurrentIndex(0)
        else:
            combo.setCurrentIndex(0)
        combo.blockSignals(False)
        self._sync_input_row(self.combo_input1_type, self.edit_input1_value)
        self._sync_input_row(self.combo_input2_type, self.edit_input2_value)

    def _logic_operand_value(self, type_combo, source_combo, bool_combo, value_edit):
        input_type = type_combo.currentData()
        if input_type == LOGIC_INPUT_TYPE_SENSOR:
            source_value = source_combo.currentData()
            return 0 if source_value is None else int(source_value)
        if input_type == LOGIC_INPUT_TYPE_CONST_SCHMITT:
            bool_value = bool_combo.currentData()
            return 1 if bool_value is None else int(bool_value)
        if input_type == LOGIC_INPUT_TYPE_CONST_ANALOG:
            try:
                return int(value_edit.text())
            except Exception:
                return 0
        return 0

    def _emit_logic_changed(self, *_args):
        self.logicChanged.emit()

    def apply_dict(self, data):
        if not isinstance(data, dict):
            return

        self.check_used.setChecked(bool(data.get("isUsed", self.check_used.isChecked())))

        exp = data.get("exp", {}) or {}
        operator_index = self.combo_operator.findData(exp.get("opr", self.combo_operator.currentData()))
        if operator_index >= 0:
            self.combo_operator.setCurrentIndex(operator_index)

        input1_type = exp.get("input1Type", self.combo_input1_type.currentData())
        input2_type = exp.get("input2Type", self.combo_input2_type.currentData())
        input1_index = self.combo_input1_type.findData(input1_type)
        input2_index = self.combo_input2_type.findData(input2_type)
        if input1_index >= 0:
            self.combo_input1_type.setCurrentIndex(input1_index)
        if input2_index >= 0:
            self.combo_input2_type.setCurrentIndex(input2_index)

        input1_const = exp.get("input1Const", exp.get("input1ID", 0))
        input2_const = exp.get("input2Const", exp.get("input2ID", 0))
        self.combo_input1_source.setCurrentIndex(max(0, self.combo_input1_source.findData(exp.get("input1ID", self.combo_input1_source.currentData()))))
        self.combo_input2_source.setCurrentIndex(max(0, self.combo_input2_source.findData(exp.get("input2ID", self.combo_input2_source.currentData()))))
        self.combo_input1_bool.setCurrentIndex(1 if int(input1_const) else 0)
        self.combo_input2_bool.setCurrentIndex(1 if int(input2_const) else 0)
        self.edit_input1_value.setText(str(int(input1_const)))
        self.edit_input2_value.setText(str(int(input2_const)))

        self.check_always_on.setChecked(
            self.combo_operator.currentData() == 0x03
            and self.combo_input1_type.currentData() == LOGIC_INPUT_TYPE_CONST_SCHMITT
            and self.combo_input2_type.currentData() == LOGIC_INPUT_TYPE_CONST_SCHMITT
            and self.combo_input1_bool.currentData() == 1
            and self.combo_input2_bool.currentData() == 1
        )

        self._sync_logic_operator_state()

    def disable_logic(self, isDisabled, index):
        if isDisabled == True:
            self.check_used.setVisible(False)
            self.outside_controlled.setText(f"CHANNEL CONTROLLED BATCH WITH OUT_{index + 1}")
            self.outside_controlled.setVisible(True)
            self.logic_group.setDisabled(True)
        else:
            self.check_used.setVisible(True)
            self.outside_controlled.setVisible(False)
            self.logic_group.setDisabled(False)

    def to_dict(self):
        return {
            "isUsed": self.check_used.isChecked(),
            "exp": {
                "input1Type": self.combo_input1_type.currentData(),
                "input1ID": self.combo_input1_source.currentData(),
                "input1Const": self._logic_operand_value(
                    self.combo_input1_type,
                    self.combo_input1_source,
                    self.combo_input1_bool,
                    self.edit_input1_value,
                ),
                "input2Type": self.combo_input2_type.currentData(),
                "input2ID": self.combo_input2_source.currentData(),
                "input2Const": self._logic_operand_value(
                    self.combo_input2_type,
                    self.combo_input2_source,
                    self.combo_input2_bool,
                    self.edit_input2_value,
                ),
                "opr": self.combo_operator.currentData(),
            },
        }

    def pack_logic_record(self):
        return struct.pack(
            LOGIC_RECORD_FORMAT,
            1 if self.check_used.isChecked() else 0,
            self.combo_input1_type.currentData(),
            self._logic_operand_value(
                self.combo_input1_type,
                self.combo_input1_source,
                self.combo_input1_bool,
                self.edit_input1_value,
            ),
            self.combo_input2_type.currentData(),
            self._logic_operand_value(
                self.combo_input2_type,
                self.combo_input2_source,
                self.combo_input2_bool,
                self.edit_input2_value,
            ),
            self.combo_operator.currentData(),
        )

    def _on_always_on_toggled(self, checked):
        if checked:
            # Force operator E and both inputs to CONST_SCHMITT = TRUE
            # Operator E value is 0x03, CONST_SCHMITT input type is 0x01
            opr_value = 0x03
            const_schmitt = 0x01
            # Set operator (this will repopulate allowed input type combos)
            idx = self.combo_operator.findData(opr_value)
            if idx >= 0:
                self.combo_operator.setCurrentIndex(idx)
            self._sync_logic_operator_state()

            # Set both input types to CONST_SCHMITT if available
            try:
                idx1 = self.combo_input1_type.findData(const_schmitt)
                if idx1 >= 0:
                    self.combo_input1_type.setCurrentIndex(idx1)
            except Exception:
                pass
            try:
                idx2 = self.combo_input2_type.findData(const_schmitt)
                if idx2 >= 0:
                    self.combo_input2_type.setCurrentIndex(idx2)
            except Exception:
                pass

            # Set constant values to TRUE (1)
            self.combo_input1_bool.setCurrentIndex(1)
            self.combo_input2_bool.setCurrentIndex(1)

            # Disable editing of input types and values while Always ON
            self.combo_input1_type.setEnabled(False)
            self.combo_input2_type.setEnabled(False)
            self.combo_input1_bool.setEnabled(False)
            self.combo_input2_bool.setEnabled(False)
            self.edit_input1_value.setEnabled(False)
            self.edit_input2_value.setEnabled(False)
            self.combo_operator.setEnabled(False)
        else:
            # Re-enable and refresh operator-derived state
            self.combo_operator.setEnabled(True)
            self.combo_input1_type.setEnabled(True)
            self.combo_input2_type.setEnabled(True)
            # Let the operator logic decide whether input2 should be enabled
            self._sync_logic_operator_state()

    def _logic_var_label(self, value):
        return {
            0x00: "SCHMITT",
            0x01: "ANALOG",
            0x02: "ANY",
            0x03: "NONE",
        }.get(value, str(value))

    def _populate_input_type_combo(self, combo, allowed_values, current_value=None):
        combo.blockSignals(True)
        combo.clear()
        for value in allowed_values:
            combo.addItem(self._input_type_label(value), value)
        if current_value in allowed_values:
            combo.setCurrentIndex(combo.findData(current_value))
        elif allowed_values:
            combo.setCurrentIndex(0)
        combo.blockSignals(False)

    def _input_type_label(self, value):
        return {
            0x00: "SENSOR",
            0x01: "CONST_SCHMITT",
            0x02: "CONST_ANALOG",
            0x03: "UNSET",
        }.get(value, str(value))

    def _sync_logic_operator_state(self):
        operator = self.combo_operator.currentData()
        allowed_first_var, allowed_second_var = LOGIC_OPERATOR_RELATIONS.get(operator, (0x03, 0x03))
        self.label_relation.setText(
            f"First: {self._logic_var_label(allowed_first_var)} | Second: {self._logic_var_label(allowed_second_var)}"
        )

        first_allowed = LOGIC_VAR_TO_INPUT_TYPES.get(allowed_first_var, [0x03])
        second_allowed = LOGIC_VAR_TO_INPUT_TYPES.get(allowed_second_var, [0x03])

        current_first = self.combo_input1_type.currentData()
        current_second = self.combo_input2_type.currentData()
        self._populate_input_type_combo(self.combo_input1_type, first_allowed, current_first)
        self._populate_input_type_combo(self.combo_input2_type, second_allowed, current_second)
        self._sync_input_row(self.combo_input1_type, self.edit_input1_value)
        self._sync_input_row(self.combo_input2_type, self.edit_input2_value)

        # Store allowed var types for sensor source filtering (SCHMITT/ANALOG/ANY)
        self._allowed_first_var = allowed_first_var
        self._allowed_second_var = allowed_second_var
        # Refresh available sensor sources to reflect new allowed var filters
        try:
            self.refresh_sensor_sources()
        except Exception:
            pass

        second_enabled = allowed_second_var != 0x03
        self.combo_input2_type.setEnabled(second_enabled)
        self.edit_input2_value.setEnabled(second_enabled and self.combo_input2_type.currentData() != 0x03)
        if not second_enabled:
            self.edit_input2_value.setText("0")

    def _sync_input_row(self, type_combo, value_edit):
        input_type = type_combo.currentData()
        is_sensor = input_type == 0x00
        is_const = input_type in (0x01, 0x02)
        source_combo = self.combo_input1_source if type_combo is self.combo_input1_type else self.combo_input2_source
        bool_combo = self.combo_input1_bool if type_combo is self.combo_input1_type else self.combo_input2_bool

        source_combo.setVisible(is_sensor)
        source_combo.setEnabled(is_sensor)
        bool_combo.setVisible(input_type == 0x01)
        bool_combo.setEnabled(input_type == 0x01)
        value_edit.setVisible(input_type == 0x02)
        value_edit.setEnabled(input_type == 0x02)
        if input_type == 0x00:
            value_edit.setPlaceholderText("Input ID")
        elif input_type in (0x01, 0x02):
            value_edit.setPlaceholderText("Constant value")
        else:
            value_edit.setPlaceholderText("Unused")


class InputsConfigPage(QWidget):
    inputsChanged = pyqtSignal()

    @staticmethod
    def _make_column_separator():
        separator = QFrame()
        separator.setFrameShape(QFrame.VLine)
        separator.setFrameShadow(QFrame.Plain)
        separator.setLineWidth(1)
        separator.setStyleSheet("color: #444444;")
        return separator

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setStyleSheet(PAGE_LABEL_STYLE)

        outer = QVBoxLayout(self)
        outer.setContentsMargins(20, 20, 20, 20)
        outer.setSpacing(12)
        outer.setAlignment(Qt.AlignTop)

        summary = QLabel(
            "This tab only changes the viewport. Physical inputs expose Analog/Digital selection; CAN inputs add bus and decoding fields."
        )
        summary.setWordWrap(True)
        outer.addWidget(summary)

        physical_group = QGroupBox("Physical inputs")
        physical_layout = QGridLayout(physical_group)
        physical_layout.setContentsMargins(10, 12, 10, 10)
        physical_layout.setHorizontalSpacing(10)
        physical_layout.setVerticalSpacing(8)
        physical_layout.addWidget(QLabel("Input"), 0, 0)
        physical_layout.addWidget(self._make_column_separator(), 0, 1)
        physical_layout.addWidget(QLabel("Used by outputs"), 0, 2)
        physical_layout.addWidget(self._make_column_separator(), 0, 3)
        physical_layout.addWidget(QLabel("Interpretation"), 0, 4)

        self.physical_rows = []
        for index in range(8):
            label = QLabel(f"Physical input {index + 1}")
            combo = QComboBox()
            usage = QLabel("-")
            usage.setWordWrap(True)
            for option_label, option_value in INPUT_INTERPRETATION_LABELS:
                combo.addItem(option_label, option_value)
            physical_layout.addWidget(label, index + 1, 0)
            physical_layout.addWidget(combo, index + 1, 4)
            self.physical_rows.append({"label": label, "interpretation": combo, "usage": usage})
            combo.currentIndexChanged.connect(lambda *_: self.inputsChanged.emit())

        outer.addWidget(physical_group)

        can_group = QGroupBox("CAN inputs")
        can_layout = QGridLayout(can_group)
        can_layout.setContentsMargins(10, 12, 10, 10)
        can_layout.setHorizontalSpacing(10)
        can_layout.setVerticalSpacing(8)
        can_layout.addWidget(QLabel("Input"), 0, 0)
        can_layout.addWidget(self._make_column_separator(), 0, 1)
        can_layout.addWidget(QLabel("Used"), 0, 2)
        can_layout.addWidget(self._make_column_separator(), 0, 3)
        can_layout.addWidget(QLabel("Used by outputs"), 0, 4)
        can_layout.addWidget(self._make_column_separator(), 0, 5)
        can_layout.addWidget(QLabel("CAN instance"), 0, 6)
        can_layout.addWidget(self._make_column_separator(), 0, 7)
        can_layout.addWidget(QLabel("CAN ID"), 0, 8)
        can_layout.addWidget(self._make_column_separator(), 0, 9)
        can_layout.addWidget(QLabel("Offset"), 0, 10)
        can_layout.addWidget(self._make_column_separator(), 0, 11)
        can_layout.addWidget(QLabel("Data type"), 0, 12)
        can_layout.addWidget(self._make_column_separator(), 0, 13)
        can_layout.addWidget(QLabel("Interpretation"), 0, 14)

        self.can_rows = []
        for index in range(16):
            label = QLabel(f"CAN input {index + 1}")

            check_used = QCheckBox()
            check_used.setStyleSheet(CHECKBOX_STYLE)

            combo_instance = QComboBox()
            for option_label, option_value in CAN_INSTANCE_LABELS:
                combo_instance.addItem(option_label, option_value)

            spin_can_id = _make_spinbox(0, 0x7FF, 0)
            spin_can_id.setDisplayIntegerBase(16)
            spin_can_id.setPrefix("0x")

            spin_offset = _make_spinbox(0, 7, 0)

            combo_data_type = QComboBox()
            for option_label, option_value in CAN_INPUT_DATA_TYPE_LABELS:
                combo_data_type.addItem(option_label, option_value)

            combo_interpretation = QComboBox()
            for option_label, option_value in INPUT_INTERPRETATION_LABELS:
                combo_interpretation.addItem(option_label, option_value)
            usage = QLabel("-")
            usage.setWordWrap(True)

            can_layout.addWidget(label, index + 1, 0)
            can_layout.addWidget(self._make_column_separator(), index + 1, 1)
            can_layout.addWidget(check_used, index + 1, 2)
            can_layout.addWidget(self._make_column_separator(), index + 1, 3)
            can_layout.addWidget(usage, index + 1, 4)
            can_layout.addWidget(self._make_column_separator(), index + 1, 5)
            can_layout.addWidget(combo_instance, index + 1, 6)
            can_layout.addWidget(self._make_column_separator(), index + 1, 7)
            can_layout.addWidget(spin_can_id, index + 1, 8)
            can_layout.addWidget(self._make_column_separator(), index + 1, 9)
            can_layout.addWidget(spin_offset, index + 1, 10)
            can_layout.addWidget(self._make_column_separator(), index + 1, 11)
            can_layout.addWidget(combo_data_type, index + 1, 12)
            can_layout.addWidget(self._make_column_separator(), index + 1, 13)
            can_layout.addWidget(combo_interpretation, index + 1, 14)

            self.can_rows.append(
                {
                    "label": label,
                    "used": check_used,
                    "instance": combo_instance,
                    "can_id": spin_can_id,
                    "offset": spin_offset,
                    "data_type": combo_data_type,
                    "interpretation": combo_interpretation,
                    "usage": usage,
                }
            )
            check_used.toggled.connect(lambda *_: self.inputsChanged.emit())
            combo_instance.currentIndexChanged.connect(lambda *_: self.inputsChanged.emit())
            spin_can_id.valueChanged.connect(lambda *_: self.inputsChanged.emit())
            spin_offset.valueChanged.connect(lambda *_: self.inputsChanged.emit())
            combo_data_type.currentIndexChanged.connect(lambda *_: self.inputsChanged.emit())
            combo_interpretation.currentIndexChanged.connect(lambda *_: self.inputsChanged.emit())

        outer.addWidget(can_group)

    def set_usage_by_input(self, usage_by_input):
        usage_by_input = usage_by_input or {}

        for index, row in enumerate(self.physical_rows):
            outputs = usage_by_input.get(index, [])
            row["usage"].setText(", ".join(outputs) if outputs else "-")

        for index, row in enumerate(self.can_rows):
            outputs = usage_by_input.get(8 + index, [])
            row["usage"].setText(", ".join(outputs) if outputs else "-")

    def _packed_input_mode(self, interpretation_combo):
        output = interpretation_combo.currentData()
        return IN_MODE_SCHMITT if output == 0x00 else IN_MODE_ANALOG

    def _unpacked_input_mode(self, interpretation_value):
        return 0x00 if interpretation_value == IN_MODE_SCHMITT else 0x01

    def to_dict(self):
        return {
            "physical": [
                {
                    "location": index,
                    "type": IN_TYPE_PHY,
                    "mode": self._packed_input_mode(row["interpretation"]),
                }
                for index, row in enumerate(self.physical_rows)
            ],
            "can": [
                {
                    "isUsed": row["used"].isChecked(),
                    "canInstance": row["instance"].currentData(),
                    "canId": row["can_id"].value(),
                    "offset": row["offset"].value(),
                    "dataType": row["data_type"].currentData(),
                    "location": index,
                    "type": IN_TYPE_CAN,
                    "mode": self._packed_input_mode(row["interpretation"]),
                }
                for index, row in enumerate(self.can_rows)
            ],
        }

    def apply_dict(self, data):
        if not isinstance(data, dict):
            return

        physical = data.get("physical", []) or []
        for index, row_data in enumerate(physical[: len(self.physical_rows)]):
            if not isinstance(row_data, dict):
                continue
            mode_index = self._unpacked_input_mode(row_data.get("mode", self._packed_input_mode(self.physical_rows[index]["interpretation"]))   )
            # mode_index = self.physical_rows[index]["interpretation"].findData(
            #     IN_MODE_SCHMITT if int(row_data.get("mode", IN_MODE_SCHMITT)) == IN_MODE_SCHMITT else 0x01
            # )
            if mode_index >= 0:
                self.physical_rows[index]["interpretation"].setCurrentIndex(mode_index)

        can_rows = data.get("can", []) or []
        for index, row_data in enumerate(can_rows[: len(self.can_rows)]):
            if not isinstance(row_data, dict):
                continue
            self.can_rows[index]["used"].setChecked(bool(row_data.get("isUsed", self.can_rows[index]["used"].isChecked())))
            instance_index = self.can_rows[index]["instance"].findData(row_data.get("canInstance", self.can_rows[index]["instance"].currentData()))
            if instance_index >= 0:
                self.can_rows[index]["instance"].setCurrentIndex(instance_index)
            self.can_rows[index]["can_id"].setValue(int(row_data.get("canId", self.can_rows[index]["can_id"].value())))
            self.can_rows[index]["offset"].setValue(int(row_data.get("offset", self.can_rows[index]["offset"].value())))
            data_type_index = self.can_rows[index]["data_type"].findData(row_data.get("dataType", self.can_rows[index]["data_type"].currentData()))
            if data_type_index >= 0:
                self.can_rows[index]["data_type"].setCurrentIndex(data_type_index)
            mode_index = self._unpacked_input_mode(row_data.get("mode", self._packed_input_mode(self.can_rows[index]["interpretation"])))
            # mode_index = self.can_rows[index]["interpretation"].findData(
            #     IN_MODE_SCHMITT if int(row_data.get("mode", IN_MODE_SCHMITT)) == IN_MODE_SCHMITT else 0x01
            # )
            if mode_index >= 0:
                self.can_rows[index]["interpretation"].setCurrentIndex(mode_index)

    def pack_can_inputs(self):
        payload = []
        for row in self.can_rows:
            payload.append(
                struct.pack(
                    CAN_INPUT_RECORD_FORMAT,
                    1 if row["used"].isChecked() else 0,
                    row["instance"].currentData(),
                    row["can_id"].value(),
                    row["offset"].value(),
                    row["data_type"].currentData(),
                )
            )
        return b"".join(payload)

    def pack_inputs(self):
        payload = []
        for index, row in enumerate(self.physical_rows):
            payload.append(
                struct.pack(
                    INPUT_RECORD_FORMAT,
                    index,
                    IN_TYPE_PHY,
                    self._packed_input_mode(row["interpretation"]),
                )
            )
        for index, row in enumerate(self.can_rows):
            payload.append(
                struct.pack(
                    INPUT_RECORD_FORMAT,
                    index,
                    IN_TYPE_CAN,
                    self._packed_input_mode(row["interpretation"]),
                )
            )
        return b"".join(payload)

    def get_sensor_sources(self, allowed_var=None):
        """
        Return a list of available sensor sources as (label, id) tuples.
        If allowed_var is provided (0x00 SCHMITT, 0x01 ANALOG, 0x02 ANY), filter
        physical and CAN inputs to only those matching the requested interpretation.
        """
        sources = []

        for index in range(8):
            # interpretation currentData: 0x00 = Digital (Schmitt), 0x01 = Analog
            interp = self.physical_rows[index]["interpretation"].currentData()
            if allowed_var is None or allowed_var == 0x02:
                include = True
            elif allowed_var == 0x00:
                include = interp == 0x00
            elif allowed_var == 0x01:
                include = interp == 0x01
            else:
                include = True

            if include:
                sources.append((f"PHY input {index + 1}", index))

        for index, row in enumerate(self.can_rows):
            if not row["used"].isChecked():
                continue
            interp = row["interpretation"].currentData()
            if allowed_var is None or allowed_var == 0x02:
                include = True
            elif allowed_var == 0x00:
                include = interp == 0x00
            elif allowed_var == 0x01:
                include = interp == 0x01
            else:
                include = True

            if not include:
                continue

            can_id = row["can_id"].value()
            can_instance = row["instance"].currentData()
            offset = row["offset"].value()
            sources.append(
                (
                    f"CAN input {index + 1} (inst {can_instance + 1}, 0x{can_id:03X}, off {offset})",
                    8 + index,
                )
            )

        return sources


class ConfigTab(QWidget):
    send_binary_requested = pyqtSignal(object)
    send_reset_requested = pyqtSignal()
    request_config_requested = pyqtSignal()
    can_frames_changed = pyqtSignal(object)

    def __init__(self, parent=None):
        super().__init__(parent)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(8)

        title_row = QHBoxLayout()
        title_row.setContentsMargins(12, 12, 12, 12)
        title_label = QLabel("Device control configuration")
        title_label.setStyleSheet("font-size: 16px; font-weight: bold;")
        title_row.addWidget(title_label)
        title_row.addStretch(1)

        self.btn_load_binary = QPushButton("Load binary")
        self.btn_load_binary.clicked.connect(self.load_binary)
        self.btn_load_binary.setStyleSheet(ACTION_BUTTON_STYLE)
        title_row.addWidget(self.btn_load_binary)

        self.btn_export_binary = QPushButton("Export binary")
        self.btn_export_binary.clicked.connect(self.export_binary)
        self.btn_export_binary.setStyleSheet(ACTION_BUTTON_STYLE)
        title_row.addWidget(self.btn_export_binary)

        self.btn_load = QPushButton("Load JSON")
        self.btn_load.clicked.connect(self.load_json)
        self.btn_load.setStyleSheet(ACTION_BUTTON_STYLE)
        title_row.addWidget(self.btn_load)

        self.btn_export = QPushButton("Export JSON")
        self.btn_export.clicked.connect(self.export_json)
        self.btn_export.setStyleSheet(ACTION_BUTTON_STYLE)
        title_row.addWidget(self.btn_export)

        divider = QFrame()
        divider.setFrameShape(QFrame.VLine)
        divider.setFrameShadow(QFrame.Plain)
        divider.setStyleSheet("color: #2D2D2D; background-color: #111111; max-width: 1px; margin: 0px 15px;")
        title_row.addWidget(divider)

        self.btn_send_binary = QPushButton("Send to device")
        self.btn_send_binary.clicked.connect(self._send_binary)
        self.btn_send_binary.setStyleSheet(SEND_DEVICE_BUTTON_STYLE)
        title_row.addWidget(self.btn_send_binary)

        self.btn_request_config = QPushButton("Request config")
        self.btn_request_config.clicked.connect(self._request_config)
        self.btn_request_config.setStyleSheet(ACTION_BUTTON_STYLE)
        title_row.addWidget(self.btn_request_config)

        self.btn_reset_device = CtrlClickButton("Reset device")
        self.btn_reset_device.setToolTip("Works only with Ctrl + Click")
        self.btn_reset_device.ctrlClicked.connect(self._send_reset)
        self.btn_reset_device.setStyleSheet(RESET_BUTTON_STYLE)
        title_row.addWidget(self.btn_reset_device)
        layout.addLayout(title_row)

        # subtitle = QLabel(
        #     "One tab per channel. Type is display-only and derived from the channel number; channels 1-8 export spoc fields as 0. "
        #     "Use Load JSON to restore a saved configuration or Export binary for a packed little-endian payload."
        # )
        # subtitle.setWordWrap(True)
        # layout.addWidget(subtitle)

        self.tabs = QTabWidget()
        self.tabs.setMovable(True)
        self.tabs.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        inputs_scroll = QScrollArea()
        inputs_scroll.setWidgetResizable(True)
        inputs_scroll.setFrameShape(QScrollArea.NoFrame)
        inputs_scroll.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.inputs_page = InputsConfigPage()
        inputs_scroll.setWidget(self.inputs_page)
        self.tabs.addTab(inputs_scroll, "Inputs")
        self.channel_widgets = []
        for index in range(CHANNEL_COUNT):
            scroll = QScrollArea()
            scroll.setWidgetResizable(True)
            scroll.setFrameShape(QScrollArea.NoFrame)
            scroll.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
            page = ChannelConfigPage(index)
            page.logic_page.set_sensor_source_provider(self.inputs_page.get_sensor_sources)
            page.set_pwm_source_provider(self.inputs_page.get_sensor_sources)
            page.logic_page.logicChanged.connect(self._refresh_input_usage_summary)
            page.edit_name.textChanged.connect(self._refresh_input_usage_summary)
            page.batchChanged.connect(self._refresh_batch_relations)
            page.modeChanged.connect(self._verify_overall_mode_validity)
            self.channel_widgets.append(page)
            scroll.setWidget(page)
            self.tabs.addTab(scroll, f"Channel {index + 1}")
        layout.addWidget(self.tabs, 1)

        self.inputs_page.inputsChanged.connect(self._refresh_logic_sensor_sources)
        self.inputs_page.inputsChanged.connect(self._refresh_pwm_sensor_sources)
        self.inputs_page.inputsChanged.connect(self._emit_can_frames_changed)
        self._refresh_logic_sensor_sources()
        self._refresh_pwm_sensor_sources()
        self._emit_can_frames_changed()

        transfer_box = QGroupBox("Transfer status")
        transfer_box.setStyleSheet(TRANSFER_GROUP_STYLE)
        transfer_layout = QVBoxLayout(transfer_box)
        transfer_layout.setContentsMargins(12, 12, 12, 12)
        transfer_layout.setSpacing(6)

        transfer_row = QHBoxLayout()
        transfer_row.setContentsMargins(0, 0, 0, 0)
        transfer_row.setSpacing(8)
        self.lbl_send_status = QLabel("Idle")
        self.lbl_send_status.setStyleSheet("font-weight: bold; background-color: none; color: #E0E0E0;")
        self.lbl_send_status.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Preferred)
        transfer_row.addWidget(self.lbl_send_status)
        self.progress_send = QProgressBar()
        self.progress_send.setRange(0, 100)
        self.progress_send.setValue(0)
        self.progress_send.setTextVisible(True)
        self.progress_send.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        transfer_row.addWidget(self.progress_send, 1)
        transfer_layout.addLayout(transfer_row)
        layout.addWidget(transfer_box)

    def collect_config(self):
        return {
            "channels": [widget.to_dict() for widget in self.channel_widgets],
            "inputs": self.inputs_page.to_dict(),
            "logic": [widget.logic_page.to_dict() for widget in self.channel_widgets],
        }

    def apply_config(self, config):
        if isinstance(config, dict):
            inputs_data = config.get("inputs")
            if isinstance(inputs_data, dict):
                self.inputs_page.apply_dict(inputs_data)

        channels = config.get("channels") if isinstance(config, dict) else config
        if not isinstance(channels, list):
            raise ValueError("Expected a channels list in the JSON file")

        for index, channel_data in enumerate(channels[: len(self.channel_widgets)]):
            if isinstance(channel_data, dict):
                self.channel_widgets[index].apply_dict(channel_data)
                logic_data = channel_data.get("logic")
                if isinstance(logic_data, dict):
                    self.channel_widgets[index].logic_page.apply_dict(logic_data)
                
        if isinstance(config, dict):
            logic_data = config.get("logic")
            if isinstance(logic_data, list):
                for index, logic_item in enumerate(logic_data[: len(self.channel_widgets)]):
                    if isinstance(logic_item, dict):
                        self.channel_widgets[index].logic_page.apply_dict(logic_item)

        self._refresh_batch_relations()
        self._refresh_logic_sensor_sources()
        self._emit_can_frames_changed()

    def get_defined_can_frame_options(self):
        # Build a unique list of arbitration IDs from enabled CAN input rows.
        options = []
        seen_ids = set()
        for index, row in enumerate(self.inputs_page.can_rows):
            if not row["used"].isChecked():
                continue
            can_id = int(row["can_id"].value())
            if can_id in seen_ids:
                continue
            seen_ids.add(can_id)
            instance = int(row["instance"].currentData()) + 1
            options.append((f"CAN input {index + 1} (inst {instance}, 0x{can_id:03X})", can_id))
        return options

    def _emit_can_frames_changed(self):
        self.can_frames_changed.emit(self.get_defined_can_frame_options())

    def _refresh_logic_sensor_sources(self):
        for channel_widget in self.channel_widgets:
            channel_widget.logic_page.refresh_sensor_sources()
        self._refresh_input_usage_summary()

    def _refresh_pwm_sensor_sources(self):
        for channel_widget in self.channel_widgets:
            channel_widget.refresh_pwm_sources()

    def _refresh_batch_relations(self):
        batch_controlled = [-1] * 16
        for index, channel_widget in enumerate(self.channel_widgets):
            batch = channel_widget.combo_batch.currentData()
            if batch != -1 and batch != index and index < 8:
                batch_controlled[batch] = index

        for index, channel_widget in enumerate(self.channel_widgets):
            channel_widget.logic_page.disable_logic(batch_controlled[index] != -1, batch_controlled[index])

        for_removal = [item for i, x in enumerate(batch_controlled) if x != -1 for item in (i, x)]

        for index, channel_widget in enumerate(self.channel_widgets):
            channel_widget.combo_batch.blockSignals(True)
            current_batch = channel_widget.combo_batch.currentData()
            channel_widget.fill_batch_combo(index)
            batch_index = channel_widget.combo_batch.findData(current_batch)
            channel_widget.combo_batch.setCurrentIndex(batch_index)
            channel_widget.combo_batch.blockSignals(False)

        for index, channel_widget in enumerate(self.channel_widgets):
            if index not in for_removal:
                for remove in for_removal:
                    if index != remove:
                        remove_index = channel_widget.combo_batch.findData(remove)
                        if remove_index >= 0:
                            channel_widget.combo_batch.removeItem(remove_index)

    def _verify_overall_mode_validity(self):
        channels_in_pwm = []
        for index, channel_widget in enumerate(self.channel_widgets):
            mode = channel_widget.combo_mode.currentData()
            if mode == OUT_MODE_PWM:
                channels_in_pwm.append(index)

        if len(channels_in_pwm) > 4:
            str_channels_in_pwm = "".join([f"OUT_{x + 1}\n" for x in channels_in_pwm])
            QMessageBox.critical(
                self,
                "PWM setup error",
                f"Maximum number of channels used for PWM is 4, asked for {len(channels_in_pwm)}.\n List:\n {str_channels_in_pwm}",
            )

    def _refresh_input_usage_summary(self):
        usage_by_input = {}

        for output_index, channel_widget in enumerate(self.channel_widgets):
            logic_data = channel_widget.logic_page.to_dict()
            if not logic_data.get("isUsed"):
                continue

            output_name = channel_widget.edit_name.text().strip() or f"OUT_{output_index + 1}"
            output_label = f"CH{output_index + 1}:{output_name}"

            exp = logic_data.get("exp", {}) or {}
            for type_key, id_key in (("input1Type", "input1ID"), ("input2Type", "input2ID")):
                if exp.get(type_key) != LOGIC_INPUT_TYPE_SENSOR:
                    continue

                source_id = exp.get(id_key)
                if source_id is None:
                    continue
                try:
                    source_id = int(source_id)
                except Exception:
                    continue

                if source_id not in usage_by_input:
                    usage_by_input[source_id] = []
                if output_label not in usage_by_input[source_id]:
                    usage_by_input[source_id].append(output_label)

        self.inputs_page.set_usage_by_input(usage_by_input)

    def _build_output_payload(self):
        return b"".join(widget.pack_binary_record() for widget in self.channel_widgets)

    def _build_can_inputs_payload(self):
        return self.inputs_page.pack_can_inputs()

    def _build_inputs_payload(self):
        return self.inputs_page.pack_inputs()

    def _build_logic_payload(self):
        return b"".join(widget.logic_page.pack_logic_record() for widget in self.channel_widgets)

    def _build_binary_payload(self):
        # Payload order matches the device parser: output config, CAN inputs, inputs, logic.
        return (
            self._build_output_payload()
            + self._build_can_inputs_payload()
            + self._build_inputs_payload()
            + self._build_logic_payload()
        )

    def _send_binary(self):
        self.set_isotp_state(0, "Preparing transfer", busy=True)
        self.send_binary_requested.emit(self._build_binary_payload())

    def _send_reset(self):
        self.set_isotp_state(0, "Preparing reset", busy=True)
        self.send_reset_requested.emit()

    def _request_config(self):
        self.set_isotp_state(0, "Preparing config request", busy=True)
        self.request_config_requested.emit()

    def set_isotp_state(self, progress, status=None, busy=None):
        self.progress_send.setValue(max(0, min(100, int(progress))))
        if status is not None:
            self.lbl_send_status.setText(status)
        if busy is not None:
            self.btn_send_binary.setEnabled(not busy)
            self.btn_reset_device.setEnabled(not busy)
            self.btn_request_config.setEnabled(not busy)

    def load_json(self):
        file_path, _ = QFileDialog.getOpenFileName(
            self,
            "Load output channel configuration",
            str(CONFIG_DIR),
            "JSON Files (*.json);;All Files (*)",
        )
        if not file_path:
            return

        try:
            with open(file_path, "r", encoding="utf-8") as handle:
                loaded = json.load(handle)
            self.apply_config(loaded)
        except Exception as exc:
            QMessageBox.critical(self, "Load failed", f"Could not load JSON: {exc}")

    def load_binary(self):
        file_path, _ = QFileDialog.getOpenFileName(
            self,
            "Load packed STM32 binary",
            str(CONFIG_DIR),
            "Binary Files (*.bin);;All Files (*)",
        )
        if not file_path:
            return

        try:
            with open(file_path, "rb") as handle:
                data = handle.read()
            self.load_binary_payload(data)
        except Exception as exc:
            QMessageBox.critical(self, "Load failed", f"Could not load binary: {exc}")

    def load_binary_payload(self, data):
        try:
            self._load_binary_payload(data)
        except Exception as exc:
            QMessageBox.critical(self, "Load failed", f"Could not load binary: {exc}")

    def _load_binary_payload(self, data):
        expected_size = (
            OUTPUT_CONFIG_TOTAL_SIZE
            + CAN_INPUT_TOTAL_SIZE
            + INPUT_TOTAL_SIZE
            + LOGIC_TOTAL_SIZE
        )
        legacy_output_size = LEGACY_OUTPUT_CONFIG_TOTAL_SIZE
        payload = None
        if len(data) == expected_size:
            payload = data
        elif len(data) == legacy_output_size:
            payload = data
        elif len(data) >= 5 and data[:4] == BINARY_MAGIC:
            payload = data[5:]
            if len(payload) not in (expected_size, legacy_output_size):
                raise ValueError("Binary header found but payload size mismatch")
        else:
            raise ValueError(f"Invalid binary size: {len(data)} bytes")

        def _clamp(v, lo, hi):
            try:
                iv = int(v)
            except Exception:
                return lo
            if iv < lo:
                return lo
            if iv > hi:
                return hi
            return iv

        if len(payload) == legacy_output_size:
            channels = []

        def _build_output_channel_dict(values, include_pwm):
            if include_pwm:
                (
                    ch_id,
                    type_v,
                    mode_v,
                    spoc_id,
                    spoc_ch,
                    name_bytes,
                    batch,
                    after_err_beh,
                    after_err_latch_time,
                    act_on_safety,
                    err_retry_threshold,
                    retry_timer_interval,
                    use_soc,
                    nominal_threshold,
                    allow_inrush,
                    inrush_window_from_start,
                    inrush_threshold,
                    inrush_time_threshold,
                    use_i2t,
                    nominal_current,
                    nominal_current_sq,
                    time_threshold,
                    i2t_threshold,
                    base_duty,
                    duty_input,
                    *axis_values,
                    use_softstart,
                    softstart_start_duty,
                    softstart_end_duty,
                    softstart_time_threshold,
                ) = values
                input_axis = [int(axis_values[index]) for index in range(OUT_PWM_MAP_RESOLUTION)]
                duty_axis = [int(axis_values[OUT_PWM_MAP_RESOLUTION + index]) for index in range(OUT_PWM_MAP_RESOLUTION)]
            else:
                (
                    ch_id,
                    type_v,
                    mode_v,
                    spoc_id,
                    spoc_ch,
                    name_bytes,
                    batch,
                    after_err_beh,
                    after_err_latch_time,
                    act_on_safety,
                    err_retry_threshold,
                    retry_timer_interval,
                    use_soc,
                    nominal_threshold,
                    allow_inrush,
                    inrush_window_from_start,
                    inrush_threshold,
                    inrush_time_threshold,
                    use_i2t,
                    nominal_current,
                    nominal_current_sq,
                    time_threshold,
                    i2t_threshold,
                ) = values
                base_duty = 0
                duty_input = 0xFFFF
                input_axis = [0] * OUT_PWM_MAP_RESOLUTION
                duty_axis = [0] * OUT_PWM_MAP_RESOLUTION
                use_softstart = 0
                softstart_start_duty = 0
                softstart_end_duty = 0
                softstart_time_threshold = 0
            name = name_bytes.split(b"\0", 1)[0].decode("utf-8", errors="replace")
            pwm_cfg = {
                "baseDuty": _clamp(base_duty, 0, 100),
                "dutyInput": _clamp(duty_input, 0, 0xFFFF),
                "inputAxis": [_clamp(value, 0, 5000) for value in input_axis],
                "dutyAxis": [_clamp(value, 0, 100) for value in duty_axis],
            }
            softstart_cfg = {
                "useSoftStart": bool(use_softstart),
                "startDuty": _clamp(softstart_start_duty, 0, 100),
                "endDuty": _clamp(softstart_end_duty, 0, 100),
                "timeThreshold": _clamp(softstart_time_threshold, 0, 2147483647),
            }
            return {
                "id": ch_id,
                "type": type_v,
                "mode": mode_v,
                "spocId": spoc_id,
                "spocChId": spoc_ch,
                "name": name,
                "batch": batch,
                "pwmCfg": pwm_cfg,
                "softStart": softstart_cfg,
                "safety": {
                    "afterErrorCfg": {"behavior": after_err_beh, "latchTime": _clamp(after_err_latch_time, 0, 2147483647)},
                    "actOnSafety": bool(act_on_safety),
                    "errRetryThreshold": _clamp(err_retry_threshold, 0, 65535),
                    "retryTimerInterval": _clamp(retry_timer_interval, 0, 2147483647),
                    "socCfg": {
                        "useSoc": bool(use_soc),
                        "nominalThreshold": _clamp(nominal_threshold, 0, 65535),
                        "allowInrush": bool(allow_inrush),
                        "inrushWindowFromStart": _clamp(inrush_window_from_start, 0, UINT32_MAX),
                        "inrushThreshold": _clamp(inrush_threshold, 0, 2147483647),
                        "inrushTimeThreshold": _clamp(inrush_time_threshold, 0, 2147483647),
                    },
                    "i2tCfg": {
                        "useI2t": bool(use_i2t),
                        "nominalCurrent": _clamp(nominal_current, 0, 65535),
                        "nominalCurrentSq": _clamp(nominal_current_sq, 0, 2147483647),
                        "timeThreshold": _clamp(time_threshold, 0, 2147483647),
                        "i2tThreshold": _clamp(i2t_threshold, 0, 2147483647),
                    },
                },
            }

        if len(payload) == legacy_output_size:
            channels = []
            for i in range(CHANNEL_COUNT):
                start = i * LEGACY_BINARY_RECORD_SIZE
                rec = payload[start : start + LEGACY_BINARY_RECORD_SIZE]
                tup = struct.unpack(LEGACY_BINARY_RECORD_FORMAT, rec)
                channels.append(_build_output_channel_dict(tup, False))

            self.apply_config({"channels": channels})
            return

        offset = 0
        channels = []
        for i in range(CHANNEL_COUNT):
            rec = payload[offset : offset + BINARY_RECORD_SIZE]
            tup = struct.unpack(BINARY_RECORD_FORMAT, rec)
            channels.append(_build_output_channel_dict(tup, True))
            offset += BINARY_RECORD_SIZE
        can_inputs = []
        for index in range(CAN_INPUT_COUNT):
            rec = payload[offset : offset + CAN_INPUT_RECORD_SIZE]
            is_used, can_instance, can_id, cd_offset, data_type = struct.unpack(CAN_INPUT_RECORD_FORMAT, rec)
            can_inputs.append(
                {
                    "isUsed": bool(is_used),
                    "canInstance": can_instance,
                    "canId": can_id,
                    "offset": cd_offset,
                    "dataType": data_type,
                }
            )
            offset += CAN_INPUT_RECORD_SIZE

        inputs = []
        for index in range(INPUT_CONFIG_COUNT):
            rec = payload[offset : offset + INPUT_RECORD_SIZE]
            location, in_type, mode = struct.unpack(INPUT_RECORD_FORMAT, rec)
            inputs.append({"location": location, "type": in_type, "mode": mode})
            offset += INPUT_RECORD_SIZE

        logic = []
        for index in range(LOGIC_CONFIG_COUNT):
            rec = payload[offset : offset + LOGIC_RECORD_SIZE]
            is_used, input1_type, input1_value, input2_type, input2_value, opr = struct.unpack(LOGIC_RECORD_FORMAT, rec)
            logic.append(
                {
                    "isUsed": bool(is_used),
                    "exp": {
                        "input1Type": input1_type,
                        "input1ID": input1_value if input1_type == LOGIC_INPUT_TYPE_SENSOR else 0,
                        "input1Const": input1_value if input1_type != LOGIC_INPUT_TYPE_SENSOR else 0,
                        "input2Type": input2_type,
                        "input2ID": input2_value if input2_type == LOGIC_INPUT_TYPE_SENSOR else 0,
                        "input2Const": input2_value if input2_type != LOGIC_INPUT_TYPE_SENSOR else 0,
                        "opr": opr,
                    },
                }
            )
            offset += LOGIC_RECORD_SIZE

        # Merge mode information from the parsed `inputs` records into the CAN entries
        # so the viewport interpretation can be updated correctly for CAN inputs.
        merged_can = []
        for i, can_entry in enumerate(can_inputs[:CAN_INPUT_COUNT]):
            mode_val = None
            # inputs contains physical (first 8) then CAN entries
            can_input_index = 8 + i
            if can_input_index < len(inputs):
                mode_val = inputs[can_input_index].get("mode")
            entry = dict(can_entry)
            if mode_val is not None:
                entry["mode"] = mode_val
            merged_can.append(entry)

        self.apply_config({"channels": channels, "inputs": {"can": merged_can, "physical": inputs[:8]}, "logic": logic})

    def export_binary(self):
        default_name = str(CONFIG_DIR / "pdm_output_config.bin")
        file_path, _ = QFileDialog.getSaveFileName(
            self,
            "Export packed STM32 binary",
            default_name,
            "Binary Files (*.bin);;All Files (*)",
        )
        if not file_path:
            return

        try:
            with open(file_path, "wb") as handle:
                handle.write(self._build_binary_payload())
        except Exception as exc:
            QMessageBox.critical(self, "Export failed", f"Could not export binary: {exc}")

    def export_json(self):
        default_name = str(CONFIG_DIR / "pdm_output_config.json")
        file_path, _ = QFileDialog.getSaveFileName(
            self,
            "Export output channel configuration",
            default_name,
            "JSON Files (*.json);;All Files (*)",
        )
        if not file_path:
            return

        try:
            with open(file_path, "w", encoding="utf-8") as handle:
                json.dump(self.collect_config(), handle, indent=2)
        except Exception as exc:
            QMessageBox.critical(self, "Export failed", f"Could not export JSON: {exc}")

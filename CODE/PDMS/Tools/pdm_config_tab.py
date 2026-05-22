import json
import struct
from pathlib import Path

from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtWidgets import (
    QCheckBox,
    QComboBox,
    QFileDialog,
    QFormLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMessageBox,
    QPushButton,
    QProgressBar,
    QScrollArea,
    QSpinBox,
    QTabWidget,
    QSizePolicy,
    QVBoxLayout,
    QWidget,
)

from pdm_shared import CHANNEL_COUNT

BASE_DIR = Path(__file__).resolve().parent
CONFIG_DIR = BASE_DIR / "config"
CONFIG_DIR.mkdir(parents=True, exist_ok=True)

OUT_TYPE_BTS500 = 0x00
OUT_TYPE_SPOC2 = 0x01

OUT_MODE_UNUSED = 0x00
OUT_MODE_STD = 0x01
OUT_MODE_PWM = 0x02
OUT_MODE_BATCH = 0x03

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
        "err_retry_threshold": "Error retry threshold:",
        "retry_timer_interval": "Retry timer interval:",
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
        "use_i2t": "Use I2T protection",
        "nominal_current": "Nominal current:",
        "nominal_current_sq": "Nominal current squared:",
        "time_threshold": "Time threshold:",
        "i2t_threshold": "I2T threshold:",
    },
}

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
    "QSpinBox { background-color: #2D2D2D; border: 1px solid #444444; border-radius: 4px; padding: 4px; color: #FFFFFF; }"
)

CHECKBOX_TICK_PATH = (Path(__file__).resolve().parent / "assets" / "checkbox-tick.svg").as_posix()
CHECKBOX_STYLE = (
    "QCheckBox { color: #E0E0E0; background-color: none; padding: 4px; } "
    "QCheckBox::indicator { width: 14px; height: 14px; background-color: #2D2D2D; border: 1px solid #444444; border-radius: 3px; } "
    f'QCheckBox::indicator:checked {{ background-color: #BB86FC; border: 1px solid #BB86FC; image: url("{CHECKBOX_TICK_PATH}"); }}'
)

BINARY_MAGIC = b"PDMB"
BINARY_VERSION = 1
# Packed record layout for one T_OUT_CFG instance:
# id:u8, type:u8, mode:u8, spocId:u8, spocChId:u8, name[32], batch:u8,
# afterErrorCfg.behavior:u8, afterErrorCfg.latchTime:u32, actOnSafety:u8,
# errRetryThreshold:u16, retryTimerInterval:u32,
# socCfg.useSoc:u8, socCfg.nominalThreshold:u32, socCfg.allowInrush:u8,
# socCfg.inrushWindowFromStart:u32, socCfg.inrushThreshold:u32,
# socCfg.inrushTimeThreshold:u32,
# i2tCfg.useI2t:u8, i2tCfg.nominalCurrent:u32, i2tCfg.nominalCurrentSq:u32,
# i2tCfg.timeThreshold:u32, i2tCfg.i2tThreshold:u32.
BINARY_RECORD_FORMAT = "<" + "".join(
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
BINARY_RECORD_SIZE = struct.calcsize(BINARY_RECORD_FORMAT)

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

SEND_DEVICE_BUTTON_STYLE = (
    "QPushButton { padding: 6px 12px; background-color: #09BC8A; color: #2D2D2D; border: none; border-radius: 4px; font-weight: bold; }"
)



class ChannelConfigPage(QWidget):
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
        self.combo_mode.addItem("PWM", OUT_MODE_PWM)
        self.combo_mode.addItem("BATCH", OUT_MODE_BATCH)
        self.edit_name = QLineEdit(f"OUT_{channel_index + 1}")
        self.edit_batch = _make_spinbox(0, 16, 0)

        channel_form.addRow(FIELD_LABELS["channel"]["channel_id"], self.label_channel_id)
        channel_form.addRow(FIELD_LABELS["channel"]["type"], self.label_type)
        channel_form.addRow(FIELD_LABELS["channel"]["spoc_mapping"], self.label_spoc)
        channel_form.addRow(FIELD_LABELS["channel"]["mode"], self.combo_mode)
        channel_form.addRow(FIELD_LABELS["channel"]["name"], self.edit_name)
        channel_form.addRow(FIELD_LABELS["channel"]["batch"], self.edit_batch)
        outer.addWidget(channel_group)

        safety_group = QGroupBox("Safety")
        safety_form = QFormLayout(safety_group)

        self.combo_after_error_behavior = QComboBox()
        self.combo_after_error_behavior.addItem("NO", OUT_ERR_BEH_NO)
        self.combo_after_error_behavior.addItem("LATCH", OUT_ERR_BEH_LATCH)
        self.combo_after_error_behavior.addItem("TIME_LATCH", OUT_ERR_BEH_TIME_LATCH)
        self.combo_after_error_behavior.addItem("RETRY", OUT_ERR_BEH_RETRY)
        self.edit_after_error_latch_time = _make_spinbox(0, 2147483647, 0, " ms")
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

        self.soc_enable = QCheckBox(FIELD_LABELS["soc"]["use_soc"])
        self.soc_enable.setChecked(True)
        outer.addWidget(self.soc_enable)

        self.soc_box = QGroupBox("SOC")
        soc_form = QFormLayout(self.soc_box)
        self.edit_nominal_threshold = _make_spinbox(0, 65535, 0, " mA", 100)
        self.check_allow_inrush = QCheckBox()
        self.check_allow_inrush.setStyleSheet(CHECKBOX_STYLE)
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
        self.edit_inrush_threshold = _make_spinbox(0, 65535, 0, " mA", 100)
        self.edit_inrush_time_threshold = _make_spinbox(0, 2147483647, 0, " ms")
        soc_form.addRow(FIELD_LABELS["soc"]["nominal_threshold"], self.edit_nominal_threshold)
        soc_form.addRow(FIELD_LABELS["soc"]["allow_inrush"], self.check_allow_inrush)
        soc_form.addRow(FIELD_LABELS["soc"]["inrush_window_from_start"], inrush_window_row)
        soc_form.addRow(FIELD_LABELS["soc"]["inrush_threshold"], self.edit_inrush_threshold)
        soc_form.addRow(FIELD_LABELS["soc"]["inrush_time_threshold"], self.edit_inrush_time_threshold)
        outer.addWidget(self.soc_box)

        self.i2t_enable = QCheckBox(FIELD_LABELS["i2t"]["use_i2t"])
        self.i2t_enable.setChecked(False)
        outer.addWidget(self.i2t_enable)

        self.i2t_box = QGroupBox("I2T")
        i2t_form = QFormLayout(self.i2t_box)
        self.edit_nominal_current = _make_spinbox(0, 65535, 0, " mA", 100)
        self.edit_nominal_current_sq = QLineEdit("0")
        self.edit_nominal_current_sq.setReadOnly(True)
        self.edit_time_threshold = _make_spinbox(0, 2147483647, 0, " ms")
        self.edit_i2t_threshold = QLineEdit("0")
        self.edit_i2t_threshold.setReadOnly(True)
        i2t_form.addRow(FIELD_LABELS["i2t"]["nominal_current"], self.edit_nominal_current)
        i2t_form.addRow(FIELD_LABELS["i2t"]["nominal_current_sq"], self.edit_nominal_current_sq)
        i2t_form.addRow(FIELD_LABELS["i2t"]["time_threshold"], self.edit_time_threshold)
        i2t_form.addRow(FIELD_LABELS["i2t"]["i2t_threshold"], self.edit_i2t_threshold)
        outer.addWidget(self.i2t_box)

        self.soc_enable.toggled.connect(self._sync_soc_state)
        self.i2t_enable.toggled.connect(self._sync_i2t_state)
        self.edit_nominal_current.valueChanged.connect(self._update_i2t_fields)
        self.edit_time_threshold.valueChanged.connect(self._update_i2t_fields)
        self.check_inrush_window_infinite.toggled.connect(self._sync_inrush_window_state)

        self._apply_spoc_mapping()
        self._sync_soc_state()
        self._sync_i2t_state()
        self._sync_inrush_window_state()
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
            self.edit_inrush_window_from_start,
            self.check_inrush_window_infinite,
            self.label_inrush_window_infinite,
            self.edit_inrush_threshold,
            self.edit_inrush_time_threshold,
        ):
            widget.setEnabled(enabled)
            widget.setStyleSheet("" if enabled else "color: #222222;")
        self.soc_box.setStyleSheet("" if enabled else "QGroupBox { color: #888888; opacity: 0.2;}")
        self._sync_inrush_window_state()

    def _sync_i2t_state(self):
        enabled = self.i2t_enable.isChecked()
        for widget in (
            self.edit_nominal_current,
            self.edit_nominal_current_sq,
            self.edit_time_threshold,
            self.edit_i2t_threshold,
        ):
            widget.setEnabled(enabled)
            widget.setStyleSheet("" if enabled else "color: #222222;")
        self.i2t_box.setStyleSheet("" if enabled else "QGroupBox { color: #888888; opacity: 0.2;}")

    def _sync_inrush_window_state(self):
        infinite = self.check_inrush_window_infinite.isChecked()
        self.edit_inrush_window_from_start.setVisible(not infinite)
        self.label_inrush_window_infinite.setVisible(infinite)
        self.edit_inrush_window_from_start.setEnabled(self.soc_enable.isChecked() and not infinite)
        self.check_inrush_window_infinite.setEnabled(self.soc_enable.isChecked())

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
        self.edit_i2t_threshold.setText(str(i2t_threshold))

    def apply_dict(self, data):
        mode_value = data.get("mode", OUT_MODE_UNUSED)
        mode_index = self.combo_mode.findData(mode_value)
        if mode_index >= 0:
            self.combo_mode.setCurrentIndex(mode_index)

        self.edit_name.setText(str(data.get("name", self.edit_name.text())))
        self.edit_batch.setValue(int(data.get("batch", self.edit_batch.value())))

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
        self._sync_i2t_state()
        self._update_i2t_fields()

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
            self.edit_batch.value(),  # batch
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
            "batch": self.edit_batch.value(),
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


class ConfigTab(QWidget):
    send_binary_requested = pyqtSignal(object)

    def __init__(self, parent=None):
        super().__init__(parent)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(8)

        title_row = QHBoxLayout()
        title_row.setContentsMargins(12, 12, 12, 12)
        title_label = QLabel("Output channel configuration")
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

        self.btn_send_binary = QPushButton("Send to device")
        self.btn_send_binary.clicked.connect(self._send_binary)
        self.btn_send_binary.setStyleSheet(SEND_DEVICE_BUTTON_STYLE)
        title_row.addWidget(self.btn_send_binary)
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
        self.channel_widgets = []
        for index in range(CHANNEL_COUNT):
            scroll = QScrollArea()
            scroll.setWidgetResizable(True)
            scroll.setFrameShape(QScrollArea.NoFrame)
            scroll.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
            page = ChannelConfigPage(index)
            self.channel_widgets.append(page)
            scroll.setWidget(page)
            self.tabs.addTab(scroll, f"Channel {index + 1}")
        layout.addWidget(self.tabs, 1)

        transfer_box = QGroupBox("Transfer status")
        transfer_box.setStyleSheet(TRANSFER_GROUP_STYLE)
        transfer_layout = QVBoxLayout(transfer_box)
        transfer_layout.setContentsMargins(12, 12, 12, 12)
        transfer_layout.setSpacing(6)

        transfer_row = QHBoxLayout()
        transfer_row.setContentsMargins(0, 0, 0, 0)
        transfer_row.setSpacing(8)
        self.lbl_send_status = QLabel("Idle")
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
        return {"channels": [widget.to_dict() for widget in self.channel_widgets]}

    def apply_config(self, config):
        channels = config.get("channels") if isinstance(config, dict) else config
        if not isinstance(channels, list):
            raise ValueError("Expected a channels list in the JSON file")

        for index, channel_data in enumerate(channels[: len(self.channel_widgets)]):
            if isinstance(channel_data, dict):
                self.channel_widgets[index].apply_dict(channel_data)

    def _build_binary_payload(self):
        # STM32 expects a raw array of packed channel structs with no file header.
        return b"".join(widget.pack_binary_record() for widget in self.channel_widgets)

    def _send_binary(self):
        self.set_isotp_state(0, "Preparing transfer", busy=True)
        self.send_binary_requested.emit(self._build_binary_payload())

    def set_isotp_state(self, progress, status=None, busy=None):
        self.progress_send.setValue(max(0, min(100, int(progress))))
        if status is not None:
            self.lbl_send_status.setText(status)
        if busy is not None:
            self.btn_send_binary.setEnabled(not busy)

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

            # Accept either raw array of records or optional header PDMB + version
            payload = None
            expected_size = CHANNEL_COUNT * BINARY_RECORD_SIZE
            if len(data) == expected_size:
                payload = data
            elif len(data) >= 5 and data[:4] == BINARY_MAGIC:
                # header present: 4-byte magic + 1-byte version
                payload = data[5:]
                if len(payload) != expected_size:
                    raise ValueError("Binary header found but payload size mismatch")
            elif len(data) % BINARY_RECORD_SIZE == 0 and len(data) >= expected_size:
                # file may contain multiple concatenated records; take first CHANNEL_COUNT
                payload = data[:expected_size]
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

            channels = []
            for i in range(CHANNEL_COUNT):
                start = i * BINARY_RECORD_SIZE
                rec = payload[start : start + BINARY_RECORD_SIZE]
                tup = struct.unpack(BINARY_RECORD_FORMAT, rec)
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
                ) = tup

                name = name_bytes.split(b"\0", 1)[0].decode("utf-8", errors="replace")

                # clamp values to safe ranges expected by UI widgets to avoid OverflowError
                ch_dict = {
                    "id": ch_id,
                    "type": type_v,
                    "mode": mode_v,
                    "spocId": spoc_id,
                    "spocChId": spoc_ch,
                    "name": name,
                    "batch": batch,
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
                channels.append(ch_dict)

            self.apply_config({"channels": channels})
        except Exception as exc:
            QMessageBox.critical(self, "Load failed", f"Could not load binary: {exc}")

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
